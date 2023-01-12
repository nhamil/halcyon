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
#define GAME_TARGET_SQUARES ( \
    ok_squares & ( \
        dirs[pin_idx[sq]] | (BITBOARD_ALL * \
            ( \
                ((~pinned >> pin_idx[sq]) & 1) | \
                ((dirs[pin_idx[sq]] & (1ULL << sq)) == 0) \
            ) \
        ) \
    ) \
)

#define GAME_STARTING_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

struct game 
{
    bitboard pieces[PIECE_COUNT]; 
    bitboard colors[2]; 
    bitboard check; 
    castle_flags castle; 
    square en_passant; 
    bool in_check; 

    vector history; 
    int ply; 
    color turn; 
};

static void game_reset(game *g) 
{
    vector_clear(&g->history); 
    memset(&g->pieces, 0, sizeof(g->pieces)); 
    memset(&g->colors, 0, sizeof(g->colors)); 
    // g->check = BITBOARD_NONE; 
    g->castle = CASTLE_NONE; 
    g->en_passant = SQUARE_NONE; 
    g->in_check = false; 
    g->ply = 0; 
    g->turn = COLOR_W; 
}

static void game_create(game *g) 
{
    vector_create(&g->history, offsetof(game, history)); 
    game_reset(g); 
}

static void game_destroy(game *g) 
{
    vector_destroy(&g->history); 
}

static void game_load_fen(game *g, const char *fen) 
{
    game_reset(g); 

    // decrement once so simple incrementing while loop 
    // starts at the first character
    fen--; 

    int mode = 0, turn = 0; 
    square sq = SQUARE_A1; 
    while (*++fen) 
    {
        if (*fen == ' ') 
        {
            mode++; 
            sq = SQUARE_A1; 
            continue; 
        }
        switch (mode) 
        {
            case 0: // board setup 
                switch (*fen) 
                {
                    case 'P': 
                        g->pieces[PIECE_WP] = bitboard_set(g->pieces[PIECE_WP], square_flip_rank(sq++)); 
                        break; 
                    case 'p': 
                        g->pieces[PIECE_BP] = bitboard_set(g->pieces[PIECE_BP], square_flip_rank(sq++)); 
                        break; 
                    case 'N': 
                        g->pieces[PIECE_WN] = bitboard_set(g->pieces[PIECE_WN], square_flip_rank(sq++)); 
                        break; 
                    case 'n': 
                        g->pieces[PIECE_BN] = bitboard_set(g->pieces[PIECE_BN], square_flip_rank(sq++)); 
                        break; 
                    case 'B': 
                        g->pieces[PIECE_WB] = bitboard_set(g->pieces[PIECE_WB], square_flip_rank(sq++)); 
                        break; 
                    case 'b': 
                        g->pieces[PIECE_BB] = bitboard_set(g->pieces[PIECE_BB], square_flip_rank(sq++)); 
                        break; 
                    case 'R': 
                        g->pieces[PIECE_WR] = bitboard_set(g->pieces[PIECE_WR], square_flip_rank(sq++)); 
                        break; 
                    case 'r': 
                        g->pieces[PIECE_BR] = bitboard_set(g->pieces[PIECE_BR], square_flip_rank(sq++)); 
                        break; 
                    case 'Q': 
                        g->pieces[PIECE_WQ] = bitboard_set(g->pieces[PIECE_WQ], square_flip_rank(sq++)); 
                        break; 
                    case 'q': 
                        g->pieces[PIECE_BQ] = bitboard_set(g->pieces[PIECE_BQ], square_flip_rank(sq++)); 
                        break; 
                    case 'K': 
                        g->pieces[PIECE_WK] = bitboard_set(g->pieces[PIECE_WK], square_flip_rank(sq++)); 
                        break; 
                    case 'k': 
                        g->pieces[PIECE_BK] = bitboard_set(g->pieces[PIECE_BK], square_flip_rank(sq++)); 
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
                        g->castle |= CASTLE_WK; 
                        break; 
                    case 'k': 
                        g->castle |= CASTLE_BK; 
                        break; 
                    case 'Q': 
                        g->castle |= CASTLE_WQ; 
                        break; 
                    case 'q': 
                        g->castle |= CASTLE_BQ; 
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
                        sq = square_make(*fen - 'a', square_rank(sq)); 
                        g->en_passant = sq; 
                        break; 
                    case '1': 
                    case '2': 
                    case '3': 
                    case '4': 
                    case '5': 
                    case '6': 
                    case '7': 
                    case '8': 
                        sq = square_make(square_file(sq), *fen - '1'); 
                        g->en_passant = sq; 
                        break; 
                    case '-': 
                        sq = SQUARE_NONE; 
                        g->en_passant = sq; 
                        break; 
                    default: 
                        printf("Unknown FEN character (en passant): %c\n", *fen); 
                        sq = SQUARE_NONE; 
                        g->en_passant = sq; 
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
                        printf("Unknown FEN character (halfmove): %c\n", *fen); 
                        sq = SQUARE_NONE; 
                        g->en_passant = sq; 
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
                        sq = SQUARE_NONE; 
                        g->en_passant = sq; 
                        break; 
                }
                break; 
        }
    }
    if (mode >= 5) g->ply += turn * 2 - 2; 

    g->turn = g->ply & 1; 

    for (piece p = PIECE_P; p <= PIECE_K; p++) 
    {
        g->colors[COLOR_W] |= g->pieces[piece_make_colored(p, COLOR_W)]; 
        g->colors[COLOR_B] |= g->pieces[piece_make_colored(p, COLOR_B)]; 
    }
}

