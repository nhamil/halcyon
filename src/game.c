#include "game.h" 

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
#define TARGET_SQUARES ( \
    ok_squares & ( \
        dirs[pin_idx[sq]] | (ALL_BITS * \
            ( \
                ((~pinned >> pin_idx[sq]) & 1) | \
                ((dirs[pin_idx[sq]] & (1ULL << sq)) == 0) \
            ) \
        ) \
    ) \
)

void reset_game(game *g) 
{
    clear_vec(&g->hist); 
    memset(&g->pieces, 0, sizeof(g->pieces)); 
    memset(&g->colors, 0, sizeof(g->colors)); 
    g->check = NO_BITS; 
    g->castle = CASTLE_NONE; 
    g->ep = NO_SQ; 
    g->in_check = false; 
    g->ply = 0; 
    g->turn = COL_W; 
    g->nodes = 0; 
}

void create_game(game *g) 
{
    create_vec(&g->hist, offsetof(game, hist)); 
    reset_game(g); 
}

void create_game_copy(game *g, const game *src) 
{
    create_vec_copy(&g->hist, &src->hist); 
    memcpy(&g->pieces, &src->pieces, sizeof(g->pieces)); 
    memcpy(&g->colors, &src->colors, sizeof(g->colors)); 
    g->check = src->check; 
    g->castle = src->castle; 
    g->ep = src->ep; 
    g->in_check = src->in_check; 
    g->ply = src->ply; 
    g->turn = src->turn; 
    g->nodes = src->nodes; 
}

void destroy_game(game *g) 
{
    destroy_vec(&g->hist); 
}

void create_game_fen(game *g, const char *fen) 
{
    create_game(g); 
    load_fen(g, fen); 
}

void load_fen(game *g, const char *fen) 
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
                        g->pieces[PC_WP] = set_bit(g->pieces[PC_WP], rrank(sq++)); 
                        break; 
                    case 'p': 
                        g->pieces[PC_BP] = set_bit(g->pieces[PC_BP], rrank(sq++)); 
                        break; 
                    case 'N': 
                        g->pieces[PC_WN] = set_bit(g->pieces[PC_WN], rrank(sq++)); 
                        break; 
                    case 'n': 
                        g->pieces[PC_BN] = set_bit(g->pieces[PC_BN], rrank(sq++)); 
                        break; 
                    case 'B': 
                        g->pieces[PC_WB] = set_bit(g->pieces[PC_WB], rrank(sq++)); 
                        break; 
                    case 'b': 
                        g->pieces[PC_BB] = set_bit(g->pieces[PC_BB], rrank(sq++)); 
                        break; 
                    case 'R': 
                        g->pieces[PC_WR] = set_bit(g->pieces[PC_WR], rrank(sq++)); 
                        break; 
                    case 'r': 
                        g->pieces[PC_BR] = set_bit(g->pieces[PC_BR], rrank(sq++)); 
                        break; 
                    case 'Q': 
                        g->pieces[PC_WQ] = set_bit(g->pieces[PC_WQ], rrank(sq++)); 
                        break; 
                    case 'q': 
                        g->pieces[PC_BQ] = set_bit(g->pieces[PC_BQ], rrank(sq++)); 
                        break; 
                    case 'K': 
                        g->pieces[PC_WK] = set_bit(g->pieces[PC_WK], rrank(sq++)); 
                        break; 
                    case 'k': 
                        g->pieces[PC_BK] = set_bit(g->pieces[PC_BK], rrank(sq++)); 
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
                        sq = make_sq(*fen - 'a', get_rank(sq)); 
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
                        sq = make_sq(get_file(sq), *fen - '1'); 
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
                        printf("Unknown FEN character (halfmove): %c\n", *fen); 
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
        g->colors[COL_W] |= g->pieces[make_pc(p, COL_W)]; 
        g->colors[COL_B] |= g->pieces[make_pc(p, COL_B)]; 
    }
}

