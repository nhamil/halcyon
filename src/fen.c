#include "game.h" 
#include "piece.h"

static const char *load_fen_board(game *g, const char *fen) 
{
    square sq = A1; 

    while (*fen) 
    {
        char c = *fen++; 
        if (c == ' ') break; 

        piece pc = NO_PC; 
        switch (c) 
        {
            case 'P': pc = PC_WP; break; 
            case 'N': pc = PC_WN; break; 
            case 'B': pc = PC_WB; break; 
            case 'R': pc = PC_WR; break; 
            case 'Q': pc = PC_WQ; break; 
            case 'K': pc = PC_WK; break; 
            case 'p': pc = PC_BP; break; 
            case 'n': pc = PC_BN; break; 
            case 'b': pc = PC_BB; break; 
            case 'r': pc = PC_BR; break; 
            case 'q': pc = PC_BQ; break; 
            case 'k': pc = PC_BK; break; 

            case '8': 
            case '7': 
            case '6': 
            case '5': 
            case '4': 
            case '3': 
            case '2': 
            case '1': 
                sq += c - '0'; 
                break; 

            case '/': 
                // handled automatically 
                break; 

            default: 
                printf("info string Unknown FEN character (board setup): %c\n", c); 
                break; 
        }

        if (pc != NO_PC) 
        {
            square real_sq = rrank(sq); 
            set_mbox(&g->mailbox, real_sq, pc); 
            g->pieces[pc] = set_bit(g->pieces[pc], real_sq); 

            sq++; 
        }
    }

    return fen; 
}

static const char *load_fen_col(game *g, const char *fen) 
{
    while (*fen) 
    {
        char c = *fen++; 
        if (c == ' ') break; 

        switch (c) 
        {
            case 'w': 
                g->turn = COL_W; 
                break; 
            case 'b': 
                g->turn = COL_B; 
                break; 
            default: 
                printf("info string Unknown FEN character (color): %c\n", c); 
                break; 
        }
    }

    return fen; 
}

static const char *load_fen_castle(game *g, const char *fen) 
{
    while (*fen) 
    {
        char c = *fen++; 
        if (c == ' ') break; 

        castle_flags cf = 0; 
        switch (c) 
        {
            case 'K': cf = CASTLE_WK; break; 
            case 'Q': cf = CASTLE_WQ; break; 
            case 'k': cf = CASTLE_BK; break; 
            case 'q': cf = CASTLE_BQ; break; 
            case '-': g->castle = 0; break; 

            default: 
                printf("info string Unknown FEN character (castling): %c\n", c); 
                break; 
        }

        g->castle |= cf; 
    }

    return fen; 
}

static const char *load_fen_ep(game *g, const char *fen) 
{
    square ep = NO_SQ; 

    while (*fen) 
    {
        char c = *fen++; 
        if (c == ' ') break; 

        switch (c) 
        {
            case 'a': 
            case 'b': 
            case 'c': 
            case 'd': 
            case 'e': 
            case 'f': 
            case 'g': 
            case 'h': 
                ep = make_sq(c - 'a', get_rank(ep)); 
                break; 

            case '1': 
            case '2': 
            case '3': 
            case '4': 
            case '5': 
            case '6': 
            case '7': 
            case '8': 
                ep = make_sq(get_file(ep), c - '1'); 
                break; 

            case '-': 
                ep = NO_SQ; 
                break; 

            default: 
                printf("info string Unknown FEN character (en passant): %c\n", c); 
                break; 
        }
    }

    g->ep = ep; 

    return fen; 
}

static const char *load_fen_halfmove(game *g, const char *fen) 
{
    while (*fen) 
    {
        char c = *fen++; 
        if (c == ' ') break; 

        switch (c) 
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
                g->halfmove = g->halfmove * 10 + c - '0'; 
                break; 

            default: 
                printf("info string Unknown FEN character (halfmove): %c\n", c); 
                break; 
        }
    }

    return fen; 
}

static const char *load_fen_turn(game *g, const char *fen) 
{
    bool found = false; 

    while (*fen) 
    {
        char c = *fen++; 
        if (c == ' ') break; 

        switch (c) 
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
                found = true; 
                g->ply = g->ply * 10 + c - '0'; 
                break; 

            default: 
                printf("info string Unknown FEN character (turn): %c\n", c); 
                break; 
        }
    }

    g->ply *= 2; 
    g->ply += g->turn; 

    if (found) g->ply -= 2; 

    return fen; 
}

void load_fen(game *g, const char *fen) 
{
    reset_game(g); 

    fen = load_fen_board(g, fen); 
    fen = load_fen_col(g, fen); 
    fen = load_fen_castle(g, fen); 
    fen = load_fen_ep(g, fen); 
    fen = load_fen_halfmove(g, fen); 
    fen = load_fen_turn(g, fen); 

    for (piece pc = PC_P; pc < PC_CNT; pc++) 
    {
        g->colors[get_col(pc)] |= g->pieces[pc]; 
    }

    g->all = g->colors[COL_W] | g->colors[COL_B]; 
    g->movement = ~g->colors[g->turn]; 

    g->in_check = is_attacked(g, lsb(g->pieces[make_pc(PC_K, g->turn)]), g->turn); 

    for (square sq = A1; sq <= H8; sq++) 
    {
        for (piece pc = 0; pc < PC_CNT; pc++) 
        {
            if (get_bit(g->pieces[pc], sq)) 
            {
                g->hash ^= sq_pc_zb(sq, pc); 
            }
        }
    }

    g->hash ^= castle_zb(g->castle); 
    g->hash ^= ep_zb(g->ep); 
    g->hash ^= (g->turn == COL_B) * col_zb(); 
}

void to_fen(const game *g, char *out) 
{
    // board position 
    for (int rank = 7; rank >= 0; rank--) 
    {
        int empty = 0; 
        for (int file = 0; file < 8; file++) 
        {
            square sq = make_sq(file, rank); 
            if (pc_at(g, sq) != NO_PC) 
            {
                // add empty squares before this piece
                if (empty > 0) 
                {
                    *out++ = '0' + empty; 
                    empty = 0; 
                }

                // all pieces are 1 character, so dereferencing 
                // should be okay 
                *out++ = *str_pc(pc_at(g, sq)); 
            }
            else 
            {
                empty++; 
            }
        }

        // remaining empty squares for the current rank 
        if (empty > 0) 
        {
            *out++ = '0' + empty; 
        }

        if (rank > 0) 
        {
            *out++ = '/'; 
        }
    }

    *out++ = ' '; 

    // color to play 
    char turn[2] = { 'w', 'b' }; 
    *out++ = turn[g->turn]; 
    *out++ = ' '; 

    // castling 
    const char *tmp = str_castle(g->castle); 
    if (!g->castle) tmp = "-"; 
    strcpy(out, tmp); 
    out += strlen(tmp); 
    *out++ = ' '; 

    // en passant 
    tmp = str_sq(g->ep); 
    if (g->ep == NO_SQ) 
    {
        *out++ = '-'; 
    }
    else 
    {
        strcpy(out, tmp); 
        out += strlen(tmp); 
    }
    *out++ = ' '; 

    // halfmove clock
    sprintf(out, "%d", g->halfmove); 
    out += strlen(out); 
    *out++ = ' '; 

    // fullmove counter 
    sprintf(out, "%d", g->ply / 2 + 1); 
    out += strlen(out); 
    *out++ = ' '; 

    // end string 
    *out = 0; 
}