static void game_create_fen(game *g, const char *fen) 
{
    game_create(g); 
    game_load_fen(g, fen); 
}

// assumes there is a white piece on the square, otherwise returns WP
static inline piece game_white_piece_at(const game *g, square sq) 
{
    piece p = 0; 
    p |= bitboard_at(g->pieces[PIECE_WP], sq) * PIECE_WP; 
    p |= bitboard_at(g->pieces[PIECE_WN], sq) * PIECE_WN; 
    p |= bitboard_at(g->pieces[PIECE_WB], sq) * PIECE_WB; 
    p |= bitboard_at(g->pieces[PIECE_WR], sq) * PIECE_WR; 
    p |= bitboard_at(g->pieces[PIECE_WQ], sq) * PIECE_WQ; 
    p |= bitboard_at(g->pieces[PIECE_WK], sq) * PIECE_WK; 
    return p; 
}

// assumes there is a black piece on the square, otherwise returns WP
static inline piece game_black_piece_at(const game *g, square sq) 
{
    piece p = 0; 
    p |= bitboard_at(g->pieces[PIECE_BP], sq) * PIECE_BP; 
    p |= bitboard_at(g->pieces[PIECE_BN], sq) * PIECE_BN; 
    p |= bitboard_at(g->pieces[PIECE_BB], sq) * PIECE_BB; 
    p |= bitboard_at(g->pieces[PIECE_BR], sq) * PIECE_BR; 
    p |= bitboard_at(g->pieces[PIECE_BQ], sq) * PIECE_BQ; 
    p |= bitboard_at(g->pieces[PIECE_BK], sq) * PIECE_BK; 
    return p; 
}

// assumes there is a piece on the square, otherwise returns WP 
static inline piece game_piece_at(const game *g, square sq) 
{
    return game_white_piece_at(g, sq) | game_black_piece_at(g, sq); 
}

static inline void game_pop_move(game *g) 
{
    vector_get(&g->history, g->history.size - 1, g); 
    vector_pop(&g->history); 
    g->ply--; 
    g->turn = color_get_other(g->turn); 
}

static inline bool game_in_check(const game *g, color for_color, bitboard check_king) 
{
    color col = for_color; 
    color opp = color_get_other(col); 

    // check if any of these squares are attacked 
    bitboard target = check_king; 

    // blockers include both colors' pieces other than the target squares 
    bitboard occupants = ((g->colors[col] & ~target) | g->colors[opp]); 

    bitboard attacks = BITBOARD_NONE; 
    bitboard tmp; 

    BITBOARD_FOR_EACH_BIT(target, 
    {
        // pawn 
        if (col == COLOR_W) 
        {
            // check for black pawns north of the king 
            tmp  = bitboard_shift_nw(target); 
            tmp |= bitboard_shift_ne(target); 
            attacks |= tmp & g->pieces[PIECE_BP]; 
        }
        else 
        {
            // check for white pawns south of the king 
            tmp  = bitboard_shift_sw(target); 
            tmp |= bitboard_shift_se(target); 
            attacks |= tmp & g->pieces[PIECE_WP]; 
        }

        // knight 
        tmp = BITBOARD_KNIGHT_MOVES[sq]; 
        attacks |= tmp & g->pieces[piece_make_colored(PIECE_N, opp)]; 

        // bishop (and queen)
        tmp  = bitboard_cast_ray(sq, BITBOARD_SLIDE_DIAG[square_diagonal(sq)], occupants); 
        tmp |= bitboard_cast_ray(sq, BITBOARD_SLIDE_ANTI[square_antidiagonal(sq)], occupants); 
        attacks |= tmp & (g->pieces[piece_make_colored(PIECE_B, opp)] | g->pieces[piece_make_colored(PIECE_Q, opp)]); 

        // rook (and queen)
        tmp  = bitboard_cast_ray(sq, BITBOARD_SLIDE_FILE[square_file(sq)], occupants); 
        tmp |= bitboard_cast_ray(sq, BITBOARD_SLIDE_RANK[square_rank(sq)], occupants); 
        attacks |= tmp & (g->pieces[piece_make_colored(PIECE_R, opp)] | g->pieces[piece_make_colored(PIECE_Q, opp)]); 

        // king 
        tmp = BITBOARD_KING_MOVES[sq]; 
        attacks |= tmp & g->pieces[piece_make_colored(PIECE_K, opp)]; 

        if (attacks) return true; 
    });

    return false; 
}

