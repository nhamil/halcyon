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

typedef uint64_t move; 

static inline move move_make(square from, square to, piece pc, piece promote) 
{
    return from | to << 6 | pc << 12 | (uint64_t) promote << 32; 
}

static inline move move_make_allow_en_passant(square from, square to, piece pc, square ep) 
{
    return move_make(from, to, pc, pc) | (uint64_t) ep << 26; 
}

static inline move move_make_take_en_passant(square from, square to, piece pc, int take) 
{
    return move_make(from, to, pc, pc) | (uint64_t) take << 36; 
}

static inline move move_make_castle(square from, square to, piece pc, int castle_index) 
{
    return move_make(from, to, pc, pc) | (uint64_t) castle_index << 37; 
}

static inline square move_get_from_square(move m) 
{
    return m & 63; 
}

static inline square move_get_to_square(move m) 
{
    return (m >> 6) & 63; 
}

static inline piece move_get_from_piece(move m) 
{
    return (m >> 12) & 15; 
}

static inline piece move_get_to_piece(move m) 
{
    return (m >> 16) & 15; 
}

static inline int move_get_castle(move m) 
{
    return (m >> 37) & 7; 
}

static inline square move_get_en_passant_square(move m) 
{
    return (m >> 26) & 63; 
}

static inline bool move_takes_en_passant(move m) 
{
    return (m >> 36) & 1; 
}

static inline piece move_get_promotion_piece(move m) 
{
    return (m >> 32) & 15; 
}

static inline bool move_has_promotion(move m) 
{
    return move_get_from_piece(m) != move_get_promotion_piece(m); 
}

static void move_print(move m) 
{
    if (move_has_promotion(m)) 
    {
        printf("%16lx %s%s%s\n", m, square_string(move_get_from_square(m)), square_string(move_get_to_square(m)), piece_string_colorless(move_get_promotion_piece(m))); 
    }
    else 
    {
        printf("%16lx %s%s\n", m, square_string(move_get_from_square(m)), square_string(move_get_to_square(m))); 
    }
}

static void move_print_end(move m, const char *end) 
{
    if (move_has_promotion(m)) 
    {
        printf("%s%s%s%s", square_string(move_get_from_square(m)), square_string(move_get_to_square(m)), piece_string_colorless(move_get_promotion_piece(m)), end); 
    }
    else 
    {
        printf("%s%s%s", square_string(move_get_from_square(m)), square_string(move_get_to_square(m)), end); 
    }
}

static int move_snprintf(move m, char *out, size_t n) 
{
    if (move_has_promotion(m)) 
    {
        return snprintf(out, n, "%s%s%s", square_string(move_get_from_square(m)), square_string(move_get_to_square(m)), piece_string_colorless(move_get_promotion_piece(m))); 
    }
    else 
    {
       return snprintf(out, n, "%s%s", square_string(move_get_from_square(m)), square_string(move_get_to_square(m))); 
    }
}
