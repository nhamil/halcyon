#pragma once 

#include <stdint.h> 

#define COLOR_W 0 
#define COLOR_B 1 

#define PIECE_P 0
#define PIECE_N 1
#define PIECE_B 2
#define PIECE_R 3
#define PIECE_Q 4
#define PIECE_K 5
#define PIECE_WP 0
#define PIECE_WN 1
#define PIECE_WB 2
#define PIECE_WR 3
#define PIECE_WQ 4
#define PIECE_WK 5
#define PIECE_BP 6
#define PIECE_BN 7
#define PIECE_BB 8
#define PIECE_BR 9
#define PIECE_BQ 10
#define PIECE_BK 11
#define PIECE_COUNT 12

typedef int color; 
typedef int piece; 

static inline color color_get_other(color c) 
{
    return c ^ 1; 
}

static inline color piece_get_color(piece p) 
{
    return p >= 6; 
}

static inline piece piece_get_colorless(piece p) 
{
    return p - (p >= 6) * 6; 
}

static inline piece piece_make_colored(piece colorless, color col) 
{
    return colorless + col * 6; 
}

static inline piece piece_make_recolor(piece p, color col) 
{
    return piece_make_colored(piece_get_colorless(p), col); 
}

static inline const char *piece_string(piece p) 
{
    static const char *STR[] = 
    {
        "P", "N", "B", "R", "Q", "K", 
        "p", "n", "b", "r", "q", "k"
    };
    return STR[p]; 
}

static inline const char *piece_string_colorless(piece p) 
{
    static const char *STR[] = 
    {
        "p", "n", "b", "r", "q", "k", 
        "p", "n", "b", "r", "q", "k"
    };
    return STR[p]; 
}
