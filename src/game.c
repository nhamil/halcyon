#include "game.h" 

#include <stdlib.h> 
#include <string.h> 

#include "bitboard.h"
#include "castle.h"
#include "mailbox.h"
#include "move.h"
#include "movegen.h"
#include "piece.h"
#include "square.h"
#include "zobrist.h"

game *new_game(void) 
{
    game *g = malloc(sizeof(game)); 
    reset_game(g); 
    return g; 
}

void free_game(game *g) 
{
    free(g); 
}

void reset_game(game *g) 
{
    init_zb(); 
    init_mbox(&g->mailbox); 
    memset(g->pieces, 0, sizeof(g->pieces)); 
    memset(g->colors, 0, sizeof(g->colors)); 
    memset(g->counts, 0, sizeof(g->counts)); 
    g->all = 0; 
    g->movement = 0; 
    g->hash = 0; 
    g->castle = 0; 
    g->ep = NO_SQ; 
    g->ply = 0; 
    g->halfmove = 0; 
    g->turn = COL_W; 
    g->in_check = false; 
    g->nodes = 0; 
    g->depth = 0; 
}

void copy_game(game *g, const game *from) 
{
    init_zb(); 
    g->mailbox = from->mailbox; 
    memcpy(g->pieces, from->pieces, sizeof(g->pieces)); 
    memcpy(g->colors, from->colors, sizeof(g->colors)); 
    memcpy(g->counts, from->counts, sizeof(g->counts)); 
    g->all = from->all; 
    g->movement = from->movement; 
    g->hash = from->hash; 
    g->castle = from->castle; 
    g->ep = from->ep; 
    g->ply = from->ply; 
    g->halfmove = from->halfmove; 
    g->turn = from->turn; 
    g->in_check = from->in_check; 
    g->nodes = 0; 
    g->depth = from->depth; 
    for (int i = 0; i < g->depth; i++) 
    {
        g->hist[i] = from->hist[i]; 
    }
}

void print_game(const game *g) 
{
    char fen[FEN_LEN]; 
    to_fen(g, fen); 

    printf("+---+---+---+---+---+---+---+---+\n"); 
    for (int rank = 7; rank >= 0; rank--) 
    {
        printf("| "); 
        for (int file = 0; file < 8; file++) 
        {
            square sq = make_sq(file, rank); 
            printf("%s | ", str_pc(pc_at(g, sq))); 
        }
        printf("%d\n", rank + 1); 
        printf("+---+---+---+---+---+---+---+---+\n"); 
    }
    printf("  A   B   C   D   E   F   G   H  \n"); 

    for (color col = COL_W; col < COL_CNT; col++) 
    {
        for (piece pc = PC_P; pc < PC_TYPE_CNT; pc++) 
        {
            piece cpc = make_pc(pc, col); 
            printf("%d%s ", g->counts[cpc], str_pc(cpc)); 
        }
    }
    printf("\n"); 

    printf("%s\nHash: ", fen); 
    print_zb_end(g->hash, "\n");

    printf("Ply: %d, ", g->ply); 
    printf("Halfmove: %d, ", g->halfmove); 
    printf("Castling: %s\n", str_castle(g->castle)); 

    printf("En passant: %s, ", str_sq(g->ep)); 
    printf("In check: %s\n", g->in_check ? "yes" : "no"); 

    printf("Special draw: %s, ", is_special_draw(g) ? "yes" : "no");

    printf("%s to move\n", g->turn ? "Black" : "White"); 
}

