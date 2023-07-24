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

int PassedPawnValues[] = 
{
    0, 10, 15, 20, 30, 40, 50, 0, 
};

int BishopPair = 32; 

int PawnStructureValues[] =
{
    -12, // isolated 
    -5,  // backward 
    -16, // doubled (once for every set)
    -55, // tripled (once for every set)
};

/**
 * Does not include PC_K as this should not be parameterized 
 */
int PcTypeValues[] = 
{
    92,  322,  329,  515,  960, 
};

int AttackUnitValues[] = 
{
      0,    4,    9,   10,   12,   14,   16,   18, 
     22,   24,   28,   20,   33,   35,   36,   37, 
     42,   50,   58,   66,   74,   82,   95,  108, 
    121,  134,  147,  160,  173,  186,  199,  212, 
    225,  238,  251,  264,  277,  290,  303,  316, 
    329,  342,  363,  372,  381,  412,  420,  424, 
    428,  451,  456,  461,  478,  483,  485,  487, 
    492,  492,  500,  500,  500,  500,  503,  506, 
};

int PcSq[2][6][64] = 
{
    {
        { // pawn
               0,    0,    0,    0,    0,    0,    0,    0, 
              65,   65,   65,   58,   65,   53,   53,   54, 
              25,   25,   35,   27,   42,   35,   10,   17, 
               8,    2,    8,   34,   35,    6,  -10,    2, 
               7,   14,   -3,   14,   15,   -3,  -11,   -3, 
              -1,   -5,   -6,    2,   -4,   -7,   10,    2, 
              -2,   -5,   -5,  -12,  -10,   19,    7,   -4, 
               0,    0,    0,    0,    0,    0,    0,    0, 
        }, 
        { // knight 
             -65,  -31,  -15,  -17,  -23,  -35,  -27,  -62, 
             -30,  -23,   14,    7,   14,   15,  -21,  -55, 
             -31,    1,   17,   30,   30,   19,   -2,  -33, 
             -15,   -4,    8,   35,   19,   30,    8,  -15, 
             -16,    0,   30,   13,   20,   28,   -5,  -15, 
             -31,    1,    7,   28,   19,    1,    5,  -43, 
             -25,  -34,   11,    4,   -4,   -5,  -25,  -37, 
             -49,  -25,  -23,  -42,  -36,  -27,  -25,  -42, 
        }, 
        { // bishop 
             -10,  -18,   -3,    0,  -12,  -18,  -25,  -26, 
             -18,   -3,   13,   11,    3,   15,   12,  -16, 
             -14,   15,   20,   13,   16,   20,    5,    2, 
             -13,    7,   17,    9,   19,   17,  -10,    2, 
             -13,   -9,   20,   25,   25,    8,    5,   -7, 
             -13,   11,    0,   15,    7,   10,   13,    5, 
             -13,    0,  -15,  -10,    2,    3,   12,   -7, 
             -35,   -6,  -21,  -12,    1,  -18,    5,  -34, 
        }, 
        { // rook
               9,    7,   15,   15,   15,   14,    6,   14, 
              19,   25,   23,   25,   25,   25,   24,    8, 
              10,   14,   15,   15,   14,    4,   11,   -6, 
               7,    9,   13,   15,   15,   -7,    2,  -10, 
              -2,   15,   15,   12,  -10,   -8,   -4,  -12, 
             -13,    0,   -6,    7,   -8,  -14,   15,  -12, 
             -20,   -1,   -2,   -6,  -11,  -15,  -15,  -20, 
              -4,   -9,   -1,    3,    7,   -1,  -11,  -15, 
        }, 
        { // queen
             -19,    1,    5,    7,   -4,    4,  -12,   -6, 
             -15,   -8,   -3,   13,   -9,   14,   -3,    2, 
              -7,   -9,    0,  -10,    2,   -8,  -15,  -25, 
               1,   11,    2,    2,   -9,   -4,  -15,  -15, 
              -3,   -4,    2,    8,    8,  -10,   -6,   -8, 
             -10,   13,    8,    7,    4,   -9,   15,  -25, 
             -25,    0,    8,   13,    1,    3,    3,  -25, 
              -5,  -25,  -13,    8,   -2,   -8,  -25,  -35, 
        }, 
        { // king 
             -18,  -25,  -25,  -35,  -35,  -25,  -25,  -30, 
             -15,  -25,  -25,  -41,  -35,  -25,  -25,  -15, 
             -15,  -25,  -38,  -60,  -47,  -27,  -25,  -15, 
             -28,  -27,  -40,  -53,  -49,  -37,  -25,  -15, 
             -15,  -31,  -34,  -44,  -31,  -16,  -15,  -17, 
             -14,  -23,  -23,   -7,   -5,   -7,   -5,  -13, 
              10,   19,   13,   10,   15,   -1,   34,    6, 
              14,   23,   10,  -15,    9,   -5,   41,    8,  
        }
    }, 
    {
        { // pawn
               0,    0,    0,    0,    0,    0,    0,    0, 
             112,  115,  115,   97,  103,  115,  115,   90, 
              55,   55,   55,   41,   33,   30,   44,   55, 
              42,   35,   27,   15,   15,   15,   24,   17, 
              26,   26,   19,    5,    5,   16,   17,   15, 
              15,   15,   14,    9,   16,   22,   18,    8, 
              20,   11,    3,  -23,   -5,   12,    9,    2, 
               0,    0,    0,    0,    0,    0,    0,    0, 
        }, 
        { // knight 
             -65,  -37,  -15,  -15,  -33,  -38,  -37,  -65, 
             -32,  -21,   12,   -1,    3,   15,  -13,  -55, 
             -18,   -3,   25,   25,   16,   13,   -4,  -38, 
             -15,   15,   24,   31,   23,   12,    2,  -15, 
             -15,   -4,   25,   23,   10,   11,   -1,  -31, 
             -43,   -1,   17,   18,    5,    6,  -10,  -44, 
             -37,  -31,  -15,  -10,  -10,   -7,  -19,  -33, 
             -65,  -32,  -33,  -31,  -29,  -27,  -27,  -53, 
        }, 
        { // bishop 
              -5,   -5,    4,    4,   -9,   -6,   -9,  -21, 
               1,   15,   15,   14,    3,    9,    7,   -7, 
               2,   15,   20,   11,   14,   19,    5,   -7, 
              -4,   12,   12,   24,   14,    5,    9,    1, 
              -4,   -9,   23,    6,   15,   13,    3,   -8, 
             -15,   15,    9,   14,   14,    2,   13,    3, 
             -22,    8,   -2,   -4,    5,   -2,   -4,   -7, 
             -35,  -13,  -25,   -7,   -8,  -13,  -21,  -26, 
        }, 
        { // rook
              -7,    9,   15,   15,   15,   15,    6,   14, 
              20,   25,   25,   25,   25,   23,   18,   13, 
              10,   15,   15,   15,   15,   15,    5,    5, 
              10,   15,   15,   14,    3,    0,  -12,   -7, 
              10,   15,   14,    3,   -5,   -9,  -11,  -20, 
              -1,    3,   13,    7,  -14,    0,   -1,  -12, 
             -20,  -13,    1,   -1,  -15,  -11,  -15,  -20, 
              -9,  -11,    6,   16,   -2,    0,  -11,   -8, 
        }, 
        { // queen
             -18,   -1,    0,   -3,  -16,    2,   -9,   -8, 
             -10,    1,    2,   13,    7,   12,    0,   -9, 
               4,    3,   17,   11,   -6,   -4,    2,  -11, 
              -7,    2,   16,    6,    6,  -10,   -8,  -15, 
              -2,    6,   13,   15,    8,    5,    1,  -12, 
              -9,    1,   11,    2,   10,    0,   -1,  -24, 
             -15,    7,   11,    8,    3,   -7,  -15,  -24, 
             -17,  -21,   -8,    1,   -1,  -16,  -25,  -35, 
        }, 
        { // king 
             -47,  -25,  -15,  -15,  -15,  -15,  -25,  -50, 
             -25,   -5,   15,   10,   15,   15,   -5,  -25, 
             -15,   15,   14,   18,   18,   25,   15,  -15, 
             -15,    8,   17,   17,   22,   18,   20,  -15, 
             -36,   -3,    7,   13,   13,   11,    7,  -26, 
             -40,  -10,   -5,    3,    9,    7,    7,  -27, 
             -42,  -21,  -14,   -7,  -10,   -3,  -17,  -32, 
             -53,  -54,  -33,  -39,  -34,  -39,  -45,  -65, 
        }
    }, 
};

