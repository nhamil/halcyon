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

int BishopPair = 24; 

int PawnStructureValues[] =
{
    -11, // isolated 
    -7,  // backward 
    -11, // doubled (once for every set)
    -49, // tripled (once for every set)
};

/**
 * Does not include PC_K as this should not be parameterized 
 */
int PcTypeValues[] = 
{
    93,  316,  325,  509,  966, 
};

int AttackUnitValues[] = 
{
       0,    5,    9,   11,   12,   14,   15,   15, 
      14,   28,   30,   33,   33,   36,   44,   46, 
      54,   62,   70,   78,   86,   94,  107,  120, 
     133,  146,  159,  172,  185,  198,  211,  224, 
     237,  250,  263,  276,  289,  302,  315,  328, 
     341,  354,  375,  380,  393,  424,  428,  432, 
     440,  463,  468,  473,  478,  483,  488,  493, 
     498,  498,  500,  500,  500,  500,  509,  512, 
};

int PcSq[2][6][64] = 
{
    {
        { // pawn
               0,    0,    0,    0,    0,    0,    0,    0, 
              59,   59,   59,   52,   59,   59,   47,   50, 
              19,   19,   29,   21,   39,   29,    9,   11, 
              14,   -4,    8,   34,   34,    2,   -4,   -4, 
               6,    9,   -9,   11,   11,   -5,   -9,   -9, 
              -3,   -7,   -1,    8,   -9,   -1,    4,   -4, 
              -4,    1,    1,  -12,  -12,   19,    1,    2, 
               0,    0,    0,    0,    0,    0,    0,    0, 
        }, 
        { // knight 
             -59,  -31,  -21,  -21,  -29,  -39,  -32,  -56, 
             -31,  -29,    9,    1,    9,    9,  -15,  -49, 
             -37,   -5,   11,   24,   24,   13,    1,  -39, 
             -21,   -4,    7,   29,   25,   24,   14,  -21, 
             -21,   -6,   24,   11,   26,   22,    1,  -21, 
             -29,    7,    2,   24,   14,    1,   11,  -39, 
             -31,  -29,    9,   10,    0,   -9,  -29,  -31, 
             -43,  -31,  -21,  -36,  -39,  -21,  -31,  -48, 
        }, 
        { // bishop 
             -11,  -19,   -9,   -1,   -9,  -14,  -19,  -21, 
             -16,   -9,    9,    5,    9,    9,    9,  -10, 
             -19,    9,   14,   19,   19,   14,    9,   -1, 
             -19,    1,   14,    3,   19,   11,   -4,   -4, 
             -19,   -3,   19,   19,   19,   14,   -1,   -1, 
             -19,    5,    3,   16,    1,   16,   19,   -1, 
             -19,   -4,   -9,   -9,    5,    9,   13,   -1, 
             -29,   -1,  -19,   -6,   -1,  -19,   -1,  -28, 
        }, 
        { // rook
               9,    8,    9,    9,    9,    8,    0,    8, 
              13,   19,   17,   19,   19,   19,   18,   14, 
               4,    8,    9,    9,    8,    9,    9,    0, 
               1,    3,    7,    9,    9,   -9,    8,  -14, 
               4,    9,    9,    9,   -9,   -2,    2,   -6, 
              -7,    6,   -9,    9,   -2,   -8,    9,   -6, 
             -14,    5,    4,   -3,   -5,   -9,   -9,  -14, 
              -5,   -4,    0,    8,   10,   -6,   -5,   -9, 
        }, 
        { // queen
             -25,   -4,   -1,    3,   -8,   -2,  -18,  -12, 
             -19,   -9,   -9,    7,   -6,    8,   -9,   -4, 
              -1,   -9,   -4,   -4,    1,   -4,   -9,  -19, 
               2,    6,   -4,   -4,   -3,    2,   -9,   -9, 
              -9,   -9,   -4,   14,   14,   -4,   -1,   -6, 
              -4,    7,   14,    7,   -2,   -4,    9,  -19, 
             -19,    6,   14,    7,    2,   -3,    9,  -19, 
             -11,  -19,  -19,    4,   -4,   -3,  -19,  -29, 
        }, 
        { // king 
             -21,  -31,  -31,  -41,  -41,  -31,  -31,  -32, 
             -21,  -31,  -31,  -41,  -41,  -31,  -31,  -21, 
             -21,  -31,  -37,  -54,  -41,  -31,  -31,  -21, 
             -34,  -33,  -46,  -59,  -43,  -38,  -31,  -21, 
             -13,  -37,  -39,  -48,  -34,  -22,  -21,  -11, 
             -19,  -29,  -29,  -13,  -11,  -11,  -11,   -7, 
              16,   13,    9,    9,    9,   -7,   29,   11, 
              11,   29,   13,   -9,    7,    1,   37,   14, 
        }
    }, 
    {
        { // pawn
               0,    0,    0,    0,    0,    0,    0,    0, 
             106,  109,  109,   91,  109,  109,  109,   91, 
              49,   49,   49,   35,   37,   35,   41,   49, 
              39,   38,   22,   21,   21,   21,   22,   21, 
              20,   20,   22,   11,   11,   11,   11,   11, 
               9,   12,   19,    3,   11,   19,   13,    4, 
              14,    5,    7,  -25,  -11,    9,    5,    8, 
               0,    0,    0,    0,    0,    0,    0,    0, 
        }, 
        { // knight 
             -59,  -31,  -21,  -21,  -39,  -38,  -31,  -59, 
             -31,  -27,    9,    2,    9,    9,  -11,  -49, 
             -24,   -9,   19,   19,   22,    7,    2,  -37, 
             -21,   11,   23,   25,   29,    6,    2,  -21, 
             -21,   -9,   19,   29,   16,   17,   -4,  -27, 
             -39,   -4,   19,   24,   11,    1,   -4,  -39, 
             -31,  -26,   -9,   -4,   -4,   -9,  -25,  -31, 
             -59,  -34,  -39,  -36,  -23,  -21,  -31,  -59, 
        }, 
        { // bishop 
             -11,  -11,   -1,   -1,  -15,   -6,   -8,  -15, 
              -5,    9,    9,    8,    9,    9,    4,   -1, 
              -4,    9,   14,   17,   15,   13,    9,   -1, 
             -10,    6,    6,   18,   11,    4,   12,   -5, 
              -1,   -8,   19,    1,   19,   19,    4,   -2, 
              -9,   10,    6,   15,   18,    7,   19,   -1, 
             -19,    7,    4,   -9,    9,   -8,   -4,   -1, 
             -29,  -19,  -19,   -1,  -11,  -19,  -19,  -20, 
        }, 
        { // rook
              -1,    4,    9,    9,    9,    9,    0,    8, 
              14,   19,   19,   19,   19,   19,   19,   14, 
               4,    9,    9,    9,    9,    9,    9,    4, 
               4,    9,    9,    9,    9,    6,   -6,   -3, 
               4,    9,    9,    9,   -6,   -9,   -9,  -14, 
              -7,    9,    7,    5,   -8,    4,    5,   -6, 
             -14,   -9,    7,   -6,   -9,   -5,   -9,  -14, 
              -3,   -7,    4,   13,    0,    6,   -7,   -9, 
        }, 
        { // queen
             -24,   -2,   -6,   -3,  -14,   -4,  -12,  -14, 
             -15,   -5,   -4,    7,    3,    6,   -5,  -14, 
              -1,    4,   11,    9,   -4,    2,    1,   -5, 
              -3,    2,   10,    0,   11,   -4,   -2,   -9, 
              -8,    6,    7,   12,   14,    3,    6,   -8, 
              -3,    5,    5,    5,    4,    0,   -2,  -19, 
             -16,    9,   13,    2,    8,   -4,   -9,  -19, 
             -15,  -16,  -13,    3,    3,  -10,  -19,  -29, 
        }, 
        { // king 
             -41,  -31,  -21,  -21,  -21,  -21,  -31,  -44, 
             -31,  -11,    9,    9,    9,    9,  -11,  -31, 
             -21,    9,   18,   24,   24,   19,    9,  -21, 
             -21,   10,   11,   11,   28,   21,   14,  -21, 
             -30,   -9,    6,   11,   16,   17,    9,  -21, 
             -39,   -4,    1,    6,   15,   13,   13,  -21, 
             -48,  -27,   -8,   -2,   -4,    0,  -11,  -37, 
             -59,  -49,  -39,  -39,  -36,  -39,  -39,  -59, 
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
