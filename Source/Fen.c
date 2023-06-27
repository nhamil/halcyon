/**
 * @file Fen.c
 * @author Nicholas Hamilton 
 * @date 2023-02-20
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Loads a game from FEN notation. 
 */

#include "BBoard.h"
#include "Game.h" 
#include "Piece.h"

static const char* LoadFenBoard(Game* g, const char* fen) 
{
    Square sq = A1; 

    while (*fen) 
    {
        char c = *fen++; 
        if (c == ' ') break; 

        Piece pc = NO_PC; 
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
            Square realSq = RRank(sq); 
            SetMBox(&g->Mailbox, realSq, pc); 
            g->Pieces[pc] = SetBit(g->Pieces[pc], realSq); 

            sq++; 
        }
    }

    return fen; 
}

static const char* LoadFenCol(Game* g, const char* fen) 
{
    while (*fen) 
    {
        char c = *fen++; 
        if (c == ' ') break; 

        switch (c) 
        {
            case 'w': 
                g->Turn = COL_W; 
                break; 
            case 'b': 
                g->Turn = COL_B; 
                break; 
            default: 
                printf("info string Unknown FEN character (color): %c\n", c); 
                break; 
        }
    }

    return fen; 
}

static const char* LoadFenCastle(Game* g, const char* fen) 
{
    while (*fen) 
    {
        char c = *fen++; 
        if (c == ' ') break; 

        CastleFlags cf = 0; 
        switch (c) 
        {
            case 'K': cf = CASTLE_WK; break; 
            case 'Q': cf = CASTLE_WQ; break; 
            case 'k': cf = CASTLE_BK; break; 
            case 'q': cf = CASTLE_BQ; break; 
            case '-': g->Castle = 0; break; 

            default: 
                printf("info string Unknown FEN character (castling): %c\n", c); 
                break; 
        }

        g->Castle |= cf; 
    }

    return fen; 
}

static const char* LoadFenEP(Game* g, const char* fen) 
{
    Square ep = NO_SQ; 

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
                ep = MakeSq(c - 'a', GetRank(ep)); 
                break; 

            case '1': 
            case '2': 
            case '3': 
            case '4': 
            case '5': 
            case '6': 
            case '7': 
            case '8': 
                ep = MakeSq(GetFile(ep), c - '1'); 
                break; 

            case '-': 
                ep = NO_SQ; 
                break; 

            default: 
                printf("info string Unknown FEN character (en passant): %c\n", c); 
                break; 
        }
    }

    g->EP = ep; 

    return fen; 
}

static const char* LoadFenHalfmove(Game* g, const char* fen) 
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
                g->Halfmove = g->Halfmove * 10 + c - '0'; 
                break; 

            default: 
                printf("info string Unknown FEN character (halfmove): %c\n", c); 
                break; 
        }
    }

    return fen; 
}

static const char* LoadFenTurn(Game* g, const char* fen) 
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
                g->Ply = g->Ply * 10 + c - '0'; 
                break; 

            default: 
                printf("info string Unknown FEN character (turn): %c\n", c); 
                break; 
        }
    }

    g->Ply *= 2; 
    g->Ply += g->Turn; 

    if (found) g->Ply -= 2; 

    return fen; 
}

void LoadFen(Game* g, const char* fen) 
{
    ResetGame(g); 

    fen = LoadFenBoard(g, fen); 
    fen = LoadFenCol(g, fen); 
    fen = LoadFenCastle(g, fen); 
    fen = LoadFenEP(g, fen); 
    fen = LoadFenHalfmove(g, fen); 
    fen = LoadFenTurn(g, fen); 

    for (Piece pc = PC_P; pc < NUM_PC; pc++) 
    {
        g->Colors[GetCol(pc)] |= g->Pieces[pc]; 
        g->Counts[pc] = Popcnt(g->Pieces[pc]); 
    }

    g->All = g->Colors[COL_W] | g->Colors[COL_B]; 
    g->Movement = ~g->Colors[g->Turn]; 

    g->InCheck = IsAttacked(g, Lsb(g->Pieces[MakePc(PC_K, g->Turn)]), g->Turn); 

    for (Square sq = A1; sq <= H8; sq++) 
    {
        for (Piece pc = 0; pc < NUM_PC; pc++) 
        {
            if (GetBit(g->Pieces[pc], sq)) 
            {
                g->Hash ^= SqPcZb(sq, pc); 
            }
        }
    }

    g->Hash ^= CastleZb(g->Castle); 
    g->Hash ^= EPZb(g->EP); 
    g->Hash ^= (g->Turn == COL_B) * ColZb(); 
}

void ToFen(const Game* g, char* out) 
{
    // board position 
    for (int rank = 7; rank >= 0; rank--) 
    {
        int empty = 0; 
        for (int file = 0; file < 8; file++) 
        {
            Square sq = MakeSq(file, rank); 
            if (PcAt(g, sq) != NO_PC) 
            {
                // add empty squares before this piece
                if (empty > 0) 
                {
                    *out++ = '0' + empty; 
                    empty = 0; 
                }

                // all pieces are 1 character, so dereferencing 
                // should be okay 
                *out++ = *StrPc(PcAt(g, sq)); 
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
    *out++ = turn[g->Turn]; 
    *out++ = ' '; 

    // castling 
    const char* tmp = StrCastle(g->Castle); 
    if (!g->Castle) tmp = "-"; 
    strcpy(out, tmp); 
    out += strlen(tmp); 
    *out++ = ' '; 

    // en passant 
    tmp = StrSq(g->EP); 
    if (g->EP == NO_SQ) 
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
    sprintf(out, "%d", g->Halfmove); 
    out += strlen(out); 
    *out++ = ' '; 

    // fullmove counter 
    sprintf(out, "%d", g->Ply / 2 + 1); 
    out += strlen(out); 
    *out++ = ' '; 

    // end string 
    *out = 0; 
}
