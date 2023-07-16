/**
 * @file Eval.c
 * @author Nicholas Hamilton 
 * @date 2023-02-20
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Implements static board evaluation. 
 */

#include "Game.h" 

int AttackUnitValues[] = 
{
    0, 0, 1, 2, 3, 5, 
    8, 11, 14, 
    19, 24, 29, 34, 39, 
    47, 55, 63, 71, 79, 87, 95, 103, 
    116, 129, 142, 155, 168, 181, 194, 207, 220, 233, 246, 259, 272, 
    285, 298, 311, 324, 337, 350, 363, 376, 389, 402, 415, 428, 441, 
    449, 454, 459, 464, 469, 474, 479, 484, 489, 494, 499, 
    500, 500, 500, 500, 500
};

int PawnStructureValues[] =
{
    -20, // isolated 
    -15, // backward 
    -20, // doubled (once for every set)
    -50, // tripled (once for every set)
};

int PassedPawnValues[] = 
{
    0, 10, 15, 20, 30, 40, 50, 0, 
};

int PcSq[2][6][64] = 
{
    {
        { // pawn
            0,   0,   0,   0,   0,   0,   0,   0, 
            50,  50,  50,  50,  50,  50,  50,  50, 
            10,  10,  20,  30,  30,  20,  10,  10, 
            5,   5,  10,  25,  25,  10,   5,   5, 
            0,   0,   0,  20,  20,   0,   0,   0, 
            5,  -5, -10,   0,   0, -10,  -5,   5, 
            5,  10,  10, -20, -20,  10,  10,   5, 
            0,   0,   0,   0,   0,   0,   0,   0
        }, 
        { // knight 
            -50, -40, -30, -30, -30, -30, -40, -50, 
            -40, -20,   0,   0,   0,   0, -20, -40, 
            -30,   0,  10,  15,  15,  10,   0, -30, 
            -30,   5,  15,  20,  20,  15,   5, -30, 
            -30,   0,  15,  20,  20,  15,   0, -30, 
            -30,   5,  10,  15,  15,  10,   5, -30, 
            -40, -20,   0,   5,   5,   0, -20, -40, 
            -50, -40, -30, -30, -30, -30, -40, -50 
        }, 
        { // bishop 
            -20, -10, -10, -10, -10, -10, -10, -20, 
            -10,   0,   0,   0,   0,   0,   0, -10, 
            -10,   0,   5,  10,  10,   5,   0, -10, 
            -10,   5,   5,  10,  10,   5,   5, -10, 
            -10,   0,  10,  10,  10,  10,   0, -10, 
            -10,  10,  10,  10,  10,  10,  10, -10, 
            -10,   5,   0,   0,   0,   0,   5, -10, 
            -20, -10, -10, -10, -10, -10, -10, -20 
        }, 
        { // rook
             0,   0,   0,   0,   0,   0,   0,   0,
             5,  10,  10,  10,  10,  10,  10,   5, 
            -5,   0,   0,   0,   0,   0,   0,  -5, 
            -5,   0,   0,   0,   0,   0,   0,  -5, 
            -5,   0,   0,   0,   0,   0,   0,  -5, 
            -5,   0,   0,   0,   0,   0,   0,  -5, 
            -5,   0,   0,   0,   0,   0,   0,  -5, 
             0,   0,   0,   5,   5,   0,   0,   0 
        }, 
        { // queen
            -20, -10, -10,  -5,  -5, -10, -10, -20, 
            -10,   0,   0,   0,   0,   0,   0, -10, 
            -10,   0,   5,   5,   5,   5,   0, -10, 
              0,   0,   5,   5,   5,   5,   0,   0, 
              0,   0,   5,   5,   5,   5,   0,   0, 
            -10,   0,   5,   5,   5,   5,   0, -10, 
            -10,   0,   5,   0,   0,   5,   0, -10, 
            -20, -10, -10,  -5,  -5, -10, -10, -20, 
        }, 
        { // king 
            -30, -40, -40, -50, -50, -40, -40, -30, 
            -30, -40, -40, -50, -50, -40, -40, -30, 
            -30, -40, -40, -50, -50, -40, -40, -30, 
            -30, -40, -40, -50, -50, -40, -40, -30, 
            -20, -30, -30, -40, -40, -30, -30, -20, 
            -10, -20, -20, -20, -20, -20, -20, -10, 
            20,  20,   0,   0,   0,   0,  20,  20, 
            20,  30,  10,   0,   0,  10,  30,  20 
        }
    }, 
    {
        { // pawn
            0,   0,   0,   0,   0,   0,   0,   0, 
          100, 100, 100, 100, 100, 100, 100, 100, 
           40,  40,  40,  40,  40,  40,  40,  40,
           30,  30,  30,  30,  30,  30,  30,  30,  
           20,  20,  20,  20,  20,  20,  20,  20,  
           10,  10,  10,  10,  10,  10,  10,  10, 
            5,   0,   0, -20, -20,   0,   0,   5, 
            0,   0,   0,   0,   0,   0,   0,   0
        }, 
        { // knight 
            -50, -40, -30, -30, -30, -30, -40, -50, 
            -40, -20,   0,   0,   0,   0, -20, -40, 
            -30,   0,  10,  15,  15,  10,   0, -30, 
            -30,   5,  15,  20,  20,  15,   5, -30, 
            -30,   0,  15,  20,  20,  15,   0, -30, 
            -30,   5,  10,  15,  15,  10,   5, -30, 
            -40, -20,   0,   5,   5,   0, -20, -40, 
            -50, -40, -30, -30, -30, -30, -40, -50 
        }, 
        { // bishop 
            -20, -10, -10, -10, -10, -10, -10, -20, 
            -10,   0,   0,   0,   0,   0,   0, -10, 
            -10,   0,   5,  10,  10,   5,   0, -10, 
            -10,   5,   5,  10,  10,   5,   5, -10, 
            -10,   0,  10,  10,  10,  10,   0, -10, 
            -10,  10,  10,  10,  10,  10,  10, -10, 
            -10,   5,   0,   0,   0,   0,   5, -10, 
            -20, -10, -10, -10, -10, -10, -10, -20 
        }, 
        { // rook
            0,   0,   0,   0,   0,   0,   0,   0,
            5,  10,  10,  10,  10,  10,  10,   5, 
           -5,   0,   0,   0,   0,   0,   0,  -5, 
           -5,   0,   0,   0,   0,   0,   0,  -5, 
           -5,   0,   0,   0,   0,   0,   0,  -5, 
           -5,   0,   0,   0,   0,   0,   0,  -5, 
           -5,   0,   0,   0,   0,   0,   0,  -5, 
            0,   0,   0,   5,   5,   0,   0,   0 
        }, 
        { // queen
            -20, -10, -10,  -5,  -5, -10, -10, -20, 
            -10,   0,   0,   0,   0,   0,   0, -10, 
            -10,   0,   5,   5,   5,   5,   0, -10, 
              0,   0,   5,   5,   5,   5,   0,   0, 
              0,   0,   5,   5,   5,   5,   0,   0, 
            -10,   0,   5,   5,   5,   5,   0, -10, 
            -10,   0,   5,   0,   0,   5,   0, -10, 
            -20, -10, -10,  -5,  -5, -10, -10, -20, 
        }, 
        { // king 
            -50, -40, -30, -30, -30, -30, -40, -50, 
            -40, -20,   0,   0,   0,   0, -20, -40, 
            -30,   0,  10,  15,  15,  10,   0, -30, 
            -30,   5,  15,  20,  20,  15,   5, -30, 
            -30,   0,  15,  20,  20,  15,   0, -30, 
            -30,   5,  10,  15,  15,  10,   5, -30, 
            -40, -20,   0,   5,   5,   0, -20, -40, 
            -50, -40, -30, -30, -30, -30, -40, -50 
        }
    }, 
};

