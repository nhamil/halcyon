/**
 * @file Move.h
 * @author Nicholas Hamilton 
 * @date 2023-01-12
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Encodes and decodes a single move. 
 */

#pragma once 

#include <stdbool.h> 
#include <stdint.h> 
#include <stdio.h> 

#include "Bitboard.h"
#include "Castle.h"
#include "Piece.h" 
#include "Square.h" 

#define NoMove 0

typedef enum MoveCastleIndex
{
    MoveCastleNone = 0,  
    MoveCastleWK,  
    MoveCastleWQ,  
    MoveCastleBK,  
    MoveCastleBQ, 
} MoveCastleIndex;

typedef enum MovePromotionIndex
{
    PromoteNone = 0,
    PromoteN, 
    PromoteB,  
    PromoteR,  
    PromoteQ, 
} MovePromotionIndex; 

static const Bitboard MoveCastleBitsK[5] = 
{
    0, 
    1ULL << E1 | 1ULL << G1, 
    1ULL << E1 | 1ULL << C1, 
    1ULL << E8 | 1ULL << G8, 
    1ULL << E8 | 1ULL << C8
};

static const Bitboard MoveCastleBitsR[5] = 
{
    0, 
    1ULL << H1 | 1ULL << F1, 
    1ULL << A1 | 1ULL << D1, 
    1ULL << H8 | 1ULL << F8, 
    1ULL << A8 | 1ULL << D8
};

static const Bitboard MoveCastleBitsAll[5] = 
{
    0, 
    1ULL << E1 | 1ULL << G1 | 1ULL << H1 | 1ULL << F1, 
    1ULL << E1 | 1ULL << C1 | 1ULL << A1 | 1ULL << D1, 
    1ULL << E8 | 1ULL << G8 | 1ULL << H8 | 1ULL << F8, 
    1ULL << E8 | 1ULL << C8 | 1ULL << A8 | 1ULL << D8
};

static const Square MoveCastleSquareK[5][2] = 
{
    { NoSquare, NoSquare }, 
    { E1, G1 }, 
    { E1, C1 }, 
    { E8, G8 }, 
    { E8, C8 }
};

static const Piece MoveCastlePieceK[5] = 
{
    NoPiece, 
    PieceWK, 
    PieceWK, 
    PieceBK, 
    PieceBK
};

static const Square MoveCastleSquareR[5][2] = 
{
    { NoSquare, NoSquare }, 
    { H1, F1 }, 
    { A1, D1 }, 
    { H8, F8 }, 
    { A8, D8 }
};

static const Piece MoveCastlePieceR[5] = 
{
    NoPiece, 
    PieceWR, 
    PieceWR, 
    PieceBR, 
    PieceBR
};

static const CastleFlags MoveCastleRemove[NumSquares] = 
{
    [A1] = CastleWQ, 
    [H1] = CastleWK, 
    [A8] = CastleBQ, 
    [H8] = CastleBK, 
    [E1] = CastleW, 
    [E8] = CastleB, 
};

/**
 * square from: 0-5 
 * square to: 6-11 
 * piece pc: 12-15 
 * piece pro: 16-19 
 * piece tgt: 20-23 
 * bool takesEnPassant: 24-24 
 * unsigned castle: 25-28 
 * bool check: 29-29
 */
typedef U32 Move; 

/**
 * Creates a standard move 
 * 
 * @param from From square 
 * @param to To square 
 * @param pc Piece to move 
 * @param tgt Piece to capture (or NoPiece)
 * @param check Is check 
 * @return The move
 */
static inline Move MakeMove(Square from, Square to, Piece pc, Piece tgt, bool check) 
{
    return ((Move) from) | ((Move) to << 6) | ((Move) pc << 12) | ((Move) pc << 16) | ((Move) tgt << 20) | ((Move) check << 29); 
}

/**
 * Creates promotion move 
 * 
 * @param from From square 
 * @param to To square 
 * @param pc Piece to move 
 * @param pro Promotion piece 
 * @param tgt Piece to capture (no NoPiece)
 * @param check Is check 
 * @return The move
 */
static inline Move MakePromotionMove(Square from, Square to, Piece pc, Piece pro, Piece tgt, bool check) 
{
    return ((Move) from) | ((Move) to << 6) | ((Move) pc << 12) | ((Move) pro << 16) | ((Move) tgt << 20) | ((Move) check << 29); 
}

/**
 * Creates a (potential) en passant move. 
 * 
 * @param from From square 
 * @param to To square 
 * @param pc Piece to move 
 * @param tgt Piece to capture 
 * @param ep Is the move en passant
 * @param check Is check 
 * @return The move
 */
static inline Move MakeEnPassantMove(Square from, Square to, Piece pc, Piece tgt, bool ep, bool check) 
{
    return ((Move) from) | ((Move) to << 6) | ((Move) pc << 12) | ((Move) pc << 16) | ((Move) tgt << 20) | ((Move) ep << 24) | ((Move) check << 29); 
}