void push_move(game *g, move mv) 
{
    g->nodes++; 

    move_hist *hist = g->hist + g->depth++; 
    hist->halfmove = g->halfmove; 
    hist->ep = g->ep; 
    hist->castle = g->castle; 
    hist->in_check = g->in_check; 

    
    hist->hash = g->hash; 

    color col = g->turn; 
    color opp = opp_col(col); 

    g->hash ^= col_zb(); 
    g->hash ^= ep_zb(g->ep); 

    piece pc = from_pc(mv); 
    piece pro = pro_pc(mv); 
    piece tgt = tgt_pc(mv); 

    square src = from_sq(mv); 
    square dst = to_sq(mv); 

    bool ep = is_ep(mv); 
    int cas_idx = castle_idx(mv); 

    bboard src_pos = BB[src]; 
    bboard dst_pos = BB[dst]; 

    if (is_capture(mv) || get_no_col(pc) == PC_P) 
    {
        g->halfmove = 0; 
    }
    else 
    {
        g->halfmove++; 
    }

    if (ep) // en passant 
    {
        square rm = g->ep - 8 * col_sign(g->turn); 
        bboard rm_pos = BB[rm]; 

        g->colors[col] ^= src_pos ^ dst_pos; 
        g->colors[opp] ^= rm_pos; 

        g->hash ^= sq_pc_zb(src, pc); 
        g->hash ^= sq_pc_zb(dst, pc); 
        g->hash ^= sq_pc_zb(rm, tgt); 

        g->pieces[pc] ^= src_pos ^ dst_pos; 
        g->pieces[tgt] ^= rm_pos; 
        
        clear_mbox(&g->mailbox, src); 
        clear_mbox(&g->mailbox, rm); 
        set_mbox(&g->mailbox, dst, pc); 

        g->counts[tgt]--; 
        g->ep = NO_SQ; 
    }
    else if (cas_idx) // castling 
    {
        g->colors[col] ^= MOVE_CASTLE_BB_ALL[cas_idx]; 

        g->hash ^= sq_pc_zb(MOVE_CASTLE_SQ_K[cas_idx][0], MOVE_CASTLE_PC_K[cas_idx]); 
        g->hash ^= sq_pc_zb(MOVE_CASTLE_SQ_K[cas_idx][1], MOVE_CASTLE_PC_K[cas_idx]); 
        g->hash ^= sq_pc_zb(MOVE_CASTLE_SQ_R[cas_idx][0], MOVE_CASTLE_PC_R[cas_idx]); 
        g->hash ^= sq_pc_zb(MOVE_CASTLE_SQ_R[cas_idx][1], MOVE_CASTLE_PC_R[cas_idx]); 

        g->pieces[pc] ^= MOVE_CASTLE_BB_K[cas_idx]; 
        g->pieces[make_pc(PC_R, col)] ^= MOVE_CASTLE_BB_R[cas_idx]; 

        clear_mbox(&g->mailbox, MOVE_CASTLE_SQ_K[cas_idx][0]); 
        clear_mbox(&g->mailbox, MOVE_CASTLE_SQ_R[cas_idx][0]); 
        set_mbox(&g->mailbox, MOVE_CASTLE_SQ_K[cas_idx][1], pc); 
        set_mbox(&g->mailbox, MOVE_CASTLE_SQ_R[cas_idx][1], make_pc(PC_R, col)); 

        g->ep = NO_SQ; 
    }
    else if (pc != pro) // promotion
    {
        g->colors[col] ^= src_pos ^ dst_pos; 
        g->colors[opp] ^= (tgt != NO_PC) * dst_pos; 

        g->hash ^= sq_pc_zb(src, pc); 
        g->hash ^= sq_pc_zb(dst, pro); 
        g->hash ^= sq_pc_zb(dst, tgt); 

        g->pieces[pc] ^= src_pos; 
        g->pieces[pro] ^= dst_pos; 
        g->pieces[tgt] ^= dst_pos; 
        
        clear_mbox(&g->mailbox, src); 
        set_mbox(&g->mailbox, dst, pro); 

        g->counts[tgt]--; 
        g->counts[pc]--; 
        g->counts[pro]++; 
        g->ep = NO_SQ; 
    }
    else // standard move 
    {
        g->colors[col] ^= src_pos ^ dst_pos; 
        g->colors[opp] ^= (tgt != NO_PC) * dst_pos; 

        g->hash ^= sq_pc_zb(src, pc); 
        g->hash ^= sq_pc_zb(dst, pc); 
        g->hash ^= sq_pc_zb(dst, tgt); 

        g->pieces[pc] ^= src_pos ^ dst_pos; 
        g->pieces[tgt] ^= dst_pos; 
        
        clear_mbox(&g->mailbox, src); 
        set_mbox(&g->mailbox, dst, pc); 

        g->counts[tgt]--; 
        if (get_no_col(pc) == PC_P && abs(src - dst) == 16) 
        {
            g->ep = col_sign(g->turn) * 8 + src; 
        }
        else 
        {
            g->ep = NO_SQ; 
        }
    }

    g->castle &= ~(MOVE_CASTLE_RM[src] | MOVE_CASTLE_RM[dst]); 
    g->all = g->colors[COL_W] | g->colors[COL_B]; 

    g->hash ^= ep_zb(g->ep); 
    g->hash ^= castle_zb(g->castle ^ hist->castle); 

    // for next movement: 
    //   can move to any square without own pieces 
    g->movement = ~g->colors[opp]; 
    g->ply++; 
    g->turn = opp; 
    g->in_check = is_check(mv); 

    VALIDATE_GAME_MOVE(g, mv, "pushing"); 
}