// does not check if the move is legal 
static void game_push_move(game *g, move m) 
{
    // save current state 
    vector_push(&g->history, g); 

    square from = move_get_from_square(m); 
    square to = move_get_to_square(m); 
    square rm_tgt = to; // remove opponent's piece on this square (can be different from `to` if en passant)
    square new_ep = move_get_en_passant_square(m); 

    piece pc = move_get_from_piece(m); 
    piece pro = move_get_promotion_piece(m); 
    piece tgt; 

    // note: tgt will be WP if rm_tgt is empty 
    if (g->turn == COLOR_W) 
    {
        rm_tgt -= 8 * move_takes_en_passant(m); // target piece is south (behind the capturing pawn) if ep
        tgt = game_black_piece_at(g, rm_tgt); 
    }
    else // B
    {
        rm_tgt += 8 * move_takes_en_passant(m); // target piece is north (behind the capturing pawn) if ep
        tgt = game_white_piece_at(g, rm_tgt); 
    }

    // take moving piece off the board 
    g->pieces[pc] = bitboard_clear(g->pieces[pc], from); 

    // WP if rm_tgt is empty, but this is okay because if pc is WP it will re-enable this square right after 
    g->pieces[tgt] = bitboard_clear(g->pieces[tgt], rm_tgt); 

    // place piece onto new square 
    g->pieces[pro] = bitboard_set(g->pieces[pro], to); 

    color cur_col = g->turn, opp_col = color_get_other(cur_col); 

    // remove rooks from aggregate (in case of castling) 
    g->colors[COLOR_W] ^= g->pieces[PIECE_WR]; 
    g->colors[COLOR_B] ^= g->pieces[PIECE_BR]; 

    int castle = move_get_castle(m); 
    g->check = g->pieces[piece_make_colored(PIECE_K, cur_col)] | BITBOARD_CASTLE_TARGETS[castle]; 

    // move the rooks when castling 
    g->pieces[PIECE_WR] = (g->pieces[PIECE_WR] & BITBOARD_CASTLE_KEEP_WR[castle]) | BITBOARD_CASTLE_ADD_WR[castle]; 
    g->pieces[PIECE_BR] = (g->pieces[PIECE_BR] & BITBOARD_CASTLE_KEEP_BR[castle]) | BITBOARD_CASTLE_ADD_BR[castle]; 

    castle_flags rem_cf = CASTLE_NONE; 
    // no castling if the king moves
    rem_cf |= CASTLE_W  * (pc == PIECE_WK); 
    rem_cf |= CASTLE_B  * (pc == PIECE_BK); 
    // no castling if the rook moves or is captured
    rem_cf |= CASTLE_WK * ((from == SQUARE_H1) | (to == SQUARE_H1)); 
    rem_cf |= CASTLE_WQ * ((from == SQUARE_A1) | (to == SQUARE_A1)); 
    rem_cf |= CASTLE_BK * ((from == SQUARE_H8) | (to == SQUARE_H8)); 
    rem_cf |= CASTLE_BQ * ((from == SQUARE_A8) | (to == SQUARE_A8)); 

    // remove castling rights if necessary 
    g->castle &= ~rem_cf; 

    // re-add rooks from aggregate (in case of castling) 
    g->colors[COLOR_W] ^= g->pieces[PIECE_WR]; 
    g->colors[COLOR_B] ^= g->pieces[PIECE_BR]; 

    // update aggregate piece tracking 
    g->colors[cur_col] = bitboard_clear(g->colors[cur_col], from); 
    g->colors[opp_col] = bitboard_clear(g->colors[opp_col], rm_tgt); 
    g->colors[cur_col] = bitboard_set(g->colors[cur_col], to); 

    // update rest of state 
    g->en_passant = new_ep; 
    g->ply++; 
    g->turn = color_get_other(g->turn); 

    // useful to know if the new player is in check 
    g->in_check = game_in_check(g, g->turn, g->pieces[piece_make_colored(PIECE_K, g->turn)]); 
}

static inline void game_movegen_push_bitboard_moves(bitboard b, square from, piece pc, vector *out) 
{
    BITBOARD_FOR_EACH_BIT(b, {
        VECTOR_PUSH_TYPE(out, move, move_make(from, sq, pc, pc)); 
    });
}

static inline void game_movegen_push_castle_move(square from, square to, piece pc, int castle_index, vector *out) 
{
    VECTOR_PUSH_TYPE(out, move, move_make_castle(from, to, pc, castle_index)); 
}

static inline void game_movegen_push_allow_ep_moves(bitboard b, square from, piece pc, int d_sq, vector *out) 
{
    BITBOARD_FOR_EACH_BIT(b, 
    {
        VECTOR_PUSH_TYPE(out, move, move_make_allow_en_passant(from, sq, pc, sq + d_sq)); 
    });
}

