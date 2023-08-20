/**
 * @file Random.h
 * @author Nicholas Hamilton 
 * @date 2023-01-12
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Xorshift pseudorandom number generator. 
 */

#pragma once 

#include <stdint.h> 

#include "Types.h" 

/**
 * Internal state for the generator. 
 */
typedef struct Random Random; 

struct Random 
{
    U64 State; 
};

/**
 * Initializes the generator state with a seed. It does not need to be deinitialized. 
 * 
 * @param r The state 
 * @param seed The seed  
 */
static void InitRandom(Random* r, U64 state) 
{
    if (state == 0) 
    {
        state = 0x123456789ABCDEF0ULL; 
    }
    r->State = state; 
}

/**
 * Gets a random 64-bit value. For higher quality values use `NextU64`. 
 * 
 * @param r The generator
 * @return Random value 
 */
static inline U64 NextRandom(Random* r) 
{
    U64 x = r->State; 
    x ^= x >> 12; 
    x ^= x << 25; 
    x ^= x >> 27; 
    r->State = x; 
    return x; // * 0x2545F4914F6CDD1DULL; 
}

/**
 * Gets a random 32-bit uint. 
 * 
 * @param r The generator
 * @return Random value 
 */
static inline U32 NextU32(Random* r) 
{
    return (U32) NextRandom(r); 
}

/**
 * Gets a random 32-bit int. 
 * 
 * @param r The generator
 * @return Random value 
 */
static inline S32 NextS32(Random* r) 
{
    U32 x = NextU32(r); 
    return *(S32*) &x; 
}

/**
 * Gets a random 64-bit uint. 
 * 
 * @param r The generator
 * @return Random value 
 */
static inline U64 NextU64(Random* r) 
{
    return (NextRandom(r) & 0xFFFFFFFF) << 32 | (NextRandom(r) & 0xFFFFFFFF); 
}

/**
 * Gets a random 64-bit int. 
 * 
 * @param r The generator
 * @return Random value 
 */
static inline S64 NextS64(Random* r) 
{
    U64 x = NextU64(r); 
    return *(S64*) &x; 
}

/**
 * Gets a random int. 
 * 
 * @param r The generator
 * @return Random value 
 */
static inline int NextInt(Random* r) 
{
    return (int) NextS32(r); 
}

/**
 * Gets a random long int. 
 * 
 * @param r The generator
 * @return Random value 
 */
static inline long NextLong(Random* r) 
{
    return (long) NextS64(r); 
}

/**
 * Gets a random long long int. 
 * 
 * @param r The generator
 * @return Random value 
 */
static inline long long NextLLong(Random* r) 
{
    return (long long) NextS64(r); 
}