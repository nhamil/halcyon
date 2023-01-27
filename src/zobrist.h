#pragma once 

#include <inttypes.h>
#include <stdint.h> 
#include <stdio.h> 

#include "castle.h" 
#include "piece.h"
#include "square.h"

typedef uint64_t zobrist; 

zobrist SQ_PC_HASH[SQ_CNT][PC_CNT]; 
zobrist CASTLE_HASH[CASTLE_ALL + 1]; 
zobrist EP_HASH[8]; 
zobrist COL_HASH; 

void init_zb(void); 

static inline zobrist sq_pc_zb(square sq, piece pc) 
{
    return SQ_PC_HASH[sq][pc]; 
}

static inline zobrist castle_zb(castle_flags cf) 
{
    return CASTLE_HASH[cf]; 
} 

static inline zobrist ep_zb(square sq) 
{
    return (sq != NO_SQ) * EP_HASH[get_file(sq)]; 
}

static inline zobrist col_zb(void) 
{
    return COL_HASH; 
} 

static void print_zb(zobrist hash) 
{
    printf("%016"PRIx64"\n", hash); 
}
