#pragma once 

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h> 
#include <string.h> 

#include "bboard.h"
#include "castle.h" 
#include "move.h" 
#include "piece.h"
#include "square.h"
#include "vector.h"

typedef struct game game; 

/*
 * Of all squares that can be legally moved to, select all that either: 
 * ..capture/block an attacker
 * ..or: 
 * ..anywhere, if: 
 * ....there is no pin on the direction from this square to the king 
 * ....or: 
 * ....this square is not in between an attacker and the king (past the attacker) 
 * ....and therefore is not pinned to this square
 */
#define GM_TGT_SQS ( \
    ok_sqs & ( \
        dirs[pin_idx[s]] | (BB_ALL * \
            ( \
                ((~pinned >> pin_idx[s]) & 1) | \
                ((dirs[pin_idx[s]] & (1ULL << s)) == 0) \
            ) \
        ) \
    ) \
)

#define GM_START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

struct game 
{
    bboard pcs[PC_CNT]; 
    bboard col[2]; 
    bboard chk; 
    castle cas; 
    square ep; 
    bool in_chk; 

    vector hist; 
    int ply; 
    color turn; 
};

static void gm_reset(game *g) 
{
    vec_clear(&g->hist); 
    memset(&g->pcs, 0, sizeof(g->pcs)); 
    memset(&g->col, 0, sizeof(g->col)); 
    g->chk = BB_NONE; 
    g->cas = CAS_NONE; 
    g->ep = SQ_NONE; 
    g->in_chk = false; 
    g->ply = 0; 
    g->turn = COL_W; 
}

static void gm_create(game *g) 
{
    vec_create(&g->hist, offsetof(game, hist)); 
    gm_reset(g); 
}

static void gm_destroy(game *g) 
{
    vec_destroy(&g->hist); 
}

static void gm_load_fen(game *g, const char *fen) 
{
    gm_reset(g); 

    // decrement once so simple incrementing while loop 
    // starts at the first character
    fen--; 

    int mode = 0, turn = 0; 
    square sq = SQ_A1; 
    while (*++fen) 
    {
        if (*fen == ' ') 
        {
            mode++; 
            sq = SQ_A1; 
            continue; 
        }
        switch (mode) 
        {
            case 0: // board setup 
                switch (*fen) 
                {
                    case 'P': 
                        g->pcs[PC_WP] = bb_set(g->pcs[PC_WP], sq_rrank(sq++)); 
                        break; 
                    case 'p': 
                        g->pcs[PC_BP] = bb_set(g->pcs[PC_BP], sq_rrank(sq++)); 
                        break; 
                    case 'N': 
                        g->pcs[PC_WN] = bb_set(g->pcs[PC_WN], sq_rrank(sq++)); 
                        break; 
                    case 'n': 
                        g->pcs[PC_BN] = bb_set(g->pcs[PC_BN], sq_rrank(sq++)); 
                        break; 
                    case 'B': 
                        g->pcs[PC_WB] = bb_set(g->pcs[PC_WB], sq_rrank(sq++)); 
                        break; 
                    case 'b': 
                        g->pcs[PC_BB] = bb_set(g->pcs[PC_BB], sq_rrank(sq++)); 
                        break; 
                    case 'R': 
                        g->pcs[PC_WR] = bb_set(g->pcs[PC_WR], sq_rrank(sq++)); 
                        break; 
                    case 'r': 
                        g->pcs[PC_BR] = bb_set(g->pcs[PC_BR], sq_rrank(sq++)); 
                        break; 
                    case 'Q': 
                        g->pcs[PC_WQ] = bb_set(g->pcs[PC_WQ], sq_rrank(sq++)); 
                        break; 
                    case 'q': 
                        g->pcs[PC_BQ] = bb_set(g->pcs[PC_BQ], sq_rrank(sq++)); 
                        break; 
                    case 'K': 
                        g->pcs[PC_WK] = bb_set(g->pcs[PC_WK], sq_rrank(sq++)); 
                        break; 
                    case 'k': 
                        g->pcs[PC_BK] = bb_set(g->pcs[PC_BK], sq_rrank(sq++)); 
                        break; 
                    case '8': 
                    case '7': 
                    case '6': 
                    case '5': 
                    case '4': 
                    case '3': 
                    case '2': 
                    case '1': 
                        sq += *fen - '0'; 
                        break; 
                    case '/': 
                        break; 
                    default: 
                        printf("Unknown FEN character (board setup): %c\n", *fen); 
                        break; 
                }
                break; 
            case 1: // color to play 
                switch (*fen) 
                {
                    case 'w': 
                        g->ply = 0; 
                        break; 
                    case 'b': 
                        g->ply = 1; 
                        break; 
                    default: 
                        printf("Unknown FEN character (active color): %c\n", *fen); 
                        break; 
                }
                break; 
            case 2: // castling rights
                switch (*fen) 
                {
                    case 'K': 
                        g->cas |= CAS_WK; 
                        break; 
                    case 'k': 
                        g->cas |= CAS_BK; 
                        break; 
                    case 'Q': 
                        g->cas |= CAS_WQ; 
                        break; 
                    case 'q': 
                        g->cas |= CAS_BQ; 
                        break; 
                    case '-': 
                        break; 
                    default: 
                        printf("Unknown FEN character (castling availability): %c\n", *fen); 
                        break; 
                }
                break; 
            case 3: // en passant 
                switch (*fen) 
                {
                    case 'a': 
                    case 'b': 
                    case 'c': 
                    case 'd': 
                    case 'e': 
                    case 'f': 
                    case 'g': 
                    case 'h': 
                        sq = sq_make(*fen - 'a', sq_rank(sq)); 
                        g->ep = sq; 
                        break; 
                    case '1': 
                    case '2': 
                    case '3': 
                    case '4': 
                    case '5': 
                    case '6': 
                    case '7': 
                    case '8': 
                        sq = sq_make(sq_file(sq), *fen - '1'); 
                        g->ep = sq; 
                        break; 
                    case '-': 
                        sq = SQ_NONE; 
                        g->ep = sq; 
                        break; 
                    default: 
                        printf("Unknown FEN character (en passant): %c\n", *fen); 
                        sq = SQ_NONE; 
                        g->ep = sq; 
                        break; 
                }
                break; 
            case 4: // halfmove
                switch (*fen) 
                {
                    case '0': 
                    case '1': 
                    case '2': 
                    case '3': 
                    case '4': 
                    case '5': 
                    case '6': 
                    case '7': 
                    case '8': 
                    case '9': 
                        // g->move50 = g->move50 * 10 + (*fen - '0'); 
                        break; 
                    default: 
                        printf("Unknown FEN character (halfmv): %c\n", *fen); 
                        sq = SQ_NONE; 
                        g->ep = sq; 
                        break; 
                }
                break; 
            case 5: // turn
                switch (*fen) 
                {
                    case '0': 
                    case '1': 
                    case '2': 
                    case '3': 
                    case '4': 
                    case '5': 
                    case '6': 
                    case '7': 
                    case '8': 
                    case '9': 
                        turn = turn * 10 + (*fen - '0'); 
                        break; 
                    default: 
                        printf("Unknown FEN character (turn): %c\n", *fen); 
                        sq = SQ_NONE; 
                        g->ep = sq; 
                        break; 
                }
                break; 
        }
    }
    if (mode >= 5) g->ply += turn * 2 - 2; 

    g->turn = g->ply & 1; 

    for (piece p = PC_P; p <= PC_K; p++) 
    {
        g->col[COL_W] |= g->pcs[pc_make(p, COL_W)]; 
        g->col[COL_B] |= g->pcs[pc_make(p, COL_B)]; 
    }
}