bool in_check(const game *g, color for_color, bboard check_king, bboard ignore, bboard add) 
{
    color col = for_color; 
    color opp = opp_col(col); 

    // check if any of these squares are attacked 
    bboard target = check_king; 

    // blockers include both colors' pieces other than the target squares 
    bboard occupants = (((g->colors[col] & ~target) | g->colors[opp]) & ~ignore) | add; 

    bboard attacks = NO_BITS; 
    bboard tmp; 

    FOR_EACH_BIT(target, 
    {
        // pawn 
        if (col == COL_W) 
        {
            // check for black pawns north of the king 
            tmp  = shift_nw(target); 
            tmp |= shift_ne(target); 
            attacks |= tmp & ((g->pieces[PC_BP] & ~ignore)); 
        }
        else 
        {
            // check for white pawns south of the king 
            tmp  = shift_sw(target); 
            tmp |= shift_se(target); 
            attacks |= tmp & ((g->pieces[PC_WP] & ~ignore)); 
        }

        // knight 
        tmp = MOVES_N[sq]; 
        attacks |= tmp & g->pieces[make_pc(PC_N, opp)]; 

        // bishop (and queen)
        tmp  = cast_ray(sq, SLIDE_DIAG[get_diag(sq)], occupants); 
        tmp |= cast_ray(sq, SLIDE_ANTI[get_anti(sq)], occupants); 
        attacks |= tmp & (g->pieces[make_pc(PC_B, opp)] | g->pieces[make_pc(PC_Q, opp)]); 

        // rook (and queen)
        tmp  = cast_ray(sq, SLIDE_FILE[get_file(sq)], occupants); 
        tmp |= cast_ray(sq, SLIDE_RANK[get_rank(sq)], occupants); 
        attacks |= tmp & (g->pieces[make_pc(PC_R, opp)] | g->pieces[make_pc(PC_Q, opp)]); 

        // king 
        tmp = MOVES_K[sq]; 
        attacks |= tmp & g->pieces[make_pc(PC_K, opp)]; 

        if (attacks) return true; 
    });

    return false; 
}

// does not check if the move is legal 
void push_move(game *g, move m) 
{
    g->nodes++; 

    // save current state 
    push_vec(&g->hist, g); 

    square from = from_sq(m); 
    square to = to_sq(m); 
    square rm_tgt = to; // remove opponent's piece on this square (can be different from `to` if en passant)
    square new_ep = ep_sq(m); 

    piece pc = from_pc(m); 
    piece pro = pro_pc(m); 
    piece tgt; 

    // note: tgt will be WP if rm_tgt is empty 
    if (g->turn == COL_W) 
    {
        rm_tgt -= 8 * takes_ep(m); // target piece is south (behind the capturing pawn) if ep
        tgt = b_pc_at(g, rm_tgt); 
    }
    else // B
    {
        rm_tgt += 8 * takes_ep(m); // target piece is north (behind the capturing pawn) if ep
        tgt = w_pc_at(g, rm_tgt); 
    }

    // take moving piece off the board 
    g->pieces[pc] = clear_bits(g->pieces[pc], from); 

    // WP if rm_tgt is empty, but this is okay because if pc is WP it will re-enable this square right after 
    g->pieces[tgt] = clear_bits(g->pieces[tgt], rm_tgt); 

    // place piece onto new square 
    g->pieces[pro] = set_bit(g->pieces[pro], to); 

    color cur_col = g->turn, oth_col = opp_col(cur_col); 

    // remove rooks from aggregate (in case of castling) 
    g->colors[COL_W] ^= g->pieces[PC_WR]; 
    g->colors[COL_B] ^= g->pieces[PC_BR]; 

    int castle = castle_idx(m); 
    g->check = g->pieces[make_pc(PC_K, cur_col)] | CASTLE_TARGETS[castle]; 

    // move the rooks when castling 
    g->pieces[PC_WR] = (g->pieces[PC_WR] & CASTLE_KEEP_WR[castle]) | CASTLE_ADD_WR[castle]; 
    g->pieces[PC_BR] = (g->pieces[PC_BR] & CASTLE_KEEP_BR[castle]) | CASTLE_ADD_BR[castle]; 

    castle_flags rem_cf = CASTLE_NONE; 
    // no castling if the king moves
    rem_cf |= CASTLE_W  * (pc == PC_WK); 
    rem_cf |= CASTLE_B  * (pc == PC_BK); 
    // no castling if the rook moves or is captured
    rem_cf |= CASTLE_WK * ((from == H1) | (to == H1)); 
    rem_cf |= CASTLE_WQ * ((from == A1) | (to == A1)); 
    rem_cf |= CASTLE_BK * ((from == H8) | (to == H8)); 
    rem_cf |= CASTLE_BQ * ((from == A8) | (to == A8)); 

    // remove castling rights if necessary 
    g->castle &= ~rem_cf; 

    // re-add rooks from aggregate (in case of castling) 
    g->colors[COL_W] ^= g->pieces[PC_WR]; 
    g->colors[COL_B] ^= g->pieces[PC_BR]; 

    // update aggregate piece tracking 
    g->colors[cur_col] = clear_bits(g->colors[cur_col], from); 
    g->colors[oth_col] = clear_bits(g->colors[oth_col], rm_tgt); 
    g->colors[cur_col] = set_bit(g->colors[cur_col], to); 

    // update rest of state 
    g->ep = new_ep; 
    g->ply++; 
    g->turn = opp_col(g->turn); 

    // useful to know if the new player is in check 
    g->in_check = in_check(g, g->turn, g->pieces[make_pc(PC_K, g->turn)], NO_BITS, NO_BITS); 
}

