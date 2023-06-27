#pragma once 

#include <stdbool.h> 
#include <stdint.h> 
#include <string.h> 

#include "Piece.h" 
#include "Square.h"

typedef struct MBox MBox; 

struct MBox 
{
    U64 Squares[4]; // 16 squares per U64 
};

static inline void InitMBox(MBox* m) 
{
    memset(m, NO_PC << 4 | NO_PC, sizeof(MBox)); 
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
