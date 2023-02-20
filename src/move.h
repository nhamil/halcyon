#pragma once 

#include <stdbool.h> 
#include <stdint.h> 
#include <stdio.h> 

#include "bitboard.h"
#include "castle.h"
#include "piece.h" 
#include "square.h" 

#define NO_MOVE 0

#define MOVE_CASTLE_NONE 0 
#define MOVE_CASTLE_WK 1 
#define MOVE_CASTLE_WQ 2 
#define MOVE_CASTLE_BK 3 
#define MOVE_CASTLE_BQ 4

#define PRO_NONE 0
#define PRO_N 1
#define PRO_B 2 
#define PRO_R 3 
#define PRO_Q 4

static const piece PRO_PC[] = 
{
    PC_P, PC_N, PC_B, PC_R, PC_Q
};

static const bboard MOVE_CASTLE_BB_K[5] = 
{
    0, 
    1ULL << E1 | 1ULL << G1, 
    1ULL << E1 | 1ULL << C1, 
    1ULL << E8 | 1ULL << G8, 
    1ULL << E8 | 1ULL << C8
};

static const bboard MOVE_CASTLE_BB_R[5] = 
{
    0, 
    1ULL << H1 | 1ULL << F1, 
    1ULL << A1 | 1ULL << D1, 
    1ULL << H8 | 1ULL << F8, 
    1ULL << A8 | 1ULL << D8
};

static const bboard MOVE_CASTLE_BB_ALL[5] = 
{
    0, 
    1ULL << E1 | 1ULL << G1 | 1ULL << H1 | 1ULL << F1, 
    1ULL << E1 | 1ULL << C1 | 1ULL << A1 | 1ULL << D1, 
    1ULL << E8 | 1ULL << G8 | 1ULL << H8 | 1ULL << F8, 
    1ULL << E8 | 1ULL << C8 | 1ULL << A8 | 1ULL << D8
};

static const square MOVE_CASTLE_SQ_K[5][2] = 
{
    { NO_SQ, NO_SQ }, 
    { E1, G1 }, 
    { E1, C1 }, 
    { E8, G8 }, 
    { E8, C8 }
};

static const square MOVE_CASTLE_PC_K[5] = 
{
    NO_PC, 
    PC_WK, 
    PC_WK, 
    PC_BK, 
    PC_BK
};

static const square MOVE_CASTLE_SQ_R[5][2] = 
{
    { NO_SQ, NO_SQ }, 
    { H1, F1 }, 
    { A1, D1 }, 
    { H8, F8 }, 
    { A8, D8 }
};

static const square MOVE_CASTLE_PC_R[5] = 
{
    NO_PC, 
    PC_WR, 
    PC_WR, 
    PC_BR, 
    PC_BR
};

static const castle_flags MOVE_CASTLE_RM[SQ_CNT] = 
{
    [A1] = CASTLE_WQ, 
    [H1] = CASTLE_WK, 
    [A8] = CASTLE_BQ, 
    [H8] = CASTLE_BK, 
    [E1] = CASTLE_W, 
    [E8] = CASTLE_B, 
};

/**
 * square from: 0-5 
 * square to: 6-11 
 * piece pc: 12-15 
 * piece pro: 16-19 
 * piece tgt: 20-23 
 * bool takes_ep: 24-24 
 * unsigned castle: 25-28 
 * bool check: 29-29
 */
typedef uint32_t move; 

static inline move make_move(square from, square to, piece pc, piece tgt, bool check) 
{
    return ((move) from) | ((move) to << 6) | ((move) pc << 12) | ((move) pc << 16) | ((move) tgt << 20) | ((move) check << 29); 
}

static inline move make_move_pro(square from, square to, piece pc, piece pro, piece tgt, bool check) 
{
    return ((move) from) | ((move) to << 6) | ((move) pc << 12) | ((move) pro << 16) | ((move) tgt << 20) | ((move) check << 29); 
}

static inline move make_move_ep(square from, square to, piece pc, piece tgt, bool ep, bool check) 
{
    return ((move) from) | ((move) to << 6) | ((move) pc << 12) | ((move) pc << 16) | ((move) tgt << 20) | ((move) ep << 24) | ((move) check << 29); 
}

static inline move make_move_castle(square from, square to, piece pc, unsigned idx, bool check) 
{
    return ((move) from) | ((move) to << 6) | ((move) pc << 12) | ((move) pc << 16) | ((move) NO_PC << 20) | ((move) idx << 25) | ((move) check << 29); 
}

static inline square from_sq(move m) 
{
    return (square) (m & 63); 
}

static inline square to_sq(move m) 
{
    return (square) ((m >> 6) & 63); 
}

static inline piece from_pc(move m) 
{
    return (piece) ((m >> 12) & 15); 
}

static inline piece pro_pc(move m) 
{
    return (piece) ((m >> 16) & 15); 
}

static inline piece tgt_pc(move m) 
{
    return (piece) ((m >> 20) & 15); 
}

static inline bool is_ep(move m) 
{
    return (bool) ((m >> 24) & 1); 
}

static inline int castle_idx(move m) 
{
    return (int) ((m >> 25) & 15); 
}

static inline bool is_pro(move m) 
{
    return pro_pc(m) != from_pc(m); 
}

static inline bool is_capture(move m) 
{
    return tgt_pc(m) != NO_PC; 
}

static inline bool is_check(move m) 
{
    return (bool) ((m >> 29) & 1); 
}

static inline bool is_tactical(move m) 
{
    return is_capture(m) || is_check(m) || is_pro(m); 
}

static inline bool is_quiet(move m) 
{
    return !is_capture(m) && !is_check(m) && !is_pro(m); 
}

static void print_move(move m) 
{
    if (is_pro(m)) 
    {
        printf("%s%s%s\n", str_sq(from_sq(m)), str_sq(to_sq(m)), str_pc_no_col(pro_pc(m))); 
    }
    else 
    {
        printf("%s%s\n", str_sq(from_sq(m)), str_sq(to_sq(m))); 
    }
}

static void print_move_end(move m, const char *end) 
{
    if (is_pro(m)) 
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
    if (is_pro(m)) 
    {
        return snprintf(out, n, "%s%s%s", str_sq(from_sq(m)), str_sq(to_sq(m)), str_pc_no_col(pro_pc(m))); 
    }
    else 
    {
       return snprintf(out, n, "%s%s", str_sq(from_sq(m)), str_sq(to_sq(m))); 
    }
}
