#pragma once 

#include <inttypes.h>
#include <limits.h> 
#include <stdbool.h>
#include <stddef.h> 
#include <string.h> 

#include "bitboard.h"
#include "castle.h" 
#include "move.h" 
#include "piece.h"
#include "square.h"
#include "vector.h"
#include "zobrist.h"

typedef struct game game; 

#define EVAL_MAX (INT_MAX - 10000)

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#define FEN_LEN 128 

struct game 
{
    bboard pieces[PC_CNT]; 
    bboard colors[2]; 
    bboard check; 
    zobrist hash; 
    castle_flags castle; 
    square ep; 
    bool in_check; 
    int halfmove; 

    vector hist; 
    int ply; 
    color turn; 

    uint64_t nodes; 
};

void create_game(game *g); 

void create_game_fen(game *g, const char *fen); 

void create_game_copy(game *g, const game *from_game); 

void destroy_game(game *g); 

void reset_game(game *g); 

void copy_game(game *g, const game *from_game); 

void load_fen(game *g, const char *fen); 

void to_fen(const game *g, char *out); 

bool in_check(const game *g, color for_color, bboard check_king, bboard ignore, bboard add); 

void pop_move(game *g); 

// does not check if the move is legal 
void push_move(game *g, move m); 

void pop_null_move(game *g); 

void push_null_move(game *g); 

void gen_moves(const game *g, vector *out); 

int evaluate(const game *g, int num_moves, bool draw); 

void print_game(const game *g); 

uint64_t perft(game *g, int depth); 

bool is_special_draw(const game *g); 

move alg_to_move(const game *g, const char *alg); 

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
static inline piece pc_at_or_wp(const game *g, square sq) 
{
    return w_pc_at(g, sq) | b_pc_at(g, sq); 
}

static inline bool is_pc_at(const game *g, square sq) 
{
    return get_bit(g->colors[COL_W], sq) | get_bit(g->colors[COL_B], sq); 
}

static inline bool no_pc_at(const game *g, square sq) 
{
    return !is_pc_at(g, sq); 
}

static inline bool is_capture(const game *g, move mv) 
{
    return takes_ep(mv) | get_bit(g->colors[opp_col(g->turn)], to_sq(mv)); 
}

static inline bool is_quiet(const game *g, move mv) 
{
    return !is_capture(g, mv); 
}

static inline bool any_side_k_p(const game *g) 
{
    const bool w_kp = g->colors[COL_W] == (g->pieces[PC_WK] | g->pieces[PC_WP]); 
    const bool b_kp = g->colors[COL_B] == (g->pieces[PC_BK] | g->pieces[PC_BP]); 
    return w_kp | b_kp;  
}

extern int PC_SQ[2][6][SQ_CNT]; 