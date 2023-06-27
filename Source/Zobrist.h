/**
 * @file Zobrist.h
 * @author Nicholas Hamilton 
 * @date 2023-01-26
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Implements Zobrist hashing. 
 */

#pragma once 

#include <inttypes.h>
#include <stdint.h> 
#include <stdio.h> 

#include "Castle.h" 
#include "Piece.h"
#include "Square.h"
#include "Types.h" 

#define ZB_SEED 0xF72B927A3EED2837ULL

typedef U64 Zobrist; 

extern Zobrist SqPcHash[NUM_SQ][NUM_PC + 1]; 
extern Zobrist CastleHash[CASTLE_ALL + 1]; 
extern Zobrist EPHash[8 + 1]; 
extern Zobrist ColHash; 

void InitZb(void); 

static inline Zobrist SqPcZb(Square sq, Piece pc) 
{
    return SqPcHash[sq][pc]; 
}

static inline Zobrist CastleZb(CastleFlags cf) 
{
    return CastleHash[cf]; 
} 

static inline Zobrist EPZb(Square sq) 
{
    return (sq != NO_SQ) * EPHash[GetFile(sq)]; 
}

static inline Zobrist ColZb(void) 
{
    return ColHash; 
} 

static void PrintZb(Zobrist hash) 
{
    printf("%016" PRIx64 "\n", hash); 
}

static void PrintZbEnd(Zobrist hash, const char* end) 
{
    printf("%016" PRIx64 "%s", hash, end); 
}

void FindPrintZb(Zobrist hash); 
