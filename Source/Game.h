/**
 * @file Game.h
 * @author Nicholas Hamilton 
 * @date 2023-01-12
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Handles a single instance of a chess game, with support for undoing moves 
 * and playing null moves. 
 */

#pragma once 

#include <inttypes.h>
#include <limits.h> 
#include <stdbool.h>
#include <stddef.h> 
#include <stdint.h>
#include <string.h> 

#include "BBoard.h"
#include "Castle.h" 
#include "Magic.h" 
#include "MBox.h"
#include "Move.h" 
#include "Piece.h"
#include "Square.h"
#include "Vector.h"
#include "Zobrist.h"

typedef struct Game Game; 
typedef struct MoveHist MoveHist; 

#define DRAW_DEPTH 4

#define MAX_DEPTH 256 

#define EVAL_MAX (INT_MAX - 10000)

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#define FEN_LEN 128 

struct MoveHist 
{
    int Halfmove; 
    Square EP; 
    CastleFlags Castle; 
    bool InCheck;
    Zobrist Hash; 
};

struct Game 
{
    MoveHist Hist[MAX_DEPTH + DRAW_DEPTH*2]; 

    MBox Mailbox; 

    BBoard Pieces[NUM_PC + 1]; // extra bitboard for NO_PC
    BBoard Colors[NUM_COLS]; 
    BBoard All; 
    BBoard Movement; 
    int Counts[NUM_PC + 1]; 

    Zobrist Hash; 
    CastleFlags Castle; 
    Square EP; 

    int Ply; 
    int Halfmove; 
    Color Turn; 
    bool InCheck; 
    int Depth; 

    U64 Nodes; 
};

Game* NewGame(void); 

void FreeGame(Game* g); 

void ResetGame(Game* g); 

void CopyGame(Game* g, const Game* from); 

void LoadFen(Game* g, const char* fen); 

void ToFen(const Game* g, char* out); 

void PushMove(Game* g, Move m); 

void PopMove(Game* g, Move m); 

void PushNullMove(Game* g); 

void PopNullMove(Game* g); 

void PrintGame(const Game* g); 

U64 Perft(Game* g, int depth); 

bool IsSpecialDraw(const Game* g);

int Evaluate(const Game* g, int nMoves, bool draw); 

bool ValidateGame(const Game* g); 

#ifdef VALIDATION 
    #define VALIDATE_GAME_MOVE(g, mv, action) \
        if (!ValidateGame(g)) \
        {\
            printf("info string ERROR validation of %s move ", action); \
            PrintMoveEnd(mv, " failed\n"); \
            exit(1); \
        }
#else 
    #define VALIDATE_GAME_MOVE(g, mv, action)
#endif 

static inline Piece PcAt(const Game* g, Square sq) 
{
    return GetMBox(&g->Mailbox, sq); 
}

static inline bool AnySideKP(const Game* g) 
{
    bool wKP = g->Colors[COL_W] == (g->Pieces[PC_WK] | g->Pieces[PC_WP]); 
    bool bKP = g->Colors[COL_B] == (g->Pieces[PC_BK] | g->Pieces[PC_BP]); 
    return wKP | bKP;  
}

/**
 * Removes move history to increase max depth. 
 * Do not use PopMove on previous moves after calling this. 
 */
static inline void NoDepth(Game* g) 
{
    int copyAmt = DRAW_DEPTH * 2; 
    if (g->Depth < copyAmt) copyAmt = g->Depth; 

    if (copyAmt > 0) 
    {
        int copyStart = g->Depth - copyAmt; 

        for (int i = 0; i < copyAmt; i++) 
        {
            g->Hist[i] = g->Hist[copyStart + i]; 
        }
    }

    g->Depth = copyAmt; 
}

static inline BBoard RAttacks(Square sq, BBoard occ) 
{
    return MagicRSlide[sq][((MagicRMask[sq] & occ) * MagicR[sq]) >> MagicRShift[sq]];
}

static inline BBoard BAttacks(Square sq, BBoard occ) 
{
    return MagicBSlide[sq][((MagicBMask[sq] & occ) * MagicB[sq]) >> MagicBShift[sq]];
}

static inline bool IsAttacked(const Game* g, Square sq, Color chkCol) 
{
    Color col = chkCol; 
    Color opp = OppCol(col); 

    BBoard occ = g->All; 

    BBoard oppP = g->Pieces[MakePc(PC_P, opp)]; 
    BBoard oppN = g->Pieces[MakePc(PC_N, opp)]; 
    BBoard oppB = g->Pieces[MakePc(PC_B, opp)]; 
    BBoard oppR = g->Pieces[MakePc(PC_R, opp)]; 
    BBoard oppQ = g->Pieces[MakePc(PC_Q, opp)]; 
    BBoard oppK = g->Pieces[MakePc(PC_K, opp)]; 

    BBoard oppRQ = (oppR | oppQ); 
    BBoard oppBQ = (oppB | oppQ); 

    BBoard chk = (RAttacks(sq, occ) & oppRQ) 
               | (BAttacks(sq, occ) & oppBQ) 
               | (oppK & MovesK[sq]) 
               | (oppN & MovesN[sq]) 
               | (oppP & AttacksP[col][sq]);     

    return chk != 0; 
}

extern int PcSq[2][NUM_PC_TYPES][NUM_SQ]; 