static void gm_create_fen(game *g, const char *fen) 
{
    gm_create(g); 
    gm_load_fen(g, fen); 
}

static inline int gm_sign(const game *g) 
{
    return 1 - 2 * g->turn; 
}

// assumes there is a white piece on the square, otherwise returns WP
static inline piece gm_w_pc_at(const game *g, square sq) 
{
    piece p = 0; 
    p |= bb_at(g->pcs[PC_WP], sq) * PC_WP; 
    p |= bb_at(g->pcs[PC_WN], sq) * PC_WN; 
    p |= bb_at(g->pcs[PC_WB], sq) * PC_WB; 
    p |= bb_at(g->pcs[PC_WR], sq) * PC_WR; 
    p |= bb_at(g->pcs[PC_WQ], sq) * PC_WQ; 
    p |= bb_at(g->pcs[PC_WK], sq) * PC_WK; 
    return p; 
}

// assumes there is a black piece on the square, otherwise returns WP
static inline piece gm_b_pc_at(const game *g, square sq) 
{
    piece p = 0; 
    p |= bb_at(g->pcs[PC_BP], sq) * PC_BP; 
    p |= bb_at(g->pcs[PC_BN], sq) * PC_BN; 
    p |= bb_at(g->pcs[PC_BB], sq) * PC_BB; 
    p |= bb_at(g->pcs[PC_BR], sq) * PC_BR; 
    p |= bb_at(g->pcs[PC_BQ], sq) * PC_BQ; 
    p |= bb_at(g->pcs[PC_BK], sq) * PC_BK; 
    return p; 
}

// assumes there is a piece on the square, otherwise returns WP 
static inline piece gm_pc_at(const game *g, square sq) 
{
    return gm_w_pc_at(g, sq) | gm_b_pc_at(g, sq); 
}

static inline void gm_pop_mv(game *g) 
{
    vec_get(&g->hist, g->hist.size - 1, g); 
    vec_pop(&g->hist); 
    g->ply--; 
    g->turn = col_r(g->turn); 
}

static inline bool gm_in_check(const game *g, color for_col, bboard chk_k, bboard ignore, bboard add) 
{
    color col = for_col; 
    color opp = col_r(col); 

    // check if any of these squares are attacked 
    bboard tgt = chk_k; 

    // blockers include both colors' pieces other than the target squares 
    bboard occ = (((g->col[col] & ~tgt) | g->col[opp]) & ~ignore) | add; 

    bboard attacks = BB_NONE; 
    bboard tmp; 

    BB_FOR_EACH(tgt, 
    {
        // pawn 
        if (col == COL_W) 
        {
            // check for black pawns north of the king 
            tmp  = bb_sh_nw(tgt); 
            tmp |= bb_sh_ne(tgt); 
            attacks |= tmp & ((g->pcs[PC_BP] & ~ignore)); 
        }
        else 
        {
            // check for white pawns south of the king 
            tmp  = bb_sh_sw(tgt); 
            tmp |= bb_sh_se(tgt); 
            attacks |= tmp & ((g->pcs[PC_WP] & ~ignore)); 
        }

        // knight 
        tmp = BB_MV_N[s]; 
        attacks |= tmp & g->pcs[pc_make(PC_N, opp)]; 

        // bishop (and queen)
        tmp  = bb_ray(s, BB_SLIDE_DIAG[sq_diag(s)], occ); 
        tmp |= bb_ray(s, BB_SLIDE_ANTI[sq_anti(s)], occ); 
        attacks |= tmp & (g->pcs[pc_make(PC_B, opp)] | g->pcs[pc_make(PC_Q, opp)]); 

        // rook (and queen)
        tmp  = bb_ray(s, BB_SLIDE_FILE[sq_file(s)], occ); 
        tmp |= bb_ray(s, BB_SLIDE_RANK[sq_rank(s)], occ); 
        attacks |= tmp & (g->pcs[pc_make(PC_R, opp)] | g->pcs[pc_make(PC_Q, opp)]); 

        // king 
        tmp = BB_MV_K[s]; 
        attacks |= tmp & g->pcs[pc_make(PC_K, opp)]; 

        if (attacks) return true; 
    });

    return false; 
}

