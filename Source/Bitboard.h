/**
 * @file Bitboard.h
 * @author Nicholas Hamilton 
 * @date 2023-01-12
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Bitboard and other bit manipulation utilities. 
 */

#pragma once 

#include <stdint.h> 
#include <stdio.h> 

#include "Piece.h"
#include "Square.h" 
#include "Types.h" 

typedef U64 Bitboard; 

#define NoFileA  0xFEFEFEFEFEFEFEFEULL 
#define NoFileH  0x7F7F7F7F7F7F7F7FULL 
#define NoFileAB 0xFCFCFCFCFCFCFCFCULL 
#define NoFileGH 0x3F3F3F3F3F3F3F3FULL 

#define Rank1    0x00000000000000FFULL 
#define Rank2    0x000000000000FF00ULL 
#define Rank7    0x00FF000000000000ULL 
#define Rank8    0xFF00000000000000ULL 

#define NoRank1  0xFFFFFFFFFFFFFF00ULL 
#define NoRank8  0x00FFFFFFFFFFFFFFULL 

#define EmptyWK  0x0000000000000060ULL 
#define EmptyWQ  0x000000000000000EULL 
#define EmptyBK  0x6000000000000000ULL 
#define EmptyBQ  0x0E00000000000000ULL 

#define AllBits  0xFFFFFFFFFFFFFFFFULL 
#define NoBits   0ULL

/**
 * Loops over all 1 bits of a bitboard. 
 * 
 * Defines `remain` bitboard and the current square `sq`. 
 * 
 * @param board Bitboard to loop over 
 * @param action Action(s) to perform for each 1 bit
 */
#define FOR_EACH_BIT(board, action) \
    do { \
        Bitboard remain = (board); \
        Square sq; \
        while (remain) \
        { \
            sq = LeastSigBit(remain); \
            remain &= InverseBits[sq]; \
            action; \
        } \
    } while (0); 

/**
 * Used for pins against the king. 
 */
typedef enum CheckDirection 
{
    CheckDirectionN = 0, 
    CheckDirectionS, 
    CheckDirectionE, 
    CheckDirectionW, 
    CheckDirectionRookEnd = CheckDirectionW, 
    CheckDirectionNE, 
    CheckDirectionNW, 
    CheckDirectionSE, 
    CheckDirectionSW, 
    NumCheckDirections, 

    CheckPieceN = NumCheckDirections, 
    CheckPieceP, 
    NumChecks
} CheckDirection;

/**
 * All possible squares a king can move to. 
 */
extern const Bitboard MovesK[NumSquares]; 

/**
 * All possible squares a knight can move to. 
 */
extern const Bitboard MovesN[NumSquares]; 

/**
 * All possible squares a pawn can attack. 
 */
extern const Bitboard AttacksP[NumColors][NumSquares]; 

/**
 * All squares moved to along a sliding movement. 
 */
extern const Bitboard SlideTo[NumSquares][NumSquares]; 

/**
 * Check direction for pins. Usage: PinIndex[kingSquare][pieceSquare]
 */
extern const U8 PinIndex[NumSquares][NumSquares]; 

/**
 * Population count of all 16-bit integers. 
 */
extern const U8 PopCount16[1 << 16]; 

/**
 * Used for flipping bitboards. 
 */
extern const U64 Reverse8Offset[8][256]; 

/**
 * Highlights all squares with the file number provided. 
 */
static const Bitboard Files[8] = 
{
    0x0101010101010101ULL, 
    0x0202020202020202ULL, 
    0x0404040404040404ULL, 
    0x0808080808080808ULL, 
    0x1010101010101010ULL, 
    0x2020202020202020ULL, 
    0x4040404040404040ULL, 
    0x8080808080808080ULL
};

/**
 * Highlights all squares with the files neighboring the one provided. 
 */
static const Bitboard AdjFiles[8] = 
{
    0x0202020202020202ULL, 
    0x0505050505050505ULL, 
    0x0A0A0A0A0A0A0A0AULL, 
    0x1414141414141414ULL, 
    0x2828282828282828ULL, 
    0x5050505050505050ULL, 
    0xA0A0A0A0A0A0A0A0ULL, 
    0x4040404040404040ULL 
};

/**
 * Highlights all squares with the file and files neighboring the one provided. 
 */
static const Bitboard SameAdjFiles[8] = 
{
    0x0303030303030303ULL, 
    0x0707070707070707ULL, 
    0x0E0E0E0E0E0E0E0EULL, 
    0x1C1C1C1C1C1C1C1CULL, 
    0x3838383838383838ULL, 
    0x7070707070707070ULL, 
    0xE0E0E0E0E0E0E0E0ULL, 
    0xC0C0C0C0C0C0C0C0ULL 
};