static inline void EvalWPcSq(const Game* g, Piece pc, int* mg, int* eg) 
{
    BBoard pcs = RRow(g->Pieces[pc]); 
    FOR_EACH_BIT(pcs, 
    {
        *mg += PcSq[0][pc][sq]; 
        *eg += PcSq[1][pc][sq]; 
    });
}

static inline void EvalBPcSq(const Game* g, Piece pc, int* mg, int* eg) 
{
    BBoard pcs = g->Pieces[MakePc(pc, COL_B)]; 
    FOR_EACH_BIT(pcs, 
    {
        *mg -= PcSq[0][pc][sq]; 
        *eg -= PcSq[1][pc][sq]; 
    });
}

static inline int EvalAttackUnits(const Game* g, Color col) 
{
    Square ksq = Lsb(g->Pieces[MakePc(PC_K, col)]); 
    BBoard region = BB[ksq] | MovesK[ksq]; 
    if (col == COL_W) 
    {
        region |= ShiftN(region); 
    }
    else 
    {
        region |= ShiftS(region); 
    }
    int units = GetAttackUnits(g, region, col); 
    if (units > 63) units = 63; 
    return AttackUnitValues[units]; 
}

static inline void EvalWPStructure(
    const Game* g, 
    int* isolated, 
    int* backward, 
    int* doubled, 
    int* tripled, 
    int* passed, 
    int* passedEval) 
{
    BBoard pawns, opp; 
    BBoard wp = g->Pieces[PC_WP]; 
    BBoard bp = g->Pieces[PC_BP]; 

    pawns = wp; 
    opp = bp; 
    FOR_EACH_BIT(pawns, 
    {
        int sqFile = GetFile(sq); 
        int sqRank = GetRank(sq); 
        int shiftForward = (sqRank + 1) * 8; // in front of but not same rank 
        int shiftBackward = (7 - sqRank) * 8; // in back of or on same rank 

        (*isolated) += (pawns & AdjFiles[sqFile]) == 0; 

        // backward if: 
        // - there are adjacent pawns ahead
        // - there are NOT adjacent pawns behind 
        // - adjacent pawns on the same rank don't matter 
        (*backward) += (sqFile > 0) * (sqFile < 7)
                     * ((pawns & (AdjFiles[sqFile] << shiftForward))  != 0)  // pawns ahead 
                     * ((pawns & (AdjFiles[sqFile] >> shiftBackward)) == 0); // no pawns behind 

        if ((opp & (SameAdjFiles[sqFile] << shiftForward)) == 0) // no opp pawns ahead 
        {
            (*passed)++; 
            (*passedEval) += PassedPawnValues[GetRank(sq)]; 
        }
    });

    for (int f = 0; f < 8; f++) 
    {
        int num = Popcnt(pawns & Files[f]); 
        if (num >= 3) 
        {
            (*tripled)++; 
        }
        else if (num == 2) 
        {
            (*doubled)++; 
        }
    }
}