void pop_move(game *g) 
{
    get_vec(&g->hist, g->hist.size - 1, g); 
    pop_vec(&g->hist); 
    g->ply--; 
    g->turn = opp_col(g->turn); 
}

void push_null_move(game *g) 
{
    g->nodes++; 

    push_vec(&g->hist, g); 

    color cur_col = g->turn; 

    g->check = g->pieces[make_pc(PC_K, cur_col)]; 
    g->ep = NO_SQ; 
    g->ply++; 
    g->turn = opp_col(cur_col); 

    // see if next player is in check 
    g->in_check = in_check(g, g->turn, g->pieces[make_pc(PC_K, g->turn)], NO_BITS, NO_BITS); 
}

void pop_null_move(game *g) 
{
    pop_move(g); 
}

static inline void gen_push(bboard b, square from, piece pc, vector *out) 
{
    FOR_EACH_BIT(b, 
    {
        PUSH_VEC(out, move, make_move(from, sq, pc, pc)); 
    });
}

static inline void gen_push_safe(const game *g, bboard b, square from, piece pc, vector *out) 
{
    FOR_EACH_BIT(b, 
    {
        if (!in_check(g, g->turn, make_pos(sq), make_pos(from), NO_BITS)) 
        {
            PUSH_VEC(out, move, make_move(from, sq, pc, pc)); 
        }
    });
}

static inline void gen_push_castle(square from, square to, piece pc, int castle_index, vector *out) 
{
    PUSH_VEC(out, move, make_move_castle(from, to, pc, castle_index)); 
}

static inline void movegen_push_ep(bboard b, square from, piece pc, int d_sq, vector *out) 
{
    FOR_EACH_BIT(b, 
    {
        PUSH_VEC(out, move, make_move_allow_ep(from, sq, pc, sq + d_sq)); 
    });
}

static inline void gen_push_pro(bboard b, square from, piece pc, color col, vector *out) 
{
    FOR_EACH_BIT(b, 
    {
        PUSH_VEC(out, move, make_move(from, sq, pc, make_pc(PC_Q, col))); 
        PUSH_VEC(out, move, make_move(from, sq, pc, make_pc(PC_R, col))); 
        PUSH_VEC(out, move, make_move(from, sq, pc, make_pc(PC_B, col))); 
        PUSH_VEC(out, move, make_move(from, sq, pc, make_pc(PC_N, col))); 
    });
}

static inline void gen_push_take_ep(const game *g, bboard b, square from, piece pc, square ep, vector *out) 
{
    FOR_EACH_BIT(b, 
    {
        if (sq != ep) 
        {
            PUSH_VEC(out, move, make_move_ep(from, sq, pc, false)); 
        }
        else if (get_col(pc) == COL_W) 
        {
            if (!in_check(g, COL_W, g->pieces[PC_WK], make_pos(from) | make_pos(ep - 8), make_pos(ep))) 
            {
                PUSH_VEC(out, move, make_move_ep(from, sq, pc, true)); 
            }
        }
        else // COL_B  
        {
            if (!in_check(g, COL_B, g->pieces[PC_BK], make_pos(from) | make_pos(ep + 8), make_pos(ep))) 
            {
                PUSH_VEC(out, move, make_move_ep(from, sq, pc, true)); 
            }
        }
    });
}