/**
 * Highlights all squares with the rank number provided. 
 */
static const Bitboard Ranks[8] = 
{
    0x00000000000000FFULL, 
    0x000000000000FF00ULL, 
    0x0000000000FF0000ULL, 
    0x00000000FF000000ULL, 
    0x000000FF00000000ULL, 
    0x0000FF0000000000ULL, 
    0x00FF000000000000ULL, 
    0xFF00000000000000ULL
};

/**
 * Highlights all squares with the ranks neighboring the one provided. 
 */
static const Bitboard AdjRanks[8] = 
{
    0x000000000000FF00ULL, 
    0x0000000000FF00FFULL, 
    0x00000000FF00FF00ULL, 
    0x000000FF00FF0000ULL, 
    0x0000FF00FF000000ULL, 
    0x00FF00FF00000000ULL, 
    0xFF00FF0000000000ULL,
    0x00FF000000000000ULL
};

/**
 * Highlights all squares with the rank and ranks neighboring the one provided. 
 */
static const Bitboard SameAdjRanks[8] = 
{
    0x000000000000FFFFULL, 
    0x0000000000FFFFFFULL, 
    0x00000000FFFFFF00ULL, 
    0x000000FFFFFF0000ULL, 
    0x0000FFFFFF000000ULL, 
    0x00FFFFFF00000000ULL, 
    0xFFFFFF0000000000ULL,
    0xFFFF000000000000ULL
};

/**
 * Highlights all bits except for the square provided. 
 * Supports `NoSquare`.
 */
static const Bitboard InverseBits[65] = 
{
    ~(1ULL << 0),  ~(1ULL << 1),  ~(1ULL << 2),  ~(1ULL << 3),  ~(1ULL << 4),  ~(1ULL << 5),  ~(1ULL << 6),  ~(1ULL << 7), 
    ~(1ULL << 8),  ~(1ULL << 9),  ~(1ULL << 10), ~(1ULL << 11), ~(1ULL << 12), ~(1ULL << 13), ~(1ULL << 14), ~(1ULL << 15), 
    ~(1ULL << 16), ~(1ULL << 17), ~(1ULL << 18), ~(1ULL << 19), ~(1ULL << 20), ~(1ULL << 21), ~(1ULL << 22), ~(1ULL << 23), 
    ~(1ULL << 24), ~(1ULL << 25), ~(1ULL << 26), ~(1ULL << 27), ~(1ULL << 28), ~(1ULL << 29), ~(1ULL << 30), ~(1ULL << 31), 
    ~(1ULL << 32), ~(1ULL << 33), ~(1ULL << 34), ~(1ULL << 35), ~(1ULL << 36), ~(1ULL << 37), ~(1ULL << 38), ~(1ULL << 39), 
    ~(1ULL << 40), ~(1ULL << 41), ~(1ULL << 42), ~(1ULL << 43), ~(1ULL << 44), ~(1ULL << 45), ~(1ULL << 46), ~(1ULL << 47), 
    ~(1ULL << 48), ~(1ULL << 49), ~(1ULL << 50), ~(1ULL << 51), ~(1ULL << 52), ~(1ULL << 53), ~(1ULL << 54), ~(1ULL << 55), 
    ~(1ULL << 56), ~(1ULL << 57), ~(1ULL << 58), ~(1ULL << 59), ~(1ULL << 60), ~(1ULL << 61), ~(1ULL << 62), ~(1ULL << 63), 
    ~(0ULL)
};

/**
 * Highlights the bit for the square provided. 
 * Supports `NoSquare`. 
 */
static const Bitboard Bits[65] = 
{
    (1ULL << 0),  (1ULL << 1),  (1ULL << 2),  (1ULL << 3),  (1ULL << 4),  (1ULL << 5),  (1ULL << 6),  (1ULL << 7), 
    (1ULL << 8),  (1ULL << 9),  (1ULL << 10), (1ULL << 11), (1ULL << 12), (1ULL << 13), (1ULL << 14), (1ULL << 15), 
    (1ULL << 16), (1ULL << 17), (1ULL << 18), (1ULL << 19), (1ULL << 20), (1ULL << 21), (1ULL << 22), (1ULL << 23), 
    (1ULL << 24), (1ULL << 25), (1ULL << 26), (1ULL << 27), (1ULL << 28), (1ULL << 29), (1ULL << 30), (1ULL << 31), 
    (1ULL << 32), (1ULL << 33), (1ULL << 34), (1ULL << 35), (1ULL << 36), (1ULL << 37), (1ULL << 38), (1ULL << 39), 
    (1ULL << 40), (1ULL << 41), (1ULL << 42), (1ULL << 43), (1ULL << 44), (1ULL << 45), (1ULL << 46), (1ULL << 47), 
    (1ULL << 48), (1ULL << 49), (1ULL << 50), (1ULL << 51), (1ULL << 52), (1ULL << 53), (1ULL << 54), (1ULL << 55), 
    (1ULL << 56), (1ULL << 57), (1ULL << 58), (1ULL << 59), (1ULL << 60), (1ULL << 61), (1ULL << 62), (1ULL << 63), 
    (0ULL)
};

