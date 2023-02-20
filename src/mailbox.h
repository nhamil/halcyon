#pragma once 

#include <stdbool.h> 
#include <stdint.h> 
#include <string.h> 

#include "piece.h" 
#include "square.h"

typedef struct mbox mbox; 

struct mbox 
{
    uint64_t squares[4]; // 16 squares per u64 
};

static inline void init_mbox(mbox *m) 
{
    memset(m, NO_PC << 4 | NO_PC, sizeof(mbox)); 
}

static inline piece get_mbox(const mbox *m, square sq) 
{
    return (m->squares[sq / 16] >> (sq & 0xF) * 4) & 0xF; 
}

static inline bool occ_mbox(const mbox *m, square sq) 
{
    return get_mbox(m, sq) != NO_PC; 
}

static inline void set_mbox(mbox *m, square sq, piece pc) 
{
    // clear and set 
    m->squares[sq / 16] &= ~(0xFULL << (sq & 0xF) * 4); 
    m->squares[sq / 16] |= ((uint64_t) pc) << (sq & 0xF) * 4; 
}

static inline void clear_mbox(mbox *m, square sq) 
{
    set_mbox(m, sq, NO_PC); 
}
