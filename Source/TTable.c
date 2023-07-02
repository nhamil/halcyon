/**
 * @file TTable.c
 * @author Nicholas Hamilton 
 * @date 2023-07-01
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Implements transposition table. 
 */

#include "TTable.h" 

#include <stdlib.h> 

void CreateTTable(TTable* tt, U64 sizeInMB) 
{
    U64 entriesInSize = (sizeInMB * 1024 * 1024) / sizeof(TTableEntry); 
    // get highest power of two 
    entriesInSize = 1ULL << Msb(entriesInSize); 
    tt->Size = entriesInSize; 
    tt->Mask = tt->Size - 1; 
    tt->Entries = malloc(tt->Size * sizeof(TTableEntry)); 

    // printf("info string Created TT of size %.2fmb\n", tt->Size * sizeof(TTableEntry) / 1024.0 / 1024.0); 
} 

void DestroyTTable(TTable* tt) 
{
    free(tt->Entries); 
} 

void ResetTTable(TTable* tt) 
{
    tt->Searches = 0; 
    tt->Hits = 0; 
    tt->Collisions = 0; 
    tt->Used = 0; 
    for (U64 i = 0; i < tt->Size; i++) 
    {
        tt->Entries[i].Type = NO_NODE; 
    }
} 

TTableEntry* FindTTableEntry(TTable* tt, Zobrist key, int depth, const Game* state) 
{
    TTableEntry* entry = &tt->Entries[key & tt->Mask]; 

    tt->Searches++; 

    // fail if: 
    // - no position is stored 
    // - positions are not equal 
    // - requested depth is deeper than stored depth 
    if (entry->Type == NO_NODE || entry->Key != key || entry->Depth < depth) 
    {
        return NULL; 
    }
    // if (!EqualsTTableGame(&entry->State, state)) 
    // {
    //     printf("info string query hash is equal but position is not:\n"); 
    //     // PrintMBox(&entry->Mailbox); 
    //     PrintGame(&entry->State); 
    //     printf("vs\n"); 
    //     // PrintMBox(mbox); 
    //     PrintGame(state); 
    //     exit(1); 
    // }
    (void) state; 

    tt->Hits++; 
    return entry; 
} 

void UpdateTTable(TTable* tt, Zobrist key, int type, int score, int depth, const Game* state) 
{
    TTableEntry* entry = &tt->Entries[key & tt->Mask]; 

    if (entry->Type == NO_NODE) 
    {
        tt->Used++; 
    }
    // node exists, is it a different state? 
    else if (entry->Key != key) 
    {
        tt->Collisions++; 
    }
    // key is equal, is there a key collision? 
    (void) state; 
    // else if (!EqualsTTableGame(&entry->State, state)) 
    // {
    //     printf("info string hash is equal but position is not:\n"); 
    //     // PrintMBox(&entry->Mailbox); 
    //     PrintGame(&entry->State); 
    //     printf("vs\n"); 
    //     // PrintMBox(mbox); 
    //     PrintGame(state); 
    //     exit(1); 
    // }

    entry->Key = key; 
    entry->Type = type; 
    entry->Score = score; 
    entry->Depth = depth; 
    // CopyGame(&entry->State, state); 
} 
