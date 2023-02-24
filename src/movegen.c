#include "movegen.h" 
#include "bitboard.h"
#include "game.h"
#include "magic.h" 
#include "move.h"
#include "piece.h"

#include <stdio.h> 
#include <stdlib.h> 

#define TARGET_SQUARE_PARAMS bboard ok_squares, bboard *pin_dirs, const uint8_t *pin_idx
#define TARGET_SQUARES ( ok_squares & ( ((get_bit(pin_dirs[pin_idx[sq]], sq) == 0) * ALL_BITS) | pin_dirs[pin_idx[sq]] ) )

mvlist *new_mvlist(void) 
{
    mvlist *moves = malloc(sizeof(mvlist)); 
    moves->size = 0; 
    return moves; 
} 

void free_mvlist(mvlist *moves) 
{
    free(moves); 
}

static inline void init_mvlist(mvlist *moves) 
{
    moves->size = 0; 
}

static inline void init_mvinfo(mvinfo *info) 
{
    info->n_pieces = info->n_moves = 0; 
}

static inline void add_moves_to_info(mvinfo *info, piece pc, square from, bboard moves, int n_moves) 
{
    info->from[info->n_pieces] = from; 
    info->pieces[info->n_pieces] = pc; 
    info->moves[info->n_pieces++] = moves; 
    info->n_moves += n_moves; 
}

/**
 * Applies the mask and then adds any extra bits. 
 * `mask` will remove attackers from being considered 
 * `add` will only add blockers, they have no piece type or color 
 */
static inline bool is_attacked_mask_add(const game *g, square sq, color chk_col, bboard mask, bboard add) 
{
    color col = chk_col; 
    color opp = opp_col(col); 

    bboard occ = (g->all & mask) | add; 

    // b,r,q are not used directly 
    // applying mask to the aggregate bboards is 1 fewer "&" 

    bboard opp_p = g->pieces[make_pc(PC_P, opp)] & mask; 
    bboard opp_n = g->pieces[make_pc(PC_N, opp)] & mask; 
    bboard opp_b = g->pieces[make_pc(PC_B, opp)]; 
    bboard opp_r = g->pieces[make_pc(PC_R, opp)]; 
    bboard opp_q = g->pieces[make_pc(PC_Q, opp)]; 
    bboard opp_k = g->pieces[make_pc(PC_K, opp)] & mask; 

    bboard opp_rq = (opp_r | opp_q) & mask; 
    bboard opp_bq = (opp_b | opp_q) & mask; 

    bboard chk = (r_attacks(sq, occ) & opp_rq) 
               | (b_attacks(sq, occ) & opp_bq) 
               | (opp_k & MOVES_K[sq]) 
               | (opp_n & MOVES_N[sq]) 
               | (opp_p & ATTACKS_P[col][sq]);     

    return chk != 0; 
}

/**
 * Applies the mask and then adds any extra bits. 
 * `mask` will remove attackers from being considered 
 */
static inline bool is_attacked_mask(const game *g, square sq, color chk_col, bboard mask) 
{
    color col = chk_col; 
    color opp = opp_col(col); 

    bboard occ = (g->all & mask); 

    // b,r,q are not used directly 
    // applying mask to the aggregate bboards is 1 fewer "&" 

    bboard opp_p = g->pieces[make_pc(PC_P, opp)] & mask; 
    bboard opp_n = g->pieces[make_pc(PC_N, opp)] & mask; 
    bboard opp_b = g->pieces[make_pc(PC_B, opp)]; 
    bboard opp_r = g->pieces[make_pc(PC_R, opp)]; 
    bboard opp_q = g->pieces[make_pc(PC_Q, opp)]; 
    bboard opp_k = g->pieces[make_pc(PC_K, opp)] & mask; 

    bboard opp_rq = (opp_r | opp_q) & mask; 
    bboard opp_bq = (opp_b | opp_q) & mask; 

    bboard chk = (r_attacks(sq, occ) & opp_rq) 
               | (b_attacks(sq, occ) & opp_bq) 
               | (opp_k & MOVES_K[sq]) 
               | (opp_n & MOVES_N[sq]) 
               | (opp_p & ATTACKS_P[col][sq]);     

    return chk != 0; 
}

