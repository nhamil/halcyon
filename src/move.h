#pragma once 

#include <stdbool.h> 
#include <stdint.h> 
#include <stdio.h> 

#include "piece.h" 
#include "square.h" 

#define MOVE_CASTLE_NONE 0 
#define MOVE_CASTLE_WK 1 
#define MOVE_CASTLE_WQ 2 
#define MOVE_CASTLE_BK 3 
#define MOVE_CASTLE_BQ 4

// from:0-5 to:6-11 piece:12-15 promote:16-19 ep:20-25 take_ep:26 castle:27-29
typedef uint32_t move; 

static inline move make_move(square from, square to, piece pc, piece promote) 
{
    return from | to << 6 | pc << 12 | promote << 16; 
}

static inline move make_move_allow_ep(square from, square to, piece pc, square ep) 
{
    return make_move(from, to, pc, pc) | ep << 20; 
}

static inline move make_move_ep(square from, square to, piece pc, bool take) 
{
    return make_move(from, to, pc, pc) | take << 26; 
}

static inline move make_move_castle(square from, square to, piece pc, int castle_index) 
{
    return make_move(from, to, pc, pc) | castle_index << 27; 
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

static inline int castle_idx(move m) 
{
    return (m >> 27) & 7; 
}

static inline square ep_sq(move m) 
{
    return (m >> 20) & 63; 
}

static inline bool takes_ep(move m) 
{
    return (m >> 26) & 1; 
}

static inline piece pro_pc(move m) 
{
    return (m >> 16) & 15; 
}

static inline bool has_pro(move m) 
{
    return from_pc(m) != pro_pc(m); 
}

static void print_move(move m) 
{
    if (has_pro(m)) 
    {
        printf("%16x %s%s%s\n", m, str_sq(from_sq(m)), str_sq(to_sq(m)), str_pc_no_col(pro_pc(m))); 
    }
    else 
    {
        printf("%16x %s%s\n", m, str_sq(from_sq(m)), str_sq(to_sq(m))); 
    }
}

static void print_move_end(move m, const char *end) 
{
    if (has_pro(m)) 
    {
        printf("%s%s%s%s", str_sq(from_sq(m)), str_sq(to_sq(m)), str_pc_no_col(pro_pc(m)), end); 
    }
    else 
    {
        printf("%s%s%s", str_sq(from_sq(m)), str_sq(to_sq(m)), end); 
    }
}

static int snprintf_move(move m, char *out, size_t n) 
{
    if (has_pro(m)) 
    {
        return snprintf(out, n, "%s%s%s", str_sq(from_sq(m)), str_sq(to_sq(m)), str_pc_no_col(pro_pc(m))); 
    }
    else 
    {
       return snprintf(out, n, "%s%s", str_sq(from_sq(m)), str_sq(to_sq(m))); 
    }
}