// does not check if the move is legal 
static void gm_push_mv(game *g, move m) 
{
    // save current state 
    vec_push(&g->hist, g); 

    square from = mv_from_sq(m); 
    square to = mv_to_sq(m); 
    square rm_tgt = to; // remove opponent's piece on this square (can be different from `to` if en passant)
    square new_ep = mv_ep_sq(m); 

    piece pc = mv_from_pc(m); 
    piece pro = mv_pro(m); 
    piece tgt; 

    // note: target will be WP if rm_tgt is empty 
    if (g->turn == COL_W) 
    {
        rm_tgt -= 8 * mv_ep(m); // target piece is south (behind the capturing pawn) if ep
        tgt = gm_b_pc_at(g, rm_tgt); 
    }
    else // B
    {
        rm_tgt += 8 * mv_ep(m); // target piece is north (behind the capturing pawn) if ep
        tgt = gm_w_pc_at(g, rm_tgt); 
    }

    // take moving piece off the board 
    g->pcs[pc] = bb_clear(g->pcs[pc], from); 

    // WP if rm_tgt is empty, but this is okay because if piece is WP it will re-enable this square right after 
    g->pcs[tgt] = bb_clear(g->pcs[tgt], rm_tgt); 

    // place piece onto new square 
    g->pcs[pro] = bb_set(g->pcs[pro], to); 

    color cur_col = g->turn, opp_col = col_r(cur_col); 

    // remove rooks from aggregate (in case of castling) 
    g->col[COL_W] ^= g->pcs[PC_WR]; 
    g->col[COL_B] ^= g->pcs[PC_BR]; 

    int castles = mv_cas(m); 
    g->chk = g->pcs[pc_make(PC_K, cur_col)] | BB_CAS_TGTS[castles]; 

    // move the rooks when castling 
    g->pcs[PC_WR] = (g->pcs[PC_WR] & BB_CAS_KEEP_WR[castles]) | BB_CAS_ADD_WR[castles]; 
    g->pcs[PC_BR] = (g->pcs[PC_BR] & BB_CAS_KEEP_BR[castles]) | BB_CAS_ADD_BR[castles]; 

    castle rem_cf = CAS_NONE; 
    // no castling if the king moves
    rem_cf |= CAS_W  * (pc == PC_WK); 
    rem_cf |= CAS_B  * (pc == PC_BK); 
    // no castling if the rook moves or is captured
    rem_cf |= CAS_WK * ((from == SQ_H1) | (to == SQ_H1)); 
    rem_cf |= CAS_WQ * ((from == SQ_A1) | (to == SQ_A1)); 
    rem_cf |= CAS_BK * ((from == SQ_H8) | (to == SQ_H8)); 
    rem_cf |= CAS_BQ * ((from == SQ_A8) | (to == SQ_A8)); 

    // remove castling rights if necessary 
    g->cas &= ~rem_cf; 

    // re-add rooks from aggregate (in case of castling) 
    g->col[COL_W] ^= g->pcs[PC_WR]; 
    g->col[COL_B] ^= g->pcs[PC_BR]; 

    // update aggregate piece tracking 
    g->col[cur_col] = bb_clear(g->col[cur_col], from); 
    g->col[opp_col] = bb_clear(g->col[opp_col], rm_tgt); 
    g->col[cur_col] = bb_set(g->col[cur_col], to); 

    // update rest of state 
    g->ep = new_ep; 
    g->ply++; 
    g->turn = col_r(g->turn); 

    // useful to know if the new player is in check 
    g->in_chk = gm_in_check(g, g->turn, g->pcs[pc_make(PC_K, g->turn)], BB_NONE, BB_NONE); 
}

static inline void gm_mvgen_bb(bboard b, square from, piece pc, vector *out) 
{
    BB_FOR_EACH(b, 
    {
        VEC_PUSH(out, move, mv_make(from, s, pc, pc)); 
    });
}

static inline void gm_mvgen_safe(const game *g, bboard b, square from, piece pc, vector *out) 
{
    BB_FOR_EACH(b, 
    {
        if (!gm_in_check(g, g->turn, bb_pos(s), bb_pos(from), BB_NONE)) 
        {
            VEC_PUSH(out, move, mv_make(from, s, pc, pc)); 
        }
    });
}

static inline void gm_mvgen_cas_move(square from, square to, piece pc, int cas_index, vector *out) 
{
    VEC_PUSH(out, move, mv_make_cas(from, to, pc, cas_index)); 
}

static inline void gm_mvgen_allow_ep(bboard b, square from, piece pc, int d_sq, vector *out) 
{
    BB_FOR_EACH(b, 
    {
        VEC_PUSH(out, move, mv_make_allow_ep(from, s, pc, s + d_sq)); 
    });
}

static inline void gm_mvgen_pro(bboard b, square from, piece pc, color col, vector *out) 
{
    BB_FOR_EACH(b, 
    {
        VEC_PUSH(out, move, mv_make(from, s, pc, pc_make(PC_Q, col))); 
        VEC_PUSH(out, move, mv_make(from, s, pc, pc_make(PC_R, col))); 
        VEC_PUSH(out, move, mv_make(from, s, pc, pc_make(PC_B, col))); 
        VEC_PUSH(out, move, mv_make(from, s, pc, pc_make(PC_N, col))); 
    });
}

static inline void gm_mvgen_might_ep(const game *g, bboard b, square from, piece pc, square ep, vector *out) 
{
    BB_FOR_EACH(b, 
    {
        if (s != ep) 
        {
            VEC_PUSH(out, move, mv_make_ep(from, s, pc, false)); 
        }
        else if (pc_col(pc) == COL_W) 
        {
            if (!gm_in_check(g, COL_W, g->pcs[PC_WK], bb_pos(from) | bb_pos(ep - 8), bb_pos(ep))) 
            {
                VEC_PUSH(out, move, mv_make_ep(from, s, pc, true)); 
            }
        }
        else // COL_B  
        {
            if (!gm_in_check(g, COL_B, g->pcs[PC_BK], bb_pos(from) | bb_pos(ep + 8), bb_pos(ep))) 
            {
                VEC_PUSH(out, move, mv_make_ep(from, s, pc, true)); 
            }
        }
    });
}

