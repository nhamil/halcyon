#pragma once 

#include <inttypes.h>
#include <limits.h> 
#include <stdbool.h>
#include <stddef.h> 
#include <stdint.h>
#include <string.h> 

#include "bitboard.h"
#include "castle.h" 
#include "magic.h" 
#include "mailbox.h"
#include "move.h" 
#include "piece.h"
#include "square.h"
#include "vector.h"
#include "zobrist.h"

typedef struct game game; 
typedef struct move_hist move_hist; 

#define DRAW_DEPTH 4

#define MAX_DEPTH 256 

#define EVAL_MAX (INT_MAX - 10000)

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#define FEN_LEN 128 

struct move_hist 
{
    int halfmove; 
    square ep; 
    castle_flags castle; 
    bool in_check;
    zobrist hash; 
};

struct game 
{
    move_hist hist[MAX_DEPTH + DRAW_DEPTH*2]; 

    mbox mailbox; 

    bboard pieces[PC_CNT + 1]; // extra bitboard for NO_PC
    bboard colors[COL_CNT]; 
    bboard all; 
    bboard movement; 

    zobrist hash; 
    castle_flags castle; 
    square ep; 

    int ply; 
    int halfmove; 
    color turn; 
    bool in_check; 
    int depth; 

    uint64_t nodes; 
};

game *new_game(void); 

void free_game(game *g); 

void reset_game(game *g); 

void copy_game(game *g, const game *from); 

void load_fen(game *g, const char *fen); 

void to_fen(const game *g, char *out); 

void push_move(game *g, move m); 

void pop_move(game *g, move m); 

void push_null_move(game *g); 

void pop_null_move(game *g); 

void print_game(const game *g); 

uint64_t perft(game *g, int depth); 

bool is_special_draw(const game *g);

int evaluate(const game *g, int n_moves, bool draw); 

static inline piece pc_at(const game *g, square sq) 
{
    return get_mbox(&g->mailbox, sq); 
}

static inline bool any_side_k_p(const game *g) 
{
    bool w_kp = g->colors[COL_W] == (g->pieces[PC_WK] | g->pieces[PC_WP]); 
    bool b_kp = g->colors[COL_B] == (g->pieces[PC_BK] | g->pieces[PC_BP]); 
    return w_kp | b_kp;  
}

/**
 * Removes move history to increase max depth. 
 * Do not use pop_move on previous moves after calling this. 
 */
static inline void no_depth(game *g) 
{
    int copy_amt = DRAW_DEPTH * 2; 
    if (g->depth < copy_amt) copy_amt = g->depth; 

    if (copy_amt > 0) 
    {
        int copy_start = g->depth - copy_amt; 

        for (int i = 0; i < copy_amt; i++) 
        {
            g->hist[i] = g->hist[copy_start + i]; 
        }
    }

    g->depth = copy_amt; 
}

static inline bboard r_attacks(square sq, bboard occ) 
{
    return MAGIC_R_SLIDE[sq][((MAGIC_R_MASK[sq] & occ) * MAGIC_R[sq]) >> MAGIC_R_SHIFT[sq]];
}

static inline bboard b_attacks(square sq, bboard occ) 
{
    return MAGIC_B_SLIDE[sq][((MAGIC_B_MASK[sq] & occ) * MAGIC_B[sq]) >> MAGIC_B_SHIFT[sq]];
}

static inline bool is_attacked(const game *g, square sq, color chk_col) 
{
    color col = chk_col; 
    color opp = opp_col(col); 

    bboard occ = g->all; 

    bboard opp_p = g->pieces[make_pc(PC_P, opp)]; 
    bboard opp_n = g->pieces[make_pc(PC_N, opp)]; 
    bboard opp_b = g->pieces[make_pc(PC_B, opp)]; 
    bboard opp_r = g->pieces[make_pc(PC_R, opp)]; 
    bboard opp_q = g->pieces[make_pc(PC_Q, opp)]; 
    bboard opp_k = g->pieces[make_pc(PC_K, opp)]; 

    bboard opp_rq = (opp_r | opp_q); 
    bboard opp_bq = (opp_b | opp_q); 

    bboard chk = (r_attacks(sq, occ) & opp_rq) 
               | (b_attacks(sq, occ) & opp_bq) 
               | (opp_k & MOVES_K[sq]) 
               | (opp_n & MOVES_N[sq]) 
               | (opp_p & ATTACKS_P[col][sq]);     

    return chk != 0; 
}

extern int PC_SQ[2][PC_TYPE_CNT][SQ_CNT]; 
