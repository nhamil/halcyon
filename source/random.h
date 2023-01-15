#pragma once 

#include <stdint.h> 

typedef struct rng_state rng_state; 

struct rng_state 
{
    uint64_t state; 
};

static void rng_init(rng_state *r, uint64_t state) 
{
    if (state == 0) 
    {
        state = 0x123456789ABCDEF0ULL; 
    }
    r->state = state; 
}

static inline uint64_t rng_next(rng_state *r) 
{
    uint64_t x = r->state; 
    x ^= x >> 12; 
    x ^= x << 25; 
    x ^= x >> 27; 
    r->state = x; 
    return x; // * 0x2545F4914F6CDD1DULL; 
}

static inline uint32_t rng_next_u32(rng_state *r) 
{
    return (uint32_t) rng_next(r); 
}

static inline uint32_t rng_next_s32(rng_state *r) 
{
    uint32_t x = rng_next_u32(r); 
    return *(int32_t *) &x; 
}

static inline uint64_t rng_next_u64(rng_state *r) 
{
    return (rng_next(r) & 0xFFFFFFFF) << 32 | (rng_next(r) & 0xFFFFFFFF); 
}

static inline uint64_t rng_next_s64(rng_state *r) 
{
    uint64_t x = rng_next_u64(r); 
    return *(int64_t *) &x; 
}

static inline int rng_next_int(rng_state *r) 
{
    return (int) rng_next_s32(r); 
}

static inline long rng_next_long(rng_state *r) 
{
    return (long) rng_next_s64(r); 
}

static inline long long rng_next_long_long(rng_state *r) 
{
    return (long long) rng_next_s64(r); 
}