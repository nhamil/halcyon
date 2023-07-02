/**
 * @file MBox.h
 * @author Nicholas Hamilton 
 * @date 2023-02-20
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Defines mailbox, used for quick piece queries. 
 */

#pragma once 

#include <stdbool.h> 
#include <stdint.h> 
#include <stdio.h> 
#include <string.h> 

#include "Piece.h" 
#include "Square.h"
#include "Types.h" 

typedef struct MBox MBox; 

struct MBox 
{
    U64 Squares[4]; // 16 squares per U64 
};

static inline void InitMBox(MBox* m) 
{
    memset(m, NO_PC << 4 | NO_PC, sizeof(MBox)); 
}

static inline void InitMBoxCopy(MBox* m, const MBox* from) 
{
    *m = *from; 
}

static inline bool EqualsMBox(const MBox* a, const MBox* b) 
{
    if (a->Squares[0] != b->Squares[0]) return false; 
    if (a->Squares[1] != b->Squares[1]) return false; 
    if (a->Squares[2] != b->Squares[2]) return false; 
    if (a->Squares[3] != b->Squares[3]) return false; 
    return true; 
}

static inline Piece GetMBox(const MBox* m, Square sq) 
{
    return (m->Squares[sq / 16] >> (sq & 0xF) * 4) & 0xF; 
}

static inline bool OccMBox(const MBox* m, Square sq) 
{
    return GetMBox(m, sq) != NO_PC; 
}

static inline void SetMBox(MBox* m, Square sq, Piece pc) 
{
    // clear and set 
    m->Squares[sq / 16] &= ~(0xFULL << (sq & 0xF) * 4); 
    m->Squares[sq / 16] |= ((U64) pc) << (sq & 0xF) * 4; 
}

static inline void ClearMBox(MBox* m, Square sq) 
{
    SetMBox(m, sq, NO_PC); 
}

static inline void PrintMBox(const MBox* m) 
{
    for (int rank = 7; rank >= 0; rank--) 
    {
        for (int file = 0; file < 8; file++) 
        {
            Piece pc = GetMBox(m, MakeSq(file, rank)); 
            if (pc != NO_PC) 
            {
                printf("%s ", StrPc(pc)); 
            }
            else 
            {
                printf("  "); 
            }
        }
        printf("\n"); 
    }
}