static inline void gen_p(const game *g, color col, bboard ok_squares, const bboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    bboard empty = ~(g->colors[COL_W] | g->colors[COL_B]); 
    bboard ep = (g->ep != NO_SQ) * make_pos(g->ep); 
    bboard cur_pos, attack, target_squares; 

    if (col == COL_W) 
    {
        FOR_EACH_BIT(g->pieces[PC_WP], 
        {
            cur_pos = make_pos(sq); 

            // move forward if square is empty 
            attack = shift_n(cur_pos) & empty; 

            // attack left if there is a piece or en passant
            attack |= shift_nw(cur_pos) & (g->colors[COL_B] | ep); 

            // attack right if there is a piece or en passant
            attack |= shift_ne(cur_pos) & (g->colors[COL_B] | ep); 

            target_squares = TARGET_SQUARES; 
            attack &= target_squares; 

            // move 2 squares if on starting square 
            movegen_push_ep((get_rank(sq) == 1) * (shift_nn(cur_pos) & empty & shift_n(empty) & target_squares), sq, PC_WP, -8, out); 

            // add non-promoting moves to list 
            gen_push_take_ep(g, attack & NO_RANK_8, sq, PC_WP, g->ep, out); 

            // add promoting moves to list 
            gen_push_pro(attack & RANK_8, sq, PC_WP, COL_W, out); 
        });
    }
    else 
    {
        FOR_EACH_BIT(g->pieces[PC_BP], 
        {
            cur_pos = make_pos(sq); 

            // move forward if square is empty 
            attack = shift_s(cur_pos) & empty; 

            // attack left if there is a piece or en passant
            attack |= shift_sw(cur_pos) & (g->colors[COL_W] | ep); 

            // attack right if there is a piece or en passant
            attack |= shift_se(cur_pos) & (g->colors[COL_W] | ep); 

            target_squares = TARGET_SQUARES; 
            attack &= target_squares; 

            // move 2 squares if on starting square 
            movegen_push_ep((get_rank(sq) == 6) * (shift_ss(cur_pos) & empty & shift_s(empty) & target_squares), sq, PC_BP, 8, out); 

            // add non-promoting moves to list 
            gen_push_take_ep(g, attack & NO_RANK_1, sq, PC_BP, g->ep, out); 

            // add promoting moves to list 
            gen_push_pro(attack & RANK_1, sq, PC_BP, COL_B, out); 
        });
    }
}

static inline void gen_rem_illegal(const game *state, size_t start, vector *out) 
{
    game *g = (game *) state; 
    size_t i = start; 

    while (i < out->size) 
    {
        push_move(g, AT_VEC(out, move, i)); 

        if (in_check(g, opp_col(g->turn), g->check, NO_BITS, NO_BITS)) 
        {
            set_vec(out, i, at_vec(out, out->size - 1)); 
            pop_vec(out); 
        }
        else 
        {
            i++; 
        }

        pop_move(g); 
    }
}

static inline void gen_k(const game *g, color col, vector *out) 
{
    bboard occupants = g->colors[COL_W] | g->colors[COL_B]; 
    bboard not_cur_col = ~g->colors[col]; 
    bboard attack; 

    piece pc = make_pc(PC_K, col); 
    FOR_EACH_BIT(g->pieces[pc], 
    {
        attack = MOVES_K[sq] & not_cur_col; 
        gen_push_safe(g, attack, sq, pc, out); 

        int c_wk = 0 != (opp_col(col) * ((g->castle & CASTLE_WK) != 0) * ((occupants & EMPTY_WK) == 0)); 
        int c_wq = 0 != (opp_col(col) * ((g->castle & CASTLE_WQ) != 0) * ((occupants & EMPTY_WQ) == 0)); 
        int c_bk = 0 != (col * ((g->castle & CASTLE_BK) != 0) * ((occupants & EMPTY_BK) == 0)); 
        int c_bq = 0 != (col * ((g->castle & CASTLE_BQ) != 0) * ((occupants & EMPTY_BQ) == 0)); 

        if (c_wk && !in_check(g, col, g->pieces[pc] | CASTLE_TARGETS[MOVE_CASTLE_WK], NO_BITS, NO_BITS)) 
        {
            gen_push_castle(E1, G1, PC_WK, MOVE_CASTLE_WK, out); 
        }

        if (c_wq && !in_check(g, col, g->pieces[pc] | CASTLE_TARGETS[MOVE_CASTLE_WQ], NO_BITS, NO_BITS)) 
        {
            gen_push_castle(E1, C1, PC_WK, MOVE_CASTLE_WQ, out); 
        }

        if (c_bk && !in_check(g, col, g->pieces[pc] | CASTLE_TARGETS[MOVE_CASTLE_BK], NO_BITS, NO_BITS)) 
        {
            gen_push_castle(E8, G8, PC_BK, MOVE_CASTLE_BK, out); 
        }

        if (c_bq && !in_check(g, col, g->pieces[pc] | CASTLE_TARGETS[MOVE_CASTLE_BQ], NO_BITS, NO_BITS)) 
        {
            gen_push_castle(E8, C8, PC_BK, MOVE_CASTLE_BQ, out); 
        }
    });
}

