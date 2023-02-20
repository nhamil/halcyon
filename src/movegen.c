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

static inline void gen_p_info(const game *g, TARGET_SQUARE_PARAMS, mvinfo *info) 
{
    piece pc = make_pc(PC_P, g->turn); 
    bboard pcs = g->pieces[pc]; 
    bboard opp = g->colors[opp_col(g->turn)]; 
    bboard empty = ~g->all; 

    square ksq = lsb(g->pieces[make_pc(PC_K, g->turn)]); 

    if (g->turn == COL_W) 
    {
        FOR_EACH_BIT(pcs, 
        {
            bboard pos = make_pos(sq); 
            bboard to = (shift_n(pos) | (shift_nn(pos & RANK_2) & shift_n(empty))) & empty; 
            to |= (shift_ne(pos) | shift_nw(pos)) & opp; 
            to &= TARGET_SQUARES; 

            if (g->ep != NO_SQ && (pos & ATTACKS_P[opp_col(g->turn)][g->ep])) 
            {
                if (!is_attacked_mask_add(g, ksq, g->turn, ~(make_pos(sq) | make_pos(g->ep - 8 * col_sign(g->turn))), make_pos(g->ep))) 
                {
                    to |= make_pos(g->ep); 
                }
            }

            add_moves_to_info(info, pc, sq, to, popcnt(to) + popcnt(to & RANK_8) * 3); 
        });
    }
    else 
    {
        FOR_EACH_BIT(pcs, 
        {
            bboard pos = make_pos(sq); 
            bboard to = (shift_s(pos) | (shift_ss(pos & RANK_7) & shift_s(empty))) & empty; 
            to |= (shift_se(pos) | shift_sw(pos)) & opp; 
            to &= TARGET_SQUARES; 

            if (g->ep != NO_SQ && (pos & ATTACKS_P[opp_col(g->turn)][g->ep])) 
            {
                if (!is_attacked_mask_add(g, ksq, g->turn, ~(make_pos(sq) | make_pos(g->ep - 8 * col_sign(g->turn))), make_pos(g->ep))) 
                {
                    to |= make_pos(g->ep); 
                }
            }

            add_moves_to_info(info, pc, sq, to, popcnt(to) + popcnt(to & RANK_1) * 3); 
        });
    }
}

static inline void gen_k_info(const game *g, mvinfo *info) 
{
    piece pc = make_pc(PC_K, g->turn); 
    bboard pcs = g->pieces[pc]; 
    square ksq = lsb(pcs); 

    color col = g->turn; 

    bboard to = 0; 
    int n_moves = 0; 

    bboard occ = g->all & ~pcs; 

    FOR_EACH_BIT(MOVES_K[ksq] & g->movement, 
    {
        if (!is_attacked_colmask(g, sq, col, occ)) 
        {
            to = set_bit(to, sq); 
            n_moves++; 
        }
    });

    if (g->castle) 
    {
        if ((g->turn == COL_W) && (g->castle & CASTLE_WK) && (g->all & EMPTY_WK) == 0) 
        {
            if (!is_attacked_colmask(g, E1, col, occ) && !is_attacked_colmask(g, F1, col, occ) && !is_attacked_colmask(g, G1, col, occ)) 
            {
                to = set_bit(to, G1); 
                n_moves++; 
            }
        }
        if ((g->turn == COL_W) && (g->castle & CASTLE_WQ) && (g->all & EMPTY_WQ) == 0) 
        {
            if (!is_attacked_colmask(g, E1, col, occ) && !is_attacked_colmask(g, D1, col, occ) && !is_attacked_colmask(g, C1, col, occ)) 
            {
                to = set_bit(to, C1); 
                n_moves++; 
            }
        }
        if ((g->turn == COL_B) && (g->castle & CASTLE_BK) && (g->all & EMPTY_BK) == 0) 
        {
            if (!is_attacked_colmask(g, E8, col, occ) && !is_attacked_colmask(g, F8, col, occ) && !is_attacked_colmask(g, G8, col, occ)) 
            {
                to = set_bit(to, G8); 
                n_moves++; 
            }
        }
        if ((g->turn == COL_B) && (g->castle & CASTLE_BQ) && (g->all & EMPTY_BQ) == 0) 
        {
            if (!is_attacked_colmask(g, E8, col, occ) && !is_attacked_colmask(g, D8, col, occ) && !is_attacked_colmask(g, C8, col, occ)) 
            {
                to = set_bit(to, C8); 
                n_moves++; 
            }
        }
    }
    
    add_moves_to_info(info, pc, ksq, to, n_moves); 
}