static inline void gm_mvgen_p(const game *g, color col, bboard ok_sqs, const bboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    bboard empty = ~(g->col[COL_W] | g->col[COL_B]); 
    bboard ep = (g->ep != SQ_NONE) * bb_pos(g->ep); 
    bboard cur_pos, attack, target_sqs; 

    if (col == COL_W) 
    {
        BB_FOR_EACH(g->pcs[PC_WP], 
        {
            cur_pos = bb_pos(s); 

            // move forward if square is empty 
            attack = bb_sh_n(cur_pos) & empty; 

            // attack left if there is a piece or en passant
            attack |= bb_sh_nw(cur_pos) & (g->col[COL_B] | ep); 

            // attack right if there is a piece or en passant
            attack |= bb_sh_ne(cur_pos) & (g->col[COL_B] | ep); 

            target_sqs = GM_TGT_SQS; 
            attack &= target_sqs; 

            // move 2 squares if on starting square 
            gm_mvgen_allow_ep((sq_rank(s) == 1) * (bb_sh_nn(cur_pos) & empty & bb_sh_n(empty) & target_sqs), s, PC_WP, -8, out); 

            // add non-promoting moves to list 
            gm_mvgen_might_ep(g, attack & BB_NO_8_RANK, s, PC_WP, g->ep, out); 

            // add promoting moves to list 
            gm_mvgen_pro(attack & BB_8_RANK, s, PC_WP, COL_W, out); 
        });
    }
    else 
    {
        BB_FOR_EACH(g->pcs[PC_BP], 
        {
            cur_pos = bb_pos(s); 

            // move forward if square is empty 
            attack = bb_sh_s(cur_pos) & empty; 

            // attack left if there is a piece or en passant
            attack |= bb_sh_sw(cur_pos) & (g->col[COL_W] | ep); 

            // attack right if there is a piece or en passant
            attack |= bb_sh_se(cur_pos) & (g->col[COL_W] | ep); 

            target_sqs = GM_TGT_SQS; 
            attack &= target_sqs; 

            // move 2 squares if on starting square 
            gm_mvgen_allow_ep((sq_rank(s) == 6) * (bb_sh_ss(cur_pos) & empty & bb_sh_s(empty) & target_sqs), s, PC_BP, 8, out); 

            // add non-promoting moves to list 
            gm_mvgen_might_ep(g, attack & BB_NO_1_RANK, s, PC_BP, g->ep, out); 

            // add promoting moves to list 
            gm_mvgen_pro(attack & BB_1_RANK, s, PC_BP, COL_B, out); 
        });
    }
}

static inline void gm_mvgen_k(const game *g, color col, vector *out) 
{
    bboard occ = g->col[COL_W] | g->col[COL_B]; 
    bboard not_cur_col = ~g->col[col]; 
    bboard attack; 

    piece pc = pc_make(PC_K, col); 
    BB_FOR_EACH(g->pcs[pc], 
    {
        attack = BB_MV_K[s] & not_cur_col; 
        gm_mvgen_safe(g, attack, s, pc, out); 

        int c_wk = 0 != (col_r(col) * ((g->cas & CAS_WK) != 0) * ((occ & BB_EMPTY_WK) == 0)); 
        int c_wq = 0 != (col_r(col) * ((g->cas & CAS_WQ) != 0) * ((occ & BB_EMPTY_WQ) == 0)); 
        int c_bk = 0 != (col * ((g->cas & CAS_BK) != 0) * ((occ & BB_EMPTY_BK) == 0)); 
        int c_bq = 0 != (col * ((g->cas & CAS_BQ) != 0) * ((occ & BB_EMPTY_BQ) == 0)); 

        if (c_wk && !gm_in_check(g, col, g->pcs[pc] | BB_CAS_TGTS[MV_CAS_WK], BB_NONE, BB_NONE)) 
        {
            gm_mvgen_cas_move(SQ_E1, SQ_G1, PC_WK, MV_CAS_WK, out); 
        }

        if (c_wq && !gm_in_check(g, col, g->pcs[pc] | BB_CAS_TGTS[MV_CAS_WQ], BB_NONE, BB_NONE)) 
        {
            gm_mvgen_cas_move(SQ_E1, SQ_C1, PC_WK, MV_CAS_WQ, out); 
        }

        if (c_bk && !gm_in_check(g, col, g->pcs[pc] | BB_CAS_TGTS[MV_CAS_BK], BB_NONE, BB_NONE)) 
        {
            gm_mvgen_cas_move(SQ_E8, SQ_G8, PC_BK, MV_CAS_BK, out); 
        }

        if (c_bq && !gm_in_check(g, col, g->pcs[pc] | BB_CAS_TGTS[MV_CAS_BQ], BB_NONE, BB_NONE)) 
        {
            gm_mvgen_cas_move(SQ_E8, SQ_C8, PC_BK, MV_CAS_BQ, out); 
        }
    });
}

static inline void gm_mvgen_n(const game *g, color col, bboard ok_sqs, const bboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    // empty squares 
    bboard not_cur_col = ~g->col[col]; 
    bboard attack; 

    piece pc = pc_make(PC_N, col); 
    BB_FOR_EACH(g->pcs[pc], 
    {
        attack = BB_MV_N[s] & not_cur_col; 
        attack &= GM_TGT_SQS;  
        gm_mvgen_bb(attack, s, pc, out); 
    });
}

