#pragma once 

#include <stdatomic.h> 
#include <time.h> 

#include <pthread.h> 

#include "game.h" 

#define INF_DEPTH (-1) 
#define INF_TIME (-1) 

typedef struct pv_line pv_line; 
typedef struct search_thread search_thread; 
typedef struct search_params search_params; 

struct pv_line 
{
    size_t n_moves; 
    move moves[128]; 
};

struct search_thread 
{
    game board; 
    pthread_t thread; 
    int tgt_depth; 
    int tgt_time; 
    clock_t end_at; 
    bool should_exit; 

    pthread_mutex_t lock; 
    pv_line pv; 
    uint64_t nodes; 
    uint64_t nps; 
    int depth; 
    int eval; 
    bool running; 
};

struct search_params 
{
    const game *board; 
    int depth; 
    int time_ms; 
};

void create_search_thread(search_thread *st); 

void destroy_search_thread(search_thread *st); 

void stop_search_thread(search_thread *st); 

static void init_search_params(search_params *sp, const game *board, int depth, int time_ms) 
{
    sp->board = board; 
    sp->depth = depth; 
    sp->time_ms = time_ms; 
}

void search(search_thread *st, search_params *params); 