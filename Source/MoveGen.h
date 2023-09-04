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

#include "Bitboard.h"
#include "Game.h" 
#include "Move.h"

/**
 * Used to store moves for a single ply. 
 */
typedef struct MoveList MoveList; 

/**
 * Maximum possible moves a color has in a legal position. 
 */
#define MaxMovesPerTurn 218

struct MoveList 
{
    Move Moves[MaxMovesPerTurn]; 
    U8 Size; 
};

/**
 * Swaps two elements. 
 * 
 * @param m Move list 
 * @param a First index
 * @param b Second index 
 */
static inline void SwapMoves(MoveList* m, U8 a, U8 b) 
{
    Move tmp = m->Moves[a]; 
    m->Moves[a] = m->Moves[b]; 
    m->Moves[b] = tmp; 
}

/**
 * Adds one move to a move list. 
 * 
 * @param m Move list 
 * @param mv Move to add 
 */
static inline void PushMoveToList(MoveList* m, Move mv) 
{
    m->Moves[m->Size++] = mv; 
}

/**
 * Pops elements to reach a specified size. 
 * 
 * @param m Move list 
 * @param toSize Target size 
 */
static inline void PopMovesToSize(MoveList* m, U64 toSize) 
{
    m->Size = toSize; 
}

/**
 * Clears all elements. 
 * 
 * @param m Move list 
 */
static inline void ClearMoves(MoveList* m) 
{
    m->Size = 0; 
}

/**
 * Generate moves and store them in a list. 
 * 
 * @param g Game state
 * @param moves List output 
 */
void GenMoves(const Game* g, MoveList* moves); 

/**
 * Generate capture (and promotion) moves and store them in a list. 
 * 
 * @param g Game state 
 * @param moves List output
 */
void GenCaptureMoves(const Game* g, MoveList* moves); 

/**
 * Generate quiet moves and store them in a list. 
 * 
 * @param g Game state 
 * @param moves List output
 */
void GenQuietMoves(const Game* g, MoveList* moves); 
