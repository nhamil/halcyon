#pragma once 

#include <stdint.h> 

#define COL_W 0 
#define COL_B 1 

#define PC_P 0
#define PC_N 1
#define PC_B 2
#define PC_R 3
#define PC_Q 4
#define PC_K 5
#define PC_WP 0
#define PC_WN 1
#define PC_WB 2
#define PC_WR 3
#define PC_WQ 4
#define PC_WK 5
#define PC_BP 6
#define PC_BN 7
#define PC_BB 8
#define PC_BR 9
#define PC_BQ 10
#define PC_BK 11
#define PC_CNT 12

typedef int color; 
typedef int piece; 

static inline color opp_col(color c) 
{
    return c ^ 1; 
}

static inline color get_col(piece p) 
{
    return p >= 6; 
}

static inline piece get_no_col(piece p) 
{
    return p - (p >= 6) * 6; 
}

static inline piece make_pc(piece colorless, color col) 
{
    return colorless + col * 6; 
}

static inline piece recolor(piece p, color col) 
{
    return make_pc(get_no_col(p), col); 
}

static inline const char *str_pc(piece p) 
{
    static const char *STR[] = 
    {
        "P", "N", "B", "R", "Q", "K", 
        "p", "n", "b", "r", "q", "k"
    };
    return STR[p]; 
}

static inline const char *str_pc_no_col(piece p) 
{
    static const char *STR[] = 
    {
        "p", "n", "b", "r", "q", "k", 
        "p", "n", "b", "r", "q", "k"
    };
    return STR[p]; 
}
