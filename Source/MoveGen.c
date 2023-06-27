#include "MoveGen.h" 
#include "BBoard.h"
#include "Game.h"
#include "Magic.h" 
#include "Move.h"
#include "Piece.h"

#include <stdio.h> 
#include <stdlib.h> 

#define TGT_SQ_PARAMS BBoard okSquares, BBoard* pinDirs, const U8* pinIdx
#define TGT_SQ_PARAM_TYPES BBoard, BBoard *, const U8 *
#define TGT_SQS ( okSquares & ( ((GetBit(pinDirs[pinIdx[sq]], sq) == 0) * ALL_BITS) | pinDirs[pinIdx[sq]] ) )

MvList* NewMvList(void) 
{
    MvList* moves = malloc(sizeof(MvList)); 
    moves->Size = 0; 
    return moves; 
} 

void FreeMvList(MvList* moves) 
{
    free(moves); 
}

static inline void InitMvList(MvList* moves) 
{
    moves->Size = 0; 
}

static inline void InitMvInfo(MvInfo* info) 
{
    info->NumPieces = info->NumMoves = 0; 
}

static inline void AddMovesToInfo(MvInfo* info, Piece pc, Square from, BBoard moves, int nMoves) 
{
    info->From[info->NumPieces] = from; 
    info->Pieces[info->NumPieces] = pc; 
    info->Moves[info->NumPieces++] = moves; 
    info->NumMoves += nMoves; 
}

/**
 * Applies the mask and then adds any extra bits. 
 * `mask` will remove attackers from being considered 
 * `add` will only add blockers, they have no piece type or color 
 */
static inline bool IsAttackedMaskAdd(const Game* g, Square sq, Color chkCol, BBoard mask, BBoard add) 
{
    Color col = chkCol; 
    Color opp = OppCol(col); 

    BBoard occ = (g->All & mask) | add; 

    // b,r,q are not used directly 
    // applying mask to the aggregate BBoards is 1 fewer "&" 

    BBoard oppP = g->Pieces[MakePc(PC_P, opp)] & mask; 
    BBoard oppN = g->Pieces[MakePc(PC_N, opp)] & mask; 
    BBoard oppB = g->Pieces[MakePc(PC_B, opp)]; 
    BBoard oppR = g->Pieces[MakePc(PC_R, opp)]; 
    BBoard oppQ = g->Pieces[MakePc(PC_Q, opp)]; 
    BBoard oppK = g->Pieces[MakePc(PC_K, opp)] & mask; 

    BBoard oppRQ = (oppR | oppQ) & mask; 
    BBoard oppBQ = (oppB | oppQ) & mask; 

    BBoard chk = (RAttacks(sq, occ) & oppRQ) 
               | (BAttacks(sq, occ) & oppBQ) 
               | (oppK & MovesK[sq]) 
               | (oppN & MovesN[sq]) 
               | (oppP & AttacksP[col][sq]);     

    return chk != 0; 
}

/**
 * Applies the mask and then adds any extra bits. 
 * `mask` will remove attackers from being considered 
 */
static inline bool IsAttackedMask(const Game* g, Square sq, Color chkCol, BBoard mask) 
{
    Color col = chkCol; 
    Color opp = OppCol(col); 

    BBoard occ = (g->All & mask); 

    // b,r,q are not used directly 
    // applying mask to the aggregate BBoards is 1 fewer "&" 

    BBoard oppP = g->Pieces[MakePc(PC_P, opp)] & mask; 
    BBoard oppN = g->Pieces[MakePc(PC_N, opp)] & mask; 
    BBoard oppB = g->Pieces[MakePc(PC_B, opp)]; 
    BBoard oppR = g->Pieces[MakePc(PC_R, opp)]; 
    BBoard oppQ = g->Pieces[MakePc(PC_Q, opp)]; 
    BBoard oppK = g->Pieces[MakePc(PC_K, opp)] & mask; 

    BBoard oppRQ = (oppR | oppQ) & mask; 
    BBoard oppBQ = (oppB | oppQ) & mask; 

    BBoard chk = (RAttacks(sq, occ) & oppRQ) 
               | (BAttacks(sq, occ) & oppBQ) 
               | (oppK & MovesK[sq]) 
               | (oppN & MovesN[sq]) 
               | (oppP & AttacksP[col][sq]);     

    return chk != 0; 
}

