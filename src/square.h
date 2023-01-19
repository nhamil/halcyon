#pragma once 

#define NO_SQ 0 
#define A1 0
#define B1 1
#define C1 2
#define D1 3
#define E1 4
#define F1 5
#define G1 6
#define H1 7
#define A2 8
#define B2 9
#define C2 10
#define D2 11
#define E2 12
#define F2 13
#define G2 14
#define H2 15
#define A3 16
#define B3 17
#define C3 18
#define D3 19
#define E3 20
#define F3 21
#define G3 22
#define H3 23
#define A4 24
#define B4 25
#define C4 26
#define D4 27
#define E4 28
#define F4 29
#define G4 30
#define H4 31
#define A5 32
#define B5 33
#define C5 34
#define D5 35
#define E5 36
#define F5 37
#define G5 38
#define H5 39
#define A6 40
#define B6 41
#define C6 42
#define D6 43
#define E6 44
#define F6 45
#define G6 46
#define H6 47
#define A7 48
#define B7 49
#define C7 50
#define D7 51
#define E7 52
#define F7 53
#define G7 54
#define H7 55
#define A8 56
#define B8 57
#define C8 58
#define D8 59
#define E8 60
#define F8 61
#define G8 62
#define H8 63
#define SQ_CNT 64

typedef int square; 

static inline square make_sq(int file, int rank) 
{
    return file | rank << 3; 
}

static inline int get_file(square sq) 
{
    return sq & 7; 
}

static inline int get_rank(square sq) 
{
    return sq >> 3; 
}

static inline int get_diag(square sq) 
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

static inline int get_anti(square sq) 
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

static inline square rrank(square sq) 
{
    return make_sq(get_file(sq), 7 - get_rank(sq)); 
}

static inline const char* str_sq(square sq) 
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