#pragma once 

#include <stdint.h> 

typedef struct random_state random_state; 

struct random_state 
{
    uint64_t state; 
};

static void random_init(random_state *r, uint64_t state) 
{
    if (state == 0) 
    {
        state = 0x123456789ABCDEF0ULL; 
    }
    r->state = state; 
}

static inline uint64_t random_next(random_state *r) 
{
    uint64_t x = r->state; 
    x ^= x >> 12; 
    x ^= x << 25; 
    x ^= x >> 27; 
    r->state = x; 
    return x; // * 0x2545F4914F6CDD1DULL; 
}

static inline uint32_t random_next_u32(random_state *r) 
{
    return (uint32_t) random_next(r); 
}

static inline uint32_t random_next_s32(random_state *r) 
{
    uint32_t x = random_next_u32(r); 
    return *(int32_t *) &x; 
}

static inline uint64_t random_next_u64(random_state *r) 
{
    return (random_next(r) & 0xFFFFFFFF) << 32 | (random_next(r) & 0xFFFFFFFF); 
}

static inline uint64_t random_next_s64(random_state *r) 
{
    uint64_t x = random_next_u64(r); 
    return *(int64_t *) &x; 
}

static inline int random_next_int(random_state *r) 
{
    return (int) random_next_s32(r); 
}

static inline long random_next_long(random_state *r) 
{
    return (long) random_next_s64(r); 
}

static inline long long random_next_long_long(random_state *r) 
{
    return (long long) random_next_s64(r); 
}