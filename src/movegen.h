#pragma once 

#include "bitboard.h"
#include "game.h" 
#include "move.h"

typedef struct mvlist mvlist; 
typedef struct mvinfo mvinfo; 

#define MAX_MOVES_PER_TURN 218

struct mvlist 
{
    move moves[MAX_MOVES_PER_TURN * MAX_DEPTH]; 
    size_t size; 
};

static inline void swap_mvlist(mvlist *m, size_t a, size_t b) 
{
    move tmp = m->moves[a]; 
    m->moves[a] = m->moves[b]; 
    m->moves[b] = tmp; 
}

static inline void pop_mvlist(mvlist *m, size_t to_size) 
{
    m->size = to_size; 
}

static inline void clear_mvlist(mvlist *m) 
{
    m->size = 0; 
}

mvlist *new_mvlist(void); 

void free_mvlist(mvlist *moves); 

struct mvinfo 
{
    bboard moves[64]; 
    piece pieces[64]; 
    square from[64]; 
    int n_pieces; 
    int n_moves; 
};

void gen_mvinfo(const game *g, mvinfo *info); 

void gen_moves_mvinfo(const game *g, const mvinfo *info, mvlist *moves); 

static inline void gen_moves(const game *g, mvlist *moves) 
{
    mvinfo info; 
    gen_mvinfo(g, &info); 
    gen_moves_mvinfo(g, &info, moves); 
}