static inline void game_movegen_push_promotion_moves(bitboard b, square from, piece pc, color col, vector *out) 
{
    BITBOARD_FOR_EACH_BIT(b, {
        VECTOR_PUSH_TYPE(out, move, move_make(from, sq, pc, piece_make_colored(PIECE_Q, col))); 
        VECTOR_PUSH_TYPE(out, move, move_make(from, sq, pc, piece_make_colored(PIECE_R, col))); 
        VECTOR_PUSH_TYPE(out, move, move_make(from, sq, pc, piece_make_colored(PIECE_B, col))); 
        VECTOR_PUSH_TYPE(out, move, move_make(from, sq, pc, piece_make_colored(PIECE_N, col))); 
    });
}

static inline void game_movegen_push_might_take_en_passant_moves(const game *g, bitboard b, square from, piece pc, square ep, vector *out) 
{
    BITBOARD_FOR_EACH_BIT(b, {
        move m = move_make_take_en_passant(from, sq, pc, sq == ep); 
        vector_push(out, &m); 

        if (sq == ep) 
        {
            // TODO check if legal without trying the move 
            game_push_move((game *) g, m); 
            if (game_in_check(g, color_get_other(g->turn), g->pieces[piece_make_colored(PIECE_K, color_get_other(g->turn))])) 
            {
                vector_pop(out); 
            }
            game_pop_move((game *) g); 
        }
    });
}

static inline void game_movegen_add_pawn_moves(const game *g, color col, bitboard ok_squares, const bitboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    bitboard empty = ~(g->colors[COLOR_W] | g->colors[COLOR_B]); 
    bitboard ep = (g->en_passant != SQUARE_NONE) * bitboard_make_position(g->en_passant); 
    bitboard cur_pos, attack, target_squares; 

    if (col == COLOR_W) 
    {
        BITBOARD_FOR_EACH_BIT(g->pieces[PIECE_WP], 
        {
            cur_pos = bitboard_make_position(sq); 

            // move forward if square is empty 
            attack = bitboard_shift_n(cur_pos) & empty; 

            // attack left if there is a piece or en passant
            attack |= bitboard_shift_nw(cur_pos) & (g->colors[COLOR_B] | ep); 

            // attack right if there is a piece or en passant
            attack |= bitboard_shift_ne(cur_pos) & (g->colors[COLOR_B] | ep); 

            target_squares = GAME_TARGET_SQUARES; 
            attack &= target_squares; 

            // move 2 squares if on starting square 
            game_movegen_push_allow_ep_moves((square_rank(sq) == 1) * (bitboard_shift_nn(cur_pos) & empty & bitboard_shift_n(empty) & target_squares), sq, PIECE_WP, -8, out); 

            // add non-promoting moves to list 
            game_movegen_push_might_take_en_passant_moves(g, attack & BITBOARD_NO_8_RANK, sq, PIECE_WP, g->en_passant, out); 

            // add promoting moves to list 
            game_movegen_push_promotion_moves(attack & BITBOARD_8_RANK, sq, PIECE_WP, COLOR_W, out); 
        });
    }
    else 
    {
        BITBOARD_FOR_EACH_BIT(g->pieces[PIECE_BP], 
        {
            cur_pos = bitboard_make_position(sq); 

            // move forward if square is empty 
            attack = bitboard_shift_s(cur_pos) & empty; 

            // attack left if there is a piece or en passant
            attack |= bitboard_shift_sw(cur_pos) & (g->colors[COLOR_W] | ep); 

            // attack right if there is a piece or en passant
            attack |= bitboard_shift_se(cur_pos) & (g->colors[COLOR_W] | ep); 

            target_squares = GAME_TARGET_SQUARES; 
            attack &= target_squares; 

            // move 2 squares if on starting square 
            game_movegen_push_allow_ep_moves((square_rank(sq) == 6) * (bitboard_shift_ss(cur_pos) & empty & bitboard_shift_s(empty) & target_squares), sq, PIECE_BP, 8, out); 

            // add non-promoting moves to list 
            game_movegen_push_might_take_en_passant_moves(g, attack & BITBOARD_NO_1_RANK, sq, PIECE_BP, g->en_passant, out); 

            // add promoting moves to list 
            game_movegen_push_promotion_moves(attack & BITBOARD_1_RANK, sq, PIECE_BP, COLOR_B, out); 
        });
    }
}

static inline void game_movegen_remove_illegal_moves(const game *state, size_t start, vector *out) 
{
    game *g = (game *) state; 
    size_t i = start; 

    while (i < out->size) 
    {
        game_push_move(g, VECTOR_AT_TYPE(out, move, i)); 

        if (game_in_check(g, color_get_other(g->turn), g->check)) 
        {
            vector_set(out, i, vector_at(out, out->size - 1)); 
            vector_pop(out); 
        }
        else 
        {
            i++; 
        }

        game_pop_move(g); 
    }
}

