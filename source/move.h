#pragma once 

#include <stdbool.h> 
#include <stdint.h> 
#include <stdio.h> 

#include "piece.h" 
#include "square.h" 

#define MV_CAS_NONE 0 
#define MV_CAS_WK 1 
#define MV_CAS_WQ 2 
#define MV_CAS_BK 3 
#define MV_CAS_BQ 4

// from:0-5 to:6-11 pc:12-15 promote:16-19 ep:20-25 take_ep:26 castle:27-29
typedef uint32_t move; 

static inline move mv_make(square from, square to, piece pc, piece pro) 
{
    return from | to << 6 | pc << 12 | pro << 16; 
}

static inline move mv_make_allow_ep(square from, square to, piece pc, square ep) 
{
    return mv_make(from, to, pc, pc) | ep << 20; 
}

static inline move mv_make_ep(square from, square to, piece pc, bool take) 
{
    return mv_make(from, to, pc, pc) | take << 26; 
}

static inline move mv_make_cas(square from, square to, piece pc, int cas_idx) 
{
    return mv_make(from, to, pc, pc) | cas_idx << 27; 
}

static inline square mv_from_sq(move m) 
{
    return m & 63; 
}

static inline square mv_to_sq(move m) 
{
    return (m >> 6) & 63; 
}

static inline piece mv_from_pc(move m) 
{
    return (m >> 12) & 15; 
}

static inline int mv_cas(move m) 
{
    return (m >> 27) & 7; 
}

static inline square mv_ep_sq(move m) 
{
    return (m >> 20) & 63; 
}

static inline bool mv_ep(move m) 
{
    return (m >> 26) & 1; 
}

static inline piece mv_pro(move m) 
{
    return (m >> 16) & 15; 
}

static inline bool mv_has_pro(move m) 
{
    return mv_from_pc(m) != mv_pro(m); 
}

static void mv_print(move m) 
{
    if (mv_has_pro(m)) 
    {
        printf("%16x %s%s%s\n", m, sq_str(mv_from_sq(m)), sq_str(mv_to_sq(m)), pc_str_no_col(mv_pro(m))); 
    }
    else 
    {
        printf("%16x %s%s\n", m, sq_str(mv_from_sq(m)), sq_str(mv_to_sq(m))); 
    }
}

static void mv_print_end(move m, const char *end) 
{
    if (mv_has_pro(m)) 
    {
        printf("%s%s%s%s", sq_str(mv_from_sq(m)), sq_str(mv_to_sq(m)), pc_str_no_col(mv_pro(m)), end); 
    }
    else 
    {
        printf("%s%s%s", sq_str(mv_from_sq(m)), sq_str(mv_to_sq(m)), end); 
    }
}

static int mv_snprintf(move m, char *out, size_t n) 
{
    if (mv_has_pro(m)) 
    {
        return snprintf(out, n, "%s%s%s", sq_str(mv_from_sq(m)), sq_str(mv_to_sq(m)), pc_str_no_col(mv_pro(m))); 
    }
    else 
    {
       return snprintf(out, n, "%s%s", sq_str(mv_from_sq(m)), sq_str(mv_to_sq(m))); 
    }
}
