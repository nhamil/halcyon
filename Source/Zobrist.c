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
            SqPcHash[i][j] = NextRandom(&r); 
        }
    }

    for (U64 i = 0; i < 8; i++) 
    {
        EPHash[i] = NextRandom(&r); 
    }

    Zobrist castle[4]; 
    for (U64 i = 0; i < 4; i++) 
    {
        castle[i] = NextRandom(&r); 
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

    ColHash = NextRandom(&r); 

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