static inline void game_movegen_add_king_moves(const game *g, color col, vector *out) 
{
    bitboard occupants = g->colors[COLOR_W] | g->colors[COLOR_B]; 
    bitboard not_cur_col = ~g->colors[col]; 
    bitboard attack; 

    size_t start = out->size; 

    piece pc = piece_make_colored(PIECE_K, col); 
    BITBOARD_FOR_EACH_BIT(g->pieces[pc], 
    {
        attack = BITBOARD_KING_MOVES[sq] & not_cur_col; 
        game_movegen_push_bitboard_moves(attack, sq, pc, out); 

        int c_wk = 0 != (color_get_other(col) * ((g->castle & CASTLE_WK) != 0) * ((occupants & BITBOARD_EMPTY_WK) == 0)); 
        int c_wq = 0 != (color_get_other(col) * ((g->castle & CASTLE_WQ) != 0) * ((occupants & BITBOARD_EMPTY_WQ) == 0)); 
        int c_bk = 0 != (col * ((g->castle & CASTLE_BK) != 0) * ((occupants & BITBOARD_EMPTY_BK) == 0)); 
        int c_bq = 0 != (col * ((g->castle & CASTLE_BQ) != 0) * ((occupants & BITBOARD_EMPTY_BQ) == 0)); 

        if (c_wk) 
        {
            game_movegen_push_castle_move(SQUARE_E1, SQUARE_G1, PIECE_WK, MOVE_CASTLE_WK, out); 
        }

        if (c_wq) 
        {
            game_movegen_push_castle_move(SQUARE_E1, SQUARE_C1, PIECE_WK, MOVE_CASTLE_WQ, out); 
        }

        if (c_bk) 
        {
            game_movegen_push_castle_move(SQUARE_E8, SQUARE_G8, PIECE_BK, MOVE_CASTLE_BK, out); 
        }

        if (c_bq) 
        {
            game_movegen_push_castle_move(SQUARE_E8, SQUARE_C8, PIECE_BK, MOVE_CASTLE_BQ, out); 
        }
    });

    game_movegen_remove_illegal_moves(g, start, out); 
}

static inline void game_movegen_add_knight_moves(const game *g, color col, bitboard ok_squares, const bitboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    // empty squares 
    bitboard not_cur_col = ~g->colors[col]; 
    bitboard attack; 

    piece pc = piece_make_colored(PIECE_N, col); 
    BITBOARD_FOR_EACH_BIT(g->pieces[pc], 
    {
        attack = BITBOARD_KNIGHT_MOVES[sq] & not_cur_col; 
        attack &= GAME_TARGET_SQUARES;  
        game_movegen_push_bitboard_moves(attack, sq, pc, out); 
    });
}

static inline void game_movegen_add_bishop_moves(const game *g, color col, bitboard ok_squares, const bitboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    bitboard not_cur_col = ~g->colors[col]; 
    bitboard occupants = g->colors[COLOR_W] | g->colors[COLOR_B]; 
    bitboard attack; 

    piece pc = piece_make_colored(PIECE_B, col); 
    BITBOARD_FOR_EACH_BIT(g->pieces[pc], 
    {
        attack  = bitboard_cast_ray(sq, BITBOARD_SLIDE_DIAG[square_diagonal(sq)], occupants); 
        attack |= bitboard_cast_ray(sq, BITBOARD_SLIDE_ANTI[square_antidiagonal(sq)], occupants); 
        attack &= not_cur_col; 
        attack &= GAME_TARGET_SQUARES; 
        game_movegen_push_bitboard_moves(attack, sq, pc, out); 
    })
}

static inline void game_movegen_add_rook_moves(const game *g, color col, bitboard ok_squares, const bitboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    bitboard not_cur_col = ~g->colors[col]; 
    bitboard occupants = g->colors[COLOR_W] | g->colors[COLOR_B]; 
    bitboard attack; 

    piece pc = piece_make_colored(PIECE_R, col); 
    BITBOARD_FOR_EACH_BIT(g->pieces[pc], 
    {
        attack  = bitboard_cast_ray(sq, BITBOARD_SLIDE_FILE[square_file(sq)], occupants); 
        attack |= bitboard_cast_ray(sq, BITBOARD_SLIDE_RANK[square_rank(sq)], occupants); 
        attack &= not_cur_col; 
        attack &= GAME_TARGET_SQUARES; 

        game_movegen_push_bitboard_moves(attack, sq, pc, out); 
    })
}

