#pragma once 

#include <stdint.h> 

typedef struct rand_state rand_state; 

struct rand_state 
{
    uint64_t state; 
};

static void init_rand(rand_state *r, uint64_t state) 
{
    if (state == 0) 
    {
        state = 0x123456789ABCDEF0ULL; 
    }
    r->state = state; 
}

static inline uint64_t next_rand(rand_state *r) 
{
    uint64_t x = r->state; 
    x ^= x >> 12; 
    x ^= x << 25; 
    x ^= x >> 27; 
    r->state = x; 
    return x; // * 0x2545F4914F6CDD1DULL; 
}

static inline uint32_t next_rand_u32(rand_state *r) 
{
    return (uint32_t) next_rand(r); 
}

static inline uint32_t next_rand_s32(rand_state *r) 
{
    uint32_t x = next_rand_u32(r); 
    return *(int32_t *) &x; 
}

static inline uint64_t next_rand_u64(rand_state *r) 
{
    return (next_rand(r) & 0xFFFFFFFF) << 32 | (next_rand(r) & 0xFFFFFFFF); 
}

static inline uint64_t next_rand_s64(rand_state *r) 
{
    uint64_t x = next_rand_u64(r); 
    return *(int64_t *) &x; 
}
