#include "search.h" 
#include "game.h"
#include "move.h"
#include "piece.h"
#include "vector.h"

#include <string.h>
#include <time.h> 

#define CHECK_TIME 100000

typedef struct search_data search_data; 

static const int PC_VALUES[] = 
{
    100, 310, 320, 500, 900, 10000
};

static inline bool out_of_time(const search_ctx *ctx) 
{
    return ctx->should_exit || (ctx->pv.n_moves && ctx->tgt_time >= 0 && clock() > ctx->end_at); 
}

static inline bool handle_out_of_time(search_ctx *ctx) 
{
    if (ctx->check_time++ < CHECK_TIME) return false; 

    ctx->check_time = 0; 

    // check for time limit
    if (out_of_time(ctx)) 
    {
        if (ctx->pv.n_moves) 
        {
            printf("bestmove "); 
            print_move_end(ctx->pv.moves[0], "\n"); 
            fflush(stdout); 
        }
        else 
        {
            // no best move 
            printf("bestmove a1a1\n"); 
            fflush(stdout); 
        }

        pthread_exit(NULL); 
        return true; 
    }

    return false; 
}

// assumes the move is a capture 
static inline int mvv_lva(search_ctx *ctx, move mv) 
{
    piece pc_type = get_no_col(from_pc(mv)); 

    if (takes_ep(mv)) 
    {
        return 10 * (PC_VALUES[PC_P] - PC_VALUES[pc_type]); 
    }
    else 
    {
        return 10 * (PC_VALUES[get_no_col(pc_at_or_wp(&ctx->board, to_sq(mv)))] - PC_VALUES[pc_type]); 
    }   
}

static inline int qmove_val(search_ctx *ctx, move mv) 
{
    if (is_capture(&ctx->board, mv)) 
    {
        return mvv_lva(ctx, mv); 
    }

    if (has_pro(mv)) 
    {
        return PC_VALUES[get_no_col(pro_pc(mv))]; 
    }

    return -100000; 
}

static inline move next_qmove(search_ctx *ctx, size_t start) 
{
    vector *moves = &ctx->moves; 

    // swap current index with highest priority move 
    size_t best_i = start; 
    int best_val = mvv_lva(ctx, AT_VEC(moves, move, start)); 
    for (size_t i = start + 1; i < moves->size; i++) 
    {
        move mv = AT_VEC(moves, move, i); 
        int val = qmove_val(ctx, mv); 
        
        if (val > best_val) 
        {
            best_i = i; 
            best_val = val; 
        }
    }

    // put best move in first place and return it 
    swap_vec(moves, start, best_i); 
    return AT_VEC(moves, move, start); 
}

static inline int move_val(search_ctx *ctx, move mv) 
{
    game *g = &ctx->board; 
    piece pc_type = get_no_col(from_pc(mv)); 

    // previous pv has highest priority 
    // first move is ply 1, not ply 0
    // so check <= instead of <
    if (ctx->in_pv && ctx->ply <= (ssize_t) ctx->pv.n_moves) 
    {
        if (mv == ctx->pv.moves[ctx->ply - 1]) return 200000; 
    }

    // capture
    if (is_capture(g, mv)) 
    {
        return 100000 + mvv_lva(ctx, mv); 
    }

    if (is_quiet(g, mv)) 
    {
        // killer move 
        if (ctx->killer[ctx->ply][0] == mv) 
        {
            return 100001; 
        }
        if (ctx->killer[ctx->ply][1] == mv) 
        {
            return 100000; 
        }

        // history heuristic 
        int hist = ctx->history[g->turn][pc_type][to_sq(mv)]; 
        if (hist > 0) 
        {
            return hist; 
        }
    }
    
    // promotion
    if (has_pro(mv)) 
    {
        return PC_VALUES[get_no_col(pro_pc(mv))]; 
    }

    // default 
    return -100000 + PC_SQ[pc_type][to_sq(mv)] - PC_SQ[pc_type][from_sq(mv)]; 
}

static inline move next_move(search_ctx *ctx, size_t start) 
{
    vector *moves = &ctx->moves; 

    // swap current index with highest priority move 
    size_t best_i = start; 
    int best_val = mvv_lva(ctx, AT_VEC(moves, move, start)); 
    for (size_t i = start + 1; i < moves->size; i++) 
    {
        move mv = AT_VEC(moves, move, i); 
        int val = move_val(ctx, mv); 
        
        if (val > best_val) 
        {
            best_i = i; 
            best_val = val; 
        }
    }

    // put best move in first place and return it 
    swap_vec(moves, start, best_i); 
    return AT_VEC(moves, move, start); 
}