static inline void game_movegen_add_queen_moves(const game *g, color col, bitboard ok_squares, const bitboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    bitboard not_cur_col = ~g->colors[col]; 
    bitboard occupants = g->colors[COLOR_W] | g->colors[COLOR_B]; 
    bitboard attack; 

    piece pc = piece_make_colored(PIECE_Q, col); 
    BITBOARD_FOR_EACH_BIT(g->pieces[pc], 
    {
        attack  = bitboard_cast_ray(sq, BITBOARD_SLIDE_FILE[square_file(sq)], occupants); 
        attack |= bitboard_cast_ray(sq, BITBOARD_SLIDE_RANK[square_rank(sq)], occupants); 
        attack |= bitboard_cast_ray(sq, BITBOARD_SLIDE_DIAG[square_diagonal(sq)], occupants); 
        attack |= bitboard_cast_ray(sq, BITBOARD_SLIDE_ANTI[square_antidiagonal(sq)], occupants); 
        attack &= not_cur_col; 
        attack &= GAME_TARGET_SQUARES; 
        game_movegen_push_bitboard_moves(attack, sq, pc, out); 
    })
}

static void game_generate_moves(const game *g, vector *out) 
{
    color col = g->turn; 
    color opp = color_get_other(col); 

    bitboard target = g->pieces[piece_make_colored(PIECE_K, col)]; 
    bitboard col_occ = g->colors[col]; 
    bitboard opp_occ = g->colors[opp]; 
    bitboard opp_rocc = bitboard_reverse(opp_occ); 

    bitboard opp_card = g->pieces[piece_make_colored(PIECE_R, opp)] | g->pieces[piece_make_colored(PIECE_Q, opp)]; 
    bitboard opp_diag = g->pieces[piece_make_colored(PIECE_B, opp)] | g->pieces[piece_make_colored(PIECE_Q, opp)]; 

    square sq = bitboard_least_significant_bit(target), rsq = 63 - sq; 

    bitboard dirs[BITBOARD_CHECK_COUNT]; 
    uint8_t blocking[BITBOARD_CHECK_DIR_COUNT]; 

    uint16_t attacked = 0; 
    uint16_t pinned = 0; 

    // diagonal movement to occupied square (ignoring current color pieces and including non-relevant enemy pieces)
    dirs[BITBOARD_CHECK_DIR_NW] = bitboard_cast_positive_ray(sq, BITBOARD_SLIDE_ANTI[square_antidiagonal(sq)], opp_occ); 
    dirs[BITBOARD_CHECK_DIR_SE] = bitboard_reverse(bitboard_cast_positive_ray(rsq, BITBOARD_SLIDE_ANTI[square_antidiagonal(rsq)], opp_rocc)); 
    dirs[BITBOARD_CHECK_DIR_NE] = bitboard_cast_positive_ray(sq, BITBOARD_SLIDE_DIAG[square_diagonal(sq)], opp_occ); 
    dirs[BITBOARD_CHECK_DIR_SW] = bitboard_reverse(bitboard_cast_positive_ray(rsq, BITBOARD_SLIDE_DIAG[square_diagonal(rsq)], opp_rocc)); 

    // cardinal movement to occupied square (ignoring current color pieces and including non-relevant enemy pieces)
    dirs[BITBOARD_CHECK_DIR_N] = bitboard_cast_positive_ray(sq, BITBOARD_SLIDE_FILE[square_file(sq)], opp_occ); 
    dirs[BITBOARD_CHECK_DIR_S] = bitboard_reverse(bitboard_cast_positive_ray(rsq, BITBOARD_SLIDE_FILE[square_file(rsq)], opp_rocc)); 
    dirs[BITBOARD_CHECK_DIR_E] = bitboard_cast_positive_ray(sq, BITBOARD_SLIDE_RANK[square_rank(sq)], opp_occ); 
    dirs[BITBOARD_CHECK_DIR_W] = bitboard_reverse(bitboard_cast_positive_ray(rsq, BITBOARD_SLIDE_RANK[square_rank(rsq)], opp_rocc)); 

    // knight 
    dirs[BITBOARD_CHECK_PIECE_N] = BITBOARD_KNIGHT_MOVES[sq] & g->pieces[piece_make_colored(PIECE_N, opp)]; 

    // pawn 
    if (col == COLOR_W) 
    {
        dirs[BITBOARD_CHECK_PIECE_P]  = bitboard_shift_nw(target); 
        dirs[BITBOARD_CHECK_PIECE_P] |= bitboard_shift_ne(target); 
    }
    else 
    {
        dirs[BITBOARD_CHECK_PIECE_P]  = bitboard_shift_sw(target); 
        dirs[BITBOARD_CHECK_PIECE_P] |= bitboard_shift_se(target); 
    }
    dirs[BITBOARD_CHECK_PIECE_P] &= g->pieces[piece_make_colored(PIECE_P, opp)]; 

    // determine number of blockers (assuming that the direction has an attacker) 
    blocking[BITBOARD_CHECK_DIR_N] = bitboard_pop_count(dirs[BITBOARD_CHECK_DIR_N] & col_occ); 
    blocking[BITBOARD_CHECK_DIR_S] = bitboard_pop_count(dirs[BITBOARD_CHECK_DIR_S] & col_occ); 
    blocking[BITBOARD_CHECK_DIR_E] = bitboard_pop_count(dirs[BITBOARD_CHECK_DIR_E] & col_occ); 
    blocking[BITBOARD_CHECK_DIR_W] = bitboard_pop_count(dirs[BITBOARD_CHECK_DIR_W] & col_occ); 
    blocking[BITBOARD_CHECK_DIR_NE] = bitboard_pop_count(dirs[BITBOARD_CHECK_DIR_NE] & col_occ); 
    blocking[BITBOARD_CHECK_DIR_NW] = bitboard_pop_count(dirs[BITBOARD_CHECK_DIR_NW] & col_occ); 
    blocking[BITBOARD_CHECK_DIR_SE] = bitboard_pop_count(dirs[BITBOARD_CHECK_DIR_SE] & col_occ); 
    blocking[BITBOARD_CHECK_DIR_SW] = bitboard_pop_count(dirs[BITBOARD_CHECK_DIR_SW] & col_occ); 

    // first figure out which directions have an attacker (ignore current color pinned pieces)

    // directions might hit a non-attacking opponent piece, or even no piece at all
    attacked |= (((dirs[BITBOARD_CHECK_DIR_N] & opp_card) != 0) & (blocking[BITBOARD_CHECK_DIR_N] <= 1)) << BITBOARD_CHECK_DIR_N; 
    attacked |= (((dirs[BITBOARD_CHECK_DIR_S] & opp_card) != 0) & (blocking[BITBOARD_CHECK_DIR_S] <= 1)) << BITBOARD_CHECK_DIR_S; 
    attacked |= (((dirs[BITBOARD_CHECK_DIR_E] & opp_card) != 0) & (blocking[BITBOARD_CHECK_DIR_E] <= 1)) << BITBOARD_CHECK_DIR_E; 
    attacked |= (((dirs[BITBOARD_CHECK_DIR_W] & opp_card) != 0) & (blocking[BITBOARD_CHECK_DIR_W] <= 1)) << BITBOARD_CHECK_DIR_W; 
    attacked |= (((dirs[BITBOARD_CHECK_DIR_NE] & opp_diag) != 0) & (blocking[BITBOARD_CHECK_DIR_NE] <= 1)) << BITBOARD_CHECK_DIR_NE; 
    attacked |= (((dirs[BITBOARD_CHECK_DIR_NW] & opp_diag) != 0) & (blocking[BITBOARD_CHECK_DIR_NW] <= 1)) << BITBOARD_CHECK_DIR_NW; 
    attacked |= (((dirs[BITBOARD_CHECK_DIR_SE] & opp_diag) != 0) & (blocking[BITBOARD_CHECK_DIR_SE] <= 1)) << BITBOARD_CHECK_DIR_SE; 
    attacked |= (((dirs[BITBOARD_CHECK_DIR_SW] & opp_diag) != 0) & (blocking[BITBOARD_CHECK_DIR_SW] <= 1)) << BITBOARD_CHECK_DIR_SW;
    // there are already checked against correct opponent pieces and cannot be blocked 
    attacked |= (dirs[BITBOARD_CHECK_PIECE_N] != 0) << BITBOARD_CHECK_PIECE_N; 
    attacked |= (dirs[BITBOARD_CHECK_PIECE_P] != 0) << BITBOARD_CHECK_PIECE_P; 

    // determine pins 

    // these need to have an attacker, and exactly one friendly piece in the way 
    pinned |= ((attacked >> BITBOARD_CHECK_DIR_N) & (blocking[BITBOARD_CHECK_DIR_N] == 1)) << BITBOARD_CHECK_DIR_N; 
    pinned |= ((attacked >> BITBOARD_CHECK_DIR_S) & (blocking[BITBOARD_CHECK_DIR_S] == 1)) << BITBOARD_CHECK_DIR_S; 
    pinned |= ((attacked >> BITBOARD_CHECK_DIR_E) & (blocking[BITBOARD_CHECK_DIR_E] == 1)) << BITBOARD_CHECK_DIR_E; 
    pinned |= ((attacked >> BITBOARD_CHECK_DIR_W) & (blocking[BITBOARD_CHECK_DIR_W] == 1)) << BITBOARD_CHECK_DIR_W; 
    pinned |= ((attacked >> BITBOARD_CHECK_DIR_NE) & (blocking[BITBOARD_CHECK_DIR_NE] == 1)) << BITBOARD_CHECK_DIR_NE; 
    pinned |= ((attacked >> BITBOARD_CHECK_DIR_NW) & (blocking[BITBOARD_CHECK_DIR_NW] == 1)) << BITBOARD_CHECK_DIR_NW; 
    pinned |= ((attacked >> BITBOARD_CHECK_DIR_SE) & (blocking[BITBOARD_CHECK_DIR_SE] == 1)) << BITBOARD_CHECK_DIR_SE; 
    pinned |= ((attacked >> BITBOARD_CHECK_DIR_SW) & (blocking[BITBOARD_CHECK_DIR_SW] == 1)) << BITBOARD_CHECK_DIR_SW; 

    attacked &= ~pinned; 

    int n_checks = bitboard_pop_count_16(attacked); 

    if (n_checks == 0) 
    {
        // all moves 
        game_movegen_add_pawn_moves(g, col, BITBOARD_ALL, dirs, pinned, BITBOARD_PIN_INDEX[sq], out); 
        game_movegen_add_knight_moves(g, col, BITBOARD_ALL, dirs, pinned, BITBOARD_PIN_INDEX[sq], out); 
        game_movegen_add_bishop_moves(g, col, BITBOARD_ALL, dirs, pinned, BITBOARD_PIN_INDEX[sq], out); 
        game_movegen_add_rook_moves(g, col, BITBOARD_ALL, dirs, pinned, BITBOARD_PIN_INDEX[sq], out); 
        game_movegen_add_queen_moves(g, col, BITBOARD_ALL, dirs, pinned, BITBOARD_PIN_INDEX[sq], out); 
        game_movegen_add_king_moves(g, col, out); 
    }
    else if (n_checks == 1) 
    {
        // all moves, but must remove check 

        // for sliding attacking piece: 
        //   all squares that block or capture the piece
        // knight or pawn: 
        //   only allows capturing the piece (as it cannot be blocked)
        bitboard target_squares = dirs[bitboard_least_significant_bit(attacked)]; 

        game_movegen_add_pawn_moves(g, col, target_squares | ((g->en_passant != SQUARE_NONE) * bitboard_make_position(g->en_passant)), dirs, pinned, BITBOARD_PIN_INDEX[sq], out); 
        game_movegen_add_knight_moves(g, col, target_squares, dirs, pinned, BITBOARD_PIN_INDEX[sq], out); 
        game_movegen_add_bishop_moves(g, col, target_squares, dirs, pinned, BITBOARD_PIN_INDEX[sq], out); 
        game_movegen_add_rook_moves(g, col, target_squares, dirs, pinned, BITBOARD_PIN_INDEX[sq], out); 
        game_movegen_add_queen_moves(g, col, target_squares, dirs, pinned, BITBOARD_PIN_INDEX[sq], out); 
        game_movegen_add_king_moves(g, col, out); 
    }
    else // double check: must move the king 
    {
        // move king 
        game_movegen_add_king_moves(g, col, out); 
    }
}

