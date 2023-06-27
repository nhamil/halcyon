/**
 * @file MoveGen.h
 * @author Nicholas Hamilton 
 * @date 2023-02-20
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Defines movement generation utilities. 
 */

#pragma once 

#include "BBoard.h"
#include "Game.h" 
#include "Move.h"

typedef struct MvList MvList; 
typedef struct MvInfo MvInfo; 

#define MAX_MOVES_PER_TURN 218

struct MvList 
{
    Move Moves[MAX_MOVES_PER_TURN * MAX_DEPTH]; 
    U64 Size; 
};

static inline void SwapMvList(MvList* m, U64 a, U64 b) 
{
    Move tmp = m->Moves[a]; 
    m->Moves[a] = m->Moves[b]; 
    m->Moves[b] = tmp; 
}

static inline void PopMvList(MvList* m, U64 toSize) 
{
    m->Size = toSize; 
}

static inline void ClearMvList(MvList* m) 
{
    m->Size = 0; 
}

MvList* NewMvList(void); 

void FreeMvList(MvList* moves); 

struct MvInfo 
{
    BBoard Moves[64]; 
    Piece Pieces[64]; 
    Square From[64]; 
    int NumPieces; 
    int NumMoves; 
};

void GenMvInfo(const Game* g, MvInfo* info); 

void GenMovesMvInfo(const Game* g, const MvInfo* info, MvList* moves); 

static inline void GenMoves(const Game* g, MvList* moves) 
{
    MvInfo info; 
    GenMvInfo(g, &info); 
    GenMovesMvInfo(g, &info, moves); 
}
