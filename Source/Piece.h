/**
 * @file Piece.h
 * @author Nicholas Hamilton 
 * @date 2023-01-12
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Piece and color constants and utilities. 
 */

#pragma once 

#include <stdint.h> 

/**
 * Represents different sides. 
 */
typedef enum Color
{
    ColorW = 0, 
    ColorB, 
    NumColors,
} Color; 

/**
 * Type of piece (uncolored). 
 */
typedef enum PieceType
{
    PieceP = 0,
    PieceN = 1,
    PieceB = 2,
    PieceR = 3,
    PieceQ = 4,
    PieceK = 5,
    NumPieceTypes = 6,
    NoPieceType = NumPieceTypes
} PieceType; 

/**
 * Colored pieces. 
 */
typedef enum Piece
{
    PieceWP = 0,
    PieceWN = 1,
    PieceWB = 2,
    PieceWR = 3,
    PieceWQ = 4,
    PieceWK = 5,
    PieceBP = 6,
    PieceBN = 7,
    PieceBB = 8,
    PieceBR = 9,
    PieceBQ = 10,
    PieceBK = 11,
    NumPieces = 12,
    NoPiece = NumPieces
} Piece; 

/**
 * @param c Color
 * @return Sign modifier for the color 
 */
static inline int ColorSign(Color c) 
{
    return 1 - 2 * c; 
}

/**
 * @param c Starting color
 * @return The opposing color 
 */
static inline Color OppositeColor(Color c) 
{
    return c ^ 1; 
}

/**
 * @param pc A colored piece 
 * @return Color of the piece 
 */
static inline Color ColorOfPiece(Piece p) 
{
    return p >= 6; 
}

/**
 * Removes color from a piece type 
 * 
 * @param pc Colored piece type 
 * @return Uncolored piece type 
 */
static inline PieceType TypeOfPiece(Piece p) 
{
    return (PieceType) (p - (p >= 6) * 6); 
}

/**
 * Colors a piece type. The type should not already be colored. 
 * 
 * If the type is colored then use `Recolor` instead. 
 * 
 * @param type Uncolored piece type 
 * @param c Desired color 
 * @return Colored piece 
 */
static inline Piece MakePiece(PieceType colorless, Color col) 
{
    return (Piece) (colorless + col * 6); 
}

/**
 * Recolors a piece. 
 * 
 * @param pc Any piece type
 * @param c New color 
 * @return Recolored piece
 */
static inline Piece Recolor(Piece p, Color col) 
{
    return MakePiece(TypeOfPiece(p), col); 
}

/**
 * Gets a readable string for a piece. 
 * 
 * @param pc The piece 
 * @return String for the piece 
 */
static inline const char* PieceString(Piece p) 
{
    static const char* Str[] = 
    {
        "P", "N", "B", "R", "Q", "K", 
        "p", "n", "b", "r", "q", "k", 
        " ", " ", " ", " "
    };
    return Str[p]; 
}

/**
 * Gets a readable string for the uncolored type of a piece. 
 * 
 * @param pc The piece 
 * @return String for the piece 
 */
static inline const char* PieceTypeString(Piece p) 
{
    static const char* Str[] = 
    {
        "p", "n", "b", "r", "q", "k", 
        "p", "n", "b", "r", "q", "k", 
        " ", " ", " ", " "
    };
    return Str[p]; 
}
