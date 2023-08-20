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

#define InfDepth (-1) 
#define InfTime (-1) 

typedef struct PVLine PVLine; 
typedef struct SearchContext SearchContext; 
typedef struct SearchParams SearchParams; 

/**
 * Principal variation. 
 */
struct PVLine 
{
    U64 NumMoves; 
    Move Moves[MaxDepth]; 
};

/**
 * Data used by search. 
 */
struct SearchContext 
{
    Game* State; 
    int StartPly; 
    pthread_t Thread; 
    int TargetDepth; 
    int TargetTimeMs; 
    clock_t EndAt; 
    bool ShouldExit; 

    pthread_mutex_t Lock; 
    PVLine BestLine; 
    U64 Nodes; 
    U64 Nps; 
    int Depth; 
    int Eval; 
    bool Running; 
    clock_t StartAt; 
    clock_t CurMoveAt; 
    clock_t NextMessageAt; 

    MoveList* Moves; 
    PVLine Lines[MaxDepth]; 
    TTable Transpositions; 
    U64 NumNodes; 
    U64 NumLeaves; 
    U64 NumQNodes; 
    U64 NumQLeaves; 
    int CheckTime; 
    int Ply; 
    Move Killer[MaxDepth][2]; 
    int History[2][NumPieces][NumSquares]; 
    bool NullMove; 
    bool InPV; 
    int Contempt; 
    int ColorContempt; 
    Color StartColor; 
};

/**
 * Search settings 
 */
struct SearchParams 
{
    const Game* Board; 
    int Depth; 
    int TimeMs; 
};

/**
 * @param eval Evaluation score 
 * @return True if the evaluation is mate-in-N, otherwise false
 */
static inline bool IsMateScore(int eval) 
{
    return eval > 90000 || eval < -90000; 
}

/**
 * @param eval Evaluation score 
 * @return True if the new score is weaker, otherwise false
 */
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

/**
 * Initializes a search context. 
 * 
 * @param ctx The context
 */
void CreateSearchContext(SearchContext* ctx); 

/**
 * Deinitializes a search context. 
 * 
 * @param ctx The context
 */
void DestroySearchContext(SearchContext* ctx); 

/**
 * Stops the search thread if it is running. 
 * 
 * @param ctx The context
 */
void StopSearchContext(SearchContext* ctx); 

/**
 * Blocks the current thread until search is complete. 
 * 
 * @param ctx The context 
 */
void WaitForSearchContext(SearchContext* ctx); 

/**
 * Creates search settings 
 * 
 * @param sp The settings
 * @param board Board state 
 * @param depth Target depth (or InfDepth)
 * @param timeMs Target time (or InfTime)
 */
static void InitSearchParams(SearchParams* sp, const Game* board, int depth, int timeMs) 
{
    sp->Board = board; 
    sp->Depth = depth; 
    sp->TimeMs = timeMs; 
}

/**
 * Starts searching the specified board position on a new thread. 
 * If the context was already searching then the old search stops. 
 * 
 * @param ctx The context
 * @param params Search parameters 
 */
void Search(SearchContext* ctx, SearchParams* params); 

/**
 * Returns quiescence search on the board position. 
 * 
 * @param ctx The context 
 * @return Quiescence search value 
 */
int BasicQSearch(SearchContext* ctx); 
