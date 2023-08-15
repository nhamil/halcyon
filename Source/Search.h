/**
 * @file Search.h
 * @author Nicholas Hamilton 
 * @date 2023-01-19
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Defines functions for finding the best move. 
 */

#pragma once 

#include <stdatomic.h> 
#include <time.h> 

#include <pthread.h> 

#include "Game.h" 
#include "MoveGen.h"
#include "Piece.h"
#include "Square.h"
#include "TTable.h" 

#define INF_DEPTH (-1) 
#define INF_TIME (-1) 

typedef struct PVLine PVLine; 
typedef struct SearchCtx SearchCtx; 
typedef struct SearchParams SearchParams; 

struct PVLine 
{
    U64 NumMoves; 
    Move Moves[MAX_DEPTH]; 
};

struct SearchCtx 
{
    Game* Board; 
    int StartPly; 
    pthread_t Thread; 
    int TgtDepth; 
    int TgtTime; 
    clock_t EndAt; 
    bool ShouldExit; 

    pthread_mutex_t Lock; 
    PVLine PV; 
    U64 Nodes; 
    U64 Nps; 
    int Depth; 
    int Eval; 
    bool Running; 
    clock_t StartAt; 
    clock_t CurMoveAt; 
    clock_t NextMsgAt; 

    MvList* Moves; 
    PVLine Lines[MAX_DEPTH]; 
    TTable TT; 
    U64 NumNodes; 
    U64 NumLeaves; 
    U64 NumQNodes; 
    U64 NumQLeaves; 
    int CheckTime; 
    int Ply; 
    Move Killer[MAX_DEPTH][2]; 
    int History[2][NUM_PC][NUM_SQ]; 
    bool NullMove; 
    bool InPV; 
    int Contempt; 
    int ColContempt; 
    Color StartCol; 
};

struct SearchParams 
{
    const Game* Board; 
    int Depth; 
    int TimeMs; 
};

static inline bool IsMateScore(int eval) 
{
    return eval > 90000 || eval < -90000; 
}

static inline bool IsScoreLessStrong(int eval, int newEval) 
{
    if (eval < 0) 
    {
        return newEval > eval; 
    }
    else if (eval > 0) 
    {
        return newEval < eval; 
    }
    else 
    {
        return false; 
    }
}

void CreateSearchCtx(SearchCtx* ctx); 

void DestroySearchCtx(SearchCtx* ctx); 

void StopSearchCtx(SearchCtx* ctx); 

void WaitSearchCtx(SearchCtx* ctx); 

static void InitSearchParams(SearchParams* sp, const Game* board, int depth, int timeMs) 
{
    sp->Board = board; 
    sp->Depth = depth; 
    sp->TimeMs = timeMs; 
}

void Search(SearchCtx* ctx, SearchParams* params); 

int BasicQSearch(SearchCtx* ctx); 

int QStabilize(Game* g, MvList* moves, int alpha, int beta, int depth, Game* out); 
