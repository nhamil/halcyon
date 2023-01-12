#pragma once 

#define SQUARE_NONE 0 
#define SQUARE_A1 0
#define SQUARE_B1 1
#define SQUARE_C1 2
#define SQUARE_D1 3
#define SQUARE_E1 4
#define SQUARE_F1 5
#define SQUARE_G1 6
#define SQUARE_H1 7
#define SQUARE_A2 8
#define SQUARE_B2 9
#define SQUARE_C2 10
#define SQUARE_D2 11
#define SQUARE_E2 12
#define SQUARE_F2 13
#define SQUARE_G2 14
#define SQUARE_H2 15
#define SQUARE_A3 16
#define SQUARE_B3 17
#define SQUARE_C3 18
#define SQUARE_D3 19
#define SQUARE_E3 20
#define SQUARE_F3 21
#define SQUARE_G3 22
#define SQUARE_H3 23
#define SQUARE_A4 24
#define SQUARE_B4 25
#define SQUARE_C4 26
#define SQUARE_D4 27
#define SQUARE_E4 28
#define SQUARE_F4 29
#define SQUARE_G4 30
#define SQUARE_H4 31
#define SQUARE_A5 32
#define SQUARE_B5 33
#define SQUARE_C5 34
#define SQUARE_D5 35
#define SQUARE_E5 36
#define SQUARE_F5 37
#define SQUARE_G5 38
#define SQUARE_H5 39
#define SQUARE_A6 40
#define SQUARE_B6 41
#define SQUARE_C6 42
#define SQUARE_D6 43
#define SQUARE_E6 44
#define SQUARE_F6 45
#define SQUARE_G6 46
#define SQUARE_H6 47
#define SQUARE_A7 48
#define SQUARE_B7 49
#define SQUARE_C7 50
#define SQUARE_D7 51
#define SQUARE_E7 52
#define SQUARE_F7 53
#define SQUARE_G7 54
#define SQUARE_H7 55
#define SQUARE_A8 56
#define SQUARE_B8 57
#define SQUARE_C8 58
#define SQUARE_D8 59
#define SQUARE_E8 60
#define SQUARE_F8 61
#define SQUARE_G8 62
#define SQUARE_H8 63
#define SQUARE_COUNT 64

typedef int square; 

static inline square square_make(int file, int rank) 
{
    return file | rank << 3; 
}

static inline int square_file(square sq) 
{
    return sq & 7; 
}

static inline int square_rank(square sq) 
{
    return sq >> 3; 
}

static inline int square_diagonal(square sq) 
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

static inline int square_antidiagonal(square sq) 
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

static inline square square_flip_rank(square sq) 
{
    return square_make(square_file(sq), 7 - square_rank(sq)); 
}

static inline const char* square_string(square sq) 
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