static inline void gen_n(const game *g, color col, bboard ok_squares, const bboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    // empty squares 
    bboard not_cur_col = ~g->colors[col]; 
    bboard attack; 

    piece pc = make_pc(PC_N, col); 
    FOR_EACH_BIT(g->pieces[pc], 
    {
        attack = MOVES_N[sq] & not_cur_col; 
        attack &= TARGET_SQUARES;  
        gen_push(attack, sq, pc, out); 
    });
}

static inline void gen_b(const game *g, color col, bboard ok_squares, const bboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    bboard not_cur_col = ~g->colors[col]; 
    bboard occupants = g->colors[COL_W] | g->colors[COL_B]; 
    bboard attack; 

    piece pc = make_pc(PC_B, col); 
    FOR_EACH_BIT(g->pieces[pc], 
    {
        attack  = cast_ray(sq, SLIDE_DIAG[get_diag(sq)], occupants); 
        attack |= cast_ray(sq, SLIDE_ANTI[get_anti(sq)], occupants); 
        attack &= not_cur_col; 
        attack &= TARGET_SQUARES; 
        gen_push(attack, sq, pc, out); 
    })
}

static inline void gen_r(const game *g, color col, bboard ok_squares, const bboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    bboard not_cur_col = ~g->colors[col]; 
    bboard occupants = g->colors[COL_W] | g->colors[COL_B]; 
    bboard attack; 

    piece pc = make_pc(PC_R, col); 
    FOR_EACH_BIT(g->pieces[pc], 
    {
        attack  = cast_ray(sq, SLIDE_FILE[get_file(sq)], occupants); 
        attack |= cast_ray(sq, SLIDE_RANK[get_rank(sq)], occupants); 
        attack &= not_cur_col; 
        attack &= TARGET_SQUARES; 

        gen_push(attack, sq, pc, out); 
    })
}

static inline void gen_q(const game *g, color col, bboard ok_squares, const bboard *dirs, uint16_t pinned, const uint8_t *pin_idx, vector *out) 
{
    bboard not_cur_col = ~g->colors[col]; 
    bboard occupants = g->colors[COL_W] | g->colors[COL_B]; 
    bboard attack; 

    piece pc = make_pc(PC_Q, col); 
    FOR_EACH_BIT(g->pieces[pc], 
    {
        attack  = cast_ray(sq, SLIDE_FILE[get_file(sq)], occupants); 
        attack |= cast_ray(sq, SLIDE_RANK[get_rank(sq)], occupants); 
        attack |= cast_ray(sq, SLIDE_DIAG[get_diag(sq)], occupants); 
        attack |= cast_ray(sq, SLIDE_ANTI[get_anti(sq)], occupants); 
        attack &= not_cur_col; 
        attack &= TARGET_SQUARES; 
        gen_push(attack, sq, pc, out); 
    })
}

