/**
 * @file Magic.h
 * @author Nicholas Hamilton 
 * @date 2023-02-20
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Magic bitboard constants. 
 */

#pragma once 

#include "Bitboard.h" 

extern const Bitboard MagicRShift[NumSquares]; 
extern const Bitboard MagicBShift[NumSquares]; 

extern const Bitboard MagicRMask[NumSquares]; 
extern const Bitboard MagicBMask[NumSquares]; 

extern const Bitboard* MagicRSlide[NumSquares]; 
extern const Bitboard* MagicBSlide[NumSquares]; 

extern const Bitboard MagicR[NumSquares]; 
extern const Bitboard MagicB[NumSquares]; 
