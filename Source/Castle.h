/**
 * @file Castle.h
 * @author Nicholas Hamilton 
 * @date 2023-01-12
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Bit flags for castling. 
 */

#pragma once 

/**
 * Represents castling types that are currently available.
 */
typedef enum CastleFlags
{
    CastleWK = 1, 
    CastleWQ = 2, 
    CastleW = 3, 

    CastleBK = 4, 
    CastleBQ = 8, 
    CastleB = 12, 

    CastleAll = 15, 
    CastleNone = 0 
} CastleFlags;

/**
 * Returns FEN-style string describing castling availability. 
 * 
 * @param cf Castle flags
 * @return Castle availability string
 */
static const char* CastleString(CastleFlags cf) 
{
    static const char* Str[] = 
    {
        "(none)", 
        "K", 
        "Q", 
        "KQ", 
        "k", 
        "Kk", 
        "Qk", 
        "KQk", 
        "q", 
        "Kq", 
        "Qq", 
        "KQq", 
        "kq", 
        "Kkq", 
        "Qkq", 
        "KQkq", 
    };
    return Str[cf]; 
}
