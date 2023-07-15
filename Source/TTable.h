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
#include "MBox.h"
#include "Move.h" 
#include "Types.h" 
#include "Zobrist.h" 

#define NO_NODE 0
#define USED_NODE 1
#define PV_NODE 2 
#define FAIL_HIGH 3 
#define FAIL_LOW 4 

typedef struct TTable TTable; 
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

void CreateTTable(TTable* tt, U64 sizeInMB); 

void DestroyTTable(TTable* tt); 

void ResetTTable(TTable* tt); 

TTableEntry* FindTTableEntry(TTable* tt, Zobrist key, const Game* state); 

void UpdateTTable(TTable* tt, Zobrist key, int type, int score, int depth, Move mv, const Game* state); 