void pop_move(game *g, move mv) 
{
    color opp = g->turn; 
    color col = opp_col(opp); 

    g->ply--; 
    g->turn = col; 

    move_hist *hist = g->hist + --g->depth; 
    g->halfmove = hist->halfmove; 
    g->ep = hist->ep; 
    g->castle = hist->castle; 
    g->in_check = hist->in_check; 
    g->hash = hist->hash; 

    piece pc = from_pc(mv); 
    piece pro = pro_pc(mv); 
    piece tgt = tgt_pc(mv); 

    square src = from_sq(mv); 
    square dst = to_sq(mv); 

    bool ep = is_ep(mv); 
    int cas_idx = castle_idx(mv); 

    bboard src_pos = BB[src]; 
    bboard dst_pos = BB[dst]; 

    if (ep) 
    {
        bboard rm_pos = BB[g->ep - 8 * col_sign(g->turn)]; 

        g->colors[col] ^= src_pos ^ dst_pos; 
        g->colors[opp] ^= rm_pos; 

        g->pieces[pc] ^= src_pos ^ dst_pos; 
        g->pieces[tgt] ^= rm_pos; 
        
        set_mbox(&g->mailbox, src, pc); 
        set_mbox(&g->mailbox, g->ep - 8 * col_sign(g->turn), tgt); 
        clear_mbox(&g->mailbox, dst); 

        g->counts[tgt]++; 
    }
    else if (cas_idx) 
    {
        g->colors[col] ^= MOVE_CASTLE_BB_ALL[cas_idx]; 

        g->pieces[pc] ^= MOVE_CASTLE_BB_K[cas_idx]; 
        g->pieces[make_pc(PC_R, col)] ^= MOVE_CASTLE_BB_R[cas_idx]; 

        clear_mbox(&g->mailbox, MOVE_CASTLE_SQ_K[cas_idx][1]); 
        clear_mbox(&g->mailbox, MOVE_CASTLE_SQ_R[cas_idx][1]); 
        set_mbox(&g->mailbox, MOVE_CASTLE_SQ_K[cas_idx][0], pc); 
        set_mbox(&g->mailbox, MOVE_CASTLE_SQ_R[cas_idx][0], make_pc(PC_R, col)); 
    }
    else if (pc != pro) 
    {
        g->colors[col] ^= src_pos ^ dst_pos; 
        g->colors[opp] ^= (tgt != NO_PC) * dst_pos; 

        g->pieces[pc] ^= src_pos; 
        g->pieces[pro] ^= dst_pos; 
        g->pieces[tgt] ^= dst_pos; 
        
        set_mbox(&g->mailbox, src, pc); 
        set_mbox(&g->mailbox, dst, tgt); 

        g->counts[tgt]++; 
        g->counts[pc]++; 
        g->counts[pro]--; 
    }
    else 
    {
        g->colors[col] ^= src_pos ^ dst_pos; 
        g->colors[opp] ^= (tgt != NO_PC) * dst_pos; 

        g->pieces[pc] ^= src_pos ^ dst_pos; 
        g->pieces[tgt] ^= dst_pos; 
        
        set_mbox(&g->mailbox, src, pc); 
        set_mbox(&g->mailbox, dst, tgt); 

        g->counts[tgt]++; 
    }

    g->all = g->colors[COL_W] | g->colors[COL_B]; 
    g->movement = ~g->colors[col]; 

    VALIDATE_GAME_MOVE(g, mv, "popping"); 
}

