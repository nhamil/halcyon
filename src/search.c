#include "search.h" 

#include <time.h> 

typedef struct search_data search_data; 

struct search_data 
{
    uint64_t nodes; 
};

void init_search(search_data *sd) 
{
    sd->nodes = 0; 
}

static inline int move_val(const game *g, move mv) 
{
    static const int VALUES[] = 
    {
        100, 320, 330, 500, 900, 10000
    };

    piece pc_type = get_no_col(from_pc(mv)); 

    if (!is_capture(g, mv)) 
    {
        // non-capturing moves should be evaluated last 
        return -100000 + PC_SQ[pc_type][to_sq(mv)] - PC_SQ[pc_type][from_sq(mv)]; 
    }

    return 10 * VALUES[get_no_col(pc_at_or_wp(g, to_sq(mv)))] - VALUES[pc_type]; 
}

static int cmp_moves(void *ctx, const void *ma, const void *mb) 
{
    const game *g = ctx; 
    move a = *(move *) ma; 
    move b = *(move *) mb; 

    int va = move_val(g, a); 
    int vb = move_val(g, b); 

    // descending order 
    return (va < vb) - (va > vb); 
}

void reorder_moves(const game *g, vector *moves, size_t start) 
{
    sort_vec_start(moves, start, cmp_moves, g); 
}

static inline int qsearch(search_data *sd, game *g, int alpha, int beta, int depth, vector *moves) 
{
    size_t start = moves->size; 

    gen_moves(g, moves); 
    // reorder_moves(g, moves, start); 

    int stand_pat = col_sign(g) * eval(g, moves->size - start); 

    if (stand_pat >= beta) 
    {
        pop_vec_to_size(moves, start); 
        return beta; 
    }
    if (stand_pat > alpha) 
    {
        alpha = stand_pat; 
    }

    if (depth >= 1) 
    {
        for (size_t i = start; i < moves->size; i++) 
        {
            size_t best_i = i; 
            int mv_val = move_val(g, AT_VEC(moves, move, i)); 
            for (size_t j = i + 1; j < moves->size; j++) 
            {
                int j_val = move_val(g, AT_VEC(moves, move, j)); 
                if (j_val > mv_val) 
                {
                    mv_val = j_val; 
                    best_i = j; 
                }
            }
            swap_vec(moves, i, best_i); 

            move mv = AT_VEC(moves, move, i); 

            // only consider captures 
            if (is_capture(g, mv)) 
            {
                push_move(g, mv); 
                int score = -qsearch(sd, g, -beta, -alpha, depth - 1, moves); 
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
    }

    pop_vec_to_size(moves, start); 
    return alpha; 
}

typedef struct pv_line pv_line; 
struct pv_line 
{
    size_t n_moves; 
    move moves[128]; 
};

static inline int negamax(search_data *sd, game *g, int alpha, int beta, int depth, bool null_move, vector *moves, pv_line *pv) 
{
    pv_line line; 
    size_t start = moves->size; 

    gen_moves(g, moves); 
    // reorder_moves(g, moves, start); 
    int num_moves = moves->size - start; 

    // for (size_t i = start; i < moves->size; i++) 
    // {
    //     move mv = AT_VEC(moves, move, i); 
    //     print_move_end(mv, " "); 
    //     printf("%d\n", move_val(g, mv)); 
    // }
    // return 0; 

    if (num_moves == 0 || depth <= 0) 
    {
        pv->n_moves = 0; 
        pop_vec_to_size(moves, start); 
        return qsearch(sd, g, alpha, beta, 32, moves); 
    }

    if (null_move) 
    {
        const int R = 2; 
        // don't do null move if: 
        // - depth is too shallow 
        // - in check 
        // - either side has only pawns 
        if (depth >= 1 + R && !g->in_check && !any_side_k_p(g)) 
        {
            push_null_move(g); 
            int eval = -negamax(sd, g, -beta, -beta + 1, depth - 1 - R, false, moves, pv); 
            pop_null_move(g); 

            if (eval >= beta) 
            {
                pv->n_moves = 0; 
                pop_vec_to_size(moves, start); 
                return beta; 
            }
        }
    }

    for (size_t i = start; i < moves->size; i++) 
    {
        size_t best_i = i; 
        int mv_val = move_val(g, AT_VEC(moves, move, i)); 
        for (size_t j = i + 1; j < moves->size; j++) 
        {
            int j_val = move_val(g, AT_VEC(moves, move, j)); 
            if (j_val > mv_val) 
            {
                mv_val = j_val; 
                best_i = j; 
            }
        }
        swap_vec(moves, i, best_i); 

        push_move(g, AT_VEC(moves, move, i)); 
        int eval = -negamax(sd, g, -beta, -alpha, depth - 1, null_move, moves, &line); 
        pop_move(g); 

        if (eval >= beta) 
        {
            pop_vec_to_size(moves, start); 
            return beta; 
        }

        if (AT_VEC(moves, move, i) == 0) 
        {
            printf("what\n"); 
            fflush(stdout); 
        }

        if (eval > alpha) 
        {
            alpha = eval; 
            pv->moves[0] = AT_VEC(moves, move, i); 
            memcpy(pv->moves + 1, line.moves, line.n_moves * sizeof(move)); 
            pv->n_moves = line.n_moves + 1; 
        }
    }

    pop_vec_to_size(moves, start); 
    return alpha; 
}

void search(game *g, int search_depth, vector *pv, int *eval) 
{
    vector moves; 
    CREATE_VEC(&moves, move); 

    clock_t search_start = clock(); 

    pv_line line; 
    for (int depth = 1; depth <= search_depth; depth++) 
    {
        search_data sd; 
        init_search(&sd); 

        clear_vec(&moves); 
        clock_t start = clock(); 
        int eval = negamax(&sd, g, -EVAL_MAX, EVAL_MAX, depth, true, &moves, &line); 

        clock_t end = clock(); 

        float duration = (end - start); 
        if (duration <= 0) duration = 1; 
        duration /= CLOCKS_PER_SEC; 
        float nps = g->nodes / duration; 

        float start_duration = (end - search_start); 
        if (start_duration <= 0) start_duration = 1; 
        start_duration /= CLOCKS_PER_SEC / 1000; 

        printf("info depth %d seldepth %zu multipv 1 score cp %d time %.0f nodes %"PRIu64" nps %.0f pv ", depth, line.n_moves, eval, start_duration, g->nodes, nps); 
        for (size_t i = 0; i < line.n_moves; i++) 
        {
            print_move_end(line.moves[i], " "); 
        }
        printf("\n"); 

        fflush(stdout); 
    }

    printf("bestmove "); 
    print_move_end(line.moves[0], "\n"); 

    destroy_vec(&moves); 
}