/**
 * Applies the mask and then checks if the square is attacked. 
 * `mask` will ONLY remove pieces from the aggregate BBoard, those squares can still be included in attackers  
 */
static inline bool IsAttackedColMask(const Game* g, Square sq, Color chkCol, BBoard mask) 
{
    Color col = chkCol; 
    Color opp = OppCol(col); 

    BBoard occ = (g->All & mask); 

    // b,r,q are not used directly 
    // applying mask to the aggregate BBoards is 1 fewer "&" 

    BBoard oppP = g->Pieces[MakePc(PC_P, opp)]; 
    BBoard oppN = g->Pieces[MakePc(PC_N, opp)]; 
    BBoard oppB = g->Pieces[MakePc(PC_B, opp)]; 
    BBoard oppR = g->Pieces[MakePc(PC_R, opp)]; 
    BBoard oppQ = g->Pieces[MakePc(PC_Q, opp)]; 
    BBoard oppK = g->Pieces[MakePc(PC_K, opp)]; 

    BBoard oppRQ = (oppR | oppQ); 
    BBoard oppBQ = (oppB | oppQ); 

    BBoard chk = (RAttacks(sq, occ) & oppRQ) 
               | (BAttacks(sq, occ) & oppBQ) 
               | (oppK & MovesK[sq]) 
               | (oppN & MovesN[sq]) 
               | (oppP & AttacksP[col][sq]);     

    return chk != 0; 
}

#define FN_GEN_P_INFO(col, shift1, shift2, shift2Rank, proRank) \
    Piece pc = MakePc(PC_P, col); \
    BBoard pcs = g->Pieces[pc]; \
    BBoard opp = g->Colors[OppCol(col)]; \
    BBoard empty = ~g->All; \
    Square ksq = Lsb(g->Pieces[MakePc(PC_K, col)]); \
    FOR_EACH_BIT(pcs, \
    {\
        BBoard pos = BB[sq]; \
        BBoard to = (shift1(pos) | (shift2(pos & shift2Rank) & shift1(empty))) & empty; \
        to |= AttacksP[col][sq] & opp; \
        to &= TGT_SQS; \
        if (g->EP != NO_SQ && (pos & AttacksP[OppCol(col)][g->EP])) \
        {\
            if (!IsAttackedMaskAdd(g, ksq, col, ~(BB[sq] | BB[g->EP - 8 * ColSign(col)]), BB[g->EP])) \
            {\
                to |= BB[g->EP]; \
            }\
        }\
        AddMovesToInfo(info, pc, sq, to, Popcnt(to) + Popcnt(to & proRank) * 3); \
    });\

