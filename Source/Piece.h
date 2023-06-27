#pragma once 

#include <stdint.h> 

#define COL_W 0 
#define COL_B 1 
#define NUM_COLS 2

#define PC_P 0
#define PC_N 1
#define PC_B 2
#define PC_R 3
#define PC_Q 4
#define PC_K 5
#define NUM_PC_TYPES 6
#define NO_PC_TYPE NUM_PC_TYPES

#define PC_WP 0
#define PC_WN 1
#define PC_WB 2
#define PC_WR 3
#define PC_WQ 4
#define PC_WK 5
#define PC_BP 6
#define PC_BN 7
#define PC_BB 8
#define PC_BR 9
#define PC_BQ 10
#define PC_BK 11
#define NUM_PC 12
#define NO_PC NUM_PC

typedef unsigned Color; 
typedef unsigned Piece; 

static inline int ColSign(Color c) 
{
    return 1 - 2 * c; 
}

static inline Color OppCol(Color c) 
{
    return c ^ 1; 
}

static inline Color GetCol(Piece p) 
{
    return p >= 6; 
}

static inline Piece GetNoCol(Piece p) 
{
    return p - (p >= 6) * 6; 
}

static inline Piece MakePc(Piece colorless, Color col) 
{
    return colorless + col * 6; 
}

static inline Piece Recolor(Piece p, Color col) 
{
    return MakePc(GetNoCol(p), col); 
}

static inline const char* StrPc(Piece p) 
{
    static const char* Str[] = 
    {
        "P", "N", "B", "R", "Q", "K", 
        "p", "n", "b", "r", "q", "k", 
        " ", " ", " ", " "
    };
    return Str[p]; 
}

static inline const char* StrPcType(Piece p) 
{
    static const char* Str[] = 
    {
        "p", "n", "b", "r", "q", "k", 
        "p", "n", "b", "r", "q", "k", 
        " ", " ", " ", " "
    };
    return Str[p]; 
}
