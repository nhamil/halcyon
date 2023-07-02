/**
 * @file Zobrist.c
 * @author Nicholas Hamilton 
 * @date 2023-01-26
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Implements Zobrist hashing utilities. 
 */

#include "Zobrist.h"

#include <stdbool.h>

#include "Castle.h"
#include "Move.h" 
#include "Random.h" 

bool s_IsZbInit = false; 

Zobrist SqPcHash[NUM_SQ][NUM_PC + 1]; 
Zobrist CastleHash[CASTLE_ALL + 1]; 
Zobrist EPHash[8 + 1]; 
Zobrist ColHash; 

void InitZb(void) 
{
    if (s_IsZbInit) return; 

    Random r; 
    InitRandom(&r, ZB_SEED); 

    for (U64 i = 0; i < NUM_SQ; i++) 
    {
        for (U64 j = 0; j < NUM_PC; j++) 
        {
            SqPcHash[i][j] = NextU64(&r); 
        }
    }

    for (U64 i = 0; i < 8; i++) 
    {
        EPHash[i] = NextU64(&r); 
    }

    Zobrist castle[4]; 
    for (U64 i = 0; i < 4; i++) 
    {
        castle[i] = NextU64(&r); 
    }

    for (CastleFlags index = 0; index <= CASTLE_ALL; index++) 
    {
        Zobrist hash = 0; 
        for (U64 i = 0; i <= 4; i++) 
        {
            if (index & (1 << i)) hash ^= castle[i]; 
        }
        CastleHash[index] = hash; 
    }

    ColHash = NextU64(&r); 

    s_IsZbInit = true; 
}

void FindPrintZb(Zobrist hash) 
{
    if (!hash) 
    {
        printf("[none]\n"); 
        return; 
    }

    for (U64 i = 0; i < NUM_SQ; i++) 
    {
        for (U64 j = 0; j < NUM_PC; j++) 
        {
            if (hash == SqPcHash[i][j]) 
            {
                printf("[%s, %s]\n", StrSq(i), StrPc(j)); 
                return; 
            }
        }
    }

    for (U64 i = 0; i < 8; i++) 
    {
        if (hash == EPHash[i]) 
        {
            printf("[ep %d]\n", (int) i); 
            return; 
        }
    }

    for (CastleFlags index = 0; index <= CASTLE_ALL; index++) 
    {
        if (hash == CastleHash[index]) 
        {
            printf("[castle %s]\n", StrCastle(index)); 
            return; 
        }
    }

    if (hash == ColHash) 
    {
        printf("[color]\n"); 
        return; 
    }

    printf("[unknown "); 
    PrintZbEnd(hash, "]\n"); 
}