void gen_moves(const game *g, vector *out) 
{
    color col = g->turn; 
    color opp = opp_col(col); 

    bboard target = g->pieces[make_pc(PC_K, col)]; 
    bboard col_occ = g->colors[col]; 
    bboard opp_occ = g->colors[opp]; 
    bboard opp_rocc = rev(opp_occ); 

    bboard opp_card = g->pieces[make_pc(PC_R, opp)] | g->pieces[make_pc(PC_Q, opp)]; 
    bboard opp_diag = g->pieces[make_pc(PC_B, opp)] | g->pieces[make_pc(PC_Q, opp)]; 

    square sq = lsb(target), rsq = 63 - sq; 

    bboard dirs[CHECK_CNT]; 
    uint8_t blocking[CHECK_DIR_CNT]; 

    uint16_t attacked = 0; 
    uint16_t pinned = 0; 

    // diagonal movement to occupied square (ignoring current color pieces and including non-relevant enemy pieces)
    dirs[CHECK_DIR_NW] = cast_pos_ray(sq, SLIDE_ANTI[get_anti(sq)], opp_occ); 
    dirs[CHECK_DIR_SE] = rev(cast_pos_ray(rsq, SLIDE_ANTI[get_anti(rsq)], opp_rocc)); 
    dirs[CHECK_DIR_NE] = cast_pos_ray(sq, SLIDE_DIAG[get_diag(sq)], opp_occ); 
    dirs[CHECK_DIR_SW] = rev(cast_pos_ray(rsq, SLIDE_DIAG[get_diag(rsq)], opp_rocc)); 

    // cardinal movement to occupied square (ignoring current color pieces and including non-relevant enemy pieces)
    dirs[CHECK_DIR_N] = cast_pos_ray(sq, SLIDE_FILE[get_file(sq)], opp_occ); 
    dirs[CHECK_DIR_S] = rev(cast_pos_ray(rsq, SLIDE_FILE[get_file(rsq)], opp_rocc)); 
    dirs[CHECK_DIR_E] = cast_pos_ray(sq, SLIDE_RANK[get_rank(sq)], opp_occ); 
    dirs[CHECK_DIR_W] = rev(cast_pos_ray(rsq, SLIDE_RANK[get_rank(rsq)], opp_rocc)); 

    // knight 
    dirs[CHECK_PC_N] = MOVES_N[sq] & g->pieces[make_pc(PC_N, opp)]; 

    // pawn 
    if (col == COL_W) 
    {
        dirs[CHECK_PC_P]  = shift_nw(target); 
        dirs[CHECK_PC_P] |= shift_ne(target); 
    }
    else 
    {
        dirs[CHECK_PC_P]  = shift_sw(target); 
        dirs[CHECK_PC_P] |= shift_se(target); 
    }
    dirs[CHECK_PC_P] &= g->pieces[make_pc(PC_P, opp)]; 

    // determine number of blockers (assuming that the direction has an attacker) 
    blocking[CHECK_DIR_N] = popcnt(dirs[CHECK_DIR_N] & col_occ); 
    blocking[CHECK_DIR_S] = popcnt(dirs[CHECK_DIR_S] & col_occ); 
    blocking[CHECK_DIR_E] = popcnt(dirs[CHECK_DIR_E] & col_occ); 
    blocking[CHECK_DIR_W] = popcnt(dirs[CHECK_DIR_W] & col_occ); 
    blocking[CHECK_DIR_NE] = popcnt(dirs[CHECK_DIR_NE] & col_occ); 
    blocking[CHECK_DIR_NW] = popcnt(dirs[CHECK_DIR_NW] & col_occ); 
    blocking[CHECK_DIR_SE] = popcnt(dirs[CHECK_DIR_SE] & col_occ); 
    blocking[CHECK_DIR_SW] = popcnt(dirs[CHECK_DIR_SW] & col_occ); 

    // first figure out which directions have an attacker (ignore current color pinned pieces)

    // directions might hit a non-attacking opponent piece, or even no piece at all
    attacked |= (((dirs[CHECK_DIR_N] & opp_card) != 0) & (blocking[CHECK_DIR_N] <= 1)) << CHECK_DIR_N; 
    attacked |= (((dirs[CHECK_DIR_S] & opp_card) != 0) & (blocking[CHECK_DIR_S] <= 1)) << CHECK_DIR_S; 
    attacked |= (((dirs[CHECK_DIR_E] & opp_card) != 0) & (blocking[CHECK_DIR_E] <= 1)) << CHECK_DIR_E; 
    attacked |= (((dirs[CHECK_DIR_W] & opp_card) != 0) & (blocking[CHECK_DIR_W] <= 1)) << CHECK_DIR_W; 
    attacked |= (((dirs[CHECK_DIR_NE] & opp_diag) != 0) & (blocking[CHECK_DIR_NE] <= 1)) << CHECK_DIR_NE; 
    attacked |= (((dirs[CHECK_DIR_NW] & opp_diag) != 0) & (blocking[CHECK_DIR_NW] <= 1)) << CHECK_DIR_NW; 
    attacked |= (((dirs[CHECK_DIR_SE] & opp_diag) != 0) & (blocking[CHECK_DIR_SE] <= 1)) << CHECK_DIR_SE; 
    attacked |= (((dirs[CHECK_DIR_SW] & opp_diag) != 0) & (blocking[CHECK_DIR_SW] <= 1)) << CHECK_DIR_SW;
    // there are already checked against correct opponent pieces and cannot be blocked 
    attacked |= (dirs[CHECK_PC_N] != 0) << CHECK_PC_N; 
    attacked |= (dirs[CHECK_PC_P] != 0) << CHECK_PC_P; 

    // determine pins 

    // these need to have an attacker, and exactly one friendly piece in the way 
    pinned |= ((attacked >> CHECK_DIR_N) & (blocking[CHECK_DIR_N] == 1)) << CHECK_DIR_N; 
    pinned |= ((attacked >> CHECK_DIR_S) & (blocking[CHECK_DIR_S] == 1)) << CHECK_DIR_S; 
    pinned |= ((attacked >> CHECK_DIR_E) & (blocking[CHECK_DIR_E] == 1)) << CHECK_DIR_E; 
    pinned |= ((attacked >> CHECK_DIR_W) & (blocking[CHECK_DIR_W] == 1)) << CHECK_DIR_W; 
    pinned |= ((attacked >> CHECK_DIR_NE) & (blocking[CHECK_DIR_NE] == 1)) << CHECK_DIR_NE; 
    pinned |= ((attacked >> CHECK_DIR_NW) & (blocking[CHECK_DIR_NW] == 1)) << CHECK_DIR_NW; 
    pinned |= ((attacked >> CHECK_DIR_SE) & (blocking[CHECK_DIR_SE] == 1)) << CHECK_DIR_SE; 
    pinned |= ((attacked >> CHECK_DIR_SW) & (blocking[CHECK_DIR_SW] == 1)) << CHECK_DIR_SW; 

    attacked &= ~pinned; 

    int n_checks = popcnt_16(attacked); 

    if (n_checks == 0) 
    {
        // all moves 
        gen_p(g, col, ALL_BITS, dirs, pinned, PIN_IDX[sq], out); 
        gen_n(g, col, ALL_BITS, dirs, pinned, PIN_IDX[sq], out); 
        gen_b(g, col, ALL_BITS, dirs, pinned, PIN_IDX[sq], out); 
        gen_r(g, col, ALL_BITS, dirs, pinned, PIN_IDX[sq], out); 
        gen_q(g, col, ALL_BITS, dirs, pinned, PIN_IDX[sq], out); 
        gen_k(g, col, out); 
    }
    else if (n_checks == 1) 
    {
        // all moves, but must remove check 

        // for sliding attacking piece: 
        //   all squares that block or capture the piece
        // knight or pawn: 
        //   only allows capturing the piece (as it cannot be blocked)
        bboard target_squares = dirs[lsb(attacked)]; 

        gen_p(g, col, target_squares | ((g->ep != NO_SQ) * make_pos(g->ep)), dirs, pinned, PIN_IDX[sq], out); 
        gen_n(g, col, target_squares, dirs, pinned, PIN_IDX[sq], out); 
        gen_b(g, col, target_squares, dirs, pinned, PIN_IDX[sq], out); 
        gen_r(g, col, target_squares, dirs, pinned, PIN_IDX[sq], out); 
        gen_q(g, col, target_squares, dirs, pinned, PIN_IDX[sq], out); 
        gen_k(g, col, out); 
    }
    else // double check: must move the king 
    {
        // move king 
        gen_k(g, col, out); 
    }
}