static inline void gm_mvgen_b(const game *g, color col, bboard ok_sqs, const bboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    bboard not_cur_col = ~g->col[col]; 
    bboard occ = g->col[COL_W] | g->col[COL_B]; 
    bboard attack; 

    piece pc = pc_make(PC_B, col); 
    BB_FOR_EACH(g->pcs[pc], 
    {
        attack  = bb_ray(s, BB_SLIDE_DIAG[sq_diag(s)], occ); 
        attack |= bb_ray(s, BB_SLIDE_ANTI[sq_anti(s)], occ); 
        attack &= not_cur_col; 
        attack &= GM_TGT_SQS; 
        gm_mvgen_bb(attack, s, pc, out); 
    })
}

static inline void gm_mvgen_r(const game *g, color col, bboard ok_sqs, const bboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    bboard not_cur_col = ~g->col[col]; 
    bboard occ = g->col[COL_W] | g->col[COL_B]; 
    bboard attack; 

    piece pc = pc_make(PC_R, col); 
    BB_FOR_EACH(g->pcs[pc], 
    {
        attack  = bb_ray(s, BB_SLIDE_FILE[sq_file(s)], occ); 
        attack |= bb_ray(s, BB_SLIDE_RANK[sq_rank(s)], occ); 
        attack &= not_cur_col; 
        attack &= GM_TGT_SQS; 

        gm_mvgen_bb(attack, s, pc, out); 
    })
}

static inline void gm_mvgen_q(const game *g, color col, bboard ok_sqs, const bboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    bboard not_cur_col = ~g->col[col]; 
    bboard occ = g->col[COL_W] | g->col[COL_B]; 
    bboard attack; 

    piece pc = pc_make(PC_Q, col); 
    BB_FOR_EACH(g->pcs[pc], 
    {
        attack  = bb_ray(s, BB_SLIDE_FILE[sq_file(s)], occ); 
        attack |= bb_ray(s, BB_SLIDE_RANK[sq_rank(s)], occ); 
        attack |= bb_ray(s, BB_SLIDE_DIAG[sq_diag(s)], occ); 
        attack |= bb_ray(s, BB_SLIDE_ANTI[sq_anti(s)], occ); 
        attack &= not_cur_col; 
        attack &= GM_TGT_SQS; 
        gm_mvgen_bb(attack, s, pc, out); 
    })
}

