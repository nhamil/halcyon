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
    entriesInSize = 1ULL << MostSigBit(entriesInSize); 
    tt->Size = entriesInSize; 
    tt->Mask = tt->Size - 1; 
    tt->Entries = malloc(tt->Size * sizeof(TTableEntry)); 
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
        tt->Entries[i].Type = NoNode; 
    }
} 

TTableEntry* FindTTableEntry(TTable* tt, Zobrist key, const Game* state) 
{
    TTableEntry* entry = &tt->Entries[key & tt->Mask]; 

    tt->Searches++; 

    // fail if: 
    // - no position is stored 
    // - positions are not equal 
    // - requested depth is deeper than stored depth 
    if (entry->Type == NoNode || entry->Key != key)// || entry->Depth < depth) 
    {
        return NULL; 
    }
#ifdef VALIDATION
    if (!EqualsTTableGame(&entry->State, state)) 
    {
        printf("info string query hash is equal but position is not:\n"); 
        // PrintMailbox(&entry->Board); 
        PrintGame(&entry->State); 
        printf("vs\n"); 
        // PrintMailbox(mbox); 
        PrintGame(state); 
        exit(1); 
    }
#else
    (void) state; 
#endif

    tt->Hits++; 
    return entry; 
} 

void UpdateTTable(TTable* tt, Zobrist key, int type, int score, int depth, Move mv, const Game* state) 
{
    TTableEntry* entry = &tt->Entries[key & tt->Mask]; 

    if (entry->Type == NoNode) 
    {
        tt->Used++; 
    }
    // node exists, is it a different state? 
    else if (entry->Key != key) 
    {
        tt->Collisions++; 
    }
    // key is equal, is there a key collision? 
#ifdef VALIDATION
    else if (!EqualsTTableGame(&entry->State, state)) 
    {
        printf("info string hash is equal but position is not:\n"); 
        // PrintMailbox(&entry->Board); 
        PrintGame(&entry->State); 
        printf("vs\n"); 
        // PrintMailbox(mbox); 
        PrintGame(state); 
        exit(1); 
    }
#else
    (void) state; 
#endif

    entry->Key = key; 
    entry->Type = type; 
    entry->Score = score; 
    entry->Depth = depth; 
    entry->Mv = mv; 
#ifdef VALIDATION
    CopyGame(&entry->State, state); 
#endif
} 
