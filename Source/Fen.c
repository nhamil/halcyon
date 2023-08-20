/**
 * @file Fen.c
 * @author Nicholas Hamilton 
 * @date 2023-02-20
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Loads a game from FEN notation. 
 */

#include "Bitboard.h"
#include "Game.h" 
#include "Piece.h"

static const char* LoadFenBoard(Game* g, const char* fen) 
{
    Square sq = A1; 

    while (*fen) 
    {
        char c = *fen++; 
        if (c == ' ') break; 

        Piece pc = NoPiece; 
        switch (c) 
        {
            case 'P': pc = PieceWP; break; 
            case 'N': pc = PieceWN; break; 
            case 'B': pc = PieceWB; break; 
            case 'R': pc = PieceWR; break; 
            case 'Q': pc = PieceWQ; break; 
            case 'K': pc = PieceWK; break; 
            case 'p': pc = PieceBP; break; 
            case 'n': pc = PieceBN; break; 
            case 'b': pc = PieceBB; break; 
            case 'r': pc = PieceBR; break; 
            case 'q': pc = PieceBQ; break; 
            case 'k': pc = PieceBK; break; 

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

        if (pc != NoPiece) 
        {
            Square realSquare = FlipRank(sq); 
            SetPieceAt(&g->Board, realSquare, pc); 
            g->Pieces[pc] = SetBit(g->Pieces[pc], realSquare); 

            sq++; 
        }
    }

    return fen; 
}

static const char* LoadFenColor(Game* g, const char* fen) 
{
    while (*fen) 
    {
        char c = *fen++; 
        if (c == ' ') break; 

        switch (c) 
        {
            case 'w': 
                g->Turn = ColorW; 
                break; 
            case 'b': 
                g->Turn = ColorB; 
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
            case 'K': cf = CastleWK; break; 
            case 'Q': cf = CastleWQ; break; 
            case 'k': cf = CastleBK; break; 
            case 'q': cf = CastleBQ; break; 
            case '-': g->Castle = 0; break; 

            default: 
                printf("info string Unknown FEN character (castling): %c\n", c); 
                break; 
        }

        g->Castle |= cf; 
    }

    return fen; 
}

static const char* LoadFenEnPassant(Game* g, const char* fen) 
{
    Square ep = NoSquare; 

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
                ep = MakeSquare(c - 'a', GetRank(ep)); 
                break; 

            case '1': 
            case '2': 
            case '3': 
            case '4': 
            case '5': 
            case '6': 
            case '7': 
            case '8': 
                ep = MakeSquare(GetFile(ep), c - '1'); 
                break; 

            case '-': 
                ep = NoSquare; 
                break; 

            default: 
                printf("info string Unknown FEN character (en passant): %c\n", c); 
                break; 
        }
    }

    g->EnPassant = ep; 

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
    fen = LoadFenColor(g, fen); 
    fen = LoadFenCastle(g, fen); 
    fen = LoadFenEnPassant(g, fen); 
    fen = LoadFenHalfmove(g, fen); 
    fen = LoadFenTurn(g, fen); 

    for (Piece pc = 0; pc < NumPieces; pc++) 
    {
        g->Colors[ColorOfPiece(pc)] |= g->Pieces[pc]; 
        g->Counts[pc] = PopCount(g->Pieces[pc]); 
    }

    g->All = g->Colors[ColorW] | g->Colors[ColorB]; 
    g->Movement = ~g->Colors[g->Turn]; 

    g->InCheck = IsAttacked(g, LeastSigBit(g->Pieces[MakePiece(PieceK, g->Turn)]), g->Turn); 

    for (Square sq = A1; sq <= H8; sq++) 
    {
        for (Piece pc = 0; pc < NumPieces; pc++) 
        {
            if (GetBit(g->Pieces[pc], sq)) 
            {
                g->Hash ^= HashSquarePiece(sq, pc); 
            }
        }
    }

    g->Hash ^= HashCastleFlags(g->Castle); 
    g->Hash ^= HashEnPassant(g->EnPassant); 
    g->Hash ^= (g->Turn == ColorB) * HashColor(); 
}

void ToFen(const Game* g, char* out) 
{
    // board position 
    for (int rank = 7; rank >= 0; rank--) 
    {
        int empty = 0; 
        for (int file = 0; file < 8; file++) 
        {
            Square sq = MakeSquare(file, rank); 
            if (PieceAt(&g->Board, sq) != NoPiece) 
            {
                // add empty squares before this piece
                if (empty > 0) 
                {
                    *out++ = '0' + empty; 
                    empty = 0; 
                }

                // all pieces are 1 character, so dereferencing 
                // should be okay 
                *out++ = *PieceString(PieceAt(&g->Board, sq)); 
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
    const char* tmp = CastleString(g->Castle); 
    if (!g->Castle) tmp = "-"; 
    strcpy(out, tmp); 
    out += strlen(tmp); 
    *out++ = ' '; 

    // en passant 
    tmp = SquareString(g->EnPassant); 
    if (g->EnPassant == NoSquare) 
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