/**
 * Flips all ranks of a bitboard. 
 * 
 * @param b Bitboard to flip
 * @return Bitboard with mirrored ranks
 */
static inline Bitboard FlipRows(Bitboard b) 
{
    b = (b & 0xFFFF0000FFFF0000ULL) >> 16 | (b & 0x0000FFFF0000FFFFULL) << 16; 
    b = (b & 0xFF00FF00FF00FF00ULL) >>  8 | (b & 0x00FF00FF00FF00FFULL) <<  8; 
    return b >> 32 | b << 32; 
}

/**
 * Swap all bytes of a U64. 
 * 
 * @param b The value to swap
 * @return Swapped bytes
 */
static inline Bitboard BSwap(U64 b) 
{
    return FlipRows((Bitboard) b); 
}

/**
 * Flips all files of a bitboard. 
 * 
 * @param b Bitboard to flip
 * @return Bitboard with mirrored files
 */
static inline Bitboard FlipColumns(Bitboard b) 
{
    b = (b & 0xF0F0F0F0F0F0F0F0ULL) >> 4 | (b & 0x0F0F0F0F0F0F0F0FULL) << 4; 
    b = (b & 0xCCCCCCCCCCCCCCCCULL) >> 2 | (b & 0x3333333333333333ULL) << 2; 
    return (b & 0xAAAAAAAAAAAAAAAAULL) >> 1 | (b & 0x5555555555555555ULL) << 1; 
}

/**
 * Rotates a bitboard 180 degrees. 
 * This is equivalent to reversing the order of all bits. 
 * 
 * @param b Bitboard to rotate
 * @return Rotated bitboard 
 */
static inline Bitboard Rotate180(Bitboard b) 
{
    return Reverse8Offset[7][((b) & 255)] | 
           Reverse8Offset[6][((b >>  8) & 255)] | 
           Reverse8Offset[5][((b >> 16) & 255)] | 
           Reverse8Offset[4][((b >> 24) & 255)] | 
           Reverse8Offset[3][((b >> 32) & 255)] | 
           Reverse8Offset[2][((b >> 40) & 255)] | 
           Reverse8Offset[1][((b >> 48) & 255)] | 
           Reverse8Offset[0][((b >> 56))];
}

// see: https://www.chessprogramming.org/BitScan 
// finds the least significant bit of a nonzero value
// note that this will not work with a value of 0

/**
 * Finds the least significant bit of a nonzero value. 
 * 
 * Note: the value must be nonzero or the result is undefined. 
 * 
 * @param b Bitboard 
 * @return Least significant bit index
 */
static inline int LeastSigBit(Bitboard b) 
{
#if defined(BMI2) 
    return __builtin_ctzll(b); 
#else 
    // pre-calculated hash results  
    static const int Positions[] = 
    {
        63,  0,  1, 28,  2,  6, 29, 54, 
         3, 18, 13,  7, 50, 30, 21, 55, 
        26,  4, 16, 19, 14, 42,  8, 44, 
        51, 10, 39, 31, 46, 22, 34, 56, 
        62, 27,  5, 53, 17, 12, 49, 20, 
        25, 15, 41, 43,  9, 38, 45, 33, 
        61, 52, 11, 48, 24, 40, 37, 32, 
        60, 47, 23, 36, 59, 35, 58, 57 
    };

    // b & -b isolates the least significant bit: 
    // -b first flips all bits, so the bits before the LSB are all 1. 
    // After flipping all bits, 1 is added to the result to make 
    // 2's complement -b. This reverts all bits before LSB back to 0, 
    // and the LSB bit to 1. b & -b therefore produces a number with 
    // a single 1 bit as all bits before LSB must be zero and all bits 
    // after are flipped so they are zero as well. 
    
    // multiply (b & -b) that by a pre-generated constant to perfect 
    // hash all possible nonzero values to unique 6-bit indices in a 
    // lookup table
    return Positions[(b & -b) * 0x45949D0DED5CC7EULL >> 58]; 
#endif
}

/**
 * Finds the most significant bit of a nonzero value. 
 * 
 * Note: the value must be nonzero or the result is undefined. 
 * 
 * @param b Bitboard 
 * @return Most significant bit index 
 */
