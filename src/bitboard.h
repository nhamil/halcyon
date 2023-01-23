#pragma once 

#include <stdint.h> 
#include <stdio.h> 

#include "square.h" 

#define NO_FILE_A  0xFEFEFEFEFEFEFEFEULL 
#define NO_FILE_H  0x7F7F7F7F7F7F7F7FULL 

#define NO_FILE_AB 0xFCFCFCFCFCFCFCFCULL 
#define NO_FILE_GH 0x3F3F3F3F3F3F3F3FULL 

#define RANK_1     0x00000000000000FFULL
#define RANK_8     0xFF00000000000000ULL

#define NO_RANK_1  0xFFFFFFFFFFFFFF00ULL
#define NO_RANK_8  0x00FFFFFFFFFFFFFFULL

#define CHECK_WK  0x0000000000000070ULL
#define CHECK_WQ  0x000000000000001CULL
#define CHECK_BK  0x7000000000000000ULL
#define CHECK_BQ  0x1C00000000000000ULL

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
    CHECK_DIR_NE, 
    CHECK_DIR_NW, 
    CHECK_DIR_SE, 
    CHECK_DIR_SW, 
    CHECK_DIR_CNT, 

    CHECK_PC_N = CHECK_DIR_CNT, 
    CHECK_PC_P, 
    CHECK_CNT
} check_dir;

// how many 1s in a byte
static const int POPCNT_8[] = 
{
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8, 
};

typedef uint64_t bboard; 

static inline bboard make_pos(square sq) 
{
    return 1ULL << sq; 
}

static inline bboard rrow(bboard b) 
{
    b = (b & 0xFFFFFFFF00000000ULL) >> 32 | b << 32; 
    b = (b & 0xFFFF0000FFFF0000ULL) >> 16 | (b & 0x0000FFFF0000FFFFULL) << 16; 
    b = (b & 0xFF00FF00FF00FF00ULL) >>  8 | (b & 0x00FF00FF00FF00FFULL) <<  8; 
    return b; 
}

static inline bboard rcol(bboard b) 
{
    b = (b & 0xF0F0F0F0F0F0F0F0ULL) >> 4 | (b & 0x0F0F0F0F0F0F0F0FULL) << 4; 
    b = (b & 0xCCCCCCCCCCCCCCCCULL) >> 2 | (b & 0x3333333333333333ULL) << 2; 
    b = (b & 0xAAAAAAAAAAAAAAAAULL) >> 1 | (b & 0x5555555555555555ULL) << 1; 
    return b; 
}

