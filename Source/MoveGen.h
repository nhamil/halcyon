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
 * Used to store moves. 
 */
typedef struct MoveList MoveList; 

/**
 * Stores moves without needing to add them all to a list. 
 */
typedef struct MoveInfo MoveInfo; 

/**
 * Maximum possible moves a color has in a legal position. 
 */
#define MaxMovesPerTurn 218

struct MoveList 
{
    Move Moves[MaxMovesPerTurn * MaxDepth]; 
    U64 Size; 
};

/**
 * Swaps two elements. 
 * 
 * @param m Move list 
 * @param a First index
 * @param b Second index 
 */
static inline void SwapMoves(MoveList* m, U64 a, U64 b) 
{
    Move tmp = m->Moves[a]; 
    m->Moves[a] = m->Moves[b]; 
    m->Moves[b] = tmp; 
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
 * Allocates a new move list. 
 * 
 * @return New move list 
 */
MoveList* NewMoveList(void); 

/**
 * Frees a move list. 
 * 
 * @param moves Move list 
 */
void FreeMoveList(MoveList* moves); 

struct MoveInfo 
{
    Bitboard Moves[64]; 
    Piece Pieces[64]; 
    Square From[64]; 
    int NumPiecess; 
    int NumMoves; 
};

/**
 * Generate moves without listing them out. 
 * 
 * @param g Game state
 * @param info Info output 
 */
void GenMoveInfo(const Game* g, MoveInfo* info); 

/**
 * Put move info moves into a list 
 * 
 * @param g Game state 
 * @param info Move info 
 * @param moves List output
 */
void GenMovesFromInfo(const Game* g, const MoveInfo* info, MoveList* moves); 

/**
 * Generate moves and store them in a list. 
 * 
 * @param g Game state
 * @param moves List output 
 */
static inline void GenMoves(const Game* g, MoveList* moves) 
{
    MoveInfo info; 
    GenMoveInfo(g, &info); 
    GenMovesFromInfo(g, &info, moves); 
}