static inline int eval_bb(bboard pcs, const int eval[64]) 
{
    int sum = 0; 

    FOR_EACH_BIT(pcs, 
    {
        sum += eval[sq]; 
    });

    return sum; 
}

int eval(const game *g, int num_moves) 
{
    int eval = 0; 

    if (num_moves == 0) 
    {
        if (g->in_check) 
        {
            // lower value the farther out the mate is (prioritize faster mates)
            return (100000 - g->ply) * (-1 + 2 * g->turn); 
        }
        else 
        {
            // stalemate
            return 0; 
        }
    }

    // material 
    eval += 100 * (popcnt(g->pieces[PC_WP]) - popcnt(g->pieces[PC_BP])); 
    eval += 310 * (popcnt(g->pieces[PC_WN]) - popcnt(g->pieces[PC_BN])); 
    eval += 320 * (popcnt(g->pieces[PC_WB]) - popcnt(g->pieces[PC_BB])); 
    eval += 500 * (popcnt(g->pieces[PC_WR]) - popcnt(g->pieces[PC_BR])); 
    eval += 900 * (popcnt(g->pieces[PC_WQ]) - popcnt(g->pieces[PC_BQ])); 
    eval += 10000 * (popcnt(g->pieces[PC_WK]) - popcnt(g->pieces[PC_BK])); 

    // bishop pair 
    eval += 15 * ((popcnt(g->pieces[PC_WB]) >= 2) - (popcnt(g->pieces[PC_BB]) >= 2)); 

    eval += eval_bb(g->pieces[PC_WP], PC_SQ[PC_P]); 
    eval -= eval_bb(rrow(g->pieces[PC_BP]), PC_SQ[PC_P]); 

    eval += eval_bb(g->pieces[PC_WN], PC_SQ[PC_N]); 
    eval -= eval_bb(rrow(g->pieces[PC_BN]), PC_SQ[PC_N]); 

    eval += eval_bb(g->pieces[PC_WB], PC_SQ[PC_B]); 
    eval -= eval_bb(rrow(g->pieces[PC_BB]), PC_SQ[PC_B]); 

    eval += eval_bb(g->pieces[PC_WR], PC_SQ[PC_R]); 
    eval -= eval_bb(rrow(g->pieces[PC_BR]), PC_SQ[PC_R]); 

    eval += eval_bb(g->pieces[PC_WQ], PC_SQ[PC_Q]); 
    eval -= eval_bb(rrow(g->pieces[PC_BQ]), PC_SQ[PC_Q]); 

    eval += eval_bb(g->pieces[PC_WK], PC_SQ[PC_K]); 
    eval -= eval_bb(rrow(g->pieces[PC_BK]), PC_SQ[PC_K]); 

    return eval; 
}