static inline bboard rev(bboard b) 
{
    return rrow(rcol(b)); 
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

static inline int popcnt(bboard b) 
{
    return POPCNT_8[b & 255] + 
           POPCNT_8[(b >>  8) & 255] + 
           POPCNT_8[(b >> 16) & 255] + 
           POPCNT_8[(b >> 24) & 255] + 
           POPCNT_8[(b >> 32) & 255] + 
           POPCNT_8[(b >> 40) & 255] + 
           POPCNT_8[(b >> 48) & 255] + 
           POPCNT_8[(b >> 56) & 255];
}

static inline int popcnt_16(uint16_t b) 
{
    return POPCNT_8[b & 255] + 
           POPCNT_8[(b >> 8) & 255];
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

static inline bboard clear_bits(bboard b, square sq) 
{
    return b & ~(1ULL << sq); 
}

static void print_bits(bboard b) 
{
    b = rrow(b); 
    for (int i = 0; i < 64; i++) 
    {
        printf("%s %s", ((unsigned) (b >> i) & 1) ? "1" : ".", (i + 1) % 8 == 0 ? "\n" : ""); 
    }
}

static void print_bits_bin(bboard b) 
{
    for (int i = 0; i < 64; i++) 
    {
        printf("%u", (unsigned) (b >> (63 - i)) & 1); 
    }
    printf("\n"); 
}

static const bboard CASTLE_TARGETS[] = 
{
    NO_BITS, 
    CHECK_WK, 
    CHECK_WQ, 
    CHECK_BK, 
    CHECK_BQ
};

static const bboard CASTLE_KEEP_WR[] = 
{
    0xFFFFFFFFFFFFFFFFULL,
    0xFFFFFFFFFFFFFF7FULL, // remove H1
    0xFFFFFFFFFFFFFFFEULL, // remove A1
    0xFFFFFFFFFFFFFFFFULL,
    0xFFFFFFFFFFFFFFFFULL,
};

static const bboard CASTLE_KEEP_BR[] = 
{
    0xFFFFFFFFFFFFFFFFULL,
    0xFFFFFFFFFFFFFFFFULL,
    0xFFFFFFFFFFFFFFFFULL,
    0x7FFFFFFFFFFFFFFFULL, // remove H8
    0xFEFFFFFFFFFFFFFFULL, // remove A8
};

static const bboard CASTLE_ADD_WR[] = 
{
    0x0000000000000000ULL, 
    0x0000000000000020ULL, // add F1
    0x0000000000000008ULL, // add D1
    0x0000000000000000ULL, 
    0x0000000000000000ULL, 
};

static const bboard CASTLE_ADD_BR[] = 
{
    0x0000000000000000ULL, 
    0x0000000000000000ULL, 
    0x0000000000000000ULL, 
    0x2000000000000000ULL, // add F8
    0x0800000000000000ULL, // add D8
};

static const bboard SLIDE_FILE[] = 
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

static const bboard SLIDE_RANK[] = 
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

static const bboard SLIDE_DIAG[] = 
{
    0x0100000000000000ULL,
    0x0201000000000000ULL,
    0x0402010000000000ULL,
    0x0804020100000000ULL,
    0x1008040201000000ULL,
    0x2010080402010000ULL,
    0x4020100804020100ULL,
    0x8040201008040201ULL,
    0x0080402010080402ULL,
    0x0000804020100804ULL,
    0x0000008040201008ULL,
    0x0000000080402010ULL,
    0x0000000000804020ULL,
    0x0000000000008040ULL,
    0x0000000000000080ULL
};

static const bboard SLIDE_ANTI[] = 
{
    0x0000000000000001ULL,
    0x0000000000000102ULL,
    0x0000000000010204ULL,
    0x0000000001020408ULL,
    0x0000000102040810ULL,
    0x0000010204081020ULL,
    0x0001020408102040ULL,
    0x0102040810204080ULL,
    0x0204081020408000ULL,
    0x0408102040800000ULL,
    0x0810204080000000ULL,
    0x1020408000000000ULL,
    0x2040800000000000ULL,
    0x4080000000000000ULL,
    0x8000000000000000ULL
};

static const bboard MOVES_K[] = 
{
    0x0000000000000302ULL, 
    0x0000000000000705ULL, 
    0x0000000000000E0AULL, 
    0x0000000000001C14ULL, 
    0x0000000000003828ULL, 
    0x0000000000007050ULL, 
    0x000000000000E0A0ULL, 
    0x000000000000C040ULL, 
    0x0000000000030203ULL, 
    0x0000000000070507ULL, 
    0x00000000000E0A0EULL, 
    0x00000000001C141CULL, 
    0x0000000000382838ULL, 
    0x0000000000705070ULL, 
    0x0000000000E0A0E0ULL, 
    0x0000000000C040C0ULL, 
    0x0000000003020300ULL, 
    0x0000000007050700ULL, 
    0x000000000E0A0E00ULL, 
    0x000000001C141C00ULL, 
    0x0000000038283800ULL, 
    0x0000000070507000ULL, 
    0x00000000E0A0E000ULL, 
    0x00000000C040C000ULL, 
    0x0000000302030000ULL, 
    0x0000000705070000ULL, 
    0x0000000E0A0E0000ULL, 
    0x0000001C141C0000ULL, 
    0x0000003828380000ULL, 
    0x0000007050700000ULL, 
    0x000000E0A0E00000ULL, 
    0x000000C040C00000ULL, 
    0x0000030203000000ULL, 
    0x0000070507000000ULL, 
    0x00000E0A0E000000ULL, 
    0x00001C141C000000ULL, 
    0x0000382838000000ULL, 
    0x0000705070000000ULL, 
    0x0000E0A0E0000000ULL, 
    0x0000C040C0000000ULL, 
    0x0003020300000000ULL, 
    0x0007050700000000ULL, 
    0x000E0A0E00000000ULL, 
    0x001C141C00000000ULL, 
    0x0038283800000000ULL, 
    0x0070507000000000ULL, 
    0x00E0A0E000000000ULL, 
    0x00C040C000000000ULL, 
    0x0302030000000000ULL, 
    0x0705070000000000ULL, 
    0x0E0A0E0000000000ULL, 
    0x1C141C0000000000ULL, 
    0x3828380000000000ULL, 
    0x7050700000000000ULL, 
    0xE0A0E00000000000ULL, 
    0xC040C00000000000ULL, 
    0x0203000000000000ULL, 
    0x0507000000000000ULL, 
    0x0A0E000000000000ULL, 
    0x141C000000000000ULL, 
    0x2838000000000000ULL, 
    0x5070000000000000ULL, 
    0xA0E0000000000000ULL, 
    0x40C0000000000000ULL
};

static const bboard MOVES_N[] = 
{
    0x0000000000020400ULL, 
    0x0000000000050800ULL, 
    0x00000000000A1100ULL, 
    0x0000000000142200ULL, 
    0x0000000000284400ULL, 
    0x0000000000508800ULL, 
    0x0000000000A01000ULL, 
    0x0000000000402000ULL, 
    0x0000000002040004ULL, 
    0x0000000005080008ULL, 
    0x000000000A110011ULL, 
    0x0000000014220022ULL, 
    0x0000000028440044ULL, 
    0x0000000050880088ULL, 
    0x00000000A0100010ULL, 
    0x0000000040200020ULL, 
    0x0000000204000402ULL, 
    0x0000000508000805ULL, 
    0x0000000A1100110AULL, 
    0x0000001422002214ULL, 
    0x0000002844004428ULL, 
    0x0000005088008850ULL, 
    0x000000A0100010A0ULL, 
    0x0000004020002040ULL, 
    0x0000020400040200ULL, 
    0x0000050800080500ULL, 
    0x00000A1100110A00ULL, 
    0x0000142200221400ULL, 
    0x0000284400442800ULL, 
    0x0000508800885000ULL, 
    0x0000A0100010A000ULL, 
    0x0000402000204000ULL, 
    0x0002040004020000ULL, 
    0x0005080008050000ULL, 
    0x000A1100110A0000ULL, 
    0x0014220022140000ULL, 
    0x0028440044280000ULL, 
    0x0050880088500000ULL, 
    0x00A0100010A00000ULL, 
    0x0040200020400000ULL, 
    0x0204000402000000ULL, 
    0x0508000805000000ULL, 
    0x0A1100110A000000ULL, 
    0x1422002214000000ULL, 
    0x2844004428000000ULL, 
    0x5088008850000000ULL, 
    0xA0100010A0000000ULL, 
    0x4020002040000000ULL, 
    0x0400040200000000ULL, 
    0x0800080500000000ULL, 
    0x1100110A00000000ULL, 
    0x2200221400000000ULL, 
    0x4400442800000000ULL, 
    0x8800885000000000ULL, 
    0x100010A000000000ULL, 
    0x2000204000000000ULL, 
    0x0004020000000000ULL, 
    0x0008050000000000ULL, 
    0x00110A0000000000ULL, 
    0x0022140000000000ULL, 
    0x0044280000000000ULL, 
    0x0088500000000000ULL, 
    0x0010A00000000000ULL, 
    0x0020400000000000ULL
}; 

static const bboard POS_MOVES_R[] = 
{
    0x01010101010101FEULL,
    0x02020202020202FCULL,
    0x04040404040404F8ULL,
    0x08080808080808F0ULL,
    0x10101010101010E0ULL,
    0x20202020202020C0ULL,
    0x4040404040404080ULL,
    0x8080808080808000ULL,
    0x010101010101FE00ULL,
    0x020202020202FC00ULL,
    0x040404040404F800ULL,
    0x080808080808F000ULL,
    0x101010101010E000ULL,
    0x202020202020C000ULL,
    0x4040404040408000ULL,
    0x8080808080800000ULL,
    0x0101010101FE0000ULL,
    0x0202020202FC0000ULL,
    0x0404040404F80000ULL,
    0x0808080808F00000ULL,
    0x1010101010E00000ULL,
    0x2020202020C00000ULL,
    0x4040404040800000ULL,
    0x8080808080000000ULL,
    0x01010101FE000000ULL,
    0x02020202FC000000ULL,
    0x04040404F8000000ULL,
    0x08080808F0000000ULL,
    0x10101010E0000000ULL,
    0x20202020C0000000ULL,
    0x4040404080000000ULL,
    0x8080808000000000ULL,
    0x010101FE00000000ULL,
    0x020202FC00000000ULL,
    0x040404F800000000ULL,
    0x080808F000000000ULL,
    0x101010E000000000ULL,
    0x202020C000000000ULL,
    0x4040408000000000ULL,
    0x8080800000000000ULL,
    0x0101FE0000000000ULL,
    0x0202FC0000000000ULL,
    0x0404F80000000000ULL,
    0x0808F00000000000ULL,
    0x1010E00000000000ULL,
    0x2020C00000000000ULL,
    0x4040800000000000ULL,
    0x8080000000000000ULL,
    0x01FE000000000000ULL,
    0x02FC000000000000ULL,
    0x04F8000000000000ULL,
    0x08F0000000000000ULL,
    0x10E0000000000000ULL,
    0x20C0000000000000ULL,
    0x4080000000000000ULL,
    0x8000000000000000ULL,
    0xFE00000000000000ULL,
    0xFC00000000000000ULL,
    0xF800000000000000ULL,
    0xF000000000000000ULL,
    0xE000000000000000ULL,
    0xC000000000000000ULL,
    0x8000000000000000ULL,
    0x0000000000000000ULL
};

static const bboard POS_MOVES_B[] = 
{
    0x8040201008040200ULL,
    0x0080402010080500ULL,
    0x0000804020110A00ULL,
    0x0000008041221400ULL,
    0x0000000182442800ULL,
    0x0000010204885000ULL,
    0x000102040810A000ULL,
    0x0102040810204000ULL,
    0x4020100804020000ULL,
    0x8040201008050000ULL,
    0x00804020110A0000ULL,
    0x0000804122140000ULL,
    0x0000018244280000ULL,
    0x0001020488500000ULL,
    0x0102040810A00000ULL,
    0x0204081020400000ULL,
    0x2010080402000000ULL,
    0x4020100805000000ULL,
    0x804020110A000000ULL,
    0x0080412214000000ULL,
    0x0001824428000000ULL,
    0x0102048850000000ULL,
    0x02040810A0000000ULL,
    0x0408102040000000ULL,
    0x1008040200000000ULL,
    0x2010080500000000ULL,
    0x4020110A00000000ULL,
    0x8041221400000000ULL,
    0x0182442800000000ULL,
    0x0204885000000000ULL,
    0x040810A000000000ULL,
    0x0810204000000000ULL,
    0x0804020000000000ULL,
    0x1008050000000000ULL,
    0x20110A0000000000ULL,
    0x4122140000000000ULL,
    0x8244280000000000ULL,
    0x0488500000000000ULL,
    0x0810A00000000000ULL,
    0x1020400000000000ULL,
    0x0402000000000000ULL,
    0x0805000000000000ULL,
    0x110A000000000000ULL,
    0x2214000000000000ULL,
    0x4428000000000000ULL,
    0x8850000000000000ULL,
    0x10A0000000000000ULL,
    0x2040000000000000ULL,
    0x0200000000000000ULL,
    0x0500000000000000ULL,
    0x0A00000000000000ULL,
    0x1400000000000000ULL,
    0x2800000000000000ULL,
    0x5000000000000000ULL,
    0xA000000000000000ULL,
    0x4000000000000000ULL,
    0x0000000000000000ULL,
    0x0000000000000000ULL,
    0x0000000000000000ULL,
    0x0000000000000000ULL,
    0x0000000000000000ULL,
    0x0000000000000000ULL,
    0x0000000000000000ULL,
    0x0000000000000000ULL
};

static const bboard POS_MOVES_Q[] = 
{
    0x81412111090503FEULL,
    0x02824222120A07FCULL,
    0x0404844424150EF8ULL,
    0x08080888492A1CF0ULL,
    0x10101011925438E0ULL,
    0x2020212224A870C0ULL,
    0x404142444850E080ULL,
    0x8182848890A0C000ULL,
    0x412111090503FE00ULL,
    0x824222120A07FC00ULL,
    0x04844424150EF800ULL,
    0x080888492A1CF000ULL,
    0x101011925438E000ULL,
    0x20212224A870C000ULL,
    0x4142444850E08000ULL,
    0x82848890A0C00000ULL,
    0x2111090503FE0000ULL,
    0x4222120A07FC0000ULL,
    0x844424150EF80000ULL,
    0x0888492A1CF00000ULL,
    0x1011925438E00000ULL,
    0x212224A870C00000ULL,
    0x42444850E0800000ULL,
    0x848890A0C0000000ULL,
    0x11090503FE000000ULL,
    0x22120A07FC000000ULL,
    0x4424150EF8000000ULL,
    0x88492A1CF0000000ULL,
    0x11925438E0000000ULL,
    0x2224A870C0000000ULL,
    0x444850E080000000ULL,
    0x8890A0C000000000ULL,
    0x090503FE00000000ULL,
    0x120A07FC00000000ULL,
    0x24150EF800000000ULL,
    0x492A1CF000000000ULL,
    0x925438E000000000ULL,
    0x24A870C000000000ULL,
    0x4850E08000000000ULL,
    0x90A0C00000000000ULL,
    0x0503FE0000000000ULL,
    0x0A07FC0000000000ULL,
    0x150EF80000000000ULL,
    0x2A1CF00000000000ULL,
    0x5438E00000000000ULL,
    0xA870C00000000000ULL,
    0x50E0800000000000ULL,
    0xA0C0000000000000ULL,
    0x03FE000000000000ULL,
    0x07FC000000000000ULL,
    0x0EF8000000000000ULL,
    0x1CF0000000000000ULL,
    0x38E0000000000000ULL,
    0x70C0000000000000ULL,
    0xE080000000000000ULL,
    0xC000000000000000ULL,
    0xFE00000000000000ULL,
    0xFC00000000000000ULL,
    0xF800000000000000ULL,
    0xF000000000000000ULL,
    0xE000000000000000ULL,
    0xC000000000000000ULL,
    0x8000000000000000ULL,
    0x0000000000000000ULL
};

#include "pin_idx.h"
