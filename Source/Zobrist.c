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

bool IsHashInit = false; 

Zobrist SquarePieceHashValues[NumSquares][NumPieces + 1]; 
Zobrist CastleHashValues[CastleAll + 1]; 
Zobrist EnPassantHashValues[8 + 1]; 
Zobrist ColorHashValue; 

void InitHash(void) 
{
    if (IsHashInit) return; 

    Random r; 
    InitRandom(&r, ZobristSeed); 

    for (U64 i = 0; i < NumSquares; i++) 
    {
        for (U64 j = 0; j < NumPieces; j++) 
        {
            SquarePieceHashValues[i][j] = NextU64(&r); 
        }
    }

    for (U64 i = 0; i < 8; i++) 
    {
        EnPassantHashValues[i] = NextU64(&r); 
    }

    Zobrist castle[4]; 
    for (U64 i = 0; i < 4; i++) 
    {
        castle[i] = NextU64(&r); 
    }

    for (CastleFlags index = 0; index <= CastleAll; index++) 
    {
        Zobrist hash = 0; 
        for (U64 i = 0; i <= 4; i++) 
        {
            if (index & (1 << i)) hash ^= castle[i]; 
        }
        CastleHashValues[index] = hash; 
    }

    ColorHashValue = NextU64(&r); 

    IsHashInit = true; 
}

void FindPrintHash(Zobrist hash) 
{
    if (!hash) 
    {
        printf("[none]\n"); 
        return; 
    }

    for (U64 i = 0; i < NumSquares; i++) 
    {
        for (U64 j = 0; j < NumPieces; j++) 
        {
            if (hash == SquarePieceHashValues[i][j]) 
            {
                printf("[%s, %s]\n", SquareString(i), PieceString(j)); 
                return; 
            }
        }
    }

    for (U64 i = 0; i < 8; i++) 
    {
        if (hash == EnPassantHashValues[i]) 
        {
            printf("[ep %d]\n", (int) i); 
            return; 
        }
    }

    for (CastleFlags index = 0; index <= CastleAll; index++) 
    {
        if (hash == CastleHashValues[index]) 
        {
            printf("[castle %s]\n", CastleString(index)); 
            return; 
        }
    }

    if (hash == ColorHashValue) 
    {
        printf("[color]\n"); 
        return; 
    }

    printf("[unknown "); 
    PrintHashEnd(hash, "]\n"); 
}