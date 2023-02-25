#include "zobrist.h"

#include <stdbool.h>

#include "castle.h"
#include "move.h" 
#include "random.h" 

bool is_zb_init = false; 

zobrist SQ_PC_HASH[SQ_CNT][PC_CNT + 1]; 
zobrist CASTLE_HASH[CASTLE_ALL + 1]; 
zobrist EP_HASH[8 + 1]; 
zobrist COL_HASH; 

void init_zb(void) 
{
    if (is_zb_init) return; 

    rand_state r; 
    init_rand(&r, ZB_SEED); 

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

void find_print_zb(zobrist hash) 
{
    if (!hash) 
    {
        printf("[none]\n"); 
        return; 
    }

    for (size_t i = 0; i < SQ_CNT; i++) 
    {
        for (size_t j = 0; j < PC_CNT; j++) 
        {
            if (hash == SQ_PC_HASH[i][j]) 
            {
                printf("[%s, %s]\n", str_sq(i), str_pc(j)); 
                return; 
            }
        }
    }

    for (size_t i = 0; i < 8; i++) 
    {
        if (hash == EP_HASH[i]) 
        {
            printf("[ep %d]\n", (int) i); 
            return; 
        }
    }

    for (castle_flags index = 0; index <= CASTLE_ALL; index++) 
    {
        if (hash == CASTLE_HASH[index]) 
        {
            printf("[castle %s]\n", str_castle(index)); 
            return; 
        }
    }

    if (hash == COL_HASH) 
    {
        printf("[color]\n"); 
        return; 
    }

    printf("[unknown "); 
    print_zb_end(hash, "]\n"); 
}