static inline void EvalBPStructure(
    const Game* g, 
    int* isolated, 
    int* backward, 
    int* doubled, 
    int* tripled, 
    int* passed, 
    int* passedEval) 
{
    BBoard pawns, opp; 
    BBoard wp = g->Pieces[PC_WP]; 
    BBoard bp = g->Pieces[PC_BP]; 

    pawns = bp; 
    opp = wp; 
    FOR_EACH_BIT(pawns, 
    {
        int sqFile = GetFile(sq); 
        int sqRank = GetRank(sq); 
        int shiftForward = (8 - sqRank) * 8; // in front of but not same rank 
        int shiftBackward = (sqRank) * 8; // in back of or on same rank 

        (*isolated) += (pawns & AdjFiles[sqFile]) == 0; 

        // backward if: 
        // - there are adjacent pawns ahead
        // - there are NOT adjacent pawns behind 
        // - adjacent pawns on the same rank don't matter 
        (*backward) += (sqFile > 0) * (sqFile < 7)
                     * ((pawns & (AdjFiles[sqFile] >> shiftForward))  != 0)  // pawns ahead 
                     * ((pawns & (AdjFiles[sqFile] << shiftBackward)) == 0); // no pawns behind 

        if ((opp & (SameAdjFiles[sqFile] >> shiftForward)) == 0) // no opp pawns ahead 
        {
            (*passed)++; 
            (*passedEval) += PassedPawnValues[7 - GetRank(sq)]; 
        }
    });

    for (int f = 0; f < 8; f++) 
    {
        int num = Popcnt(pawns & Files[f]); 
        if (num >= 3) 
        {
            (*tripled)++; 
        }
        else if (num == 2) 
        {
            (*doubled)++; 
        }
    }
}