static void gm_mvs(const game *g, vector *out) 
{
    color col = g->turn; 
    color opp = col_r(col); 

    bboard tgt = g->pcs[pc_make(PC_K, col)]; 
    bboard col_occ = g->col[col]; 
    bboard opp_occ = g->col[opp]; 
    bboard opp_rocc = bb_r(opp_occ); 

    bboard opp_card = g->pcs[pc_make(PC_R, opp)] | g->pcs[pc_make(PC_Q, opp)]; 
    bboard opp_diag = g->pcs[pc_make(PC_B, opp)] | g->pcs[pc_make(PC_Q, opp)]; 

    square sq = bb_lsb(tgt), rsq = 63 - sq; 

    bboard dirs[BB_CHECK_CNT]; 
    uint8_t blocking[BB_CHECK_DIR_CNT]; 

    uint16_t attacked = 0; 
    uint16_t pinned = 0; 

    // diag movement to occupied square (ignoring current color pieces and including non-relevant enemy pcs)
    dirs[BB_CHECK_NW] = bb_ray_pos(sq, BB_SLIDE_ANTI[sq_anti(sq)], opp_occ); 
    dirs[BB_CHECK_SE] = bb_r(bb_ray_pos(rsq, BB_SLIDE_ANTI[sq_anti(rsq)], opp_rocc)); 
    dirs[BB_CHECK_NE] = bb_ray_pos(sq, BB_SLIDE_DIAG[sq_diag(sq)], opp_occ); 
    dirs[BB_CHECK_SW] = bb_r(bb_ray_pos(rsq, BB_SLIDE_DIAG[sq_diag(rsq)], opp_rocc)); 

    // cardinal movement to occupied square (ignoring current color pieces and including non-relevant enemy pcs)
    dirs[BB_CHECK_N] = bb_ray_pos(sq, BB_SLIDE_FILE[sq_file(sq)], opp_occ); 
    dirs[BB_CHECK_S] = bb_r(bb_ray_pos(rsq, BB_SLIDE_FILE[sq_file(rsq)], opp_rocc)); 
    dirs[BB_CHECK_E] = bb_ray_pos(sq, BB_SLIDE_RANK[sq_rank(sq)], opp_occ); 
    dirs[BB_CHECK_W] = bb_r(bb_ray_pos(rsq, BB_SLIDE_RANK[sq_rank(rsq)], opp_rocc)); 

    // knight 
    dirs[BB_CHECK_PC_N] = BB_MV_N[sq] & g->pcs[pc_make(PC_N, opp)]; 

    // pawn 
    if (col == COL_W) 
    {
        dirs[BB_CHECK_PC_P]  = bb_sh_nw(tgt); 
        dirs[BB_CHECK_PC_P] |= bb_sh_ne(tgt); 
    }
    else 
    {
        dirs[BB_CHECK_PC_P]  = bb_sh_sw(tgt); 
        dirs[BB_CHECK_PC_P] |= bb_sh_se(tgt); 
    }
    dirs[BB_CHECK_PC_P] &= g->pcs[pc_make(PC_P, opp)]; 

    // determine number of blockers (assuming that the direction has an attacker) 
    blocking[BB_CHECK_N] = bb_popcnt(dirs[BB_CHECK_N] & col_occ); 
    blocking[BB_CHECK_S] = bb_popcnt(dirs[BB_CHECK_S] & col_occ); 
    blocking[BB_CHECK_E] = bb_popcnt(dirs[BB_CHECK_E] & col_occ); 
    blocking[BB_CHECK_W] = bb_popcnt(dirs[BB_CHECK_W] & col_occ); 
    blocking[BB_CHECK_NE] = bb_popcnt(dirs[BB_CHECK_NE] & col_occ); 
    blocking[BB_CHECK_NW] = bb_popcnt(dirs[BB_CHECK_NW] & col_occ); 
    blocking[BB_CHECK_SE] = bb_popcnt(dirs[BB_CHECK_SE] & col_occ); 
    blocking[BB_CHECK_SW] = bb_popcnt(dirs[BB_CHECK_SW] & col_occ); 

    // first figure out which directions have an attacker (ignore current color pinned pcs)

    // directions might hit a non-attacking opponent pc, or even no piece at all
    attacked |= (((dirs[BB_CHECK_N] & opp_card) != 0) & (blocking[BB_CHECK_N] <= 1)) << BB_CHECK_N; 
    attacked |= (((dirs[BB_CHECK_S] & opp_card) != 0) & (blocking[BB_CHECK_S] <= 1)) << BB_CHECK_S; 
    attacked |= (((dirs[BB_CHECK_E] & opp_card) != 0) & (blocking[BB_CHECK_E] <= 1)) << BB_CHECK_E; 
    attacked |= (((dirs[BB_CHECK_W] & opp_card) != 0) & (blocking[BB_CHECK_W] <= 1)) << BB_CHECK_W; 
    attacked |= (((dirs[BB_CHECK_NE] & opp_diag) != 0) & (blocking[BB_CHECK_NE] <= 1)) << BB_CHECK_NE; 
    attacked |= (((dirs[BB_CHECK_NW] & opp_diag) != 0) & (blocking[BB_CHECK_NW] <= 1)) << BB_CHECK_NW; 
    attacked |= (((dirs[BB_CHECK_SE] & opp_diag) != 0) & (blocking[BB_CHECK_SE] <= 1)) << BB_CHECK_SE; 
    attacked |= (((dirs[BB_CHECK_SW] & opp_diag) != 0) & (blocking[BB_CHECK_SW] <= 1)) << BB_CHECK_SW;
    // there are already checked against correct opponent pieces and cannot be blocked 
    attacked |= (dirs[BB_CHECK_PC_N] != 0) << BB_CHECK_PC_N; 
    attacked |= (dirs[BB_CHECK_PC_P] != 0) << BB_CHECK_PC_P; 

    // determine pins 

    // these need to have an attacker, and exactly one friendly piece in the way 
    pinned |= ((attacked >> BB_CHECK_N) & (blocking[BB_CHECK_N] == 1)) << BB_CHECK_N; 
    pinned |= ((attacked >> BB_CHECK_S) & (blocking[BB_CHECK_S] == 1)) << BB_CHECK_S; 
    pinned |= ((attacked >> BB_CHECK_E) & (blocking[BB_CHECK_E] == 1)) << BB_CHECK_E; 
    pinned |= ((attacked >> BB_CHECK_W) & (blocking[BB_CHECK_W] == 1)) << BB_CHECK_W; 
    pinned |= ((attacked >> BB_CHECK_NE) & (blocking[BB_CHECK_NE] == 1)) << BB_CHECK_NE; 
    pinned |= ((attacked >> BB_CHECK_NW) & (blocking[BB_CHECK_NW] == 1)) << BB_CHECK_NW; 
    pinned |= ((attacked >> BB_CHECK_SE) & (blocking[BB_CHECK_SE] == 1)) << BB_CHECK_SE; 
    pinned |= ((attacked >> BB_CHECK_SW) & (blocking[BB_CHECK_SW] == 1)) << BB_CHECK_SW; 

    attacked &= ~pinned; 

    int n_checks = bb_popcnt16(attacked); 

    if (n_checks == 0) 
    {
        // all moves 
        gm_mvgen_p(g, col, BB_ALL, dirs, pinned, BB_PIN_IDX[sq], out); 
        gm_mvgen_n(g, col, BB_ALL, dirs, pinned, BB_PIN_IDX[sq], out); 
        gm_mvgen_b(g, col, BB_ALL, dirs, pinned, BB_PIN_IDX[sq], out); 
        gm_mvgen_r(g, col, BB_ALL, dirs, pinned, BB_PIN_IDX[sq], out); 
        gm_mvgen_q(g, col, BB_ALL, dirs, pinned, BB_PIN_IDX[sq], out); 
        gm_mvgen_k(g, col, out); 
    }
    else if (n_checks == 1) 
    {
        // all moves, but must remove check 

        // for sliding attacking pc: 
        //   all squares that block or capture the piece
        // knight or pawn: 
        //   only allows capturing the piece (as it cannot be blocked)
        bboard target_sqs = dirs[bb_lsb(attacked)]; 

        gm_mvgen_p(g, col, target_sqs | ((g->ep != SQ_NONE) * bb_pos(g->ep)), dirs, pinned, BB_PIN_IDX[sq], out); 
        gm_mvgen_n(g, col, target_sqs, dirs, pinned, BB_PIN_IDX[sq], out); 
        gm_mvgen_b(g, col, target_sqs, dirs, pinned, BB_PIN_IDX[sq], out); 
        gm_mvgen_r(g, col, target_sqs, dirs, pinned, BB_PIN_IDX[sq], out); 
        gm_mvgen_q(g, col, target_sqs, dirs, pinned, BB_PIN_IDX[sq], out); 
        gm_mvgen_k(g, col, out); 
    }
    else // double check: must move the king 
    {
        // move king 
        gm_mvgen_k(g, col, out); 
    }
}

static const int GM_EVAL_P[64] = 
{
      0,   0,   0,   0,   0,   0,   0,   0, 
      5,  10,  10, -20, -20,  10,  10,   5, 
      5,  -5, -10,   0,   0, -10,  -5,   5, 
      0,   0,   0,  20,  20,   0,   0,   0, 
      5,   5,  10,  25,  25,  10,   5,   5, 
     10,  10,  20,  30,  30,  20,  10,  10, 
     50,  50,  50,  50,  50,  50,  50,  50, 
      0,   0,   0,   0,   0,   0,   0,   0
};

