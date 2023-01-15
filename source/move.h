#pragma once 

#include <stdbool.h> 
#include <stdint.h> 
#include <stdio.h> 

#include "piece.h" 
#include "square.h" 

#define CAS_IDX_NONE 0 
#define CAS_IDX_WK 1 
#define CAS_IDX_WQ 2 
#define CAS_IDX_BK 3 
#define CAS_IDX_BQ 4

// from:0-5 to:6-11 pc:12-15 promote:16-19 ep:20-25 take_ep:26 castle:27-29
typedef uint32_t move; 
typedef int cas_idx; 

static inline move make_mv(square from, square to, piece pc, piece pro) 
{
    return from | to << 6 | pc << 12 | pro << 16; 
}

static inline move make_allow_ep_mv(square from, square to, piece pc, square ep) 
{
    return make_mv(from, to, pc, pc) | ep << 20; 
}

static inline move make_ep_mv(square from, square to, piece pc, bool take) 
{
    return make_mv(from, to, pc, pc) | take << 26; 
}

static inline move make_cas_mv(square from, square to, piece pc, cas_idx idx) 
{
    return make_mv(from, to, pc, pc) | idx << 27; 
}

static inline square from_sq(move m) 
{
    return m & 63; 
}

static inline square to_sq(move m) 
{
    return (m >> 6) & 63; 
}

static inline piece from_pc(move m) 
{
    return (m >> 12) & 15; 
}

static inline int cas(move m) 
{
    return (m >> 27) & 7; 
}

static inline square ep_sq(move m) 
{
    return (m >> 20) & 63; 
}

static inline bool is_ep(move m) 
{
    return (m >> 26) & 1; 
}

static inline piece promoted(move m) 
{
    return (m >> 16) & 15; 
}

static inline bool has_pro(move m) 
{
    return from_pc(m) != promoted(m); 
}

static void print_mv(move m) 
{
    if (has_pro(m)) 
    {
        printf("%16x %s%s%s\n", m, sq_str(from_sq(m)), sq_str(to_sq(m)), pc_str_no_col(promoted(m))); 
    }
    else 
    {
        printf("%16x %s%s\n", m, sq_str(from_sq(m)), sq_str(to_sq(m))); 
    }
}

static void print_mv_end(move m, const char *end) 
{
    if (has_pro(m)) 
    {
        printf("%s%s%s%s", sq_str(from_sq(m)), sq_str(to_sq(m)), pc_str_no_col(promoted(m)), end); 
    }
    else 
    {
        printf("%s%s%s", sq_str(from_sq(m)), sq_str(to_sq(m)), end); 
    }
}

static int snprintf_mv(move m, char *out, size_t n) 
{
    if (has_pro(m)) 
    {
        return snprintf(out, n, "%s%s%s", sq_str(from_sq(m)), sq_str(to_sq(m)), pc_str_no_col(promoted(m))); 
    }
    else 
    {
       return snprintf(out, n, "%s%s", sq_str(from_sq(m)), sq_str(to_sq(m))); 
    }
}
