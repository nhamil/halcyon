#include "search.h" 

#include <time.h> 

typedef struct search_data search_data; 

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

static inline int qsearch(search_thread *thread, game *g, int alpha, int beta, int depth, vector *moves) 
{
    size_t start = moves->size; 

    gen_moves(g, moves); 
    // reorder_moves(g, moves, start); 

    // if (thread->should_exit || (thread->tgt_time >= 0 && clock() > thread->end_at)) 
    // {
    //     if (thread->pv.n_moves) 
    //     {
    //         printf("bestmove "); 
    //         print_move_end(thread->pv.moves[0], "\n"); 
    //         fflush(stdout); 
    //     }
    //     else 
    //     {
    //         // no best move 
    //         printf("bestmove a1a1\n"); 
    //         fflush(stdout); 
    //     }
        
    //     destroy_vec(moves); 
    //     pthread_exit(NULL); 
    // }

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
                int score = -qsearch(thread, g, -beta, -alpha, depth - 1, moves); 
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

static inline int negamax(search_thread *thread, game *g, int alpha, int beta, int depth, bool null_move, int pv_idx, vector *moves, pv_line *pv) 
{
    pv_line line; 
    size_t start = moves->size; 

    gen_moves(g, moves); 
    // reorder_moves(g, moves, start); 
    int num_moves = moves->size - start; 

    if (thread->should_exit || (thread->tgt_time >= 0 && clock() > thread->end_at)) 
    {
        if (thread->pv.n_moves) 
        {
            printf("bestmove "); 
            print_move_end(thread->pv.moves[0], "\n"); 
            fflush(stdout); 
        }
        else 
        {
            // no best move 
            printf("bestmove a1a1\n"); 
            fflush(stdout); 
        }

        destroy_vec(moves); 
        pthread_exit(NULL); 
    }

    if (num_moves == 0 || depth <= 0) 
    {
        pv->n_moves = 0; 
        pop_vec_to_size(moves, start); 
        return qsearch(thread, g, alpha, beta, 32, moves); 
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
            int eval = -negamax(thread, g, -beta, -beta + 1, depth - 1 - R, false, -1, moves, pv); 
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
        move mv; 
        int next_pv_idx = -1; 
        size_t best_i = i; 
        int mv_val = move_val(g, AT_VEC(moves, move, i)); 
        for (size_t j = i + 1; j < moves->size; j++) 
        {
            mv = AT_VEC(moves, move, j); 
            if (pv_idx >= 0 && (size_t) pv_idx < thread->pv.n_moves && mv == thread->pv.moves[pv_idx]) 
            {
                best_i = j; 
                next_pv_idx = pv_idx + 1; 
                break; 
            }
            else 
            {
                int j_val = move_val(g, mv); 
                if (j_val > mv_val) 
                {
                    mv_val = j_val; 
                    best_i = j; 
                }
            }
            
        }
        swap_vec(moves, i, best_i); 

        mv = AT_VEC(moves, move, i); 

        // don't reduce if: 
        // - in last PV
        // - capture 
        // - promotion 
        // - TODO gives check 
        // - depth < 3
        int sub_depth = -1; 
        bool can_lmr = true; 
        can_lmr &= (i - start) >= 4; 
        can_lmr &= pv_idx == -1; 
        can_lmr &= !is_capture(g, mv); 
        can_lmr &= from_pc(mv) == pro_pc(mv); 
        can_lmr &= depth >= 3; 
        if (can_lmr) 
        {
            sub_depth--; 
        }

        push_move(g, mv); 
        int eval = -negamax(thread, g, -beta, -alpha, depth + sub_depth, null_move, next_pv_idx, moves, &line); 
        pop_move(g); 

        if (eval >= beta) 
        {
            pop_vec_to_size(moves, start); 
            return beta; 
        }

        if (eval > alpha) 
        {
            alpha = eval; 
            pv->moves[0] = mv; 
            memcpy(pv->moves + 1, line.moves, line.n_moves * sizeof(move)); 
            pv->n_moves = line.n_moves + 1; 
        }
    }

    pop_vec_to_size(moves, start); 
    return alpha; 
}

void run_search(search_thread *thread) 
{
    vector moves; 
    CREATE_VEC(&moves, move); 

    clock_t search_start = clock(); 

    game *g = &thread->board; 

    thread->pv.n_moves = 0; 

    int tgt_depth = thread->tgt_depth; 
    if (tgt_depth < 0) tgt_depth = INT_MAX; 

    pv_line line; 
    for (int depth = 1; depth <= tgt_depth; depth++) 
    {
        clear_vec(&moves); 
        clock_t start = clock(); 
        int eval = negamax(thread, g, -EVAL_MAX, EVAL_MAX, depth, true, 0, &moves, &line); 

        clock_t end = clock(); 

        float duration = (end - start); 
        if (duration <= 0) duration = 1; 
        duration /= CLOCKS_PER_SEC; 
        float nps = g->nodes / duration; 

        float start_duration = (end - search_start); 
        if (start_duration <= 0) start_duration = 1; 
        start_duration /= CLOCKS_PER_SEC / 1000; 

        thread->pv = line; 
        thread->nodes = g->nodes; 
        thread->nps = (uint64_t) nps; 
        thread->depth = depth; 
        thread->eval = eval; 

        printf("info depth %d seldepth %zu multipv 1 score cp %d time %.0f nodes %"PRIu64" nps %.0f pv ", depth, line.n_moves, eval, start_duration, g->nodes, nps); 
        for (size_t i = 0; i < thread->pv.n_moves; i++) 
        {
            print_move_end(thread->pv.moves[i], " "); 
        }
        printf("\n"); 
        fflush(stdout); 
    }

    thread->running = false; 

    printf("bestmove "); 
    print_move_end(line.moves[0], "\n"); 
    fflush(stdout); 

    destroy_vec(&moves); 
}

static void *start_pthread_search(void *data) 
{
    search_thread *st = data; 
    run_search(st); 

    return NULL; 
}

void create_search_thread(search_thread *st) 
{
    memset(st, 0, sizeof(search_thread)); 

    // board should always be initialized
    create_game(&st->board); 

    pthread_mutex_init(&st->lock, NULL); 
} 

void destroy_search_thread(search_thread *st) 
{
    stop_search_thread(st); 
    destroy_game(&st->board); 
    pthread_mutex_destroy(&st->lock); 
}

void stop_search_thread(search_thread *st) 
{
    if (st->running) 
    {
        st->should_exit = true; 
        pthread_join(st->thread, NULL); 

        st->running = false; 

        // if (st->pv.n_moves) 
        // {
        //     printf("bestmove "); 
        //     print_move_end(st->pv.moves[0], "\n"); 
        //     fflush(stdout); 
        // }
        // else 
        // {
        //     // no best move 
        //     printf("bestmove a1a1\n"); 
        //     fflush(stdout); 
        // }

        printf("info string Stopped search\n"); 
    }
    else 
    {
        printf("info string No search to stop\n"); 
    }

    st->should_exit = false; 
    fflush(stdout); 
}

void search(search_thread *st, search_params *params) 
{
    stop_search_thread(st); 

    st->pv.n_moves = 0; 
    st->nodes = 0; 
    st->nps = 0; 
    st->depth = 0; 
    st->eval = 0; 
    st->running = true; 

    st->tgt_depth = params->depth; 
    st->tgt_time = params->time_ms; 
    st->should_exit = false; 
    if (st->tgt_time >= 0) 
    {
        st->end_at = clock() + st->tgt_time * CLOCKS_PER_SEC / 1000; 
    }

    destroy_game(&st->board); 
    create_game_copy(&st->board, params->board); 

    pthread_create(&st->thread, NULL, start_pthread_search, st); 
}