static const int GM_EVAL_N[64] = 
{
    -50, -40, -30, -30, -30, -30, -40, -50, 
    -40, -20,   0,   5,   5,   0, -20, -40, 
    -30,   5,  10,  15,  15,  10,   5, -30, 
    -30,   0,  15,  20,  20,  15,   0, -30, 
    -30,   5,  15,  20,  20,  15,   5, -30, 
    -30,   0,  10,  15,  15,  10,   0, -30, 
    -40, -20,   0,   0,   0,   0, -20, -40, 
    -50, -40, -30, -30, -30, -30, -40, -50 
};

static const int GM_EVAL_B[64] = 
{
    -20, -10, -10, -10, -10, -10, -10, -20, 
    -10,   5,   0,   0,   0,   0,   5, -10, 
    -10,  10,  10,  10,  10,  10,  10, -10, 
    -10,   0,  10,  10,  10,  10,   0, -10, 
    -10,   5,   5,  10,  10,   5,   5, -10, 
    -10,   0,   5,  10,  10,   5,   0, -10, 
    -10,   0,   0,   0,   0,   0,   0, -10, 
    -20, -10, -10, -10, -10, -10, -10, -20 
};

static const int GM_EVAL_R[64] = 
{
      0,   0,   0,   5,   5,   0,   0,   0, 
     -5,   0,   0,   0,   0,   0,   0,  -5, 
     -5,   0,   0,   0,   0,   0,   0,  -5, 
     -5,   0,   0,   0,   0,   0,   0,  -5, 
     -5,   0,   0,   0,   0,   0,   0,  -5, 
     -5,   0,   0,   0,   0,   0,   0,  -5, 
      5,  10,  10,  10,  10,  10,  10,   5, 
      0,   0,   0,   0,   0,   0,   0,   0 
};

static const int GM_EVAL_Q[64] = 
{
    -20, -10, -10,  -5,  -5, -10, -10, -20, 
    -10,   0,   5,   0,   0,   5,   0, -10, 
    -10,   0,   5,   5,   5,   5,   0, -10, 
      0,   0,   5,   5,   5,   5,   0,   0, 
      0,   0,   5,   5,   5,   5,   0,   0, 
    -10,   0,   5,   5,   5,   5,   0, -10, 
    -10,   0,   0,   0,   0,   0,   0, -10, 
    -20, -10, -10,  -5,  -5, -10, -10, -20, 
};

static const int GM_EVAL_K[64] = 
{
     20,  30,  10,   0,   0,  10,  30,  20, 
     20,  20,   0,   0,   0,   0,  20,  20, 
    -10, -20, -20, -20, -20, -20, -20, -10, 
    -20, -30, -30, -40, -40, -30, -30, -20, 
    -30, -40, -40, -50, -50, -40, -40, -30, 
    -30, -40, -40, -50, -50, -40, -40, -30, 
    -30, -40, -40, -50, -50, -40, -40, -30, 
    -30, -40, -40, -50, -50, -40, -40, -30, 
};

static inline int gm_eval_bboard(bboard pcs, const int eval[64]) 
{
    int sum = 0; 

    BB_FOR_EACH(pcs, 
    {
        sum += eval[s]; 
    });

    return sum; 
}

static inline int gm_eval(const game *g, int num_moves) 
{
    int eval = 0; 

    if (num_moves == 0) 
    {
        if (g->in_chk) 
        {
            // lower value the farther out the mate is (prioritize faster mates)
            return 100000 * (-1 + 2 * g->turn) - g->ply; 
        }
        else 
        {
            // stalemate
            return 0; 
        }
    }

    // material 
    eval += 100 * (bb_popcnt(g->pcs[PC_WP]) - bb_popcnt(g->pcs[PC_BP])); 
    eval += 310 * (bb_popcnt(g->pcs[PC_WN]) - bb_popcnt(g->pcs[PC_BN])); 
    eval += 320 * (bb_popcnt(g->pcs[PC_WB]) - bb_popcnt(g->pcs[PC_BB])); 
    eval += 500 * (bb_popcnt(g->pcs[PC_WR]) - bb_popcnt(g->pcs[PC_BR])); 
    eval += 900 * (bb_popcnt(g->pcs[PC_WQ]) - bb_popcnt(g->pcs[PC_BQ])); 
    eval += 10000 * (bb_popcnt(g->pcs[PC_WK]) - bb_popcnt(g->pcs[PC_BK])); 

    // bishop pair 
    eval += 15 * ((bb_popcnt(g->pcs[PC_WB]) >= 2) - (bb_popcnt(g->pcs[PC_BB]) >= 2)); 

    eval += gm_eval_bboard(g->pcs[PC_WP], GM_EVAL_P); 
    eval -= gm_eval_bboard(bb_rrow(g->pcs[PC_BP]), GM_EVAL_P); 

    eval += gm_eval_bboard(g->pcs[PC_WN], GM_EVAL_N); 
    eval -= gm_eval_bboard(bb_rrow(g->pcs[PC_BN]), GM_EVAL_N); 

    eval += gm_eval_bboard(g->pcs[PC_WB], GM_EVAL_B); 
    eval -= gm_eval_bboard(bb_rrow(g->pcs[PC_BB]), GM_EVAL_B); 

    eval += gm_eval_bboard(g->pcs[PC_WR], GM_EVAL_R); 
    eval -= gm_eval_bboard(bb_rrow(g->pcs[PC_BR]), GM_EVAL_R); 

    eval += gm_eval_bboard(g->pcs[PC_WQ], GM_EVAL_Q); 
    eval -= gm_eval_bboard(bb_rrow(g->pcs[PC_BQ]), GM_EVAL_Q); 

    eval += gm_eval_bboard(g->pcs[PC_WK], GM_EVAL_K); 
    eval -= gm_eval_bboard(bb_rrow(g->pcs[PC_BK]), GM_EVAL_K); 

    return eval; 
}

