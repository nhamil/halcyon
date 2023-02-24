#pragma once 

#include <stdint.h> 
#include <stdio.h> 

#include "piece.h"
#include "square.h" 

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
        bboard remain = (board); \
        square sq = A1 - 1; \
        int next; \
        while ((sq < H8) & (remain != 0)) \
        { \
            next = lsb(remain) + 1; \
            remain >>= next; \
            sq += next; \
            action; \
        } \
    } while (0); 

typedef enum check_dir 
{
    CHECK_DIR_N = 0, 
    CHECK_DIR_S, 
    CHECK_DIR_E, 
    CHECK_DIR_W, 
    CHECK_DIR_R_END = CHECK_DIR_W, 
    CHECK_DIR_NE, 
    CHECK_DIR_NW, 
    CHECK_DIR_SE, 
    CHECK_DIR_SW, 
    CHECK_DIR_CNT, 

    CHECK_PC_N = CHECK_DIR_CNT, 
    CHECK_PC_P, 
    CHECK_CNT
} check_dir;

typedef uint64_t bboard; 

extern const bboard MOVES_K[SQ_CNT]; 
extern const bboard MOVES_N[SQ_CNT]; 
extern const bboard ATTACKS_P[COL_CNT][SQ_CNT]; 

extern const bboard SLIDE_TO[SQ_CNT][SQ_CNT]; 

extern const uint8_t PIN_IDX[SQ_CNT][SQ_CNT]; 

extern const uint8_t POPCNT16[1 << 16]; 
extern const uint64_t REV8OFF[8][256]; 

static inline bboard make_pos(square sq) 
{
    return 1ULL << sq; 
}

static inline bboard rrow(bboard b) 
{
    b = (b & 0xFFFF0000FFFF0000ULL) >> 16 | (b & 0x0000FFFF0000FFFFULL) << 16; 
    b = (b & 0xFF00FF00FF00FF00ULL) >>  8 | (b & 0x00FF00FF00FF00FFULL) <<  8; 
    return b >> 32 | b << 32; 
}

static inline bboard bswap(bboard b) 
{
    return rrow(b); 
}

static inline bboard rcol(bboard b) 
{
    b = (b & 0xF0F0F0F0F0F0F0F0ULL) >> 4 | (b & 0x0F0F0F0F0F0F0F0FULL) << 4; 
    b = (b & 0xCCCCCCCCCCCCCCCCULL) >> 2 | (b & 0x3333333333333333ULL) << 2; 
    return (b & 0xAAAAAAAAAAAAAAAAULL) >> 1 | (b & 0x5555555555555555ULL) << 1; 
}

static inline bboard rev(bboard b) 
{
    // return rrow(rcol(b)); 
    return REV8OFF[7][((b) & 255)] | 
           REV8OFF[6][((b >>  8) & 255)] | 
           REV8OFF[5][((b >> 16) & 255)] | 
           REV8OFF[4][((b >> 24) & 255)] | 
           REV8OFF[3][((b >> 32) & 255)] | 
           REV8OFF[2][((b >> 40) & 255)] | 
           REV8OFF[1][((b >> 48) & 255)] | 
           REV8OFF[0][((b >> 56))];
}

// see: https://www.chessprogramming.org/Subtracting_a_Rook_from_a_Blocking_Piece
// uses the o^(o-2r) trick
// only finds positive rays (squares with values higher than [sq])
static inline bboard cast_pos_ray(square sq, bboard ray, bboard occupants) 
{
    // one bit after current square 
    bboard b2 = make_pos(sq) << 1; 

    // only use bits that the piece can move to on the line 
    bboard relevant_occ = occupants & ray; 

    // Subtract 2*b from the occupants. Since we only kept bits on the line 
    // the very first bit found on the line will be removed and ones will fill 
    // from that bit (exclusive) to the starting square (exclusive). All other 
    // bits will be untouched. So the only bits that have changed are the 
    // squares (including outside of the line) up to and including the first 
    // target found. XOR with the original occupants to get all the bits that 
    // changed and AND with the line so we ignore all changes outside of it. 
    return ((relevant_occ - b2) ^ relevant_occ) & ray; 
}

// see: https://www.chessprogramming.org/Hyperbola_Quintessence
static inline bboard cast_ray(square sq, bboard ray, bboard occupants) 
{
    // do o^(o-2r) trick on reverse board as well (for negative rays) 
    return cast_pos_ray(sq, ray, occupants) | 
           rev(cast_pos_ray(63 - sq, rev(ray), rev(occupants))); 
}

// see: https://www.chessprogramming.org/BitScan 
// finds the least significant bit of a nonzero value
// note that this will not work with a value of 0
static inline int lsb(bboard b) 
{
    // pre-calculated hash results  
    static const int POSITIONS[] = 
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
    return POSITIONS[(b & -b) * 0x45949D0DED5CC7EULL >> 58]; 
}

static inline int msb(bboard b) 
{
    return 63 - lsb(rev(b)); 
}

static inline int popcnt(bboard b) 
{
    return POPCNT16[((b >>  0) & 65535)]
         + POPCNT16[((b >> 16) & 65535)]
         + POPCNT16[((b >> 32) & 65535)]
         + POPCNT16[((b >> 48) & 65535)];
}

static inline int popcnt16(uint16_t b) 
{
    return POPCNT16[b]; 
}

static inline bboard shift_nn(bboard b) 
{
    return b << 16; 
}

static inline bboard shift_ss(bboard b) 
{
    return b >> 16; 
}

static inline bboard shift_n(bboard b) 
{
    return b << 8; 
}

static inline bboard shift_s(bboard b) 
{
    return b >> 8; 
}

static inline bboard shift_e(bboard b) 
{
    return (b << 1) & NO_FILE_A; 
}

static inline bboard shift_w(bboard b) 
{
    return (b >> 1) & NO_FILE_H; 
}

static inline bboard shift_ne(bboard b) 
{
    return (b << 9) & NO_FILE_A; 
}

static inline bboard shift_nw(bboard b) 
{
    return (b << 7) & NO_FILE_H; 
}

static inline bboard shift_se(bboard b) 
{
    return (b >> 7) & NO_FILE_A; 
}

static inline bboard shift_sw(bboard b) 
{
    return (b >> 9) & NO_FILE_H; 
}

static inline int get_bit(bboard b, square sq) 
{
    return (b >> sq) & 1; 
}

static inline bboard set_bit(bboard b, square sq) 
{
    return b | (1ULL << sq); 
}

static inline bboard clear_bit(bboard b, square sq) 
{
    return b & ~(1ULL << sq); 
}

static void print_bits(bboard b) 
{
    b = rrow(b); 
    printf("+---------------+\n"); 
    for (int i = 0; i < 64; i++) 
    {
        printf("%s%s%s", i % 8 == 0 ? "|" : "", ((unsigned) (b >> i) & 1) ? "1" : ".", (i + 1) % 8 == 0 ? "|\n" : " "); 
    }
    printf("+---------------+\n"); 
}

static void print_bits_bin(bboard b) 
{
    for (int i = 0; i < 64; i++) 
    {
        printf("%u", (unsigned) (b >> (63 - i)) & 1); 
    }
    printf("\n"); 
}
