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
#define TGT_SQS ( \
    ok_sqs & ( \
        dirs[pin_idx[s]] | (ALL_BITS * \
            ( \
                ((~pinned >> pin_idx[s]) & 1) | \
                ((dirs[pin_idx[s]] & (1ULL << s)) == 0) \
            ) \
        ) \
    ) \
)

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

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

static void reset_game(game *g) 
{
    clear_vec(&g->hist); 
    memset(&g->pcs, 0, sizeof(g->pcs)); 
    memset(&g->col, 0, sizeof(g->col)); 
    g->chk = NO_BITS; 
    g->cas = CAS_NONE; 
    g->ep = NO_SQ; 
    g->in_chk = false; 
    g->ply = 0; 
    g->turn = COL_W; 
}

static void create_game(game *g) 
{
    create_vec(&g->hist, offsetof(game, hist)); 
    reset_game(g); 
}

static void destroy_game(game *g) 
{
    destroy_vec(&g->hist); 
}

static void load_fen_game(game *g, const char *fen) 
{
    reset_game(g); 

    // decrement once so simple incrementing while loop 
    // starts at the first character
    fen--; 

    int mode = 0, turn = 0; 
    square sq = A1; 
    while (*++fen) 
    {
        if (*fen == ' ') 
        {
            mode++; 
            sq = A1; 
            continue; 
        }
        switch (mode) 
        {
            case 0: // board setup 
                switch (*fen) 
                {
                    case 'P': 
                        g->pcs[PC_WP] = set_sq(g->pcs[PC_WP], rrank(sq++)); 
                        break; 
                    case 'p': 
                        g->pcs[PC_BP] = set_sq(g->pcs[PC_BP], rrank(sq++)); 
                        break; 
                    case 'N': 
                        g->pcs[PC_WN] = set_sq(g->pcs[PC_WN], rrank(sq++)); 
                        break; 
                    case 'n': 
                        g->pcs[PC_BN] = set_sq(g->pcs[PC_BN], rrank(sq++)); 
                        break; 
                    case 'B': 
                        g->pcs[PC_WB] = set_sq(g->pcs[PC_WB], rrank(sq++)); 
                        break; 
                    case 'b': 
                        g->pcs[PC_BB] = set_sq(g->pcs[PC_BB], rrank(sq++)); 
                        break; 
                    case 'R': 
                        g->pcs[PC_WR] = set_sq(g->pcs[PC_WR], rrank(sq++)); 
                        break; 
                    case 'r': 
                        g->pcs[PC_BR] = set_sq(g->pcs[PC_BR], rrank(sq++)); 
                        break; 
                    case 'Q': 
                        g->pcs[PC_WQ] = set_sq(g->pcs[PC_WQ], rrank(sq++)); 
                        break; 
                    case 'q': 
                        g->pcs[PC_BQ] = set_sq(g->pcs[PC_BQ], rrank(sq++)); 
                        break; 
                    case 'K': 
                        g->pcs[PC_WK] = set_sq(g->pcs[PC_WK], rrank(sq++)); 
                        break; 
                    case 'k': 
                        g->pcs[PC_BK] = set_sq(g->pcs[PC_BK], rrank(sq++)); 
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
                        sq = make_sq(*fen - 'a', rank(sq)); 
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
                        sq = make_sq(file(sq), *fen - '1'); 
                        g->ep = sq; 
                        break; 
                    case '-': 
                        sq = NO_SQ; 
                        g->ep = sq; 
                        break; 
                    default: 
                        printf("Unknown FEN character (en passant): %c\n", *fen); 
                        sq = NO_SQ; 
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
                        sq = NO_SQ; 
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
                        sq = NO_SQ; 
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
        g->col[COL_W] |= g->pcs[make_pc(p, COL_W)]; 
        g->col[COL_B] |= g->pcs[make_pc(p, COL_B)]; 
    }
}

static void create_fen_game(game *g, const char *fen) 
{
    create_game(g); 
    load_fen_game(g, fen); 
}

static inline int col_sign(const game *g) 
{
    return 1 - 2 * g->turn; 
}

// assumes there is a white piece on the square, otherwise returns WP
static inline piece w_pc_at(const game *g, square sq) 
{
    piece p = 0; 
    p |= at_sq(g->pcs[PC_WP], sq) * PC_WP; 
    p |= at_sq(g->pcs[PC_WN], sq) * PC_WN; 
    p |= at_sq(g->pcs[PC_WB], sq) * PC_WB; 
    p |= at_sq(g->pcs[PC_WR], sq) * PC_WR; 
    p |= at_sq(g->pcs[PC_WQ], sq) * PC_WQ; 
    p |= at_sq(g->pcs[PC_WK], sq) * PC_WK; 
    return p; 
}

// assumes there is a black piece on the square, otherwise returns WP
static inline piece b_pc_at(const game *g, square sq) 
{
    piece p = 0; 
    p |= at_sq(g->pcs[PC_BP], sq) * PC_BP; 
    p |= at_sq(g->pcs[PC_BN], sq) * PC_BN; 
    p |= at_sq(g->pcs[PC_BB], sq) * PC_BB; 
    p |= at_sq(g->pcs[PC_BR], sq) * PC_BR; 
    p |= at_sq(g->pcs[PC_BQ], sq) * PC_BQ; 
    p |= at_sq(g->pcs[PC_BK], sq) * PC_BK; 
    return p; 
}

// assumes there is a piece on the square, otherwise returns WP 
static inline piece pc_at(const game *g, square sq) 
{
    return w_pc_at(g, sq) | b_pc_at(g, sq); 
}

static inline void pop_mv(game *g) 
{
    get_vec(&g->hist, g->hist.size - 1, g); 
    pop_vec(&g->hist); 
    g->ply--; 
    g->turn = opp_col(g->turn); 
}

static inline bool in_check(const game *g, color for_col, bboard chk_k, bboard ignore, bboard add) 
{
    color col = for_col; 
    color opp = opp_col(col); 

    // check if any of these squares are attacked 
    bboard tgt = chk_k; 

    // blockers include both colors' pieces other than the target squares 
    bboard occ = (((g->col[col] & ~tgt) | g->col[opp]) & ~ignore) | add; 

    bboard attacks = NO_BITS; 
    bboard tmp; 

    FOR_EACH_BIT(tgt, 
    {
        // pawn 
        if (col == COL_W) 
        {
            // check for black pawns north of the king 
            tmp  = sh_nw(tgt); 
            tmp |= sh_ne(tgt); 
            attacks |= tmp & ((g->pcs[PC_BP] & ~ignore)); 
        }
        else 
        {
            // check for white pawns south of the king 
            tmp  = sh_sw(tgt); 
            tmp |= sh_se(tgt); 
            attacks |= tmp & ((g->pcs[PC_WP] & ~ignore)); 
        }

        // knight 
        tmp = MV_N[s]; 
        attacks |= tmp & g->pcs[make_pc(PC_N, opp)]; 

        // bishop (and queen)
        tmp  = ray(s, SLIDE_DIAG[diag(s)], occ); 
        tmp |= ray(s, SLIDE_ANTI[anti(s)], occ); 
        attacks |= tmp & (g->pcs[make_pc(PC_B, opp)] | g->pcs[make_pc(PC_Q, opp)]); 

        // rook (and queen)
        tmp  = ray(s, SLIDE_FILE[file(s)], occ); 
        tmp |= ray(s, SLIDE_RANK[rank(s)], occ); 
        attacks |= tmp & (g->pcs[make_pc(PC_R, opp)] | g->pcs[make_pc(PC_Q, opp)]); 

        // king 
        tmp = MV_K[s]; 
        attacks |= tmp & g->pcs[make_pc(PC_K, opp)]; 

        if (attacks) return true; 
    });

    return false; 
}

// does not check if the move is legal 
static void push_mv(game *g, move m) 
{
    // save current state 
    push_vec(&g->hist, g); 

    square from = from_sq(m); 
    square to = to_sq(m); 
    square rm_tgt = to; // remove opponent's piece on this square (can be different from `to` if en passant)
    square new_ep = ep_sq(m); 

    piece pc = from_pc(m); 
    piece pro = promoted(m); 
    piece tgt; 

    // note: target will be WP if rm_tgt is empty 
    if (g->turn == COL_W) 
    {
        rm_tgt -= 8 * is_ep(m); // target piece is south (behind the capturing pawn) if ep
        tgt = b_pc_at(g, rm_tgt); 
    }
    else // B
    {
        rm_tgt += 8 * is_ep(m); // target piece is north (behind the capturing pawn) if ep
        tgt = w_pc_at(g, rm_tgt); 
    }

    // take moving piece off the board 
    g->pcs[pc] = clear_sq(g->pcs[pc], from); 

    // WP if rm_tgt is empty, but this is okay because if piece is WP it will re-enable this square right after 
    g->pcs[tgt] = clear_sq(g->pcs[tgt], rm_tgt); 

    // place piece onto new square 
    g->pcs[pro] = set_sq(g->pcs[pro], to); 

    color cur_col = g->turn, oth_col = opp_col(cur_col); 

    // remove rooks from aggregate (in case of castling) 
    g->col[COL_W] ^= g->pcs[PC_WR]; 
    g->col[COL_B] ^= g->pcs[PC_BR]; 

    int castles = cas(m); 
    g->chk = g->pcs[make_pc(PC_K, cur_col)] | CAS_TGTS[castles]; 

    // move the rooks when castling 
    g->pcs[PC_WR] = (g->pcs[PC_WR] & CAS_KEEP_WR[castles]) | CAS_ADD_WR[castles]; 
    g->pcs[PC_BR] = (g->pcs[PC_BR] & CAS_KEEP_BR[castles]) | CAS_ADD_BR[castles]; 

    castle rem_cf = CAS_NONE; 
    // no castling if the king moves
    rem_cf |= CAS_W  * (pc == PC_WK); 
    rem_cf |= CAS_B  * (pc == PC_BK); 
    // no castling if the rook moves or is captured
    rem_cf |= CAS_WK * ((from == H1) | (to == H1)); 
    rem_cf |= CAS_WQ * ((from == A1) | (to == A1)); 
    rem_cf |= CAS_BK * ((from == H8) | (to == H8)); 
    rem_cf |= CAS_BQ * ((from == A8) | (to == A8)); 

    // remove castling rights if necessary 
    g->cas &= ~rem_cf; 

    // re-add rooks from aggregate (in case of castling) 
    g->col[COL_W] ^= g->pcs[PC_WR]; 
    g->col[COL_B] ^= g->pcs[PC_BR]; 

    // update aggregate piece tracking 
    g->col[cur_col] = clear_sq(g->col[cur_col], from); 
    g->col[oth_col] = clear_sq(g->col[oth_col], rm_tgt); 
    g->col[cur_col] = set_sq(g->col[cur_col], to); 

    // update rest of state 
    g->ep = new_ep; 
    g->ply++; 
    g->turn = opp_col(g->turn); 

    // useful to know if the new player is in check 
    g->in_chk = in_check(g, g->turn, g->pcs[make_pc(PC_K, g->turn)], NO_BITS, NO_BITS); 
}

static inline void mvgen_bb(bboard b, square from, piece pc, vector *out) 
{
    FOR_EACH_BIT(b, 
    {
        PUSH_VEC(out, move, make_mv(from, s, pc, pc)); 
    });
}

static inline void mvgen_safe(const game *g, bboard b, square from, piece pc, vector *out) 
{
    FOR_EACH_BIT(b, 
    {
        if (!in_check(g, g->turn, bb(s), bb(from), NO_BITS)) 
        {
            PUSH_VEC(out, move, make_mv(from, s, pc, pc)); 
        }
    });
}

static inline void mvgen_cas_move(square from, square to, piece pc, int cas_index, vector *out) 
{
    PUSH_VEC(out, move, make_cas_mv(from, to, pc, cas_index)); 
}

static inline void mvgen_allow_ep(bboard b, square from, piece pc, int d_sq, vector *out) 
{
    FOR_EACH_BIT(b, 
    {
        PUSH_VEC(out, move, make_allow_ep_mv(from, s, pc, s + d_sq)); 
    });
}

static inline void mvgen_pro(bboard b, square from, piece pc, color col, vector *out) 
{
    FOR_EACH_BIT(b, 
    {
        PUSH_VEC(out, move, make_mv(from, s, pc, make_pc(PC_Q, col))); 
        PUSH_VEC(out, move, make_mv(from, s, pc, make_pc(PC_R, col))); 
        PUSH_VEC(out, move, make_mv(from, s, pc, make_pc(PC_B, col))); 
        PUSH_VEC(out, move, make_mv(from, s, pc, make_pc(PC_N, col))); 
    });
}

static inline void mvgen_might_ep(const game *g, bboard b, square from, piece pc, square ep, vector *out) 
{
    FOR_EACH_BIT(b, 
    {
        if (s != ep) 
        {
            PUSH_VEC(out, move, make_ep_mv(from, s, pc, false)); 
        }
        else if (pc_col(pc) == COL_W) 
        {
            if (!in_check(g, COL_W, g->pcs[PC_WK], bb(from) | bb(ep - 8), bb(ep))) 
            {
                PUSH_VEC(out, move, make_ep_mv(from, s, pc, true)); 
            }
        }
        else // COL_B  
        {
            if (!in_check(g, COL_B, g->pcs[PC_BK], bb(from) | bb(ep + 8), bb(ep))) 
            {
                PUSH_VEC(out, move, make_ep_mv(from, s, pc, true)); 
            }
        }
    });
}

static inline void mvgen_p(const game *g, color col, bboard ok_sqs, const bboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    bboard empty = ~(g->col[COL_W] | g->col[COL_B]); 
    bboard ep = (g->ep != NO_SQ) * bb(g->ep); 
    bboard cur_pos, attack, tgt_sqs; 

    if (col == COL_W) 
    {
        FOR_EACH_BIT(g->pcs[PC_WP], 
        {
            cur_pos = bb(s); 

            // move forward if square is empty  
            attack = sh_n(cur_pos) & empty; 

            // attack left if there is a piece or en passant
            attack |= sh_nw(cur_pos) & (g->col[COL_B] | ep); 

            // attack right if there is a piece or en passant
            attack |= sh_ne(cur_pos) & (g->col[COL_B] | ep); 

            tgt_sqs = TGT_SQS; 
            attack &= tgt_sqs; 

            // move 2 squares if on starting square 
            mvgen_allow_ep((rank(s) == 1) * (sh_nn(cur_pos) & empty & sh_n(empty) & tgt_sqs), s, PC_WP, -8, out); 

            // add non-promoting moves to list 
            mvgen_might_ep(g, attack & NO_RANK_8, s, PC_WP, g->ep, out); 

            // add promoting moves to list 
            mvgen_pro(attack & RANK_8, s, PC_WP, COL_W, out); 
        });
    }
    else 
    {
        FOR_EACH_BIT(g->pcs[PC_BP], 
        {
            cur_pos = bb(s); 

            // move forward if square is empty 
            attack = sh_s(cur_pos) & empty; 

            // attack left if there is a piece or en passant
            attack |= sh_sw(cur_pos) & (g->col[COL_W] | ep); 

            // attack right if there is a piece or en passant
            attack |= sh_se(cur_pos) & (g->col[COL_W] | ep); 

            tgt_sqs = TGT_SQS; 
            attack &= tgt_sqs; 

            // move 2 squares if on starting square 
            mvgen_allow_ep((rank(s) == 6) * (sh_ss(cur_pos) & empty & sh_s(empty) & tgt_sqs), s, PC_BP, 8, out); 

            // add non-promoting moves to list 
            mvgen_might_ep(g, attack & NO_RANK_1, s, PC_BP, g->ep, out); 

            // add promoting moves to list 
            mvgen_pro(attack & RANK_1, s, PC_BP, COL_B, out); 
        });
    }
}

static inline void mvgen_k(const game *g, color col, vector *out) 
{
    bboard occ = g->col[COL_W] | g->col[COL_B]; 
    bboard not_cur_col = ~g->col[col]; 
    bboard attack; 

    piece pc = make_pc(PC_K, col); 
    FOR_EACH_BIT(g->pcs[pc], 
    {
        attack = MV_K[s] & not_cur_col; 
        mvgen_safe(g, attack, s, pc, out); 

        int c_wk = 0 != (opp_col(col) * ((g->cas & CAS_WK) != 0) * ((occ & EMPTY_WK) == 0)); 
        int c_wq = 0 != (opp_col(col) * ((g->cas & CAS_WQ) != 0) * ((occ & EMPTY_WQ) == 0)); 
        int c_bk = 0 != (col * ((g->cas & CAS_BK) != 0) * ((occ & EMPTY_BK) == 0)); 
        int c_bq = 0 != (col * ((g->cas & CAS_BQ) != 0) * ((occ & EMPTY_BQ) == 0)); 

        if (c_wk && !in_check(g, col, g->pcs[pc] | CAS_TGTS[CAS_IDX_WK], NO_BITS, NO_BITS)) 
        {
            mvgen_cas_move(E1, G1, PC_WK, CAS_IDX_WK, out); 
        }

        if (c_wq && !in_check(g, col, g->pcs[pc] | CAS_TGTS[CAS_IDX_WQ], NO_BITS, NO_BITS)) 
        {
            mvgen_cas_move(E1, C1, PC_WK, CAS_IDX_WQ, out); 
        }

        if (c_bk && !in_check(g, col, g->pcs[pc] | CAS_TGTS[CAS_IDX_BK], NO_BITS, NO_BITS)) 
        {
            mvgen_cas_move(E8, G8, PC_BK, CAS_IDX_BK, out); 
        }

        if (c_bq && !in_check(g, col, g->pcs[pc] | CAS_TGTS[CAS_IDX_BQ], NO_BITS, NO_BITS)) 
        {
            mvgen_cas_move(E8, C8, PC_BK, CAS_IDX_BQ, out); 
        }
    });
}

static inline void mvgen_n(const game *g, color col, bboard ok_sqs, const bboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    // empty squares 
    bboard not_cur_col = ~g->col[col]; 
    bboard attack; 

    piece pc = make_pc(PC_N, col); 
    FOR_EACH_BIT(g->pcs[pc], 
    {
        attack = MV_N[s] & not_cur_col; 
        attack &= TGT_SQS;  
        mvgen_bb(attack, s, pc, out); 
    });
}

static inline void mvgen_b(const game *g, color col, bboard ok_sqs, const bboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    bboard not_cur_col = ~g->col[col]; 
    bboard occ = g->col[COL_W] | g->col[COL_B]; 
    bboard attack; 

    piece pc = make_pc(PC_B, col); 
    FOR_EACH_BIT(g->pcs[pc], 
    {
        attack  = ray(s, SLIDE_DIAG[diag(s)], occ); 
        attack |= ray(s, SLIDE_ANTI[anti(s)], occ); 
        attack &= not_cur_col; 
        attack &= TGT_SQS; 
        mvgen_bb(attack, s, pc, out); 
    })
}

static inline void mvgen_r(const game *g, color col, bboard ok_sqs, const bboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    bboard not_cur_col = ~g->col[col]; 
    bboard occ = g->col[COL_W] | g->col[COL_B]; 
    bboard attack; 

    piece pc = make_pc(PC_R, col); 
    FOR_EACH_BIT(g->pcs[pc], 
    {
        attack  = ray(s, SLIDE_FILE[file(s)], occ); 
        attack |= ray(s, SLIDE_RANK[rank(s)], occ); 
        attack &= not_cur_col; 
        attack &= TGT_SQS; 

        mvgen_bb(attack, s, pc, out); 
    })
}

static inline void mvgen_q(const game *g, color col, bboard ok_sqs, const bboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    bboard not_cur_col = ~g->col[col]; 
    bboard occ = g->col[COL_W] | g->col[COL_B]; 
    bboard attack; 

    piece pc = make_pc(PC_Q, col); 
    FOR_EACH_BIT(g->pcs[pc], 
    {
        attack  = ray(s, SLIDE_FILE[file(s)], occ); 
        attack |= ray(s, SLIDE_RANK[rank(s)], occ); 
        attack |= ray(s, SLIDE_DIAG[diag(s)], occ); 
        attack |= ray(s, SLIDE_ANTI[anti(s)], occ); 
        attack &= not_cur_col; 
        attack &= TGT_SQS; 
        mvgen_bb(attack, s, pc, out); 
    })
}

static void gen_mvs(const game *g, vector *out) 
{
    color col = g->turn; 
    color opp = opp_col(col); 

    bboard tgt = g->pcs[make_pc(PC_K, col)]; 
    bboard col_occ = g->col[col]; 
    bboard opp_occ = g->col[opp]; 
    bboard opp_rocc = rev(opp_occ); 

    bboard opp_card = g->pcs[make_pc(PC_R, opp)] | g->pcs[make_pc(PC_Q, opp)]; 
    bboard opp_diag = g->pcs[make_pc(PC_B, opp)] | g->pcs[make_pc(PC_Q, opp)]; 

    square sq = lsb(tgt), rsq = 63 - sq; 

    bboard dirs[CHECK_CNT]; 
    uint8_t blocking[CHECK_DIR_CNT]; 

    uint16_t attacked = 0; 
    uint16_t pinned = 0; 

    // diag movement to occupied square (ignoring current color pieces and including non-relevant enemy pcs)
    dirs[CHECK_NW] = pos_ray(sq, SLIDE_ANTI[anti(sq)], opp_occ); 
    dirs[CHECK_SE] = rev(pos_ray(rsq, SLIDE_ANTI[anti(rsq)], opp_rocc)); 
    dirs[CHECK_NE] = pos_ray(sq, SLIDE_DIAG[diag(sq)], opp_occ); 
    dirs[CHECK_SW] = rev(pos_ray(rsq, SLIDE_DIAG[diag(rsq)], opp_rocc)); 

    // cardinal movement to occupied square (ignoring current color pieces and including non-relevant enemy pcs)
    dirs[CHECK_N] = pos_ray(sq, SLIDE_FILE[file(sq)], opp_occ); 
    dirs[CHECK_S] = rev(pos_ray(rsq, SLIDE_FILE[file(rsq)], opp_rocc)); 
    dirs[CHECK_E] = pos_ray(sq, SLIDE_RANK[rank(sq)], opp_occ); 
    dirs[CHECK_W] = rev(pos_ray(rsq, SLIDE_RANK[rank(rsq)], opp_rocc)); 

    // knight 
    dirs[CHECK_PC_N] = MV_N[sq] & g->pcs[make_pc(PC_N, opp)]; 

    // pawn 
    if (col == COL_W) 
    {
        dirs[CHECK_PC_P]  = sh_nw(tgt); 
        dirs[CHECK_PC_P] |= sh_ne(tgt); 
    }
    else 
    {
        dirs[CHECK_PC_P]  = sh_sw(tgt); 
        dirs[CHECK_PC_P] |= sh_se(tgt); 
    }
    dirs[CHECK_PC_P] &= g->pcs[make_pc(PC_P, opp)]; 

    // determine number of blockers (assuming that the direction has an attacker) 
    blocking[CHECK_N] = popcnt(dirs[CHECK_N] & col_occ); 
    blocking[CHECK_S] = popcnt(dirs[CHECK_S] & col_occ); 
    blocking[CHECK_E] = popcnt(dirs[CHECK_E] & col_occ); 
    blocking[CHECK_W] = popcnt(dirs[CHECK_W] & col_occ); 
    blocking[CHECK_NE] = popcnt(dirs[CHECK_NE] & col_occ); 
    blocking[CHECK_NW] = popcnt(dirs[CHECK_NW] & col_occ); 
    blocking[CHECK_SE] = popcnt(dirs[CHECK_SE] & col_occ); 
    blocking[CHECK_SW] = popcnt(dirs[CHECK_SW] & col_occ); 

    // first figure out which directions have an attacker (ignore current color pinned pcs)

    // directions might hit a non-attacking opponent pc, or even no piece at all
    attacked |= (((dirs[CHECK_N] & opp_card) != 0) & (blocking[CHECK_N] <= 1)) << CHECK_N; 
    attacked |= (((dirs[CHECK_S] & opp_card) != 0) & (blocking[CHECK_S] <= 1)) << CHECK_S; 
    attacked |= (((dirs[CHECK_E] & opp_card) != 0) & (blocking[CHECK_E] <= 1)) << CHECK_E; 
    attacked |= (((dirs[CHECK_W] & opp_card) != 0) & (blocking[CHECK_W] <= 1)) << CHECK_W; 
    attacked |= (((dirs[CHECK_NE] & opp_diag) != 0) & (blocking[CHECK_NE] <= 1)) << CHECK_NE; 
    attacked |= (((dirs[CHECK_NW] & opp_diag) != 0) & (blocking[CHECK_NW] <= 1)) << CHECK_NW; 
    attacked |= (((dirs[CHECK_SE] & opp_diag) != 0) & (blocking[CHECK_SE] <= 1)) << CHECK_SE; 
    attacked |= (((dirs[CHECK_SW] & opp_diag) != 0) & (blocking[CHECK_SW] <= 1)) << CHECK_SW;
    // there are already checked against correct opponent pieces and cannot be blocked 
    attacked |= (dirs[CHECK_PC_N] != 0) << CHECK_PC_N; 
    attacked |= (dirs[CHECK_PC_P] != 0) << CHECK_PC_P; 

    // determine pins 

    // these need to have an attacker, and exactly one friendly piece in the way 
    pinned |= ((attacked >> CHECK_N) & (blocking[CHECK_N] == 1)) << CHECK_N; 
    pinned |= ((attacked >> CHECK_S) & (blocking[CHECK_S] == 1)) << CHECK_S; 
    pinned |= ((attacked >> CHECK_E) & (blocking[CHECK_E] == 1)) << CHECK_E; 
    pinned |= ((attacked >> CHECK_W) & (blocking[CHECK_W] == 1)) << CHECK_W; 
    pinned |= ((attacked >> CHECK_NE) & (blocking[CHECK_NE] == 1)) << CHECK_NE; 
    pinned |= ((attacked >> CHECK_NW) & (blocking[CHECK_NW] == 1)) << CHECK_NW; 
    pinned |= ((attacked >> CHECK_SE) & (blocking[CHECK_SE] == 1)) << CHECK_SE; 
    pinned |= ((attacked >> CHECK_SW) & (blocking[CHECK_SW] == 1)) << CHECK_SW; 

    attacked &= ~pinned; 

    int n_checks = popcnt16(attacked); 

    if (n_checks == 0) 
    {
        // all moves 
        mvgen_p(g, col, ALL_BITS, dirs, pinned, PIN_IDX[sq], out); 
        mvgen_n(g, col, ALL_BITS, dirs, pinned, PIN_IDX[sq], out); 
        mvgen_b(g, col, ALL_BITS, dirs, pinned, PIN_IDX[sq], out); 
        mvgen_r(g, col, ALL_BITS, dirs, pinned, PIN_IDX[sq], out); 
        mvgen_q(g, col, ALL_BITS, dirs, pinned, PIN_IDX[sq], out); 
        mvgen_k(g, col, out); 
    }
    else if (n_checks == 1) 
    {
        // all moves, but must remove check 

        // for sliding attacking pc: 
        //   all squares that block or capture the piece
        // knight or pawn: 
        //   only allows capturing the piece (as it cannot be blocked)
        bboard tgt_sqs = dirs[lsb(attacked)]; 

        mvgen_p(g, col, tgt_sqs | ((g->ep != NO_SQ) * bb(g->ep)), dirs, pinned, PIN_IDX[sq], out); 
        mvgen_n(g, col, tgt_sqs, dirs, pinned, PIN_IDX[sq], out); 
        mvgen_b(g, col, tgt_sqs, dirs, pinned, PIN_IDX[sq], out); 
        mvgen_r(g, col, tgt_sqs, dirs, pinned, PIN_IDX[sq], out); 
        mvgen_q(g, col, tgt_sqs, dirs, pinned, PIN_IDX[sq], out); 
        mvgen_k(g, col, out); 
    }
    else // double check: must move the king 
    {
        // move king 
        mvgen_k(g, col, out); 
    }
}

static const int EVAL_P[64] = 
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

static const int EVAL_N[64] = 
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

static const int EVAL_B[64] = 
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

static const int EVAL_R[64] = 
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

static const int EVAL_Q[64] = 
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

static const int EVAL_K[64] = 
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

static inline int eval_bb(bboard pcs, const int eval[64]) 
{
    int sum = 0; 

    FOR_EACH_BIT(pcs, 
    {
        sum += eval[s]; 
    });

    return sum; 
}

static inline int eval(const game *g, int num_moves) 
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
    eval += 100 * (popcnt(g->pcs[PC_WP]) - popcnt(g->pcs[PC_BP])); 
    eval += 310 * (popcnt(g->pcs[PC_WN]) - popcnt(g->pcs[PC_BN])); 
    eval += 320 * (popcnt(g->pcs[PC_WB]) - popcnt(g->pcs[PC_BB])); 
    eval += 500 * (popcnt(g->pcs[PC_WR]) - popcnt(g->pcs[PC_BR])); 
    eval += 900 * (popcnt(g->pcs[PC_WQ]) - popcnt(g->pcs[PC_BQ])); 
    eval += 10000 * (popcnt(g->pcs[PC_WK]) - popcnt(g->pcs[PC_BK])); 

    // bishop pair 
    eval += 15 * ((popcnt(g->pcs[PC_WB]) >= 2) - (popcnt(g->pcs[PC_BB]) >= 2)); 

    eval += eval_bb(g->pcs[PC_WP], EVAL_P); 
    eval -= eval_bb(rrow(g->pcs[PC_BP]), EVAL_P); 

    eval += eval_bb(g->pcs[PC_WN], EVAL_N); 
    eval -= eval_bb(rrow(g->pcs[PC_BN]), EVAL_N); 

    eval += eval_bb(g->pcs[PC_WB], EVAL_B); 
    eval -= eval_bb(rrow(g->pcs[PC_BB]), EVAL_B); 

    eval += eval_bb(g->pcs[PC_WR], EVAL_R); 
    eval -= eval_bb(rrow(g->pcs[PC_BR]), EVAL_R); 

    eval += eval_bb(g->pcs[PC_WQ], EVAL_Q); 
    eval -= eval_bb(rrow(g->pcs[PC_BQ]), EVAL_Q); 

    eval += eval_bb(g->pcs[PC_WK], EVAL_K); 
    eval -= eval_bb(rrow(g->pcs[PC_BK]), EVAL_K); 

    return eval; 
}

static void print_game(const game *g) 
{
    static const char *empty = "."; 

    vector moves; 
    CREATE_VEC(&moves, move); 
    gen_mvs(g, &moves);  
    size_t num_moves = moves.size; 

    printf("  +-----------------+\n"); 
    for (int rank = 7; rank >= 0; rank--) 
    {
        printf("%d | ", rank + 1); 
        for (int file = 0; file < 8; file++) 
        {
            square sq = make_sq(file, rank); 
            const char *str = empty; 
            for (piece p = 0; p < PC_CNT; p++) 
            {
                if (at_sq(g->pcs[p], sq)) 
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
    printf("En passant: %s\n", g->ep == NO_SQ ? "(none)" : sq_str(g->ep)); 
    printf("In check: %s\n", g->in_chk ? "yes" : "no"); 
    printf("# moves: %zu\n", num_moves); 
    printf("Static eval: %.2f\n", eval(g, num_moves) * 0.01); 
    printf("%s to move\n", g->turn ? "Black" : "White"); 

    destroy_vec(&moves); 
}

static inline uint64_t _perft(game *g, vector *moves, int depth, bool verbose) 
{
    uint64_t total = 0, start = moves->size, size; 

    if (depth > 1) 
    {
        gen_mvs(g, moves); 
        size = moves->size; 

        // print_game(g); 

        for (size_t i = start; i < size; i++) 
        {
            push_mv(g, AT_VEC(moves, move, i)); 
            total += _perft(g, moves, depth - 1, false); 
            pop_mv(g); 
        }

        pop_vec_size(moves, start); 
    }
    else if (depth == 1) 
    {
        gen_mvs(g, moves); 
        total += moves->size - start; 
        pop_vec_size(moves, start); 
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

static inline uint64_t perft(game *g, int depth) 
{
    vector moves; 
    CREATE_VEC(&moves, move); 

    uint64_t total = _perft(g, &moves, depth, true); 

    destroy_vec(&moves); 
    return total; 
}

typedef struct pv_line pv_line; 
struct pv_line 
{
    size_t n_moves; 
    move moves[128]; 
};

static inline int qsearch(game *g, int alpha, int beta, int depth, vector *moves) 
{
    size_t start = moves->size; 

    gen_mvs(g, moves); 

    int stand_pat = col_sign(g) * eval(g, moves->size - start); 

    if (stand_pat >= beta) 
    {
        pop_vec_size(moves, start); 
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
            move mv = AT_VEC(moves, move, i); 

            // only consider captures 
            if (is_ep(mv) | at_sq(g->col[opp_col(g->turn)], to_sq(mv))) 
            {
                push_mv(g, mv); 
                int score = -qsearch(g, -beta, -alpha, depth - 1, moves); 
                pop_mv(g); 

                if (score >= beta) 
                {
                    pop_vec_size(moves, start); 
                    return beta; 
                }
                if (score > alpha) 
                {
                    alpha = score; 
                }
            }
        }
    }

    pop_vec_size(moves, start); 
    return alpha; 
}

static inline int negamax(game *g, int alpha, int beta, int depth, vector *moves, pv_line *pv) 
{
    pv_line line; 
    size_t start = moves->size; 

    gen_mvs(g, moves); 
    int num_moves = moves->size - start; 

    if (num_moves == 0 || depth <= 0) 
    {
        pv->n_moves = 0; 
        pop_vec_size(moves, start); 
        return qsearch(g, alpha, beta, 32, moves); 
    }

    for (size_t i = start; i < moves->size; i++) 
    {
        push_mv(g, AT_VEC(moves, move, i)); 
        int eval = -negamax(g, -beta, -alpha, depth - 1, moves, &line); 
        pop_mv(g); 

        if (eval >= beta) 
        {
            pop_vec_size(moves, start); 
            return beta; 
        }

        if (AT_VEC(moves, move, i) == 0) 
        {
            printf("what\n"); 
            fflush(stdout); 
        }

        if (eval > alpha) 
        {
            alpha = eval; 
            pv->moves[0] = AT_VEC(moves, move, i); 
            memcpy(pv->moves + 1, line.moves, line.n_moves * sizeof(move)); 
            pv->n_moves = line.n_moves + 1; 
        }
    }

    pop_vec_size(moves, start); 
    return alpha; 
}

static inline void search(game *g, int search_depth, vector *pv, int *eval) 
{
    vector moves; 
    CREATE_VEC(&moves, move); 

    pv_line line; 
    for (int depth = 1; depth <= search_depth; depth++) 
    {
        clear_vec(&moves); 
        int eval = negamax(g, -INT_MAX, INT_MAX, depth, &moves, &line); 
        printf("info depth %d seldepth %zu multipv 1 score cp %d nodes 1 nps 1 pv ", depth, line.n_moves, eval); 
        for (size_t i = 0; i < line.n_moves; i++) 
        {
            print_mv_end(line.moves[i], " "); 
        }
        printf("\n"); 

        fflush(stdout); 
    }

    printf("bestmove "); 
    print_mv_end(line.moves[0], "\n"); 

    destroy_vec(&moves); 
}