static void gm_print(const game *g) 
{
    static const char *empty = "."; 

    vector moves; 
    VEC_CREATE(&moves, move); 
    gm_mvs(g, &moves);  
    size_t num_moves = moves.size; 

    printf("  +-----------------+\n"); 
    for (int rank = 7; rank >= 0; rank--) 
    {
        printf("%d | ", rank + 1); 
        for (int file = 0; file < 8; file++) 
        {
            square sq = sq_make(file, rank); 
            const char *str = empty; 
            for (piece p = 0; p < PC_CNT; p++) 
            {
                if (bb_at(g->pcs[p], sq)) 
                {
                    str = pc_str(p); 
                }
            }

            printf("%s ", str); 
        }
        printf("|\n"); 
    }
    printf("  +-----------------+\n"); 
    printf("    A B C D E F G H  \n"); 
    printf("Castling: %s\n", cas_str(g->cas)); 
    printf("En passant: %s\n", g->ep == SQ_NONE ? "(none)" : sq_str(g->ep)); 
    printf("In check: %s\n", g->in_chk ? "yes" : "no"); 
    printf("# moves: %zu\n", num_moves); 
    printf("Static eval: %.2f\n", gm_eval(g, num_moves) * 0.01); 
    printf("%s to move\n", g->turn ? "Black" : "White"); 

    vec_destroy(&moves); 
}

static inline uint64_t gm_perft_(game *g, vector *moves, int depth, bool verbose) 
{
    uint64_t total = 0, start = moves->size, size; 

    if (depth > 1) 
    {
        gm_mvs(g, moves); 
        size = moves->size; 

        // gm_print(g); 

        for (size_t i = start; i < size; i++) 
        {
            gm_push_mv(g, VEC_AT(moves, move, i)); 
            total += gm_perft_(g, moves, depth - 1, false); 
            gm_pop_mv(g); 
        }

        vec_pop_size(moves, start); 
    }
    else if (depth == 1) 
    {
        gm_mvs(g, moves); 
        total += moves->size - start; 
        vec_pop_size(moves, start); 
    }
    else 
    {
        total = 1; 
    }

    if (verbose) 
    {
        printf("Depth: %d, Total: %"PRIu64"", depth, total); 
    }

    return total; 
}

static inline uint64_t gm_perft(game *g, int depth) 
{
    vector moves; 
    VEC_CREATE(&moves, move); 

    uint64_t total = gm_perft_(g, &moves, depth, true); 

    vec_destroy(&moves); 
    return total; 
}

typedef struct pv_line pv_line; 
struct pv_line 
{
    size_t n_moves; 
    move moves[128]; 
};

static inline int gm_quiescence(game *g, int alpha, int beta, int depth, vector *moves) 
{
    size_t start = moves->size; 

    gm_mvs(g, moves); 

    int stand_pat = gm_sign(g) * gm_eval(g, moves->size - start); 

    if (stand_pat >= beta) 
    {
        vec_pop_size(moves, start); 
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
            move mv = VEC_AT(moves, move, i); 

            // only consider captures 
            if (mv_ep(mv) | bb_at(g->col[col_r(g->turn)], mv_to_sq(mv))) 
            {
                gm_push_mv(g, mv); 
                int score = -gm_quiescence(g, -beta, -alpha, depth - 1, moves); 
                gm_pop_mv(g); 

                if (score >= beta) 
                {
                    vec_pop_size(moves, start); 
                    return beta; 
                }
                if (score > alpha) 
                {
                    alpha = score; 
                }
            }
        }
    }

    vec_pop_size(moves, start); 
    return alpha; 
}

static inline int gm_negamax(game *g, int alpha, int beta, int depth, vector *moves, pv_line *pv) 
{
    pv_line line; 
    size_t start = moves->size; 

    gm_mvs(g, moves); 
    int num_moves = moves->size - start; 

    if (num_moves == 0 || depth <= 0) 
    {
        pv->n_moves = 0; 
        vec_pop_size(moves, start); 
        return gm_quiescence(g, alpha, beta, 32, moves); 
    }

    for (size_t i = start; i < moves->size; i++) 
    {
        gm_push_mv(g, VEC_AT(moves, move, i)); 
        int eval = -gm_negamax(g, -beta, -alpha, depth - 1, moves, &line); 
        gm_pop_mv(g); 

        if (eval >= beta) 
        {
            vec_pop_size(moves, start); 
            return beta; 
        }

        if (VEC_AT(moves, move, i) == 0) 
        {
            printf("what\n"); 
            fflush(stdout); 
        }

        if (eval > alpha) 
        {
            alpha = eval; 
            pv->moves[0] = VEC_AT(moves, move, i); 
            memcpy(pv->moves + 1, line.moves, line.n_moves * sizeof(move)); 
            pv->n_moves = line.n_moves + 1; 
        }
    }

    vec_pop_size(moves, start); 
    return alpha; 
}

static inline void gm_search(game *g, int search_depth, vector *pv, int *eval) 
{
    vector moves; 
    VEC_CREATE(&moves, move); 

    pv_line line; 
    for (int depth = 1; depth <= search_depth; depth++) 
    {
        vec_clear(&moves); 
        int eval = gm_negamax(g, -INT_MAX, INT_MAX, depth, &moves, &line); 
        printf("info depth %d seldepth %zu multipv 1 score cp %d nodes 1 nps 1 pv ", depth, line.n_moves, eval); 
        for (size_t i = 0; i < line.n_moves; i++) 
        {
            mv_print_end(line.moves[i], " "); 
        }
        printf("\n"); 

        fflush(stdout); 
    }

    printf("bestmove "); 
    mv_print_end(line.moves[0], "\n"); 

    vec_destroy(&moves); 
}