void push_null_move(game *g) 
{
    g->nodes++; 

    move_hist *hist = g->hist + g->depth++; 
    hist->halfmove = g->halfmove; 
    hist->ep = g->ep; 
    hist->castle = g->castle; 
    hist->in_check = g->in_check; 
    hist->hash = g->hash; 

    g->hash ^= col_zb(); 
    g->hash ^= ep_zb(g->ep); 

    color col = g->turn; 
    color opp = opp_col(col); 

    g->ep = NO_SQ; 
    g->hash ^= ep_zb(g->ep); 

    // for next movement: 
    //   can move to any square without own pieces 
    g->movement = ~g->colors[opp]; 

    g->ply++; 
    g->turn = opp; 

    g->in_check = is_attacked(g, lsb(g->pieces[make_pc(PC_K, opp)]), opp); 

    VALIDATE_GAME_MOVE(g, 0, "pushing null"); 
}

void pop_null_move(game *g) 
{
    color opp = g->turn; 
    color col = opp_col(opp); 

    g->ply--; 
    g->turn = col; 

    move_hist *hist = g->hist + --g->depth; 
    g->halfmove = hist->halfmove; 
    g->ep = hist->ep; 
    g->castle = hist->castle; 
    g->in_check = hist->in_check; 
    g->hash = hist->hash; 

    g->all = g->colors[COL_W] | g->colors[COL_B]; 
    g->movement = ~g->colors[col]; 

    VALIDATE_GAME_MOVE(g, 0, "popping null"); 
}

bool is_special_draw(const game *g) 
{
    // 50-move rule 
    if (g->halfmove > 99) return true; 

    for (int i = 0; i < DRAW_DEPTH; i++) 
    {
        // (g->depth-1) is previous ply (opponent made the move)
        // so our first depth to check should be (g->depth-2)
        int depth = (g->depth-2) - i*2; 

        if (depth >= 0) 
        {
            if (g->hash == g->hist[depth].hash) return true; 
            if ((g->hash ^ col_zb()) == g->hist[depth].hash) return true; 
        }
        else 
        {
            break; 
        }
    }

    return false; 
}