static inline int MostSigBit(Bitboard b) 
{
    return 63 - LeastSigBit(Rotate180(b)); 
}

/**
 * Get total number of 1 bits. 
 * 
 * @param b Bitboard 
 * @return Population count
 */
static inline int PopCount(Bitboard b) 
{
#if defined(POPCNT) 
    return __builtin_popcountll(b);
#else 
    return PopCount16[((b >>  0) & 65535)]
         + PopCount16[((b >> 16) & 65535)]
         + PopCount16[((b >> 32) & 65535)]
         + PopCount16[((b >> 48) & 65535)];
#endif
}

/**
 * Population count of a U16. 
 * 
 * @param b Value
 * @return Population count 
 */
static inline int PopCountU16(U16 b) 
{
    return PopCount16[b]; 
}

/**
 * Move all bits up (toward black) twice. 
 * 
 * @param b Bitboard
 * @return Shifted bitboard
 */
static inline Bitboard ShiftNN(Bitboard b) 
{
    return b << 16; 
}

/**
 * Move all bits down (toward white) twice. 
 * 
 * @param b Bitboard
 * @return Shifted bitboard
 */
static inline Bitboard ShiftSS(Bitboard b) 
{
    return b >> 16; 
}

/**
 * Move all bits up (toward black). 
 * 
 * @param b Bitboard
 * @return Shifted bitboard
 */
static inline Bitboard ShiftN(Bitboard b) 
{
    return b << 8; 
}

/**
 * Move all bits down (toward white) twice. 
 * 
 * @param b Bitboard
 * @return Shifted bitboard
 */
static inline Bitboard ShiftS(Bitboard b) 
{
    return b >> 8; 
}

/**
 * Move all bits right. 
 * 
 * @param b Bitboard
 * @return Shifted bitboard
 */
static inline Bitboard ShiftE(Bitboard b) 
{
    return (b << 1) & NoFileA; 
}

/**
 * Move all bits left. 
 * 
 * @param b Bitboard
 * @return Shifted bitboard
 */
static inline Bitboard ShiftW(Bitboard b) 
{
    return (b >> 1) & NoFileH; 
}

/**
 * Move all bits up and right. 
 * 
 * @param b Bitboard
 * @return Shifted bitboard
 */
static inline Bitboard ShiftNE(Bitboard b) 
{
    return (b << 9) & NoFileA; 
}

/**
 * Move all bits up and left. 
 * 
 * @param b Bitboard
 * @return Shifted bitboard
 */
static inline Bitboard ShiftNW(Bitboard b) 
{
    return (b << 7) & NoFileH; 
}

/**
 * Move all bits down and right. 
 * 
 * @param b Bitboard
 * @return Shifted bitboard
 */
static inline Bitboard ShiftSE(Bitboard b) 
{
    return (b >> 7) & NoFileA; 
}

/**
 * Move all bits down and left. 
 * 
 * @param b Bitboard
 * @return Shifted bitboard
 */
static inline Bitboard ShiftSW(Bitboard b) 
{
    return (b >> 9) & NoFileH; 
}

/**
 * Gets the value of one square. 
 * 
 * @param b Bitboard
 * @param sq The requested square
 * @return Value of the square
 */
static inline int GetBit(Bitboard b, Square sq) 
{
    return (b >> sq) & 1; 
}

/**
 * Sets the value of one square to 1. 
 * 
 * @param b Bitboard
 * @param sq The requested square
 * @return New bitboard with `sq` enabled
 */
static inline Bitboard SetBit(Bitboard b, Square sq) 
{
    return b | (1ULL << sq); 
}

/**
 * Sets the value of one square to 0. 
 * 
 * @param b Bitboard
 * @param sq The requested square
 * @return New bitboard with `sq` disabled
 */
static inline Bitboard ClearBit(Bitboard b, Square sq) 
{
    return b & ~(1ULL << sq); 
}

/**
 * Writes a formatted bitboard to stdout. 
 * 
 * @param b Bitboard to print
 */
static void PrintBits(Bitboard b) 
{
    b = FlipRows(b); 
    printf("+---------------+\n"); 
    for (int i = 0; i < 64; i++) 
    {
        printf("%s%s%s", i % 8 == 0 ? "|" : "", ((unsigned) (b >> i) & 1) ? "1" : ".", (i + 1) % 8 == 0 ? "|\n" : " "); 
    }
    printf("+---------------+\n"); 
}

/**
 * Writes a bitboard as a string of 0s and 1s to stdout. 
 * 
 * @param b Bitboard to print
 */
static void PrintBitsBin(Bitboard b) 
{
    for (int i = 0; i < 64; i++) 
    {
        printf("%u", (unsigned) (b >> (63 - i)) & 1); 
    }
    printf("\n"); 
}