static inline int qsearch(search_ctx *ctx, int alpha, int beta, int depth) 
{
    game *g = &ctx->board; 
    vector *moves = &ctx->moves; 
    size_t start = moves->size; 

    ctx->qnode_cnt++; 

    handle_out_of_time(ctx); 

    bool draw = is_special_draw(g); 

    gen_moves(g, moves); 
    size_t num_moves = moves->size - start; 

    int stand_pat = col_sign(g) * evaluate(g, num_moves, draw); 

    // check for beta cutoff
    if (stand_pat >= beta) 
    {
        pop_vec_to_size(moves, start); 
        return beta; 
    }
    
    // check if doing nothing improves score 
    if (stand_pat > alpha) 
    {
        alpha = stand_pat; 
    }

    // leaf node 
    if (depth <= 0) 
    {
        ctx->qleaf_cnt++; 
        pop_vec_to_size(moves, start); 
        return alpha; 
    }

    // search active moves 
    bool found_move = false; 
    if (!draw) 
    {
        ctx->ply++; 
        for (size_t i = start; i < moves->size; i++) 
        {
            move mv = next_qmove(ctx, i); 

            bool should_search = is_capture(g, mv) || has_pro(mv); 
            if (!should_search) continue; 

            // print_move_end(mv, " "); 
            // printf("%s x %s = %d\n", str_pc(from_pc(mv)), str_pc(pc_at_or_wp(g, to_sq(mv))), qmove_val(ctx, mv)); 

            // there is 1+ active moves, so not a leaf node 
            found_move = true; 

            // search move 
            push_move(g, mv); 
            int score = -qsearch(ctx, -beta, -alpha, depth - 1); 
            pop_move(g); 

            // beta cutoff 
            if (score >= beta) 
            {
                ctx->ply--; 
                pop_vec_to_size(moves, start); 
                return beta; 
            }

            // pv node 
            if (score > alpha) 
            {
                alpha = score; 
            }
        }
        ctx->ply--; 
    }
    if (!found_move) ctx->qleaf_cnt++; 

    pop_vec_to_size(moves, start); 
    return alpha; 
}

static inline void update_pv(search_ctx *ctx, move mv, int offset) 
{
    pv_line *dst = ctx->lines + ctx->ply + offset; 
    pv_line *src = ctx->lines + ctx->ply + 1 + offset; 

    dst->moves[0] = mv; 
    memcpy(dst->moves + 1, src->moves, src->n_moves * sizeof(move)); 
    dst->n_moves = src->n_moves + 1; 
}

static inline void clear_pv(search_ctx *ctx, int offset) 
{
    ctx->lines[ctx->ply + offset].n_moves = 0; 
}