void PrintArrayIndex(char* out, const char* ary, int index, int s1, int s2, int s3) 
{
    if (s1 == 0) 
    {
        snprintf(out, PARAM_NAME_LEN, "%s", ary);
    }
    else if (s2 == 0) 
    {
        snprintf(out, PARAM_NAME_LEN, "%s[%d]", ary, index);
    }
    else if (s3 == 0) 
    {
        int i1 = index % s1;  
        int i2 = index / s1; 
        snprintf(out, PARAM_NAME_LEN, "%s[%d][%d]", ary, i2, i1);
    }
    else 
    {
        int i1 = index % s1;  
        int i2_ = index / s1; 
        int i2 = i2_ % s2; 
        int i3 = i2_ / s2; 
        snprintf(out, PARAM_NAME_LEN, "%s[%d][%d][%d]", ary, i3, i2, i1);
    }
}

#define EVAL_PARAM(ary, s1, s2, s3) \
    if (index < (int) (sizeof(ary) / sizeof(int))) { if (name) { PrintArrayIndex(name, #ary, index, s1, s2, s3); } return ((int*) ary) + index; } \
    index -= (int) (sizeof(ary) / sizeof(int)); 

#define EVAL_1PARAM(val) \
    if (index < 1) { if (name) { PrintArrayIndex(name, #val, 0, 0, 0, 0); } return &val; } \
    index -= 1; 

int* GetEvalParam(int index, char* name) 
{
    if (index < 0) return NULL; 

    EVAL_1PARAM(BishopPair); 
    EVAL_PARAM(PawnStructureValues, 4, 0, 0); 
    EVAL_PARAM(PcTypeValues, 5, 0, 0); 
    EVAL_PARAM(AttackUnitValues, 64, 0, 0); 
    EVAL_PARAM(PcSq, 64, 6, 2); 

    if (name) name[0] = '\0'; 
    return NULL; 
}

int GetNumEvalParams(void) 
{
    int num = 0; 
    while (GetEvalParam(num, NULL)) num++; 

    return num; 
}

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

    eval += PcTypeValues[PC_P] * (wp - bp); 
    eval += PcTypeValues[PC_N] * (wn - bn); 
    eval += PcTypeValues[PC_B] * (wb - bb); 
    eval += PcTypeValues[PC_R] * (wr - br); 
    eval += PcTypeValues[PC_Q] * (wq - bq); 
    eval += 10000 * (wk - bk); 

    // bishop pair 
    eval += BishopPair * ((wb >= 2) - (bb >= 2)); 

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