bool validate_game(const game *g) 
{
    bool valid = true; 

    // piece counts 
    for (piece pc = 0; pc < PC_CNT; pc++) 
    {
        if (g->counts[pc] != popcnt(g->pieces[pc])) 
        {
            printf("info string ERROR %s count is %d but should be %d\n", str_pc(pc), g->counts[pc], popcnt(g->pieces[pc])); 
            valid = false; 
        }
    }

    // exactly 1 king 
    {
        for (color col = COL_W; col < COL_CNT; col++) 
        {
            if (g->counts[make_pc(PC_K, col)] != 1) 
            {
                printf("info string ERROR %s does not have exactly 1 king: %d\n", col ? "white" : "black", g->counts[make_pc(PC_K, col)]); 
                valid = false; 
            }
        }
    }

    // bitboards 
    {
        bboard all = 0, col[2] = { 0, 0 }; 
        for (piece pc = 0; pc < PC_CNT; pc++) 
        {
            all ^= g->pieces[pc]; 
            col[get_col(pc)] ^= g->pieces[pc]; 
        }

        if (all != g->all) { printf("info string ERROR piece bitboards don't match accumulated board\n"); valid = false; } 
        if (col[COL_W] != g->colors[COL_W]) { printf("info string ERROR piece bitboards don't match accumulated white board\n"); valid = false; } 
        if (col[COL_B] != g->colors[COL_B]) { printf("info string ERROR piece bitboards don't match accumulated black board\n"); valid = false; } 
    }

    // check 
    {
        square ksq = lsb(g->pieces[make_pc(PC_K, g->turn)]); 
        bool actually_in_check = is_attacked(g, ksq, g->turn); 
        if (actually_in_check ^ g->in_check) 
        {
            printf("info string ERROR check flag is incorrect: %d instead of %d\n", g->in_check, actually_in_check); 
            printf("info string       king on %s\n", str_sq(ksq)); 
            valid = false; 
        }
    }

    // pc_at 
    {
        for (square sq = 0; sq < SQ_CNT; sq++) 
        {
            piece pc = pc_at(g, sq); 
            if (pc > NO_SQ) 
            {
                printf("info string ERROR mailbox piece is invalid: %d at %s\n", pc, str_sq(sq)); 
                valid = false; 
            }
            else if (pc == NO_PC) 
            {
                if (get_bit(g->all, sq)) 
                {
                    printf("info string ERROR mailbox has no piece but should have one: %s\n", str_sq(sq)); 
                    valid = false; 
                }
            }
            else // valid piece 
            {
                if (!get_bit(g->pieces[pc], sq)) 
                {
                    printf("info string ERROR mailbox has %s but should be empty: %s\n", str_pc(pc), str_sq(sq)); 
                    valid = false; 
                }
            }
        }
    }

    // castling 
    {
        if (g->castle & CASTLE_WK) 
        {
            if (pc_at(g, E1) != PC_WK || pc_at(g, H1) != PC_WR) 
            {
                printf("info string ERROR white O-O flag is set but no longer possible\n"); 
                valid = false; 
            }
        }
        if (g->castle & CASTLE_WQ) 
        {
            if (pc_at(g, E1) != PC_WK || pc_at(g, A1) != PC_WR) 
            {
                printf("info string ERROR white O-O-O flag is set but no longer possible\n"); 
                valid = false; 
            }
        }
        if (g->castle & CASTLE_BK) 
        {
            if (pc_at(g, E8) != PC_BK || pc_at(g, H8) != PC_BR) 
            {
                printf("info string ERROR black O-O flag is set but no longer possible\n"); 
                valid = false; 
            }
        }
        if (g->castle & CASTLE_BQ) 
        {
            if (pc_at(g, E8) != PC_BK || pc_at(g, A8) != PC_BR) 
            {
                printf("info string ERROR black O-O-O flag is set but no longer possible\n"); 
                valid = false; 
            }
        }
    }

    // hash 
    {
        zobrist hash = 0; 

        // color 
        if (g->turn) 
        {
            hash ^= col_zb(); 
        }

        // castling 
        hash ^= castle_zb(g->castle); 

        // ep 
        hash ^= ep_zb(g->ep); 

        // pieces 
        for (piece pc = 0; pc < PC_CNT; pc++) 
        {
            FOR_EACH_BIT(g->pieces[pc], 
            {
                hash ^= sq_pc_zb(sq, pc); 
            });
        }

        if (hash != g->hash) 
        {
            printf("info string ERROR zobrist hashing is invalid: ");
            print_zb_end(g->hash, " instead of "); 
            print_zb_end(hash, ", remainder: ");
            find_print_zb(hash ^ g->hash);  
            valid = false;  
        }
    }

    if (!valid) 
    {
        printf("info string ERROR game state is not valid, exitting\n"); 
        print_game(g); 
    }

    return valid; 
}

static inline uint64_t perft_(game *g, mvlist *moves, int depth) 
{
    uint64_t total = 0, start = moves->size, size; 

    if (depth == 1) 
    {
        mvinfo info; 
        gen_mvinfo(g, &info); 
        total += info.n_moves; 
    }
    else if (depth > 1) 
    {
        gen_moves(g, moves); 
        size = moves->size; 

        // print_game(g); 

        for (size_t i = start; i < size; i++) 
        {
            move mv = moves->moves[i]; 
            push_move(g, mv); 
            total += perft_(g, moves, depth - 1); 
            pop_move(g, mv); 
        }

        pop_mvlist(moves, size); 
    }
    else 
    {
        total = 1; 
    }

    return total; 
}

uint64_t perft(game *g, int depth) 
{
    mvlist *moves = new_mvlist(); 

    uint64_t total = perft_(g, moves, depth); 
    printf("Depth: %d, Total: %" PRIu64 "", depth, total); 

    free_mvlist(moves); 
    return total; 
}