static void game_print(const game *g) 
{
    static const char *empty = "."; 

    vector moves; 
    VECTOR_CREATE_TYPE(&moves, move); 
    game_generate_moves(g, &moves);  
    size_t num_moves = moves.size; 

    printf("  +-----------------+\n"); 
    for (int rank = 7; rank >= 0; rank--) 
    {
        printf("%d | ", rank + 1); 
        for (int file = 0; file < 8; file++) 
        {
            square sq = square_make(file, rank); 
            const char *str = empty; 
            for (piece p = 0; p < PIECE_COUNT; p++) 
            {
                if (bitboard_at(g->pieces[p], sq)) 
                {
                    str = piece_string(p); 
                }
            }

            printf("%s ", str); 
        }
        printf("|\n"); 
    }
    printf("  +-----------------+\n"); 
    printf("    A B C D E F G H  \n"); 
    printf("Castling: %s\n", castle_string(g->castle)); 
    printf("En passant: %s\n", g->en_passant == SQUARE_NONE ? "(none)" : square_string(g->en_passant)); 
    printf("In check: %s\n", g->in_check ? "yes" : "no"); 
    printf("# moves: %zu\n", num_moves); 
    printf("%s to move\n", g->turn ? "Black" : "White"); 

    // for (size_t i = 0; i < num_moves; i++) 
    // {
    //     move_print(VECTOR_AT_TYPE(&moves, move, i)); 
    // }

    vector_destroy(&moves); 
}

static inline uint64_t game_perft_internal(game *g, vector *moves, int depth, bool verbose) 
{
    uint64_t total = 0, start = moves->size, size; 

    if (depth > 1) 
    {
        game_generate_moves(g, moves); 
        size = moves->size; 

        // game_print(g); 

        for (size_t i = start; i < size; i++) 
        {
            game_push_move(g, VECTOR_AT_TYPE(moves, move, i)); 
            total += game_perft_internal(g, moves, depth - 1, false); 
            game_pop_move(g); 
        }

        vector_pop_to_size(moves, start); 
    }
    else if (depth == 1) 
    {
        game_generate_moves(g, moves); 
        total += moves->size - start; 
        vector_pop_to_size(moves, start); 
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

static inline uint64_t game_perft(game *g, int depth) 
{
    vector moves; 
    VECTOR_CREATE_TYPE(&moves, move); 

    uint64_t total = game_perft_internal(g, &moves, depth, true); 

    vector_destroy(&moves); 
    return total; 
}