void print_game(const game *g) 
{
    static const char *empty = "."; 

    vector moves; 
    CREATE_VEC(&moves, move); 
    gen_moves(g, &moves);  
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
                if (get_bit(g->pieces[p], sq)) 
                {
                    str = str_pc(p); 
                }
            }

            printf("%s ", str); 
        }
        printf("|\n"); 
    }
    printf("  +-----------------+\n"); 
    printf("    A B C D E F G H  \n"); 
    printf("Castling: %s\n", str_castle(g->castle)); 
    printf("En passant: %s\n", g->ep == NO_SQ ? "(none)" : str_sq(g->ep)); 
    printf("In check: %s\n", g->in_check ? "yes" : "no"); 
    printf("# moves: %zu\n", num_moves); 
    printf("Static eval: %.2f\n", eval(g, num_moves) * 0.01); 
    printf("%s to move\n", g->turn ? "Black" : "White"); 

    destroy_vec(&moves); 
}

static inline uint64_t perft_(game *g, vector *moves, int depth, bool verbose) 
{
    uint64_t total = 0, start = moves->size, size; 

    if (depth > 1) 
    {
        gen_moves(g, moves); 
        size = moves->size; 

        // print_game(g); 

        for (size_t i = start; i < size; i++) 
        {
            push_move(g, AT_VEC(moves, move, i)); 
            total += perft_(g, moves, depth - 1, false); 
            pop_move(g); 
        }

        pop_vec_to_size(moves, start); 
    }
    else if (depth == 1) 
    {
        gen_moves(g, moves); 
        total += moves->size - start; 
        pop_vec_to_size(moves, start); 
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

uint64_t perft(game *g, int depth) 
{
    vector moves; 
    CREATE_VEC(&moves, move); 

    uint64_t total = perft_(g, &moves, depth, true); 

    destroy_vec(&moves); 
    return total; 
}