#define FN_GEN_K_INFO(col, letter, rank) \
    Piece pc = MakePc(PC_K, col); \
    BBoard pcs = g->Pieces[pc]; \
    Square ksq = Lsb(pcs); \
    BBoard to = 0; \
    int nMoves = 0; \
    BBoard occ = g->All & ~pcs; \
    FOR_EACH_BIT(MovesK[ksq] & g->Movement, \
    {\
        if (!IsAttackedColMask(g, sq, col, occ)) \
        {\
            to = SetBit(to, sq); \
            nMoves++; \
        }\
    });\
    if (((g->Castle & CASTLE_##letter##K) != 0) & ((g->All & EMPTY_##letter##K) == 0)) \
    {\
        if (!IsAttackedColMask(g, E##rank, col, occ) && !IsAttackedColMask(g, F##rank, col, occ) && !IsAttackedColMask(g, G##rank, col, occ)) \
        {\
            to = SetBit(to, G##rank); \
            nMoves++; \
        }\
    }\
    if (((g->Castle & CASTLE_##letter##Q) != 0) & ((g->All & EMPTY_##letter##Q) == 0)) \
    {\
        if (!IsAttackedColMask(g, E##rank, col, occ) && !IsAttackedColMask(g, D##rank, col, occ) && !IsAttackedColMask(g, C##rank, col, occ)) \
        {\
            to = SetBit(to, C##rank); \
            nMoves++; \
        }\
    }\
    AddMovesToInfo(info, pc, ksq, to, nMoves); 

#define FN_GEN_N_INFO(col) \
    Piece pc = MakePc(PC_N, col); \
    BBoard pcs = g->Pieces[pc]; \
    FOR_EACH_BIT(pcs, \
    {\
        BBoard to = MovesN[sq] & g->Movement & TGT_SQS; \
        AddMovesToInfo(info, pc, sq, to, Popcnt(to)); \
    });

#define FN_GEN_B_INFO(col) \
    Piece pc = MakePc(PC_B, col); \
    BBoard pcs = g->Pieces[pc]; \
    FOR_EACH_BIT(pcs, \
    {\
        BBoard to = MagicBSlide[sq][((MagicBMask[sq] & g->All) * MagicB[sq]) >> MagicBShift[sq]]; \
        to &= g->Movement & TGT_SQS; \
        AddMovesToInfo(info, pc, sq, to, Popcnt(to)); \
    });

#define FN_GEN_R_INFO(col) \
    Piece pc = MakePc(PC_R, col); \
    BBoard pcs = g->Pieces[pc]; \
    FOR_EACH_BIT(pcs, \
    {\
        BBoard to = MagicRSlide[sq][((MagicRMask[sq] & g->All) * MagicR[sq]) >> MagicRShift[sq]]; \
        to &= g->Movement & TGT_SQS; \
        AddMovesToInfo(info, pc, sq, to, Popcnt(to)); \
    });

#define FN_GEN_Q_INFO(col) \
    Piece pc = MakePc(PC_Q, col); \
    BBoard pcs = g->Pieces[pc]; \
    FOR_EACH_BIT(pcs, \
    {\
        BBoard to = MagicBSlide[sq][((MagicBMask[sq] & g->All) * MagicB[sq]) >> MagicBShift[sq]] \
                  | MagicRSlide[sq][((MagicRMask[sq] & g->All) * MagicR[sq]) >> MagicRShift[sq]]; \
        to &= g->Movement & TGT_SQS; \
        AddMovesToInfo(info, pc, sq, to, Popcnt(to)); \
    });

static inline void GenWPInfo(const Game* g, TGT_SQ_PARAMS, MvInfo* info) { FN_GEN_P_INFO(COL_W, ShiftN, ShiftNN, RANK_2, RANK_8); } 
static inline void GenBPInfo(const Game* g, TGT_SQ_PARAMS, MvInfo* info) { FN_GEN_P_INFO(COL_B, ShiftS, ShiftSS, RANK_7, RANK_1); } 
static inline void GenWKInfo(const Game* g, MvInfo* info) { FN_GEN_K_INFO(COL_W, W, 1); } 
static inline void GenBKInfo(const Game* g, MvInfo* info) { FN_GEN_K_INFO(COL_B, B, 8); } 
static inline void GenWNInfo(const Game* g, TGT_SQ_PARAMS, MvInfo* info) { FN_GEN_N_INFO(COL_W); } 
static inline void GenBNInfo(const Game* g, TGT_SQ_PARAMS, MvInfo* info) { FN_GEN_N_INFO(COL_B); } 
static inline void GenWBInfo(const Game* g, TGT_SQ_PARAMS, MvInfo* info) { FN_GEN_B_INFO(COL_W); } 
static inline void GenBBInfo(const Game* g, TGT_SQ_PARAMS, MvInfo* info) { FN_GEN_B_INFO(COL_B); } 
static inline void GenWRInfo(const Game* g, TGT_SQ_PARAMS, MvInfo* info) { FN_GEN_R_INFO(COL_W); } 
static inline void GenBRInfo(const Game* g, TGT_SQ_PARAMS, MvInfo* info) { FN_GEN_R_INFO(COL_B); } 
static inline void GenWQInfo(const Game* g, TGT_SQ_PARAMS, MvInfo* info) { FN_GEN_Q_INFO(COL_W); } 
static inline void GenBQInfo(const Game* g, TGT_SQ_PARAMS, MvInfo* info) { FN_GEN_Q_INFO(COL_B); } 

// void GenMvInfo(const Game* g, MvInfo* info) 
#define FN_GEN_MVINFO(col, letter) \
{\
    InitMvInfo(info); \
    Color opp = OppCol(col); \
    BBoard oppP = g->Pieces[MakePc(PC_P, opp)]; \
    BBoard oppN = g->Pieces[MakePc(PC_N, opp)]; \
    BBoard oppB = g->Pieces[MakePc(PC_B, opp)]; \
    BBoard oppR = g->Pieces[MakePc(PC_R, opp)]; \
    BBoard oppQ = g->Pieces[MakePc(PC_Q, opp)]; \
    BBoard oppK = g->Pieces[MakePc(PC_K, opp)]; \
    BBoard target = g->Pieces[MakePc(PC_K, col)]; \
    BBoard ksq = Lsb(target); \
    BBoard colOcc = g->Colors[col]; \
    /* relevant opp sliding pieces for each direction */ \
    BBoard oppRQ = oppR | oppQ; \
    BBoard oppBQ = oppB | oppQ; \
    /* if the king were a rook, how far could it move */ \
    BBoard kRHit  = RAttacks(ksq, g->All); \
    /* used to remove ally pieces (check for pinned pieces) */ \
    BBoard kRNoCol = ~(kRHit & colOcc); \
    /* check if removing one ally piece reveals an attacker */ \
    BBoard kRDHit = RAttacks(ksq, g->All & kRNoCol) & oppRQ; \
    /* do the above for bishop*/ \
    BBoard kBHit  = BAttacks(ksq, g->All); \
    BBoard kBNoCol = ~(kBHit & colOcc); \
    BBoard kBDHit = BAttacks(ksq, g->All & kBNoCol) & oppBQ; \
    /* sliding piece checkers*/ \
    BBoard chkSlide = (kRHit & oppRQ) | (kBHit & oppBQ); \
    /* all checkers (remaining pieces can only be captured or force the king to retreat)*/ \
    BBoard chk = chkSlide \
               | (oppK & MovesK[ksq]) \
               | (oppN & MovesN[ksq]) \
               | (oppP & AttacksP[col][ksq]); \
    BBoard dirs[9] = { 0 }; \
    const U8* pIdx = PinIdx[ksq]; \
    FOR_EACH_BIT(kRDHit | kBDHit, \
    {\
        dirs[pIdx[sq]] = SlideTo[ksq][sq]; \
    });\
    if (chk == 0) \
    {\
        Gen##letter##PInfo(g, ALL_BITS, dirs, pIdx, info); \
        Gen##letter##NInfo(g, ALL_BITS, dirs, pIdx, info); \
        Gen##letter##BInfo(g, ALL_BITS, dirs, pIdx, info); \
        Gen##letter##RInfo(g, ALL_BITS, dirs, pIdx, info); \
        Gen##letter##QInfo(g, ALL_BITS, dirs, pIdx, info); \
        Gen##letter##KInfo(g, info); \
    }\
    else \
    {\
        int nChecks = Popcnt(chk); \
        if (nChecks == 1) \
        {\
            Square attacker = Lsb(chk); \
            BBoard okSquares = SlideTo[ksq][attacker] | BB[attacker]; \
            Gen##letter##PInfo(g, okSquares, dirs, pIdx, info); \
            Gen##letter##NInfo(g, okSquares, dirs, pIdx, info); \
            Gen##letter##BInfo(g, okSquares, dirs, pIdx, info); \
            Gen##letter##RInfo(g, okSquares, dirs, pIdx, info); \
            Gen##letter##QInfo(g, okSquares, dirs, pIdx, info); \
            Gen##letter##KInfo(g, info); \
        }\
        else \
        {\
            Gen##letter##KInfo(g, info); \
        }\
    }\
}

void GenMvInfo(const Game* g, MvInfo* info) 
{
    if (g->Turn == COL_W) 
    {
        FN_GEN_MVINFO(COL_W, W); 
    }
    else 
    {
        FN_GEN_MVINFO(COL_B, B); 
    }
}

static inline BBoard DiscRAttackers(const Game* g, Square sq, Color chkCol, BBoard mask, BBoard add) 
{
    Color opp = OppCol(chkCol); 

    BBoard occ = (g->All & mask) | add; 
    BBoard oppR = g->Pieces[MakePc(PC_R, opp)]; 
    BBoard oppQ = g->Pieces[MakePc(PC_Q, opp)]; 
    BBoard oppRQ = (oppR | oppQ) & mask; 

    return RAttacks(sq, occ) & oppRQ; 
}

static inline BBoard DiscBAttackers(const Game* g, Square sq, Color chkCol, BBoard mask, BBoard add) 
{
    Color opp = OppCol(chkCol); 

    BBoard occ = (g->All & mask) | add; 
    BBoard oppB = g->Pieces[MakePc(PC_B, opp)]; 
    BBoard oppQ = g->Pieces[MakePc(PC_Q, opp)]; 
    BBoard oppBQ = (oppB | oppQ) & mask; 

    return BAttacks(sq, occ) & oppBQ; 
}

static inline BBoard DiscAttackers(const Game* g, Square sq, Color chkCol, BBoard mask, BBoard add, int pIdx) 
{
    if (pIdx == NumCheckDirs) 
    {
        return 0; 
    }
    else if (pIdx <= CheckDirRookEnd) 
    {
        return DiscRAttackers(g, sq, chkCol, mask, add); 
    }
    else 
    {
        return DiscBAttackers(g, sq, chkCol, mask, add); 
    }
}

#define DISC_ATTACKERS(col) (DiscAttackers(g, oppKSq, OppCol(col), ~BB[from], BB[sq], pIdx[from]))
#define CHECK_N (MovesN[sq] & oppK)
#define CHECK_B (BAttacks(sq, g->All & ~BB[from]) & oppK)
#define CHECK_R (RAttacks(sq, g->All & ~BB[from]) & oppK)
#define CHECK_Q ((BAttacks(sq, g->All & ~BB[from]) | RAttacks(sq, g->All & ~BB[from])) & oppK)

#define FN_GEN_P_MOVES(col, proRank, colLetter, oppLetter) \
{\
    FOR_EACH_BIT(to & ~proRank, \
    {\
        moves->Moves[moves->Size++] = MakeEPMove(from, sq, PC_##colLetter##P, (sq == g->EP) ? PC_##oppLetter##P : PcAt(g, sq), sq == g->EP, (sq == g->EP) ? (\
            (AttacksP[col][sq] & oppK) | \
            DiscAttackers(g, oppKSq, OppCol(col), ~(BB[from] | BB[sq - 8 * ColSign(col)]), BB[sq], pIdx[from]) | \
            DiscAttackers(g, oppKSq, OppCol(col), ~(BB[from] | BB[sq - 8 * ColSign(col)]), BB[sq], pIdx[sq - 8 * ColSign(col)]) \
        ) : (\
            (AttacksP[col][sq] & oppK) | DISC_ATTACKERS(col) \
        )); \
    });\
    FOR_EACH_BIT(to & proRank, \
    {\
        Piece at = PcAt(g, sq); \
        BBoard disc = DiscAttackers(g, oppKSq, OppCol(col), ~BB[from], BB[sq], pIdx[from]);\
        moves->Moves[moves->Size++] = MakeProMove(from, sq, PC_##colLetter##P, PC_##colLetter##Q, at, (CHECK_Q) | disc); \
        moves->Moves[moves->Size++] = MakeProMove(from, sq, PC_##colLetter##P, PC_##colLetter##R, at, (CHECK_R) | disc); \
        moves->Moves[moves->Size++] = MakeProMove(from, sq, PC_##colLetter##P, PC_##colLetter##B, at, (CHECK_B) | disc); \
        moves->Moves[moves->Size++] = MakeProMove(from, sq, PC_##colLetter##P, PC_##colLetter##N, at, (CHECK_N) | disc); \
    });\
}

static inline void GenWPMoves(const Game* g, Square from, BBoard to, Square oppKSq, BBoard oppK, const U8* pIdx, MvList* moves) 
{
    FN_GEN_P_MOVES(COL_W, RANK_8, W, B); 
}

static inline void GenBPMoves(const Game* g, Square from, BBoard to, Square oppKSq, BBoard oppK, const U8* pIdx, MvList* moves) 
{
    FN_GEN_P_MOVES(COL_B, RANK_1, B, W); 
}

#define FN_GEN_K_MOVES(col, letter, rank) \
    /* if any castle flag is enabled, then the king has not moved */ \
    BBoard castle = to & (((g->Castle & CASTLE_##letter) != 0) * (1ULL << G##rank | 1ULL << C##rank)); \
    to &= ~castle; \
    FOR_EACH_BIT(to, \
    {\
        moves->Moves[moves->Size++] = MakeMove(from, sq, PC_##letter##K, PcAt(g, sq), DiscAttackers(g, oppKSq, OppCol(g->Turn), ~BB[from], BB[sq], pIdx[from])); \
    });\
    FOR_EACH_BIT(castle, \
    {\
        switch (sq) \
        {\
            case G##rank: moves->Moves[moves->Size++] = MakeCastleMove(E##rank, G##rank, PC_##letter##K, MOVE_CASTLE_##letter##K, RAttacks(F##rank, g->All & ~(1ULL << E##rank)) & oppK); break; \
            case C##rank: moves->Moves[moves->Size++] = MakeCastleMove(E##rank, C##rank, PC_##letter##K, MOVE_CASTLE_##letter##Q, RAttacks(D##rank, g->All & ~(1ULL << E##rank)) & oppK); break; \
        }\
    });\

static inline void GenWKMoves(const Game* g, Square from, BBoard to, Square oppKSq, BBoard oppK, const U8* pIdx, MvList* moves) 
{
    FN_GEN_K_MOVES(COL_W, W, 1); 
}

static inline void GenBKMoves(const Game* g, Square from, BBoard to, Square oppKSq, BBoard oppK, const U8* pIdx, MvList* moves) 
{
    FN_GEN_K_MOVES(COL_B, B, 8); 
}

#define GEN_MOVES(col, atk) \
    FOR_EACH_BIT(to, \
    {\
        moves->Moves[moves->Size++] = MakeMove(from, sq, pc, PcAt(g, sq), (atk) | DISC_ATTACKERS(col)); \
    });

#define FN_GEN_MOVES(col, letter) \
{\
    BBoard oppK = g->Pieces[MakePc(PC_K, OppCol(col))]; \
    Square oppKSq = Lsb(oppK); \
    const U8* pIdx = PinIdx[oppKSq]; \
    for (int pcId = 0; pcId < info->NumPieces; pcId++) \
    {\
        BBoard to = info->Moves[pcId]; \
        Piece pc = info->Pieces[pcId]; \
        Piece type = GetNoCol(pc); \
        Square from = info->From[pcId]; \
        switch (type) \
        {\
            case PC_P: Gen##letter##PMoves(g, from, info->Moves[pcId], oppKSq, oppK, pIdx, moves); break; \
            case PC_K: Gen##letter##KMoves(g, from, info->Moves[pcId], oppKSq, oppK, pIdx, moves); break; \
            case PC_N: GEN_MOVES(col, CHECK_N); break; \
            case PC_B: GEN_MOVES(col, CHECK_B); break; \
            case PC_R: GEN_MOVES(col, CHECK_R); break; \
            case PC_Q: GEN_MOVES(col, CHECK_Q); break; \
        }\
    }\
} 

void GenMovesMvInfo(const Game* g, const MvInfo* info, MvList* moves) 
{
    if (g->Turn == COL_W) 
    {
        FN_GEN_MOVES(COL_W, W); 
    }
    else 
    {
        FN_GEN_MOVES(COL_B, B); 
    }
}