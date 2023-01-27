#include "zobrist.h"

#include <stdbool.h>

#include "random.h" 

bool is_zb_init = false; 

void init_zb(void) 
{
    if (is_zb_init) return; 

    rand_state r; 
    init_rand(&r, 0xF72B927A3EED2837ULL); 

    for (size_t i = 0; i < SQ_CNT; i++) 
    {
        for (size_t j = 0; j < PC_CNT; j++) 
        {
            SQ_PC_HASH[i][j] = next_rand(&r); 
        }
    }

    for (size_t i = 0; i < 8; i++) 
    {
        EP_HASH[i] = next_rand(&r); 
    }

    zobrist castle[4]; 
    for (size_t i = 0; i < 4; i++) 
    {
        castle[i] = next_rand(&r); 
    }

    for (castle_flags index = 0; index <= CASTLE_ALL; index++) 
    {
        zobrist hash = 0; 
        for (size_t i = 0; i <= 4; i++) 
        {
            if (index & (1 << i)) hash ^= castle[i]; 
        }
        CASTLE_HASH[index] = hash; 
    }

    COL_HASH = next_rand(&r); 

    is_zb_init = true; 
}