/**
 * Applies the mask and then checks if the square is attacked. 
 * `mask` will ONLY remove pieces from the aggregate bboard, those squares can still be included in attackers  
 */
static inline bool is_attacked_colmask(const game *g, square sq, color chk_col, bboard mask) 
{
    color col = chk_col; 
    color opp = opp_col(col); 

    bboard occ = (g->all & mask); 

    // b,r,q are not used directly 
    // applying mask to the aggregate bboards is 1 fewer "&" 

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

#define FN_GEN_P_INFO(col, shift1, shift2, shift2_rank, pro_rank) \
    piece pc = make_pc(PC_P, col); \
    bboard pcs = g->pieces[pc]; \
    bboard opp = g->colors[opp_col(col)]; \
    bboard empty = ~g->all; \
    square ksq = lsb(g->pieces[make_pc(PC_K, col)]); \
    FOR_EACH_BIT(pcs, \
    {\
        bboard pos = make_pos(sq); \
        bboard to = (shift1(pos) | (shift2(pos & shift2_rank) & shift1(empty))) & empty; \
        to |= ATTACKS_P[col][sq] & opp; \
        to &= TARGET_SQUARES; \
        if (g->ep != NO_SQ && (pos & ATTACKS_P[opp_col(col)][g->ep])) \
        {\
            if (!is_attacked_mask_add(g, ksq, col, ~(make_pos(sq) | make_pos(g->ep - 8 * col_sign(col))), make_pos(g->ep))) \
            {\
                to |= make_pos(g->ep); \
            }\
        }\
        add_moves_to_info(info, pc, sq, to, popcnt(to) + popcnt(to & pro_rank) * 3); \
    });\

static inline void gen_wp_info(const game *g, TARGET_SQUARE_PARAMS, mvinfo *info) 
{
    FN_GEN_P_INFO(COL_W, shift_n, shift_nn, RANK_2, RANK_8); 
}

static inline void gen_bp_info(const game *g, TARGET_SQUARE_PARAMS, mvinfo *info) 
{
    FN_GEN_P_INFO(COL_B, shift_s, shift_ss, RANK_7, RANK_1); 
}

#define FN_GEN_K_INFO(col, letter, rank) \
    piece pc = make_pc(PC_K, col); \
    bboard pcs = g->pieces[pc]; \
    square ksq = lsb(pcs); \
    bboard to = 0; \
    int n_moves = 0; \
    bboard occ = g->all & ~pcs; \
    FOR_EACH_BIT(MOVES_K[ksq] & g->movement, \
    {\
        if (!is_attacked_colmask(g, sq, col, occ)) \
        {\
            to = set_bit(to, sq); \
            n_moves++; \
        }\
    });\
    if (((g->castle & CASTLE_##letter##K) != 0) & ((g->all & EMPTY_##letter##K) == 0)) \
    {\
        if (!is_attacked_colmask(g, E##rank, col, occ) && !is_attacked_colmask(g, F##rank, col, occ) && !is_attacked_colmask(g, G##rank, col, occ)) \
        {\
            to = set_bit(to, G##rank); \
            n_moves++; \
        }\
    }\
    if (((g->castle & CASTLE_##letter##Q) != 0) & ((g->all & EMPTY_##letter##Q) == 0)) \
    {\
        if (!is_attacked_colmask(g, E##rank, col, occ) && !is_attacked_colmask(g, D##rank, col, occ) && !is_attacked_colmask(g, C##rank, col, occ)) \
        {\
            to = set_bit(to, C##rank); \
            n_moves++; \
        }\
    }\
    add_moves_to_info(info, pc, ksq, to, n_moves); 

static inline void gen_wk_info(const game *g, mvinfo *info) 
{
    FN_GEN_K_INFO(COL_W, W, 1); 
}

static inline void gen_bk_info(const game *g, mvinfo *info) 
{
    FN_GEN_K_INFO(COL_B, B, 8); 
}

#define FN_GEN_N_INFO(col) \
    piece pc = make_pc(PC_N, col); \
    bboard pcs = g->pieces[pc]; \
    FOR_EACH_BIT(pcs, \
    {\
        bboard to = MOVES_N[sq] & g->movement & TARGET_SQUARES; \
        add_moves_to_info(info, pc, sq, to, popcnt(to)); \
    });

static inline void gen_wn_info(const game *g, TARGET_SQUARE_PARAMS, mvinfo *info) 
{
    FN_GEN_N_INFO(COL_W); 
}

static inline void gen_bn_info(const game *g, TARGET_SQUARE_PARAMS, mvinfo *info) 
{
    FN_GEN_N_INFO(COL_B); 
}

#define FN_GEN_B_INFO(col) \
    piece pc = make_pc(PC_B, col); \
    bboard pcs = g->pieces[pc]; \
    FOR_EACH_BIT(pcs, \
    {\
        bboard to = MAGIC_B_SLIDE[sq][((MAGIC_B_MASK[sq] & g->all) * MAGIC_B[sq]) >> MAGIC_B_SHIFT[sq]]; \
        to &= g->movement & TARGET_SQUARES; \
        add_moves_to_info(info, pc, sq, to, popcnt(to)); \
    });

static inline void gen_wb_info(const game *g, TARGET_SQUARE_PARAMS, mvinfo *info) 
{
    FN_GEN_B_INFO(COL_W); 
}

static inline void gen_bb_info(const game *g, TARGET_SQUARE_PARAMS, mvinfo *info) 
{
    FN_GEN_B_INFO(COL_B); 
}

#define FN_GEN_R_INFO(col) \
    piece pc = make_pc(PC_R, col); \
    bboard pcs = g->pieces[pc]; \
    FOR_EACH_BIT(pcs, \
    {\
        bboard to = MAGIC_R_SLIDE[sq][((MAGIC_R_MASK[sq] & g->all) * MAGIC_R[sq]) >> MAGIC_R_SHIFT[sq]]; \
        to &= g->movement & TARGET_SQUARES; \
        add_moves_to_info(info, pc, sq, to, popcnt(to)); \
    });

static inline void gen_wr_info(const game *g, TARGET_SQUARE_PARAMS, mvinfo *info) 
{
    FN_GEN_R_INFO(COL_W); 
}

static inline void gen_br_info(const game *g, TARGET_SQUARE_PARAMS, mvinfo *info) 
{
    FN_GEN_R_INFO(COL_B); 
}

#define FN_GEN_Q_INFO(col) \
    piece pc = make_pc(PC_Q, col); \
    bboard pcs = g->pieces[pc]; \
    FOR_EACH_BIT(pcs, \
    {\
        bboard to = MAGIC_B_SLIDE[sq][((MAGIC_B_MASK[sq] & g->all) * MAGIC_B[sq]) >> MAGIC_B_SHIFT[sq]] \
                  | MAGIC_R_SLIDE[sq][((MAGIC_R_MASK[sq] & g->all) * MAGIC_R[sq]) >> MAGIC_R_SHIFT[sq]]; \
        to &= g->movement & TARGET_SQUARES; \
        add_moves_to_info(info, pc, sq, to, popcnt(to)); \
    });

static inline void gen_wq_info(const game *g, TARGET_SQUARE_PARAMS, mvinfo *info) 
{
    FN_GEN_Q_INFO(COL_W); 
}

static inline void gen_bq_info(const game *g, TARGET_SQUARE_PARAMS, mvinfo *info) 
{
    FN_GEN_Q_INFO(COL_B); 
}

// void gen_mvinfo(const game *g, mvinfo *info) 
#define FN_GEN_MVINFO(col, letter) \
{\
    init_mvinfo(info); \
    color opp = opp_col(col); \
    bboard opp_p = g->pieces[make_pc(PC_P, opp)]; \
    bboard opp_n = g->pieces[make_pc(PC_N, opp)]; \
    bboard opp_b = g->pieces[make_pc(PC_B, opp)]; \
    bboard opp_r = g->pieces[make_pc(PC_R, opp)]; \
    bboard opp_q = g->pieces[make_pc(PC_Q, opp)]; \
    bboard opp_k = g->pieces[make_pc(PC_K, opp)]; \
    bboard target = g->pieces[make_pc(PC_K, col)]; \
    bboard ksq = lsb(target); \
    bboard col_occ = g->colors[col]; \
    /* relevant opp sliding pieces for each direction */ \
    bboard opp_rq = opp_r | opp_q; \
    bboard opp_bq = opp_b | opp_q; \
    /* if the king were a rook, how far could it move */ \
    bboard k_r_hit  = r_attacks(ksq, g->all); \
    /* used to remove ally pieces (check for pinned pieces) */ \
    bboard k_r_no_col = ~(k_r_hit & col_occ); \
    /* check if removing one ally piece reveals an attacker */ \
    bboard k_r_d_hit = r_attacks(ksq, g->all & k_r_no_col) & opp_rq; \
    /* do the above for bishop*/ \
    bboard k_b_hit  = b_attacks(ksq, g->all); \
    bboard k_b_no_col = ~(k_b_hit & col_occ); \
    bboard k_b_d_hit = b_attacks(ksq, g->all & k_b_no_col) & opp_bq; \
    /* sliding piece checkers*/ \
    bboard chk_slide = (k_r_hit & opp_rq) | (k_b_hit & opp_bq); \
    /* all checkers (remaining pieces can only be captured or force the king to retreat)*/ \
    bboard chk = chk_slide \
               | (opp_k & MOVES_K[ksq]) \
               | (opp_n & MOVES_N[ksq]) \
               | (opp_p & ATTACKS_P[col][ksq]); \
    bboard dirs[9] = { 0 }; \
    const uint8_t *p_idx = PIN_IDX[ksq]; \
    FOR_EACH_BIT(k_r_d_hit | k_b_d_hit, \
    {\
        dirs[p_idx[sq]] = SLIDE_TO[ksq][sq]; \
    });\
    if (chk == 0) \
    {\
        gen_##letter##p_info(g, ALL_BITS, dirs, p_idx, info); \
        gen_##letter##n_info(g, ALL_BITS, dirs, p_idx, info); \
        gen_##letter##b_info(g, ALL_BITS, dirs, p_idx, info); \
        gen_##letter##r_info(g, ALL_BITS, dirs, p_idx, info); \
        gen_##letter##q_info(g, ALL_BITS, dirs, p_idx, info); \
        gen_##letter##k_info(g, info); \
    }\
    else \
    {\
        int n_checks = popcnt(chk); \
        if (n_checks == 1) \
        {\
            square attacker = lsb(chk); \
            bboard ok_squares = SLIDE_TO[ksq][attacker] | make_pos(attacker); \
            gen_##letter##p_info(g, ok_squares, dirs, p_idx, info); \
            gen_##letter##n_info(g, ok_squares, dirs, p_idx, info); \
            gen_##letter##b_info(g, ok_squares, dirs, p_idx, info); \
            gen_##letter##r_info(g, ok_squares, dirs, p_idx, info); \
            gen_##letter##q_info(g, ok_squares, dirs, p_idx, info); \
            gen_##letter##k_info(g, info); \
        }\
        else \
        {\
            gen_##letter##k_info(g, info); \
        }\
    }\
}

void gen_mvinfo(const game *g, mvinfo *info) 
{
    if (g->turn == COL_W) 
    {
        FN_GEN_MVINFO(COL_W, w); 
    }
    else 
    {
        FN_GEN_MVINFO(COL_B, b); 
    }
}

static inline bboard disc_r_attackers(const game *g, square sq, color chk_col, bboard mask, bboard add) 
{
    color opp = opp_col(chk_col); 

    bboard occ = (g->all & mask) | add; 
    bboard opp_r = g->pieces[make_pc(PC_R, opp)]; 
    bboard opp_q = g->pieces[make_pc(PC_Q, opp)]; 
    bboard opp_rq = (opp_r | opp_q) & mask; 

    return r_attacks(sq, occ) & opp_rq; 
}

static inline bboard disc_b_attackers(const game *g, square sq, color chk_col, bboard mask, bboard add) 
{
    color opp = opp_col(chk_col); 

    bboard occ = (g->all & mask) | add; 
    bboard opp_b = g->pieces[make_pc(PC_B, opp)]; 
    bboard opp_q = g->pieces[make_pc(PC_Q, opp)]; 
    bboard opp_bq = (opp_b | opp_q) & mask; 

    return b_attacks(sq, occ) & opp_bq; 
}

static inline bboard disc_attackers(const game *g, square sq, color chk_col, bboard mask, bboard add, int p_idx) 
{
    if (p_idx == CHECK_DIR_CNT) 
    {
        return 0; 
    }
    else if (p_idx <= CHECK_DIR_R_END) 
    {
        return disc_r_attackers(g, sq, chk_col, mask, add); 
    }
    else 
    {
        return disc_b_attackers(g, sq, chk_col, mask, add); 
    }
}

#define DISC_ATTACKERS(col) (disc_attackers(g, opp_ksq, opp_col(col), ~make_pos(from), make_pos(sq), p_idx[from]))
#define CHECK_N (MOVES_N[sq] & opp_k)
#define CHECK_B (b_attacks(sq, g->all & ~make_pos(from)) & opp_k)
#define CHECK_R (r_attacks(sq, g->all & ~make_pos(from)) & opp_k)
#define CHECK_Q ((b_attacks(sq, g->all & ~make_pos(from)) | r_attacks(sq, g->all & ~make_pos(from))) & opp_k)

#define FN_GEN_P_MOVES(col, pro_rank, col_letter, opp_letter) \
{\
    FOR_EACH_BIT(to & ~pro_rank, \
    {\
        moves->moves[moves->size++] = make_move_ep(from, sq, PC_##col_letter##P, (sq == g->ep) ? PC_##opp_letter##P : pc_at(g, sq), sq == g->ep, (sq == g->ep) ? (\
            (ATTACKS_P[COL_W][sq] & opp_k) | \
            disc_attackers(g, opp_ksq, opp_col(col), ~(make_pos(from) | make_pos(sq - 8 * col_sign(col))), make_pos(sq), p_idx[from]) | \
            disc_attackers(g, opp_ksq, opp_col(col), ~(make_pos(from) | make_pos(sq - 8 * col_sign(col))), make_pos(sq), p_idx[sq - 8 * col_sign(col)]) \
        ) : (\
            (ATTACKS_P[COL_W][sq] & opp_k) | DISC_ATTACKERS(col) \
        )); \
    });\
    FOR_EACH_BIT(to & pro_rank, \
    {\
        piece at = pc_at(g, sq); \
        bboard disc = disc_attackers(g, opp_ksq, opp_col(col), ~make_pos(from), make_pos(sq), p_idx[from]);\
        moves->moves[moves->size++] = make_move_pro(from, sq, PC_##col_letter##P, PC_##col_letter##Q, at, (CHECK_Q) | disc); \
        moves->moves[moves->size++] = make_move_pro(from, sq, PC_##col_letter##P, PC_##col_letter##R, at, (CHECK_R) | disc); \
        moves->moves[moves->size++] = make_move_pro(from, sq, PC_##col_letter##P, PC_##col_letter##B, at, (CHECK_B) | disc); \
        moves->moves[moves->size++] = make_move_pro(from, sq, PC_##col_letter##P, PC_##col_letter##N, at, (CHECK_N) | disc); \
    });\
}

static inline void gen_wp_moves(const game *g, square from, bboard to, square opp_ksq, bboard opp_k, const uint8_t *p_idx, mvlist *moves) 
{
    FN_GEN_P_MOVES(COL_W, RANK_8, W, B); 
}

static inline void gen_bp_moves(const game *g, square from, bboard to, square opp_ksq, bboard opp_k, const uint8_t *p_idx, mvlist *moves) 
{
    FN_GEN_P_MOVES(COL_B, RANK_1, B, W); 
}

#define FN_GEN_K_MOVES(col, letter, rank) \
    /* if any castle flag is enabled, then the king has not moved */ \
    bboard castle = to & (((g->castle & CASTLE_##letter) != 0) * (1ULL << G##rank | 1ULL << C##rank)); \
    to &= ~castle; \
    FOR_EACH_BIT(to, \
    {\
        moves->moves[moves->size++] = make_move(from, sq, PC_##letter##K, pc_at(g, sq), disc_attackers(g, opp_ksq, opp_col(g->turn), ~make_pos(from), make_pos(sq), p_idx[from])); \
    });\
    FOR_EACH_BIT(castle, \
    {\
        switch (sq) \
        {\
            case G##rank: moves->moves[moves->size++] = make_move_castle(E##rank, G##rank, PC_##letter##K, MOVE_CASTLE_##letter##K, r_attacks(F##rank, g->all & ~(1ULL << E##rank)) & opp_k); break; \
            case C##rank: moves->moves[moves->size++] = make_move_castle(E##rank, C##rank, PC_##letter##K, MOVE_CASTLE_##letter##Q, r_attacks(D##rank, g->all & ~(1ULL << E##rank)) & opp_k); break; \
        }\
    });\

static inline void gen_wk_moves(const game *g, square from, bboard to, square opp_ksq, bboard opp_k, const uint8_t *p_idx, mvlist *moves) 
{
    FN_GEN_K_MOVES(COL_W, W, 1); 
}

static inline void gen_bk_moves(const game *g, square from, bboard to, square opp_ksq, bboard opp_k, const uint8_t *p_idx, mvlist *moves) 
{
    FN_GEN_K_MOVES(COL_B, B, 8); 
}

#define GEN_MOVES(col, atk) \
    FOR_EACH_BIT(to, \
    {\
        moves->moves[moves->size++] = make_move(from, sq, pc, pc_at(g, sq), (atk) | DISC_ATTACKERS(col)); \
    });

#define FN_GEN_MOVES(col, letter) \
{\
    bboard opp_k = g->pieces[make_pc(PC_K, opp_col(col))]; \
    square opp_ksq = lsb(opp_k); \
    const uint8_t *p_idx = PIN_IDX[opp_ksq]; \
    for (int pc_id = 0; pc_id < info->n_pieces; pc_id++) \
    {\
        bboard to = info->moves[pc_id]; \
        piece pc = info->pieces[pc_id]; \
        piece type = get_no_col(pc); \
        square from = info->from[pc_id]; \
        switch (type) \
        {\
            case PC_P: gen_##letter##p_moves(g, from, info->moves[pc_id], opp_ksq, opp_k, p_idx, moves); break; \
            case PC_K: gen_##letter##k_moves(g, from, info->moves[pc_id], opp_ksq, opp_k, p_idx, moves); break; \
            case PC_N: GEN_MOVES(col, CHECK_N); break; \
            case PC_B: GEN_MOVES(col, CHECK_B); break; \
            case PC_R: GEN_MOVES(col, CHECK_R); break; \
            case PC_Q: GEN_MOVES(col, CHECK_Q); break; \
        }\
    }\
} 

void gen_moves_mvinfo(const game *g, const mvinfo *info, mvlist *moves) 
{
    if (g->turn == COL_W) 
    {
        FN_GEN_MOVES(COL_W, w); 
    }
    else 
    {
        FN_GEN_MOVES(COL_B, b); 
    }
}