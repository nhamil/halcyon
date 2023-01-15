#pragma once 

#define SQ_NONE 0 
#define SQ_A1 0
#define SQ_B1 1
#define SQ_C1 2
#define SQ_D1 3
#define SQ_E1 4
#define SQ_F1 5
#define SQ_G1 6
#define SQ_H1 7
#define SQ_A2 8
#define SQ_B2 9
#define SQ_C2 10
#define SQ_D2 11
#define SQ_E2 12
#define SQ_F2 13
#define SQ_G2 14
#define SQ_H2 15
#define SQ_A3 16
#define SQ_B3 17
#define SQ_C3 18
#define SQ_D3 19
#define SQ_E3 20
#define SQ_F3 21
#define SQ_G3 22
#define SQ_H3 23
#define SQ_A4 24
#define SQ_B4 25
#define SQ_C4 26
#define SQ_D4 27
#define SQ_E4 28
#define SQ_F4 29
#define SQ_G4 30
#define SQ_H4 31
#define SQ_A5 32
#define SQ_B5 33
#define SQ_C5 34
#define SQ_D5 35
#define SQ_E5 36
#define SQ_F5 37
#define SQ_G5 38
#define SQ_H5 39
#define SQ_A6 40
#define SQ_B6 41
#define SQ_C6 42
#define SQ_D6 43
#define SQ_E6 44
#define SQ_F6 45
#define SQ_G6 46
#define SQ_H6 47
#define SQ_A7 48
#define SQ_B7 49
#define SQ_C7 50
#define SQ_D7 51
#define SQ_E7 52
#define SQ_F7 53
#define SQ_G7 54
#define SQ_H7 55
#define SQ_A8 56
#define SQ_B8 57
#define SQ_C8 58
#define SQ_D8 59
#define SQ_E8 60
#define SQ_F8 61
#define SQ_G8 62
#define SQ_H8 63
#define SQ_CNT 64

typedef int square; 

static inline square sq_make(int file, int rank) 
{
    return file | rank << 3; 
}

static inline int sq_file(square sq) 
{
    return sq & 7; 
}

static inline int sq_rank(square sq) 
{
    return sq >> 3; 
}

static inline int sq_diag(square sq) 
{
    static const int DIAG[] = 
    {
        7, 8, 9, 10, 11, 12, 13, 14, 
        6, 7, 8, 9, 10, 11, 12, 13, 
        5, 6, 7, 8, 9, 10, 11, 12, 
        4, 5, 6, 7, 8, 9, 10, 11, 
        3, 4, 5, 6, 7, 8, 9, 10, 
        2, 3, 4, 5, 6, 7, 8, 9, 
        1, 2, 3, 4, 5, 6, 7, 8, 
        0, 1, 2, 3, 4, 5, 6, 7 
    };
    return DIAG[sq]; 
}

static inline int sq_anti(square sq) 
{
    static const int ANTI[] = 
    {
        0, 1, 2, 3, 4, 5, 6, 7, 
        1, 2, 3, 4, 5, 6, 7, 8, 
        2, 3, 4, 5, 6, 7, 8, 9, 
        3, 4, 5, 6, 7, 8, 9, 10, 
        4, 5, 6, 7, 8, 9, 10, 11, 
        5, 6, 7, 8, 9, 10, 11, 12, 
        6, 7, 8, 9, 10, 11, 12, 13, 
        7, 8, 9, 10, 11, 12, 13, 14
    };
    return ANTI[sq]; 
}

static inline square sq_rrank(square sq) 
{
    return sq_make(sq_file(sq), 7 - sq_rank(sq)); 
}

static inline const char* sq_str(square sq) 
{
    static const char* STR[] = 
    {
        "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", 
        "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2", 
        "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", 
        "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", 
        "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", 
        "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", 
        "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", 
        "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
    };
    return STR[sq]; 
}