#include "search.h" 

#include <time.h> 

typedef struct search_data search_data; 

static inline int move_val(const search_ctx *ctx, move mv, int depth, move pv) 
{
    static const int VALUES[] = 
    {
        100, 310, 320, 500, 900, 10000
    };

    const game *g = &ctx->board; 

    // always check PV first
    if (mv == pv) 
    {
        return 200000; 
    }

    piece pc_type = get_no_col(from_pc(mv)); 
    bool capture = is_capture(g, mv); 
    bool quiet = is_quiet(g, mv); 

    if (capture) 
    {
        return 10 * VALUES[get_no_col(pc_at_or_wp(g, to_sq(mv)))] - VALUES[pc_type]; 
    }

    if (quiet) 
    {
        for (int i = 0; i < MAX_KILLER; i++) 
        {
            if (mv == ctx->killer[depth][i]) 
            {
                // low-to-high value capture comes before killer move 
                // high-to-low value capture comes after killer move 
                return -i - 1; 
            }
        }
    }
    
    // remaining non-capturing moves should be evaluated last 
    return -100000 + PC_SQ[pc_type][to_sq(mv)] - PC_SQ[pc_type][from_sq(mv)]; 
}

static inline move sort_first_move(search_ctx *ctx, size_t start, int depth, int pv_idx) 
{
    vector *moves = &ctx->moves; 

    // for PV prioritization 
    move pv = NO_MOVE; 
    if (pv_idx >= 0 && (size_t) pv_idx < ctx->pv.n_moves) 
    {
        pv = ctx->pv.moves[pv_idx]; 
    }

    // swap current index with highest priority move 
    size_t best_i = start; 
    int best_val = move_val(ctx, AT_VEC(moves, move, start), depth, pv); 
    for (size_t i = start + 1; i < moves->size; i++) 
    {
        int val = move_val(ctx, AT_VEC(moves, move, i), depth, pv); 
        if (val > best_val) 
        {
            best_i = i; 
            best_val = val; 
        }
    }
    swap_vec(moves, start, best_i); 
    return AT_VEC(moves, move, start); 
}

static inline bool out_of_time(const search_ctx *ctx) 
{
    return ctx->should_exit || (ctx->tgt_time >= 0 && clock() > ctx->end_at); 
}

