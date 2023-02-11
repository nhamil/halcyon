#pragma once 

#include <stdatomic.h> 
#include <time.h> 

#include <pthread.h> 

#include "game.h" 
#include "piece.h"
#include "square.h"

#define INF_DEPTH (-1) 
#define INF_TIME (-1) 

#define MAX_DEPTH 128 

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
    pv_line lines[MAX_DEPTH]; 
    size_t node_cnt; 
    size_t leaf_cnt; 
    size_t qnode_cnt; 
    size_t qleaf_cnt; 
    int check_time; 
    int ply; 
    move killer[MAX_DEPTH][2]; 
    int history[2][PC_CNT][SQ_CNT]; 
    bool null_move; 
    bool in_pv; 
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

int basic_qsearch(const game *g); 