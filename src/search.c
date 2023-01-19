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

static inline int qsearch(search_data *sd, game *g, int alpha, int beta, int depth, vector *moves) 
{
    size_t start = moves->size; 

    gen_moves(g, moves); 

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
            move mv = AT_VEC(moves, move, i); 

            // only consider captures 
            if (takes_ep(mv) | get_bit(g->colors[opp_col(g->turn)], to_sq(mv))) 
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

static inline int negamax(search_data *sd, game *g, int alpha, int beta, int depth, vector *moves, pv_line *pv) 
{
    pv_line line; 
    size_t start = moves->size; 

    gen_moves(g, moves); 
    int num_moves = moves->size - start; 

    if (num_moves == 0 || depth <= 0) 
    {
        pv->n_moves = 0; 
        pop_vec_to_size(moves, start); 
        sd->nodes++; 
        return qsearch(sd, g, alpha, beta, 32, moves); 
    }

    for (size_t i = start; i < moves->size; i++) 
    {
        push_move(g, AT_VEC(moves, move, i)); 
        int eval = -negamax(sd, g, -beta, -alpha, depth - 1, moves, &line); 
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

    pv_line line; 
    for (int depth = 1; depth <= search_depth; depth++) 
    {
        search_data sd; 
        init_search(&sd); 

        clear_vec(&moves); 
        clock_t start = clock(); 
        int eval = negamax(&sd, g, -INT_MAX, INT_MAX, depth, &moves, &line); 
        clock_t end = clock(); 

        float duration = (end - start); 
        if (duration <= 0) duration = 1; 
        duration /= CLOCKS_PER_SEC; 
        float nps = sd.nodes / duration; 

        printf("info depth %d seldepth %zu multipv 1 score cp %d nodes %"PRIu64" nps %.2f pv ", depth, line.n_moves, eval, sd.nodes, nps); 
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