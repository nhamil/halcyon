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

#include "BBoard.h" 

extern const BBoard MagicRShift[NUM_SQ]; 
extern const BBoard MagicBShift[NUM_SQ]; 

extern const BBoard MagicRMask[NUM_SQ]; 
extern const BBoard MagicBMask[NUM_SQ]; 

extern const BBoard* MagicRSlide[NUM_SQ]; 
extern const BBoard* MagicBSlide[NUM_SQ]; 

extern const BBoard MagicR[NUM_SQ]; 
extern const BBoard MagicB[NUM_SQ]; 