static inline bool handle_out_of_time(search_ctx *ctx) 
{
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

static inline int qsearch(search_ctx *ctx, int alpha, int beta) 
{
    game *g = &ctx->board; 
    vector *moves = &ctx->moves; 
    size_t start = moves->size; 
    gen_moves(g, moves); 

    // static eval of current position 
    int stand_pat = col_sign(g) * evaluate(g, moves->size - start); 
    if (stand_pat >= beta) 
    {
        pop_vec_to_size(moves, start); 
        return beta; 
    }
    if (stand_pat > alpha) 
    {
        alpha = stand_pat; 
    }

    // keep playing moves until we get a quiet position
    for (size_t i = start; i < moves->size; i++) 
    {
        move mv = sort_first_move(ctx, i, 0, -1); 

        // only consider captures (including en passant)
        if (is_capture(g, mv)) 
        {
            push_move(g, mv); 
            int score = -qsearch(ctx, -beta, -alpha); 
            pop_move(g); 

            if (score >= beta) 
            {
                pop_vec_to_size(moves, start); 
                return beta; 
            }
            if (score > alpha) 
            {
                alpha = score; 
            }
        }
    }

    pop_vec_to_size(moves, start); 
    return alpha; 
}

static inline void update_pv(pv_line *pv, move mv, const pv_line *line) 
{
    pv->moves[0] = mv; 
    memcpy(pv->moves + 1, line->moves, line->n_moves * sizeof(move)); 
    pv->n_moves = line->n_moves + 1; 
}

static inline void clear_pv(pv_line *pv) 
{
    pv->n_moves = 0; 
}

static inline int negamax(
    search_ctx *ctx, 
    int alpha, 
    int beta, 
    int depth, 
    bool null_move, 
    int pv_idx, 
    int check_time, 
    pv_line *pv
) {
    
    pv_line line; 
    game *g = &ctx->board; 
    vector *moves = &ctx->moves; 
    size_t start = moves->size; 
    gen_moves(g, moves); 
    size_t num_moves = moves->size - start; 

    // prints best move and exits thread if out of time
    if (check_time >= 1) handle_out_of_time(ctx); 

    // one of the following: 
    // - checkmate 
    // - stalemate 
    // - deepest ply to search
    if (num_moves == 0 || depth <= 0) 
    {
        clear_pv(pv); 
        pop_vec_to_size(moves, start); 
        return qsearch(ctx, alpha, beta); 
    }

    // null move pruning
    if (null_move) 
    {
        static const int R = 2; 
        // don't do null move if: 
        // - depth is too shallow 
        // - in check 
        // - either side has only pawns 
        if (depth >= 1 + R && !g->in_check && !any_side_k_p(g)) 
        {
            push_null_move(g); 
            int eval = -negamax(ctx, -beta, -beta + 1, depth - 1 - R, false, -1, check_time - 1, pv); 
            pop_null_move(g); 

            if (eval >= beta) 
            {
                clear_pv(pv); 
                pop_vec_to_size(moves, start); 
                return beta; 
            }
        }
    }

    // handle legal moves (sorted by highest priority)
    for (size_t i = start; i < moves->size; i++) 
    {
        move mv = sort_first_move(ctx, i, depth, pv_idx);  

        // only consider next PV move if we are in the PV
        int next_pv_idx = -1; 
        if (pv_idx >= 0 && (size_t) pv_idx < ctx->pv.n_moves && ctx->pv.moves[pv_idx] == mv) 
        {
            next_pv_idx = pv_idx + 1; 
        }

        push_move(g, mv); 
        int eval = -negamax(ctx, -beta, -alpha, depth - 1, null_move, next_pv_idx, check_time - 1, &line); 
        pop_move(g); 

        if (eval >= beta) 
        {
            if (is_quiet(g, mv) && mv != ctx->killer[depth][0]) 
            {
                for (int j = MAX_KILLER - 1; j > 0; j--) 
                {
                    ctx->killer[depth][j] = ctx->killer[depth][j - 1]; 
                }
                ctx->killer[depth][0] = mv; 
            }
            
            pop_vec_to_size(moves, start); 
            return beta; 
        }
        if (eval > alpha) 
        {
            alpha = eval; 
            update_pv(pv, mv, &line); 
        }
    }

    pop_vec_to_size(moves, start); 
    return alpha; 
}

void run_search(search_ctx *ctx) 
{
    clock_t search_start = clock(); 

    game *g = &ctx->board; 
    clear_pv(&ctx->pv); 

    int tgt_depth = ctx->tgt_depth; 
    if (tgt_depth < 0) tgt_depth = INT_MAX; 

    pv_line line; 
    for (int depth = 1; depth <= tgt_depth; depth++) 
    {
        clear_vec(&ctx->moves); 

        clock_t start = clock(); 
        int eval = negamax(ctx, -EVAL_MAX, EVAL_MAX, depth, true, 0, 6, &line); 
        clock_t end = clock(); 
        
        float duration = (end - start); 
        if (duration <= 0) duration = 1; 
        duration /= CLOCKS_PER_SEC; 
        float nps = g->nodes / duration; 

        float start_duration = (end - search_start); 
        if (start_duration <= 0) start_duration = 1; 
        start_duration /= CLOCKS_PER_SEC / 1000; 

        ctx->pv = line; 
        ctx->nodes = g->nodes; 
        ctx->nps = (uint64_t) nps; 
        ctx->depth = depth; 
        ctx->eval = eval; 

        printf("info depth %d seldepth %zu multipv 1 score cp %d time %.0f nodes %"PRIu64" nps %.0f pv ", depth, line.n_moves, eval, start_duration, g->nodes, nps); 
        for (size_t i = 0; i < ctx->pv.n_moves; i++) 
        {
            print_move_end(ctx->pv.moves[i], " "); 
        }
        printf("\n"); 
        fflush(stdout); 
    }

    ctx->running = false; 

    printf("bestmove "); 
    print_move_end(line.moves[0], "\n"); 
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
    memset(ctx->killer, 0, sizeof(ctx->killer)); 

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