int EvaluateVerbose(const Game* g, int nMoves, bool draw, int contempt, bool verbose) 
{
    if (draw) 
    {   
        return contempt; 
    }

    if (nMoves == 0) 
    {
        if (g->InCheck) 
        {
            // lower value the farther out the mate is (prioritize faster mates)
            return (100000 - g->Ply) * (-1 + 2 * g->Turn); 
        }
        else 
        {
            // stalemate
            return contempt; 
        }
    }

    int wp = g->Counts[PC_WP]; 
    int bp = g->Counts[PC_BP]; 
    int wn = g->Counts[PC_WN]; 
    int bn = g->Counts[PC_BN]; 
    int wb = g->Counts[PC_WB]; 
    int bb = g->Counts[PC_BB]; 
    int wr = g->Counts[PC_WR]; 
    int br = g->Counts[PC_BR]; 
    int wq = g->Counts[PC_WQ]; 
    int bq = g->Counts[PC_BQ]; 
    int wk = g->Counts[PC_WK]; 
    int bk = g->Counts[PC_BK]; 

    int eval = 0; 
    int mg = 0; 
    int eg = 0; 

    eval += 100 * (wp - bp); 
    eval += 310 * (wn - bn); 
    eval += 320 * (wb - bb); 
    eval += 500 * (wr - br); 
    eval += 975 * (wq - bq); 
    eval += 10000 * (wk - bk); 

    // bishop pair 
    eval += 15 * ((wb >= 2) - (bb >= 2)); 

    // this is reversed because it is checking how (un)safe that color's king is 
    // and returning a higher value for less safe 
    eval += EvalAttackUnits(g, COL_B) - EvalAttackUnits(g, COL_W); 

    int wpIso = 0; 
    int wpBack = 0; 
    int wpDoub = 0; 
    int wpTrip = 0; 
    int wpPass = 0; 
    int wpPassEval = 0; 
    EvalWPStructure(g, &wpIso, &wpBack, &wpDoub, &wpTrip, &wpPass, &wpPassEval); 

    eval += wpPassEval
          + wpIso * PawnStructureValues[0] 
          + wpBack * PawnStructureValues[1] 
          + wpDoub * PawnStructureValues[2] 
          + wpTrip * PawnStructureValues[3]; 

    int bpIso = 0; 
    int bpBack = 0; 
    int bpDoub = 0; 
    int bpTrip = 0; 
    int bpPass = 0; 
    int bpPassEval = 0; 
    EvalBPStructure(g, &bpIso, &bpBack, &bpDoub, &bpTrip, &bpPass, &bpPassEval); 

    eval -= bpPassEval
          + bpIso * PawnStructureValues[0] 
          + bpBack * PawnStructureValues[1] 
          + bpDoub * PawnStructureValues[2] 
          + bpTrip * PawnStructureValues[3]; 

    EvalWPcSq(g, PC_P, &mg, &eg); 
    EvalBPcSq(g, PC_P, &mg, &eg); 
    EvalWPcSq(g, PC_N, &mg, &eg); 
    EvalBPcSq(g, PC_N, &mg, &eg); 
    EvalWPcSq(g, PC_B, &mg, &eg); 
    EvalBPcSq(g, PC_B, &mg, &eg); 
    EvalWPcSq(g, PC_R, &mg, &eg); 
    EvalBPcSq(g, PC_R, &mg, &eg); 
    EvalWPcSq(g, PC_Q, &mg, &eg); 
    EvalBPcSq(g, PC_Q, &mg, &eg); 
    EvalWPcSq(g, PC_K, &mg, &eg); 
    EvalBPcSq(g, PC_K, &mg, &eg); 

    // int p = 0 * (wp - bp);  
    int n = 1 * (wn + bn); 
    int b = 1 * (wb + bb); 
    int r = 2 * (wr + br); 
    // quick way to ignore extra queens 
    int q = 4 * ((wq > 0) + (bq > 0)); 

    // between 0 and (4+4+8+8)=24
    int phase = 24 - (n + b + r + q); 
    phase = (phase >= 0) * phase; 
    // printf("phase %d mg %d eg %d eval %d\n", phase, mg, eg, (mg * (32 - phase) + eg * phase) / 32); 

    eval += (mg * (24 - phase) + eg * phase) / 24; 
    
    if (verbose) 
    {
        printf("Game phase: %.0f / 100\n", phase / 24.0 * 100); 
        printf("Middlegame piece-square: %d\n", mg); 
        printf("Endgame piece-square: %d\n", eg); 
        printf("Isolated pawns (%d-%d): %d\n", wpIso, bpIso, (wpIso-wpIso) * PawnStructureValues[0]); 
        printf("Backward pawns (%d-%d): %d\n", wpBack, bpBack, (wpBack-bpBack) * PawnStructureValues[1]); 
        printf("Doubled pawns (%d-%d): %d\n", wpDoub, bpDoub, (wpDoub-bpDoub) * PawnStructureValues[2]); 
        printf("Tripled pawns (%d-%d): %d\n", wpTrip, bpTrip, (wpTrip-bpTrip) * PawnStructureValues[3]); 
        printf("Passed pawns (%d-%d): %d\n", wpPass, bpPass, wpPassEval - bpPassEval); 
        printf("Final evaluation: %d\n", eval); 
    }

    return eval; 
}
