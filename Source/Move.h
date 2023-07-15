/**
 * @file Move.h
 * @author Nicholas Hamilton 
 * @date 2023-01-12
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Encodes and decodes a single move. 
 */

#pragma once 

#include <stdbool.h> 
#include <stdint.h> 
#include <stdio.h> 

#include "BBoard.h"
#include "Castle.h"
#include "Piece.h" 
#include "Square.h" 

#define NO_MOVE 0

#define MOVE_CASTLE_NONE 0 
#define MOVE_CASTLE_WK 1 
#define MOVE_CASTLE_WQ 2 
#define MOVE_CASTLE_BK 3 
#define MOVE_CASTLE_BQ 4

#define PRO_NONE 0
#define PRO_N 1
#define PRO_B 2 
#define PRO_R 3 
#define PRO_Q 4

static const BBoard MoveCastleBBK[5] = 
{
    0, 
    1ULL << E1 | 1ULL << G1, 
    1ULL << E1 | 1ULL << C1, 
    1ULL << E8 | 1ULL << G8, 
    1ULL << E8 | 1ULL << C8
};

static const BBoard MoveCastleBBR[5] = 
{
    0, 
    1ULL << H1 | 1ULL << F1, 
    1ULL << A1 | 1ULL << D1, 
    1ULL << H8 | 1ULL << F8, 
    1ULL << A8 | 1ULL << D8
};

static const BBoard MoveCastleBBAll[5] = 
{
    0, 
    1ULL << E1 | 1ULL << G1 | 1ULL << H1 | 1ULL << F1, 
    1ULL << E1 | 1ULL << C1 | 1ULL << A1 | 1ULL << D1, 
    1ULL << E8 | 1ULL << G8 | 1ULL << H8 | 1ULL << F8, 
    1ULL << E8 | 1ULL << C8 | 1ULL << A8 | 1ULL << D8
};

static const Square MoveCastleSqK[5][2] = 
{
    { NO_SQ, NO_SQ }, 
    { E1, G1 }, 
    { E1, C1 }, 
    { E8, G8 }, 
    { E8, C8 }
};

static const Square MoveCastlePcK[5] = 
{
    NO_PC, 
    PC_WK, 
    PC_WK, 
    PC_BK, 
    PC_BK
};

static const Square MoveCastleSqR[5][2] = 
{
    { NO_SQ, NO_SQ }, 
    { H1, F1 }, 
    { A1, D1 }, 
    { H8, F8 }, 
    { A8, D8 }
};

static const Square MoveCastlePcR[5] = 
{
    NO_PC, 
    PC_WR, 
    PC_WR, 
    PC_BR, 
    PC_BR
};

static const CastleFlags MoveCastleRm[NUM_SQ] = 
{
    [A1] = CASTLE_WQ, 
    [H1] = CASTLE_WK, 
    [A8] = CASTLE_BQ, 
    [H8] = CASTLE_BK, 
    [E1] = CASTLE_W, 
    [E8] = CASTLE_B, 
};

/**
 * square from: 0-5 
 * square to: 6-11 
 * piece pc: 12-15 
 * piece pro: 16-19 
 * piece tgt: 20-23 
 * bool takesEP: 24-24 
 * unsigned castle: 25-28 
 * bool check: 29-29
 */
typedef U32 Move; 

static inline Move MakeMove(Square from, Square to, Piece pc, Piece tgt, bool check) 
{
    return ((Move) from) | ((Move) to << 6) | ((Move) pc << 12) | ((Move) pc << 16) | ((Move) tgt << 20) | ((Move) check << 29); 
}

static inline Move MakeProMove(Square from, Square to, Piece pc, Piece pro, Piece tgt, bool check) 
{
    return ((Move) from) | ((Move) to << 6) | ((Move) pc << 12) | ((Move) pro << 16) | ((Move) tgt << 20) | ((Move) check << 29); 
}

static inline Move MakeEPMove(Square from, Square to, Piece pc, Piece tgt, bool ep, bool check) 
{
    return ((Move) from) | ((Move) to << 6) | ((Move) pc << 12) | ((Move) pc << 16) | ((Move) tgt << 20) | ((Move) ep << 24) | ((Move) check << 29); 
}

static inline Move MakeCastleMove(Square from, Square to, Piece pc, unsigned idx, bool check) 
{
    return ((Move) from) | ((Move) to << 6) | ((Move) pc << 12) | ((Move) pc << 16) | ((Move) NO_PC << 20) | ((Move) idx << 25) | ((Move) check << 29); 
}

static inline Square FromSq(Move m) 
{
    return (Square) (m & 63); 
}

static inline Square ToSq(Move m) 
{
    return (Square) ((m >> 6) & 63); 
}

static inline Piece FromPc(Move m) 
{
    return (Piece) ((m >> 12) & 15); 
}

static inline Piece ProPc(Move m) 
{
    return (Piece) ((m >> 16) & 15); 
}

static inline Piece TgtPc(Move m) 
{
    return (Piece) ((m >> 20) & 15); 
}

static inline bool IsEP(Move m) 
{
    return (bool) ((m >> 24) & 1); 
}

static inline int CastleIdx(Move m) 
{
    return (int) ((m >> 25) & 15); 
}

static inline bool IsPro(Move m) 
{
    return ProPc(m) != FromPc(m); 
}

static inline bool IsCapture(Move m) 
{
    return TgtPc(m) != NO_PC; 
}

static inline bool IsCheck(Move m) 
{
    return (bool) ((m >> 29) & 1); 
}

static inline bool IsTactical(Move m) 
{
    return IsCapture(m) || IsCheck(m) || IsPro(m); 
}

static inline bool IsQuiet(Move m) 
{
    return !IsCapture(m) && !IsCheck(m) && !IsPro(m); 
}

static void PrintMove(Move m) 
{
    if (IsPro(m)) 
    {
        printf("%s%s%s\n", StrSq(FromSq(m)), StrSq(ToSq(m)), StrPcType(ProPc(m))); 
    }
    else 
    {
        printf("%s%s\n", StrSq(FromSq(m)), StrSq(ToSq(m))); 
    }
}

static void PrintMoveEnd(Move m, const char* end) 
{
    if (IsPro(m)) 
    {
        printf("%s%s%s%s", StrSq(FromSq(m)), StrSq(ToSq(m)), StrPcType(ProPc(m)), end); 
    }
    else 
    {
        printf("%s%s%s", StrSq(FromSq(m)), StrSq(ToSq(m)), end); 
    }
}

static int SNPrintfMove(Move m, char* out, U64 n) 
{
    if (IsPro(m)) 
    {
        return snprintf(out, n, "%s%s%s", StrSq(FromSq(m)), StrSq(ToSq(m)), StrPcType(ProPc(m))); 
    }
    else 
    {
       return snprintf(out, n, "%s%s", StrSq(FromSq(m)), StrSq(ToSq(m))); 
    }
}

static void FilePrintMoveEnd(Move m, const char* end, FILE* out) 
{
    if (IsPro(m)) 
    {
        fprintf(out, "%s%s%s%s", StrSq(FromSq(m)), StrSq(ToSq(m)), StrPcType(ProPc(m)), end); 
    }
    else 
    {
        fprintf(out, "%s%s%s", StrSq(FromSq(m)), StrSq(ToSq(m)), end); 
    }
}