static inline int negamax(search_ctx *ctx, int alpha, int beta, int depth) 
{
    // reset data on root node 
    if (ctx->ply == 0) 
    {
        ctx->node_cnt = 0; 
        ctx->leaf_cnt = 0; 
        ctx->qnode_cnt = 0; 
        ctx->qleaf_cnt = 0; 
        ctx->check_time = 0; 
        memset(ctx->killer, 0, sizeof(ctx->killer)); 
        memset(ctx->history, 0, sizeof(ctx->history)); 
        ctx->null_move = true; 
        ctx->in_pv = true; 
    }

    // init node 
    clear_pv(ctx, 0); 
    ctx->node_cnt++; 

    // break out if search should end 
    handle_out_of_time(ctx); 

    game *g = &ctx->board; 
    vector *moves = &ctx->moves; 
    size_t start = moves->size; 

    // 3-fold repetition etc 
    bool draw = is_special_draw(g); 

    // collect all moves from current position 
    // these moves must be cleared before returning
    gen_moves(g, moves); 
    size_t num_moves = moves->size - start; 

    // end of search or end of game 
    if (depth <= 0 || num_moves == 0 || draw) 
    {
        // no move found for node so this is a leaf node
        clear_pv(ctx, 0); 
        ctx->leaf_cnt++; 

        pop_vec_to_size(moves, start); 
        return qsearch(ctx, alpha, beta, 16); 
    }

    if (ctx->null_move) 
    {
        static const int R = 2; 
        // don't do null move if: 
        // - depth is too shallow 
        // - in check 
        // - either side has only pawns 
        if (depth >= 1 + R && !g->in_check && !any_side_k_p(g)) 
        {
            ctx->ply++; 
            ctx->null_move = false; 

            push_null_move(g); 
            // check if full search would have beta cutoff 
            int score = -negamax(ctx, -beta, -beta + 1, depth - 1 - R); 
            pop_null_move(g); 

            if (score >= beta) 
            {
                ctx->null_move = true; 
                ctx->ply--; 

                clear_pv(ctx, 0); 
                pop_vec_to_size(moves, start); 
                return beta; 
            }

            ctx->null_move = true; 
            ctx->ply--; 
        }
    }

    // search moves if there is remaining depth 
    // must reset: 
    // - ply 
    // - in_pv

    ctx->ply++; 
    bool found_pv = false; 
    bool node_in_pv = ctx->in_pv; 
    for (size_t i = start; i < moves->size; i++) 
    {
        move mv = next_move(ctx, i); 

        bool capture = is_capture(g, mv); 
        bool pro = has_pro(mv); 
        bool check = g->in_check; 

        // search position after applying move 
        push_move(g, mv); 
        bool gives_check = g->in_check; 

        // false if no further searching (and pruning) is needed
        bool full_search = true; 
        int score; 

        bool lmr = true; 
        lmr &= !capture; 
        lmr &= !pro; 
        lmr &= !check; 
        lmr &= !gives_check; 
        lmr &= i - start >= 4;
        lmr &= depth > 3; 
        int lmr_amt = lmr * 2; 

        if (!check && !gives_check && found_pv) // principal variation search 
        {
            // check if the move is at all better than current best 
            score = -negamax(ctx, -alpha - 1, -alpha, depth - 1 - lmr_amt); 

            if (score <= alpha || score >= beta) 
            {
                full_search = false; 
            }
        }
        else if (lmr) // late move reduction
        {
            score = -negamax(ctx, -alpha - 1, -alpha, depth - 1 - lmr_amt); 
        
            if (score <= alpha || score >= beta) 
            {
                full_search = false; 
            }
        }

        if (full_search) 
        {
            score = -negamax(ctx, -beta, -alpha, depth - 1); 
        }
        
        pop_move(g); 

        // beta cutoff 
        if (score >= beta) 
        {
            if (is_quiet(g, mv)) 
            {
                // killer move heuristic 
                if (ctx->killer[ctx->ply][0] != mv) 
                {
                    ctx->killer[ctx->ply][1] = ctx->killer[ctx->ply][0]; 
                    ctx->killer[ctx->ply][0] = mv; 
                }

                // history heuristic 
                ctx->history[g->turn][get_no_col(from_pc(mv))][to_sq(mv)] = depth * depth; 
            }

            ctx->ply--; 
            ctx->in_pv = node_in_pv; 
            pop_vec_to_size(moves, start); 
            return beta; 
        }

        // improves score: pv node 
        if (score > alpha) 
        {
            found_pv = true; 
            alpha = score; 
            update_pv(ctx, mv, -1); // ply is incremented right now: -1 offset
        }

        // only way to be in previous PV is to be the leftmost node 
        ctx->in_pv = false; 
    }
    ctx->ply--; 
    ctx->in_pv = node_in_pv; 

    pop_vec_to_size(moves, start); 
    return alpha; 
}

