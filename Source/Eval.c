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
    0, 6, 6, 24, 36, 49, 62, 0
};

int ConnectedRooks = 8; 

int OpenFile = 16; 

int BishopPair = 35; 

int PawnStructureValues[] =
{
    -11, // isolated 
    -5,  // backward 
    -14, // doubled (once for every set)
    -51, // tripled (once for every set)
};

/**
 * Does not include PC_K as this should not be parameterized 
 */
int PcTypeValues[] = 
{
    91,  321,  331,  519,  964,
};

int AttackUnitValues[] = 
{
      0,    2,    8,   10,   11,   13,   14,   15, 
     18,   22,   24,   27,   30,   31,   33,   38, 
     39,   46,   54,   62,   70,   78,   91,  104, 
    117,  130,  143,  156,  169,  182,  195,  208, 
    221,  234,  247,  260,  273,  286,  299,  312, 
    325,  338,  359,  368,  377,  408,  416,  420, 
    424,  447,  452,  457,  472,  479,  489,  491, 
    496,  496,  504,  504,  504,  504,  507,  510, 
};

int PcSq[2][6][64] = 
{
    {
        { // pawn
               0,    0,    0,    0,    0,    0,    0,    0, 
              69,   69,   69,   62,   65,   49,   49,   55, 
              29,   29,   39,   31,   46,   39,   14,   21, 
              11,    6,   10,   30,   31,    9,  -14,    0, 
               3,   10,   -3,   11,   14,   -6,  -12,   -7, 
               0,   -1,   -7,   -2,   -3,  -11,    7,   -1, 
              -6,   -1,   -9,  -16,  -14,   22,   11,   -8, 
               0,    0,    0,    0,    0,    0,    0,    0, 
        }, 
        { // knight 
             -69,  -27,  -14,  -13,  -19,  -39,  -23,  -66, 
             -34,  -19,   18,   11,   18,   19,  -18,  -51, 
             -27,    5,   21,   34,   34,   23,    2,  -29, 
             -11,    0,   12,   39,   23,   34,    5,  -11, 
             -12,    4,   32,   15,   22,   24,   -1,  -11, 
             -27,   -3,    3,   24,   23,    5,    9,  -39, 
             -29,  -30,    7,    0,   -6,   -6,  -22,  -33, 
             -53,  -29,  -27,  -38,  -32,  -23,  -29,  -46,
        }, 
        { // bishop 
             -14,  -14,   -7,   -4,  -11,  -22,  -21,  -22, 
             -22,   -6,   13,   10,    5,   11,    8,  -20, 
             -10,   19,   24,   17,   20,   24,    9,    6, 
             -10,   11,   21,   13,   23,   21,   -9,   -2, 
             -17,   -5,   16,   29,   25,    6,    1,  -11, 
              -9,   11,    1,   12,   10,   10,    9,    1, 
              -9,   -4,  -11,  -10,    4,    4,   16,  -10, 
             -35,  -10,  -22,  -16,   -3,  -22,    4,  -30,
        }, 
        { // rook
              13,   11,   19,   19,   19,   18,   10,   18, 
              23,   29,   27,   29,   29,   29,   28,   12, 
              14,   18,   19,   19,   18,    8,   15,   -2, 
              11,    8,   17,   19,   19,   -3,    6,   -6, 
               0,   11,   17,    8,   -6,   -8,   -2,  -15, 
             -17,   -3,   -5,    3,  -11,  -18,   11,  -10, 
             -24,   -5,   -6,  -10,  -15,  -19,  -19,  -24, 
              -8,   -5,   -2,   -1,    3,   -4,  -15,  -19, 
        }, 
        { // queen
             -15,    5,    9,   10,    0,    8,   -8,   -2, 
             -11,  -12,    1,   17,   -5,   18,    1,    6, 
             -10,   -5,    0,   -6,    6,   -9,  -16,  -29, 
              -3,    7,    6,    6,   -5,   -8,  -19,  -19, 
               1,    0,    6,    9,   12,  -10,   -2,  -12, 
              -7,   10,   12,    3,    6,   -5,   18,  -21, 
             -21,    2,    5,    9,    5,    4,   -1,  -21, 
              -1,  -21,  -15,    7,    1,  -12,  -29,  -31,
        }, 
        { // king 
             -21,  -21,  -21,  -31,  -31,  -21,  -21,  -29, 
             -14,  -21,  -29,  -37,  -34,  -21,  -21,  -11, 
             -16,  -29,  -38,  -63,  -46,  -31,  -21,  -11, 
             -24,  -24,  -44,  -55,  -52,  -33,  -21,  -11, 
             -11,  -27,  -30,  -40,  -27,  -12,  -11,  -18, 
             -10,  -19,  -19,   -3,   -1,   -3,   -4,  -17, 
               6,   23,   17,   14,   19,    3,   38,    2, 
              10,   27,   14,  -19,    5,   -9,   37,    4, 
        }
    }, 
    {
        { // pawn
               0,    0,    0,    0,    0,    0,    0,    0, 
             116,  119,  119,   93,   99,  111,  119,   91, 
              59,   59,   58,   39,   29,   28,   48,   55, 
              46,   34,   24,   11,   11,   11,   26,   20, 
              30,   24,   15,    1,    4,   14,   16,   11, 
              19,   19,   18,   13,   15,   20,   14,    5, 
              24,   15,    7,  -20,   -1,   13,   13,    6, 
               0,    0,    0,    0,    0,    0,    0,    0,
        }, 
        { // knight 
             -69,  -33,  -11,  -11,  -29,  -34,  -33,  -69, 
             -28,  -17,   10,    3,   -1,   11,  -17,  -51, 
             -14,    1,   29,   23,   12,   17,    0,  -34, 
             -11,   15,   28,   27,   19,   11,    3,  -16, 
             -13,   -6,   21,   19,    7,    7,   -2,  -27, 
             -39,   -5,   13,   14,    1,    2,  -14,  -40, 
             -41,  -27,  -19,  -14,  -14,  -11,  -16,  -29, 
             -62,  -36,  -37,  -27,  -25,  -31,  -31,  -57,
        }, 
        { // bishop 
              -2,   -1,    4,    8,   -5,   -4,   -5,  -17, 
               5,   19,   16,   13,    7,    8,    3,  -11, 
               6,   19,   19,   13,   12,   23,    8,   -3, 
               0,   16,   16,   28,   15,    2,    9,   -2, 
               0,   -5,   24,   10,   11,   14,    0,   -9, 
             -14,   12,   13,   12,   17,    6,    9,   -1, 
             -24,    4,    2,    0,    2,   -4,   -4,  -11, 
             -31,  -17,  -29,   -6,  -12,  -14,  -25,  -22,
        }, 
        { // rook
              -3,   13,   19,   19,   19,   19,   10,   17, 
              24,   29,   29,   24,   21,   19,   16,   17, 
              14,   19,   19,   16,   15,   19,    9,    9, 
              14,   19,   19,   15,    5,    4,   -8,   -3, 
              14,   16,   17,    6,   -1,   -5,   -7,  -16, 
               3,    5,    9,    3,  -11,   -4,   -5,  -16, 
             -17,  -16,   -3,   -5,  -19,  -15,  -19,  -24, 
              -8,   -7,    3,   12,   -6,   -4,  -13,  -12,
        }, 
        { // queen
             -14,    3,    4,   -7,  -12,    6,   -5,   -4, 
              -6,    5,    6,   17,   11,   16,    4,   -5, 
               8,    7,   21,   15,   -2,   -6,    4,  -15, 
              -3,    5,   20,   10,   10,  -14,  -12,  -19, 
               2,   10,   17,   19,   12,    1,    0,  -16, 
              -8,    4,   15,    5,   11,    2,   -4,  -21, 
             -11,    3,    7,    4,    4,  -11,  -19,  -28, 
             -21,  -25,  -12,   -3,   -5,  -20,  -29,  -39,
        }, 
        { // king 
             -51,  -21,  -12,  -12,  -11,  -11,  -21,  -54, 
             -29,   -1,   11,    6,   11,   19,   -1,  -21, 
             -12,   11,   18,   21,   22,   29,   19,  -11, 
             -11,    8,   15,   21,   26,   22,   24,  -11, 
             -32,    1,   11,   17,   17,   15,    8,  -22, 
             -36,   -6,   -1,    7,    9,    7,    3,  -27, 
             -38,  -17,  -10,   -6,   -9,   -7,  -21,  -36, 
             -57,  -50,  -29,  -35,  -38,  -37,  -49,  -69, 
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

    EVAL_1PARAM(ConnectedRooks); 
    EVAL_1PARAM(OpenFile); 
    EVAL_PARAM(PassedPawnValues, 8, 0, 0); 

    // EVAL_1PARAM(BishopPair); 
    // EVAL_PARAM(PawnStructureValues, 4, 0, 0); 
    // EVAL_PARAM(PcTypeValues, 5, 0, 0); 
    // EVAL_PARAM(AttackUnitValues, 64, 0, 0); 
    // EVAL_PARAM(PcSq, 64, 6, 2); 

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

static inline int EvalRooks(const Game* g, Color col) 
{
    Piece pc = MakePc(PC_R, col); 
    BBoard board = g->Pieces[pc]; 

    if (g->Counts[pc] >= 2) 
    {
        return ConnectedRooks * ((RAttacks(Lsb(board), g->All) & board) != 0); 
    }
    
    return 0; 
}

static inline int EvalOpenFiles(const Game* g, Color col) 
{
    BBoard rAttackers = g->Pieces[MakePc(PC_R, col)] | g->Pieces[MakePc(PC_Q, col)]; 
    BBoard pawns = g->Pieces[PC_WP] | g->Pieces[PC_BP]; 
    int total = 0; 

    for (int i = 0; i < 8; i++) 
    {
        total += ((Files[i] & pawns) == 0) * ((Files[i] & rAttackers) != 0); 
    }

    return total * OpenFile; 
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

int EvaluateVerbose(const Game* g, int ply, int nMoves, bool draw, int contempt, bool verbose) 
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
            return (99999 - ply) * (-1 + 2 * g->Turn); 
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

    int cwr = EvalRooks(g, COL_W); 
    int cbr = EvalRooks(g, COL_B); 
    eval += cwr - cbr; 

    int wOpen = EvalOpenFiles(g, COL_W); 
    int bOpen = EvalOpenFiles(g, COL_B); 
    eval += wOpen - bOpen; 

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
        printf("Connected rooks (%d-%d): %d\n", cwr, cbr, cwr - cbr); 
        printf("Open files (%d-%d): %d\n", wOpen, bOpen, wOpen - bOpen); 
        printf("Final evaluation: %d\n", eval); 
    }

    return eval; 
}
