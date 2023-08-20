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

#define ZobristSeed 0xF72B927A3EED2837ULL

typedef U64 Zobrist; 

extern Zobrist SquarePieceHashValues[NumSquares][NumPieces + 1]; 
extern Zobrist CastleHashValues[CastleAll + 1]; 
extern Zobrist EnPassantHashValues[8 + 1]; 
extern Zobrist ColorHashValue; 

/**
 * Must be called once before using hashing. 
 */
void InitHash(void); 

/**
 * @param sq Square a piece is moving to or from
 * @param pc The piece 
 * @return Hash to XOR with
 */
static inline Zobrist HashSquarePiece(Square sq, Piece pc) 
{
    return SquarePieceHashValues[sq][pc]; 
}

/**
 * @param cf Castle flags 
 * @return Hash to XOR with
 */
static inline Zobrist HashCastleFlags(CastleFlags cf) 
{
    return CastleHashValues[cf]; 
} 

/**
 * @param sq En passant square (can be `NoSquare`)
 * @return Hash to XOR with
 */
static inline Zobrist HashEnPassant(Square sq) 
{
    return (sq != NoSquare) * EnPassantHashValues[GetFile(sq)]; 
}

/**
 * @return Hash to XOR with
 */
static inline Zobrist HashColor(void) 
{
    return ColorHashValue; 
} 

/**
 * Print hash to stdout. 
 * 
 * @param hash The hash 
 */
static void PrintHash(Zobrist hash) 
{
    printf("%016" PRIx64 "\n", hash); 
}

/**
 * Print hash to stdout without newline. 
 * 
 * @param hash The hash 
 * @param end Text to write after
 */
static void PrintHashEnd(Zobrist hash, const char* end) 
{
    printf("%016" PRIx64 "%s", hash, end); 
}

/**
 * Write hash to a file without newline. 
 * 
 * @param hash The hash 
 * @param end Text to write after 
 * @param out The file
 */
static void FilePrintHashEnd(Zobrist hash, const char* end, FILE* out) 
{
    fprintf(out, "%016" PRIx64 "%s", hash, end); 
}

/**
 * Attempt to find what causes a hash and print it to stdout. 
 * 
 * @param hash The hash 
 */
void FindPrintHash(Zobrist hash); 
