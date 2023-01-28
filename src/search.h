#pragma once 

#include <stdatomic.h> 
#include <time.h> 

#include <pthread.h> 

#include "game.h" 

#define INF_DEPTH (-1) 
#define INF_TIME (-1) 

#define MAX_DEPTH 128 
#define MAX_KILLER 2

typedef struct pv_line pv_line; 
typedef struct search_ctx search_ctx; 
typedef struct search_params search_params; 

struct pv_line 
{
    size_t n_moves; 
    move moves[MAX_DEPTH]; 
};

struct search_ctx 
{
    game board; 
    int start_ply; 
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
    vector moves; 
    move killer[MAX_DEPTH][MAX_KILLER]; 
};

struct search_params 
{
    const game *board; 
    int depth; 
    int time_ms; 
};

void create_search_ctx(search_ctx *ctx); 

void destroy_search_ctx(search_ctx *ctx); 

void stop_search_ctx(search_ctx *ctx); 

static void init_search_params(search_params *sp, const game *board, int depth, int time_ms) 
{
    sp->board = board; 
    sp->depth = depth; 
    sp->time_ms = time_ms; 
}

void search(search_ctx *ctx, search_params *params); 
