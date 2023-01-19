#pragma once 

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h> 
#include <string.h> 

#include "bitboard.h"
#include "castle.h" 
#include "move.h" 
#include "piece.h"
#include "square.h"
#include "vector.h"

typedef struct game game; 

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

struct game 
{
    bboard pieces[PC_CNT]; 
    bboard colors[2]; 
    bboard check; 
    castle_flags castle; 
    square ep; 
    bool in_check; 

    vector hist; 
    int ply; 
    color turn; 
};

void create_game(game *g); 

void create_game_fen(game *g, const char *fen); 

void destroy_game(game *g); 

void reset_game(game *g); 

void load_fen(game *g, const char *fen); 

bool in_check(const game *g, color for_color, bboard check_king, bboard ignore, bboard add); 

void pop_move(game *g); 

// does not check if the move is legal 
void push_move(game *g, move m); 

void gen_moves(const game *g, vector *out); 

int eval(const game *g, int num_moves); 

void print_game(const game *g); 

uint64_t perft(game *g, int depth); 

static inline int col_sign(const game *g) 
{
    return 1 - 2 * g->turn; 
}

// assumes there is a white piece on the square, otherwise returns WP
static inline piece w_pc_at(const game *g, square sq) 
{
    piece p = 0; 
    p |= get_bit(g->pieces[PC_WP], sq) * PC_WP; 
    p |= get_bit(g->pieces[PC_WN], sq) * PC_WN; 
    p |= get_bit(g->pieces[PC_WB], sq) * PC_WB; 
    p |= get_bit(g->pieces[PC_WR], sq) * PC_WR; 
    p |= get_bit(g->pieces[PC_WQ], sq) * PC_WQ; 
    p |= get_bit(g->pieces[PC_WK], sq) * PC_WK; 
    return p; 
}

// assumes there is a black piece on the square, otherwise returns WP
static inline piece b_pc_at(const game *g, square sq) 
{
    piece p = 0; 
    p |= get_bit(g->pieces[PC_BP], sq) * PC_BP; 
    p |= get_bit(g->pieces[PC_BN], sq) * PC_BN; 
    p |= get_bit(g->pieces[PC_BB], sq) * PC_BB; 
    p |= get_bit(g->pieces[PC_BR], sq) * PC_BR; 
    p |= get_bit(g->pieces[PC_BQ], sq) * PC_BQ; 
    p |= get_bit(g->pieces[PC_BK], sq) * PC_BK; 
    return p; 
}

// assumes there is a piece on the square, otherwise returns WP 
static inline piece pc_at(const game *g, square sq) 
{
    return w_pc_at(g, sq) | b_pc_at(g, sq); 
}