static inline void gen_n_info(const game *g, TARGET_SQUARE_PARAMS, mvinfo *info) 
{
    piece pc = make_pc(PC_N, g->turn); 
    bboard pcs = g->pieces[pc]; 

    FOR_EACH_BIT(pcs, 
    {
        bboard to = MOVES_N[sq] & g->movement & TARGET_SQUARES; 
        add_moves_to_info(info, pc, sq, to, popcnt(to)); 
    });
}

static inline void gen_b_info(const game *g, TARGET_SQUARE_PARAMS, mvinfo *info) 
{
    piece pc = make_pc(PC_B, g->turn); 
    bboard pcs = g->pieces[pc]; 

    FOR_EACH_BIT(pcs, 
    {
        bboard to = MAGIC_B_SLIDE[sq][((MAGIC_B_MASK[sq] & g->all) * MAGIC_B[sq]) >> MAGIC_B_SHIFT[sq]];
        to &= g->movement & TARGET_SQUARES; 
        add_moves_to_info(info, pc, sq, to, popcnt(to)); 
    });
}

static inline void gen_r_info(const game *g, TARGET_SQUARE_PARAMS, mvinfo *info) 
{
    piece pc = make_pc(PC_R, g->turn); 
    bboard pcs = g->pieces[pc]; 

    FOR_EACH_BIT(pcs, 
    {
        bboard to = MAGIC_R_SLIDE[sq][((MAGIC_R_MASK[sq] & g->all) * MAGIC_R[sq]) >> MAGIC_R_SHIFT[sq]];
        to &= g->movement & TARGET_SQUARES; 
        add_moves_to_info(info, pc, sq, to, popcnt(to)); 
    });
}

static inline void gen_q_info(const game *g, TARGET_SQUARE_PARAMS, mvinfo *info) 
{
    piece pc = make_pc(PC_Q, g->turn); 
    bboard pcs = g->pieces[pc]; 

    FOR_EACH_BIT(pcs, 
    {
        bboard to = MAGIC_B_SLIDE[sq][((MAGIC_B_MASK[sq] & g->all) * MAGIC_B[sq]) >> MAGIC_B_SHIFT[sq]]
                  | MAGIC_R_SLIDE[sq][((MAGIC_R_MASK[sq] & g->all) * MAGIC_R[sq]) >> MAGIC_R_SHIFT[sq]];
        to &= g->movement & TARGET_SQUARES; 
        add_moves_to_info(info, pc, sq, to, popcnt(to)); 
    });
}

