/**
 * @file BBoard.h
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

#define NO_FILE_A  0xFEFEFEFEFEFEFEFEULL 
#define NO_FILE_H  0x7F7F7F7F7F7F7F7FULL 

#define NO_FILE_AB 0xFCFCFCFCFCFCFCFCULL 
#define NO_FILE_GH 0x3F3F3F3F3F3F3F3FULL 

#define RANK_1     0x00000000000000FFULL
#define RANK_2     0x000000000000FF00ULL
#define RANK_7     0x00FF000000000000ULL
#define RANK_8     0xFF00000000000000ULL

#define NO_RANK_1  0xFFFFFFFFFFFFFF00ULL
#define NO_RANK_8  0x00FFFFFFFFFFFFFFULL

#define EMPTY_WK   0x0000000000000060ULL 
#define EMPTY_WQ   0x000000000000000EULL 
#define EMPTY_BK   0x6000000000000000ULL 
#define EMPTY_BQ   0x0E00000000000000ULL 

#define ALL_BITS   0xFFFFFFFFFFFFFFFFULL 
#define NO_BITS    0ULL

// defines the bitboard `remain` and the current square of `board`, `sq` for the duration of the loop
#define FOR_EACH_BIT(board, action) do { \
        BBoard remain = (board); \
        Square sq; \
        while (remain) \
        { \
            sq = Lsb(remain); \
            remain &= InvBB[sq]; \
            action; \
        } \
    } while (0); 

#define REPEAT_0(x) 
#define REPEAT_1(x) x; 
#define REPEAT_2(x) REPEAT_1(x) REPEAT_1(x) 
#define REPEAT_3(x) REPEAT_2(x) REPEAT_1(x) 
#define REPEAT_4(x) REPEAT_3(x) REPEAT_1(x) 
#define REPEAT_5(x) REPEAT_4(x) REPEAT_1(x) 
#define REPEAT_6(x) REPEAT_5(x) REPEAT_1(x) 
#define REPEAT_7(x) REPEAT_6(x) REPEAT_1(x) 
#define REPEAT_8(x) REPEAT_7(x) REPEAT_1(x) 
#define REPEAT_9(x) REPEAT_8(x) REPEAT_1(x) 
#define REPEAT_10(x) REPEAT_9(x) REPEAT_1(x) 
#define REPEAT_N(n, x) REPEAT_##n(x) 

#define FOR_EACH_BIT_N(n, board, action) do { \
        BBoard remain = (board); \
        Square sq = A1 - 1; \
        int next; \
        REPEAT_##n( \
        { \
            next = Lsb(remain) + 1; \
            remain >>= next; \
            sq += next; \
            action; \
        }) \
    } while (0); 

typedef enum CheckDir 
{
    CheckDirN = 0, 
    CheckDirS, 
    CheckDirE, 
    CheckDirW, 
    CheckDirRookEnd = CheckDirW, 
    CheckDirNE, 
    CheckDirNW, 
    CheckDirSE, 
    CheckDirSW, 
    NumCheckDirs, 

    CheckPcN = NumCheckDirs, 
    CheckPcP, 
    NumChecks
} CheckDir;

typedef U64 BBoard; 

extern const BBoard MovesK[NUM_SQ]; 
extern const BBoard MovesN[NUM_SQ]; 
extern const BBoard AttacksP[NUM_COLS][NUM_SQ]; 

extern const BBoard SlideTo[NUM_SQ][NUM_SQ]; 

extern const U8 PinIdx[NUM_SQ][NUM_SQ]; 

extern const U8 Popcnt16[1 << 16]; 
extern const U64 Rev8Off[8][256]; 

static const BBoard Files[8] = 
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

static const BBoard AdjFiles[8] = 
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

static const BBoard SameAdjFiles[8] = 
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

static const BBoard Ranks[8] = 
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

static const BBoard AdjRanks[8] = 
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

static const BBoard SameAdjRanks[8] = 
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

static const BBoard InvBB[64] = 
{
    ~(1ULL << 0),  ~(1ULL << 1),  ~(1ULL << 2),  ~(1ULL << 3),  ~(1ULL << 4),  ~(1ULL << 5),  ~(1ULL << 6),  ~(1ULL << 7), 
    ~(1ULL << 8),  ~(1ULL << 9),  ~(1ULL << 10), ~(1ULL << 11), ~(1ULL << 12), ~(1ULL << 13), ~(1ULL << 14), ~(1ULL << 15), 
    ~(1ULL << 16), ~(1ULL << 17), ~(1ULL << 18), ~(1ULL << 19), ~(1ULL << 20), ~(1ULL << 21), ~(1ULL << 22), ~(1ULL << 23), 
    ~(1ULL << 24), ~(1ULL << 25), ~(1ULL << 26), ~(1ULL << 27), ~(1ULL << 28), ~(1ULL << 29), ~(1ULL << 30), ~(1ULL << 31), 
    ~(1ULL << 32), ~(1ULL << 33), ~(1ULL << 34), ~(1ULL << 35), ~(1ULL << 36), ~(1ULL << 37), ~(1ULL << 38), ~(1ULL << 39), 
    ~(1ULL << 40), ~(1ULL << 41), ~(1ULL << 42), ~(1ULL << 43), ~(1ULL << 44), ~(1ULL << 45), ~(1ULL << 46), ~(1ULL << 47), 
    ~(1ULL << 48), ~(1ULL << 49), ~(1ULL << 50), ~(1ULL << 51), ~(1ULL << 52), ~(1ULL << 53), ~(1ULL << 54), ~(1ULL << 55), 
    ~(1ULL << 56), ~(1ULL << 57), ~(1ULL << 58), ~(1ULL << 59), ~(1ULL << 60), ~(1ULL << 61), ~(1ULL << 62), ~(1ULL << 63), 
};

static const BBoard BB[64] = 
{
    (1ULL << 0),  (1ULL << 1),  (1ULL << 2),  (1ULL << 3),  (1ULL << 4),  (1ULL << 5),  (1ULL << 6),  (1ULL << 7), 
    (1ULL << 8),  (1ULL << 9),  (1ULL << 10), (1ULL << 11), (1ULL << 12), (1ULL << 13), (1ULL << 14), (1ULL << 15), 
    (1ULL << 16), (1ULL << 17), (1ULL << 18), (1ULL << 19), (1ULL << 20), (1ULL << 21), (1ULL << 22), (1ULL << 23), 
    (1ULL << 24), (1ULL << 25), (1ULL << 26), (1ULL << 27), (1ULL << 28), (1ULL << 29), (1ULL << 30), (1ULL << 31), 
    (1ULL << 32), (1ULL << 33), (1ULL << 34), (1ULL << 35), (1ULL << 36), (1ULL << 37), (1ULL << 38), (1ULL << 39), 
    (1ULL << 40), (1ULL << 41), (1ULL << 42), (1ULL << 43), (1ULL << 44), (1ULL << 45), (1ULL << 46), (1ULL << 47), 
    (1ULL << 48), (1ULL << 49), (1ULL << 50), (1ULL << 51), (1ULL << 52), (1ULL << 53), (1ULL << 54), (1ULL << 55), 
    (1ULL << 56), (1ULL << 57), (1ULL << 58), (1ULL << 59), (1ULL << 60), (1ULL << 61), (1ULL << 62), (1ULL << 63), 
};

static inline BBoard RRow(BBoard b) 
{
    b = (b & 0xFFFF0000FFFF0000ULL) >> 16 | (b & 0x0000FFFF0000FFFFULL) << 16; 
    b = (b & 0xFF00FF00FF00FF00ULL) >>  8 | (b & 0x00FF00FF00FF00FFULL) <<  8; 
    return b >> 32 | b << 32; 
}

static inline BBoard BSwap(BBoard b) 
{
    return RRow(b); 
}

static inline BBoard RCol(BBoard b) 
{
    b = (b & 0xF0F0F0F0F0F0F0F0ULL) >> 4 | (b & 0x0F0F0F0F0F0F0F0FULL) << 4; 
    b = (b & 0xCCCCCCCCCCCCCCCCULL) >> 2 | (b & 0x3333333333333333ULL) << 2; 
    return (b & 0xAAAAAAAAAAAAAAAAULL) >> 1 | (b & 0x5555555555555555ULL) << 1; 
}

static inline BBoard Rev(BBoard b) 
{
    // return RRow(RCol(b)); 
    return Rev8Off[7][((b) & 255)] | 
           Rev8Off[6][((b >>  8) & 255)] | 
           Rev8Off[5][((b >> 16) & 255)] | 
           Rev8Off[4][((b >> 24) & 255)] | 
           Rev8Off[3][((b >> 32) & 255)] | 
           Rev8Off[2][((b >> 40) & 255)] | 
           Rev8Off[1][((b >> 48) & 255)] | 
           Rev8Off[0][((b >> 56))];
}

// see: https://www.chessprogramming.org/Subtracting_a_Rook_from_a_Blocking_Piece
// uses the o^(o-2r) trick
// only finds positive rays (squares with values higher than [sq])
static inline BBoard CastPosRay(Square sq, BBoard ray, BBoard occupants) 
{
    // one bit after current square 
    BBoard b2 = BB[sq] << 1; 

    // only use bits that the piece can move to on the line 
    BBoard relevantOcc = occupants & ray; 

    // Subtract 2*b from the occupants. Since we only kept bits on the line 
    // the very first bit found on the line will be removed and ones will fill 
    // from that bit (exclusive) to the starting square (exclusive). All other 
    // bits will be untouched. So the only bits that have changed are the 
    // squares (including outside of the line) up to and including the first 
    // target found. XOR with the original occupants to get all the bits that 
    // changed and AND with the line so we ignore all changes outside of it. 
    return ((relevantOcc - b2) ^ relevantOcc) & ray; 
}

// see: https://www.chessprogramming.org/Hyperbola_Quintessence
static inline BBoard CastRay(Square sq, BBoard ray, BBoard occupants) 
{
    // do o^(o-2r) trick on reverse board as well (for negative rays) 
    return CastPosRay(sq, ray, occupants) | 
           Rev(CastPosRay(63 - sq, Rev(ray), Rev(occupants))); 
}

// see: https://www.chessprogramming.org/BitScan 
// finds the least significant bit of a nonzero value
// note that this will not work with a value of 0
static inline int Lsb(BBoard b) 
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

static inline int Msb(BBoard b) 
{
    return 63 - Lsb(Rev(b)); 
}

static inline int Popcnt(BBoard b) 
{
#if defined(POPCNT)
    return __builtin_popcountll(b);
#else 
    return Popcnt16[((b >>  0) & 65535)]
         + Popcnt16[((b >> 16) & 65535)]
         + Popcnt16[((b >> 32) & 65535)]
         + Popcnt16[((b >> 48) & 65535)];
#endif
}

static inline int PopcntShort(U16 b) 
{
    return Popcnt16[b]; 
}

static inline BBoard ShiftNN(BBoard b) 
{
    return b << 16; 
}

static inline BBoard ShiftSS(BBoard b) 
{
    return b >> 16; 
}

static inline BBoard ShiftN(BBoard b) 
{
    return b << 8; 
}

static inline BBoard ShiftS(BBoard b) 
{
    return b >> 8; 
}

static inline BBoard ShiftE(BBoard b) 
{
    return (b << 1) & NO_FILE_A; 
}

static inline BBoard ShiftW(BBoard b) 
{
    return (b >> 1) & NO_FILE_H; 
}

static inline BBoard ShiftNE(BBoard b) 
{
    return (b << 9) & NO_FILE_A; 
}

static inline BBoard ShiftNW(BBoard b) 
{
    return (b << 7) & NO_FILE_H; 
}

static inline BBoard ShiftSE(BBoard b) 
{
    return (b >> 7) & NO_FILE_A; 
}

static inline BBoard ShiftSW(BBoard b) 
{
    return (b >> 9) & NO_FILE_H; 
}

static inline int GetBit(BBoard b, Square sq) 
{
    return (b >> sq) & 1; 
}

static inline BBoard SetBit(BBoard b, Square sq) 
{
    return b | (1ULL << sq); 
}

static inline BBoard ClearBit(BBoard b, Square sq) 
{
    return b & ~(1ULL << sq); 
}

static void PrintBits(BBoard b) 
{
    b = RRow(b); 
    printf("+---------------+\n"); 
    for (int i = 0; i < 64; i++) 
    {
        printf("%s%s%s", i % 8 == 0 ? "|" : "", ((unsigned) (b >> i) & 1) ? "1" : ".", (i + 1) % 8 == 0 ? "|\n" : " "); 
    }
    printf("+---------------+\n"); 
}

static void PrintBitsBin(BBoard b) 
{
    for (int i = 0; i < 64; i++) 
    {
        printf("%u", (unsigned) (b >> (63 - i)) & 1); 
    }
    printf("\n"); 
}