static inline void update_search(search_ctx *ctx, clock_t search_start, clock_t start, clock_t end, int depth, int eval, const pv_line *line) 
{
    size_t nodes = ctx->board.nodes;// ctx->leaf_cnt; 

    float duration = end - start; 
    if (duration <= 0) duration = 1; 
    duration /= CLOCKS_PER_SEC; 
    float nps = nodes / duration; 

    float start_duration = (end - search_start); 
    if (start_duration <= 0) start_duration = 1; 
    start_duration /= CLOCKS_PER_SEC * 0.001; 

    ctx->pv = *line; 
    ctx->nodes = nodes; 
    ctx->nps = (uint64_t) nps; 
    ctx->depth = depth; 
    ctx->eval = eval; 

    printf("info depth %d seldepth %zu multipv 1 score cp %d time %.0f nodes %" PRIu64 " nps %.0f pv ", depth, ctx->pv.n_moves, eval, start_duration, nodes, nps); 
    for (size_t i = 0; i < ctx->pv.n_moves; i++) 
    {
        print_move_end(ctx->pv.moves[i], " "); 
    }
    printf("\n"); 
    // printf("info string nodes %zu leaves %zu mbf %.2f qpct %.0f\n", ctx->node_cnt, ctx->leaf_cnt, (double) ctx->node_cnt / (ctx->node_cnt - ctx->leaf_cnt), 100.0 * ctx->qnode_cnt / (ctx->node_cnt + ctx->qnode_cnt)); 
    fflush(stdout); 
}

void run_search(search_ctx *ctx) 
{
    clock_t start, end, search_start = clock(); 

    int tgt_depth = ctx->tgt_depth; 
    if (tgt_depth < 0) tgt_depth = INT_MAX; 
    if (tgt_depth > MAX_DEPTH) tgt_depth = MAX_DEPTH; 

    start = clock(); 
    int eval = negamax(ctx, -EVAL_MAX, EVAL_MAX, 1); 
    end = clock(); 
    update_search(ctx, search_start, start, end, 1, eval, &ctx->lines[0]); 

    for (int depth = 2; depth <= tgt_depth; depth++) 
    {
        clear_vec(&ctx->moves); 
        int last = eval; 

        int a[] = { last - 30, last - 130, last - 530, -EVAL_MAX }; 
        int b[] = { last + 30, last + 130, last + 530, EVAL_MAX }; 
        int ai = 0; 
        int bi = 0; 

        start = clock(); 
        while (true) 
        {
            eval = negamax(ctx, a[ai], b[bi], depth); 

            if (eval <= a[ai]) 
            {
                ai++; 
            }
            else if (eval >= b[bi]) 
            {
                bi++; 
            }
            else 
            {
                break; 
            }
        }
        end = clock(); 
        
        update_search(ctx, search_start, start, end, depth, eval, &ctx->lines[0]); 
    }

    ctx->running = false; 

    printf("bestmove "); 
    print_move_end(ctx->lines[0].moves[0], "\n"); 
    fflush(stdout); 
}

static void *start_pthread_search(void *data) 
{
    search_ctx *ctx = data; 
    run_search(ctx); 

    return NULL; 
}

void create_search_ctx(search_ctx *ctx) 
{
    memset(ctx, 0, sizeof(search_ctx)); 

    // board should always be initialized
    create_game(&ctx->board); 
    CREATE_VEC(&ctx->moves, move); 

    pthread_mutex_init(&ctx->lock, NULL); 
} 

void destroy_search_ctx(search_ctx *ctx) 
{
    stop_search_ctx(ctx); 
    destroy_game(&ctx->board); 
    destroy_vec(&ctx->moves); 
    pthread_mutex_destroy(&ctx->lock); 
}

void stop_search_ctx(search_ctx *ctx) 
{
    if (ctx->running) 
    {
        ctx->should_exit = true; 
        pthread_join(ctx->thread, NULL); 

        clear_vec(&ctx->moves); 
        ctx->running = false; 
    }

    ctx->should_exit = false; 
    fflush(stdout); 
}

void search(search_ctx *ctx, search_params *params) 
{
    stop_search_ctx(ctx); 

    ctx->pv.n_moves = 0; 
    ctx->nodes = 0; 
    ctx->nps = 0; 
    ctx->depth = 0; 
    ctx->eval = 0; 
    ctx->running = true; 

    clear_vec(&ctx->moves); 
    ctx->ply = 0; 
    for (int i = 0; i < MAX_DEPTH; i++) 
    {
        ctx->lines[i].n_moves = 0; 
    }

    ctx->tgt_depth = params->depth; 
    ctx->tgt_time = params->time_ms; 
    ctx->should_exit = false; 
    if (ctx->tgt_time >= 0) 
    {
        ctx->end_at = clock() + ctx->tgt_time * CLOCKS_PER_SEC / 1000; 
    }

    destroy_game(&ctx->board); 
    create_game_copy(&ctx->board, params->board); 

    pthread_create(&ctx->thread, NULL, start_pthread_search, ctx); 
}