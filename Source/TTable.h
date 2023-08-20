/**
 * @file TTable.h
 * @author Nicholas Hamilton 
 * @date 2023-07-01
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Defines transposition table. 
 */

#pragma once 

#include "Game.h" 
#include "Mailbox.h"
#include "Move.h" 
#include "Types.h" 
#include "Zobrist.h" 

/**
 * Node classification. 
 */
typedef enum 
{
    NoNode = 0, 
    UsedNode, 
    PVNode, 
    FailHigh, 
    FailLow
} NodeType;

/**
 * Transposition table. 
 */
typedef struct TTable TTable; 

/**
 * Transposition table entry data. 
 */
typedef struct TTableEntry TTableEntry; 

struct TTable 
{
    TTableEntry* Entries; 
    U64 Size; 
    U64 Mask; 
    U64 Hits; 
    U64 Searches; 
    U64 Collisions; 
    U64 Used; 
};

struct TTableEntry 
{
#ifdef VALIDATION
    Game State; 
#endif
    Zobrist Key; 
    Move Mv; 
    int Score; 
    unsigned char Depth; 
    char Type; 
};

/**
 * Initializes a transposition table to a target size. 
 * 
 * @param tt The table 
 * @param sizeInMb Target size in MiB
 */
void CreateTTable(TTable* tt, U64 sizeInMb); 

/**
 * Deinitializes a transposition table. 
 * 
 * @param tt The table 
 */
void DestroyTTable(TTable* tt); 

/**
 * Removes all table entries and clears stats. 
 * 
 * @param tt The table 
 */
void ResetTTable(TTable* tt); 

/**
 * Queries a table for an entry using a hash key. 
 * 
 * @param tt The table 
 * @param key Game state hash 
 * @param state Game state 
 * @return Entry if found, null otherwise
 */
TTableEntry* FindTTableEntry(TTable* tt, Zobrist key, const Game* state); 

/**
 * Updates a table entry. 
 * 
 * @param tt The table 
 * @param key Game state hash 
 * @param type Node type 
 * @param score Evaluation score 
 * @param depth Search depth 
 * @param mv Best move 
 * @param state Game state 
 */
void UpdateTTable(TTable* tt, Zobrist key, int type, int score, int depth, Move mv, const Game* state); 