void gen_mvinfo(const game *g, mvinfo *info) 
{
    init_mvinfo(info); 

    // TODO generating extra moves - opposite direction of pin should be allowed to move but is not 
    // TODO en passant, castling 

    color col = g->turn; 
    color opp = opp_col(col); 

    bboard opp_p = g->pieces[make_pc(PC_P, opp)]; 
    bboard opp_n = g->pieces[make_pc(PC_N, opp)]; 
    bboard opp_b = g->pieces[make_pc(PC_B, opp)]; 
    bboard opp_r = g->pieces[make_pc(PC_R, opp)]; 
    bboard opp_q = g->pieces[make_pc(PC_Q, opp)]; 
    bboard opp_k = g->pieces[make_pc(PC_K, opp)]; 


    bboard target = g->pieces[make_pc(PC_K, col)]; 
    bboard ksq = lsb(target); 
    bboard col_occ = g->colors[col]; 

    // relevant opp sliding pieces for each direction
    bboard opp_rq = opp_r | opp_q; 
    bboard opp_bq = opp_b | opp_q; 
    
    // if the king were a rook, how far could it move 
    bboard k_r_hit  = r_attacks(ksq, g->all); 
    // used to remove ally pieces (check for pinned pieces) 
    bboard k_r_no_col = ~(k_r_hit & col_occ); 
    // check if removing one ally piece reveals an attacker 
    bboard k_r_d_hit = r_attacks(ksq, g->all & k_r_no_col) & opp_rq; 

    // do the above for bishop
    bboard k_b_hit  = b_attacks(ksq, g->all); 
    bboard k_b_no_col = ~(k_b_hit & col_occ); 
    bboard k_b_d_hit = b_attacks(ksq, g->all & k_b_no_col) & opp_bq; 

    // sliding piece checkers
    bboard chk_slide = (k_r_hit & opp_rq) | (k_b_hit & opp_bq); 

    // all checkers (remaining pieces can only be captured or force the king to retreat)
    bboard chk = chk_slide 
               | (opp_k & MOVES_K[ksq]) 
               | (opp_n & MOVES_N[ksq]) 
               | (opp_p & ATTACKS_P[col][ksq]); 

    

    bboard dirs[9] = { 0 }; 

    // printf("discoveries\n"); 
    // print_bits(k_r_d_hit | k_b_d_hit); 
    // printf("sliding attackers\n"); 
    // print_bits(chk_slide);

    const uint8_t *p_idx = PIN_IDX[ksq]; 

    FOR_EACH_BIT(k_r_d_hit | k_b_d_hit, 
    {
        dirs[p_idx[sq]] = SLIDE_TO[ksq][sq]; 

        // printf("pinned at %s (index %d)\n", str_sq(sq), p_idx[sq]); 
        // print_bits(SLIDE_TO[ksq][sq]); 
    });

    if (chk == 0) 
    {
        // printf("No checks, only consider pins\n"); 
        gen_p_info(g, ALL_BITS, dirs, p_idx, info); 
        gen_n_info(g, ALL_BITS, dirs, p_idx, info); 
        gen_b_info(g, ALL_BITS, dirs, p_idx, info); 
        gen_r_info(g, ALL_BITS, dirs, p_idx, info); 
        gen_q_info(g, ALL_BITS, dirs, p_idx, info); 
        gen_k_info(g, info); 
    }
    else 
    {
        int n_checks = popcnt(chk); 
        if (n_checks == 1) 
        {
            square attacker = lsb(chk); 
            bboard ok_squares = SLIDE_TO[ksq][attacker] | make_pos(attacker); 
            // printf("1 check (at %s), consider pins and make king safe\n", str_sq(attacker)); 
            // print_bits(ok_squares); 

            // square sq = D1; 
            // bboard tgt = ( ok_squares & ( ((get_bit(dirs[p_idx[sq]], sq) == 0) * ALL_BITS) | dirs[p_idx[sq]] ) ); 
            // printf("tgt\n"); 
            // print_bits(tgt); 

            gen_p_info(g, ok_squares, dirs, p_idx, info); 
            gen_n_info(g, ok_squares, dirs, p_idx, info); 
            gen_b_info(g, ok_squares, dirs, p_idx, info); 
            gen_r_info(g, ok_squares, dirs, p_idx, info); 
            gen_q_info(g, ok_squares, dirs, p_idx, info); 
            gen_k_info(g, info); 
        }
        else 
        {
            // printf("2+ checks, must move king\n"); 
            gen_k_info(g, info); 
        }
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

static inline void gen_p_moves(const game *g, square from, bboard to, square opp_ksq, bboard opp_k, const uint8_t *p_idx, mvlist *moves) 
{
    if (g->turn == COL_W) 
    {
        FOR_EACH_BIT(to & ~RANK_8, 
        {
            moves->moves[moves->size++] = make_move_ep(from, sq, PC_WP, (sq == g->ep) ? PC_BP : pc_at(g, sq), sq == g->ep, (sq == g->ep) ? (
                (ATTACKS_P[COL_W][sq] & opp_k) | 
                disc_attackers(g, opp_ksq, opp_col(g->turn), ~(make_pos(from) | make_pos(sq - 8)), make_pos(sq), p_idx[from]) | 
                disc_attackers(g, opp_ksq, opp_col(g->turn), ~(make_pos(from) | make_pos(sq - 8)), make_pos(sq), p_idx[sq - 8])
            ) : (
                (ATTACKS_P[COL_W][sq] & opp_k) | disc_attackers(g, opp_ksq, opp_col(g->turn), ~make_pos(from), make_pos(sq), p_idx[from])
            )); 
        });
        FOR_EACH_BIT(to & RANK_8, 
        {
            piece at = pc_at(g, sq); 
            bboard disc = disc_attackers(g, opp_ksq, opp_col(g->turn), ~make_pos(from), make_pos(sq), p_idx[from]);
            moves->moves[moves->size++] = make_move_pro(from, sq, PC_WP, PC_WQ, at, ((b_attacks(sq, g->all & ~make_pos(from)) | r_attacks(sq, g->all & ~make_pos(from))) & opp_k) | disc); 
            moves->moves[moves->size++] = make_move_pro(from, sq, PC_WP, PC_WR, at, (r_attacks(sq, g->all & ~make_pos(from)) & opp_k) | disc); 
            moves->moves[moves->size++] = make_move_pro(from, sq, PC_WP, PC_WB, at, (b_attacks(sq, g->all & ~make_pos(from)) & opp_k) | disc); 
            moves->moves[moves->size++] = make_move_pro(from, sq, PC_WP, PC_WN, at, (MOVES_N[sq] & opp_k) | disc); 
        });
    }
    else 
    {
        FOR_EACH_BIT(to & ~RANK_1, 
        {
            moves->moves[moves->size++] = make_move_ep(from, sq, PC_BP, (sq == g->ep) ? PC_WP : pc_at(g, sq), sq == g->ep, (sq == g->ep) ? (
                (ATTACKS_P[COL_B][sq] & opp_k) | 
                disc_attackers(g, opp_ksq, opp_col(g->turn), ~(make_pos(from) | make_pos(sq + 8)), make_pos(sq), p_idx[from]) | 
                disc_attackers(g, opp_ksq, opp_col(g->turn), ~(make_pos(from) | make_pos(sq + 8)), make_pos(sq), p_idx[sq + 8])
            ) : (
                (ATTACKS_P[COL_B][sq] & opp_k) | disc_attackers(g, opp_ksq, opp_col(g->turn), ~make_pos(from), make_pos(sq), p_idx[from])
            )); 
        });
        FOR_EACH_BIT(to & RANK_1, 
        {
            piece at = pc_at(g, sq); 
            bboard disc = disc_attackers(g, opp_ksq, opp_col(g->turn), ~make_pos(from), make_pos(sq), p_idx[from]);
            moves->moves[moves->size++] = make_move_pro(from, sq, PC_BP, PC_BQ, at, ((b_attacks(sq, g->all & ~make_pos(from)) | r_attacks(sq, g->all & ~make_pos(from))) & opp_k) | disc); 
            moves->moves[moves->size++] = make_move_pro(from, sq, PC_BP, PC_BR, at, (r_attacks(sq, g->all & ~make_pos(from)) & opp_k) | disc); 
            moves->moves[moves->size++] = make_move_pro(from, sq, PC_BP, PC_BB, at, (b_attacks(sq, g->all & ~make_pos(from)) & opp_k) | disc); 
            moves->moves[moves->size++] = make_move_pro(from, sq, PC_BP, PC_BN, at, (MOVES_N[sq] & opp_k) | disc); 
        });
    }
}

static inline void gen_k_moves(const game *g, square from, bboard to, square opp_ksq, bboard opp_k, const uint8_t *p_idx, mvlist *moves) 
{
    if (g->turn == COL_W) 
    {
        // if any castle flag is enabled, then the king has not moved 
        bboard castle = to & (((g->castle & CASTLE_W) != 0) * (1ULL << G1 | 1ULL << C1)); 
        to &= ~castle; 

        FOR_EACH_BIT(to, 
        {
            moves->moves[moves->size++] = make_move(from, sq, PC_WK, pc_at(g, sq), disc_attackers(g, opp_ksq, opp_col(g->turn), ~make_pos(from), make_pos(sq), p_idx[from])); 
        });
        FOR_EACH_BIT(castle, 
        {
            switch (sq) 
            {
                case G1: moves->moves[moves->size++] = make_move_castle(E1, G1, PC_WK, MOVE_CASTLE_WK, r_attacks(F1, g->all & ~(1ULL << E1)) & opp_k); break; 
                case C1: moves->moves[moves->size++] = make_move_castle(E1, C1, PC_WK, MOVE_CASTLE_WQ, r_attacks(D1, g->all & ~(1ULL << E1)) & opp_k); break; 
            }
        });
    }
    else 
    {
        // if any castle flag is enabled, then the king has not moved 
        bboard castle = to & (((g->castle & CASTLE_B) != 0) * (1ULL << G8 | 1ULL << C8)); 
        to &= ~castle; 

        FOR_EACH_BIT(to, 
        {
            moves->moves[moves->size++] = make_move(from, sq, PC_BK, pc_at(g, sq), disc_attackers(g, opp_ksq, opp_col(g->turn), ~make_pos(from), make_pos(sq), p_idx[from])); 
        });
        FOR_EACH_BIT(castle, 
        {
            switch (sq) 
            {
                case G8: moves->moves[moves->size++] = make_move_castle(E8, G8, PC_BK, MOVE_CASTLE_BK, r_attacks(F8, g->all & ~(1ULL << E8)) & opp_k); break; 
                case C8: moves->moves[moves->size++] = make_move_castle(E8, C8, PC_BK, MOVE_CASTLE_BQ, r_attacks(D8, g->all & ~(1ULL << E8)) & opp_k); break; 
            }
        });
    }
}

#define GEN_MOVES(atk) \
    FOR_EACH_BIT(to, \
    {\
        moves->moves[moves->size++] = make_move(from, sq, pc, pc_at(g, sq), (atk) | disc_attackers(g, opp_ksq, opp_col(g->turn), ~make_pos(from), make_pos(sq), p_idx[from])); \
    });

void gen_moves_mvinfo(const game *g, const mvinfo *info, mvlist *moves) 
{
    // size_t start = moves->size; 

    bboard opp_k = g->pieces[make_pc(PC_K, opp_col(g->turn))]; 
    square opp_ksq = lsb(opp_k); 
    const uint8_t *p_idx = PIN_IDX[opp_ksq]; 

    for (int pc_id = 0; pc_id < info->n_pieces; pc_id++) 
    {
        bboard to = info->moves[pc_id]; 
        piece pc = info->pieces[pc_id]; 
        piece type = get_no_col(pc); 
        square from = info->from[pc_id]; 

        switch (type) 
        {
            case PC_P: gen_p_moves(g, from, info->moves[pc_id], opp_ksq, opp_k, p_idx, moves); break; 
            case PC_K: gen_k_moves(g, from, info->moves[pc_id], opp_ksq, opp_k, p_idx, moves); break; 
            case PC_N: GEN_MOVES(MOVES_N[sq] & opp_k); break; 
            case PC_B: GEN_MOVES(b_attacks(sq, g->all & ~make_pos(from)) & opp_k); break; 
            case PC_R: GEN_MOVES(r_attacks(sq, g->all & ~make_pos(from)) & opp_k); break; 
            case PC_Q: GEN_MOVES((b_attacks(sq, g->all & ~make_pos(from)) | r_attacks(sq, g->all & ~make_pos(from))) & opp_k); break; 
        }
    }

    // int n_moves = (int) (moves->size - start); 
    // if (n_moves != info->n_moves) 
    // {
    //     printf("Move count mismatch: %d vs %d\n", (int) n_moves, (int) info->n_moves); 
    // }
} 