/**
 * Creates a castling move 
 * 
 * @param from From square 
 * @param to To square 
 * @param pc Piece to move (king)
 * @param idx Type of castle 
 * @param check Is check 
 * @return The move
 */
static inline Move MakeCastleMove(Square from, Square to, Piece pc, MoveCastleIndex idx, bool check) 
{
    return ((Move) from) | ((Move) to << 6) | ((Move) pc << 12) | ((Move) pc << 16) | ((Move) NoPiece << 20) | ((Move) idx << 25) | ((Move) check << 29); 
}

/**
 * @param m The move 
 * @return Start square 
 */
static inline Square FromSquare(Move m) 
{
    return (Square) (m & 63); 
}

/**
 * @param m The move 
 * @return Target square 
 */
static inline Square ToSquare(Move m) 
{
    return (Square) ((m >> 6) & 63); 
}

/**
 * @param m The move 
 * @return Piece to move 
 */
static inline Piece FromPiece(Move m) 
{
    return (Piece) ((m >> 12) & 15); 
}

/**
 * @param m The move 
 * @return What the piece will be after the move 
 */
static inline Piece PromotionPiece(Move m) 
{
    return (Piece) ((m >> 16) & 15); 
}

/**
 * @param m The move 
 * @return Attacked piece or NoPiece
 */
static inline Piece TargetPiece(Move m) 
{
    return (Piece) ((m >> 20) & 15); 
}

/**
 * @param m The move 
 * @return Is the move en passant
 */
static inline bool IsEnPassant(Move m) 
{
    return (bool) ((m >> 24) & 1); 
}

/**
 * @param m The move 
 * @return Type of castle
 */
static inline int CastleIndex(Move m) 
{
    return (int) ((m >> 25) & 15); 
}

/**
 * @param m The move 
 * @return True if the move is a promotion, false otherwise 
 */
static inline bool IsPromotion(Move m) 
{
    return PromotionPiece(m) != FromPiece(m); 
}

/**
 * @param m The move 
 * @return True if the move is a capture, otherwise false
 */
static inline bool IsCapture(Move m) 
{
    return TargetPiece(m) != NoPiece; 
}

/**
 * @param m The move 
 * @return Does the move check the enemy king
 */
static inline bool IsCheck(Move m) 
{
    return (bool) ((m >> 29) & 1); 
}

/**
 * Should the move be considered for quiescence search. 
 * 
 * @param m The move 
 * @return Is the move tactical
 */
static inline bool IsTactical(Move m) 
{
    return IsCapture(m) || IsCheck(m) || IsPromotion(m); 
}

/**
 * @param m The move 
 * @return Is the move considered quiet
 */
static inline bool IsQuiet(Move m) 
{
    return !IsCapture(m) && !IsCheck(m) && !IsPromotion(m); 
}

/**
 * Prints a move to stdout. 
 * 
 * @param m The move 
 */
static void PrintMove(Move m) 
{
    if (IsPromotion(m)) 
    {
        printf("%s%s%s\n", SquareString(FromSquare(m)), SquareString(ToSquare(m)), PieceTypeString(PromotionPiece(m))); 
    }
    else 
    {
        printf("%s%s\n", SquareString(FromSquare(m)), SquareString(ToSquare(m))); 
    }
}

/**
 * Prints a move to stdout without newline. 
 * 
 * @param m The move 
 * @param end Text to write after the move 
 */
static void PrintMoveEnd(Move m, const char* end) 
{
    if (IsPromotion(m)) 
    {
        printf("%s%s%s%s", SquareString(FromSquare(m)), SquareString(ToSquare(m)), PieceTypeString(PromotionPiece(m)), end); 
    }
    else 
    {
        printf("%s%s%s", SquareString(FromSquare(m)), SquareString(ToSquare(m)), end); 
    }
}

/**
 * Writes a move to a buffer. 
 * 
 * @param m The move 
 * @param out The buffer
 * @param n Max buffer length 
 * @return Number of characters needed to write the move 
 */
static int SNPrintfMove(Move m, char* out, U64 n) 
{
    if (IsPromotion(m)) 
    {
        return snprintf(out, n, "%s%s%s", SquareString(FromSquare(m)), SquareString(ToSquare(m)), PieceTypeString(PromotionPiece(m))); 
    }
    else 
    {
       return snprintf(out, n, "%s%s", SquareString(FromSquare(m)), SquareString(ToSquare(m))); 
    }
}

/**
 * Writes a move to file without newline. 
 * 
 * @param m The move 
 * @param end Text to write after the move 
 * @param out The file 
 */
static void FilePrintMoveEnd(Move m, const char* end, FILE* out) 
{
    if (IsPromotion(m)) 
    {
        fprintf(out, "%s%s%s%s", SquareString(FromSquare(m)), SquareString(ToSquare(m)), PieceTypeString(PromotionPiece(m)), end); 
    }
    else 
    {
        fprintf(out, "%s%s%s", SquareString(FromSquare(m)), SquareString(ToSquare(m)), end); 
    }
}
