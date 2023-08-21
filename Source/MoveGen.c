/**
 * @file MoveGen.c
 * @author Nicholas Hamilton 
 * @date 2023-02-20
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Implements legal piece movement. 
 */

#include "MoveGen.h" 
#include "Bitboard.h"
#include "Game.h"
#include "Magic.h" 
#include "Move.h"
#include "Piece.h"

#include <stdio.h> 
#include <stdlib.h> 

/**
 * Parameters to include in move generation functions.
 */
#define TARGET_SQUARE_PARAMS Bitboard okSquares, Bitboard* pinDirs, const U8* pinIndex

/**
 * Filter out moves ignoring absolute pins. 
 */
#define TARGET_SQUARES ( okSquares & ( ((GetBit(pinDirs[pinIndex[sq]], sq) == 0) * AllBits) | pinDirs[pinIndex[sq]] ) )

MoveList* NewMoveList(void) 
{
    MoveList* moves = malloc(sizeof(MoveList)); 
    moves->Size = 0; 
    return moves; 
} 

void FreeMoveList(MoveList* moves) 
{
    free(moves); 
}

/**
 * Prepares move info for generation. 
 * 
 * @param info Move info 
 */
static inline void InitMoveInfo(MoveInfo* info) 
{
    info->NumPieces = info->NumMoves = 0; 
}

/**
 * Add all possible movement squares for a piece.
 * 
 * @param info Move info
 * @param pc The piece 
 * @param from Start square 
 * @param moves Target squares 
 * @param nMoves Total number of moves
 */
static inline void AddMovesToInfo(MoveInfo* info, Piece pc, Square from, Bitboard moves, int nMoves) 
{
    info->From[info->NumPieces] = from; 
    info->Pieces[info->NumPieces] = pc; 
    info->Moves[info->NumPieces++] = moves; 
    info->NumMoves += nMoves; 
}

/**
 * Checks if a square is attacked. 
 * Applies the mask and then adds any extra bits. 
 * 
 * @param g The game 
 * @param sq The square 
 * @param chkCol Friendly color
 * @param mask Remove attackers from being considered 
 * @param add Add blockers to occupants
 * @return True if the square is attacked, otherwise false
 */
static inline bool IsAttackedMaskAdd(const Game* g, Square sq, Color chkCol, Bitboard mask, Bitboard add) 
{
    Color col = chkCol; 
    Color opp = OppositeColor(col); 

    Bitboard occ = (g->All & mask) | add; 

    // b,r,q are not used directly 
    // applying mask to the aggregate Bitboards is 1 fewer "&" 

    Bitboard oppP = g->Pieces[MakePiece(PieceP, opp)] & mask; 
    Bitboard oppN = g->Pieces[MakePiece(PieceN, opp)] & mask; 
    Bitboard oppB = g->Pieces[MakePiece(PieceB, opp)]; 
    Bitboard oppR = g->Pieces[MakePiece(PieceR, opp)]; 
    Bitboard oppQ = g->Pieces[MakePiece(PieceQ, opp)]; 
    Bitboard oppK = g->Pieces[MakePiece(PieceK, opp)] & mask; 

    Bitboard oppRQ = (oppR | oppQ) & mask; 
    Bitboard oppBQ = (oppB | oppQ) & mask; 

    Bitboard chk = (RAttacks(sq, occ) & oppRQ) 
               | (BAttacks(sq, occ) & oppBQ) 
               | (oppK & MovesK[sq]) 
               | (oppN & MovesN[sq]) 
               | (oppP & AttacksP[col][sq]);     

    return chk != 0; 
}

/**
 * Checks if a square is attacked. 
 * 
 * @param g The game 
 * @param sq The square 
 * @param chkCol Friendly color
 * @param mask Remove attackers from being considered 
 * @return True if the square is attacked, otherwise false
 */
static inline bool IsAttackedMask(const Game* g, Square sq, Color chkCol, Bitboard mask) 
{
    Color col = chkCol; 
    Color opp = OppositeColor(col); 

    Bitboard occ = (g->All & mask); 

    // b,r,q are not used directly 
    // applying mask to the aggregate Bitboards is 1 fewer "&" 

    Bitboard oppP = g->Pieces[MakePiece(PieceP, opp)] & mask; 
    Bitboard oppN = g->Pieces[MakePiece(PieceN, opp)] & mask; 
    Bitboard oppB = g->Pieces[MakePiece(PieceB, opp)]; 
    Bitboard oppR = g->Pieces[MakePiece(PieceR, opp)]; 
    Bitboard oppQ = g->Pieces[MakePiece(PieceQ, opp)]; 
    Bitboard oppK = g->Pieces[MakePiece(PieceK, opp)] & mask; 

    Bitboard oppRQ = (oppR | oppQ) & mask; 
    Bitboard oppBQ = (oppB | oppQ) & mask; 

    Bitboard chk = (RAttacks(sq, occ) & oppRQ) 
               | (BAttacks(sq, occ) & oppBQ) 
               | (oppK & MovesK[sq]) 
               | (oppN & MovesN[sq]) 
               | (oppP & AttacksP[col][sq]);     

    return chk != 0; 
}

/**
 * Checks if a square is attacked. 
 * Applies the mask and then adds any extra bits. 
 * 
 * @param g The game 
 * @param sq The square 
 * @param chkCol Friendly color
 * @param mask ONLY remove attackers from the aggregate Bitboard, 
 *             those squares can still be included in attackers 
 * @return True if the square is attacked, otherwise false
 */
static inline bool IsAttackedColMask(const Game* g, Square sq, Color chkCol, Bitboard mask) 
{
    Color col = chkCol; 
    Color opp = OppositeColor(col); 

    Bitboard occ = (g->All & mask); 

    // b,r,q are not used directly 
    // applying mask to the aggregate Bitboards is 1 fewer "&" 

    Bitboard oppP = g->Pieces[MakePiece(PieceP, opp)]; 
    Bitboard oppN = g->Pieces[MakePiece(PieceN, opp)]; 
    Bitboard oppB = g->Pieces[MakePiece(PieceB, opp)]; 
    Bitboard oppR = g->Pieces[MakePiece(PieceR, opp)]; 
    Bitboard oppQ = g->Pieces[MakePiece(PieceQ, opp)]; 
    Bitboard oppK = g->Pieces[MakePiece(PieceK, opp)]; 

    Bitboard oppRQ = (oppR | oppQ); 
    Bitboard oppBQ = (oppB | oppQ); 

    Bitboard chk = (RAttacks(sq, occ) & oppRQ) 
               | (BAttacks(sq, occ) & oppBQ) 
               | (oppK & MovesK[sq]) 
               | (oppN & MovesN[sq]) 
               | (oppP & AttacksP[col][sq]);     

    return chk != 0; 
}

/**
 * Adds pawn moves to move info. 
 * 
 * @param col Color to move 
 * @param shift1 Function to push pawns forward one square 
 * @param shift2 Function to push pawns forward two squares 
 * @param shift2Rank Rank pawns must be on to move two squares 
 * @param proRank Rank pawns must be on to promote when moved
 */
#define FN_GEN_P_INFO(col, shift1, shift2, shift2Rank, proRank) \
    Piece pc = MakePiece(PieceP, col); \
    Bitboard pcs = g->Pieces[pc]; \
    Bitboard opp = g->Colors[OppositeColor(col)]; \
    Bitboard empty = ~g->All; \
    Square ksq = LeastSigBit(g->Pieces[MakePiece(PieceK, col)]); \
    FOR_EACH_BIT(pcs, \
    {\
        Bitboard pos = Bits[sq]; \
        /* push forward 1 or 2 squares */ \
        Bitboard to = (shift1(pos) | (shift2(pos & shift2Rank) & shift1(empty))) & empty; \
        /* captures */ \
        to |= AttacksP[col][sq] & opp; \
        /* pins */ \
        to &= TARGET_SQUARES; \
        if (g->EnPassant != NoSquare && (pos & AttacksP[OppositeColor(col)][g->EnPassant])) \
        {\
            /* make sure pawn isn't pinned for en passant */ \
            if (!IsAttackedMaskAdd(g, ksq, col, ~(Bits[sq] | Bits[g->EnPassant - 8 * ColorSign(col)]), Bits[g->EnPassant])) \
            {\
                to |= Bits[g->EnPassant]; \
            }\
        }\
        /* moving a pawn to a promotion square has 4 possible moves instead of 1 */ \
        AddMovesToInfo(info, pc, sq, to, PopCount(to) + PopCount(to & proRank) * 3); \
    });\

/**
 * Adds king moves to move info. 
 * 
 * @param col Color to move 
 * @param letter Color's letter (W or B)
 * @param rank 1-indexed starting rank of the king (1 or 8)
 */
#define FN_GEN_K_INFO(col, letter, rank) \
    Piece pc = MakePiece(PieceK, col); \
    Bitboard pcs = g->Pieces[pc]; \
    Square ksq = LeastSigBit(pcs); \
    Bitboard to = 0; \
    int nMoves = 0; \
    Bitboard occ = g->All & ~pcs; \
    /* standard movement */ \
    FOR_EACH_BIT(MovesK[ksq] & g->Movement, \
    {\
        /* would the king be in check if it moved here */ \
        if (!IsAttackedColMask(g, sq, col, occ)) \
        {\
            to = SetBit(to, sq); \
            nMoves++; \
        }\
    });\
    /* kingside castling */ \
    if (((g->Castle & Castle##letter##K) != 0) & ((g->All & Empty##letter##K) == 0)) \
    {\
        /* king must not be in check on any square it traverses through to castle */ \
        if (!IsAttackedColMask(g, E##rank, col, occ) && !IsAttackedColMask(g, F##rank, col, occ) && !IsAttackedColMask(g, G##rank, col, occ)) \
        {\
            to = SetBit(to, G##rank); \
            nMoves++; \
        }\
    }\
    /* queenside castling */ \
    if (((g->Castle & Castle##letter##Q) != 0) & ((g->All & Empty##letter##Q) == 0)) \
    {\
        /* king must not be in check on any square it traverses through to castle */ \
        if (!IsAttackedColMask(g, E##rank, col, occ) && !IsAttackedColMask(g, D##rank, col, occ) && !IsAttackedColMask(g, C##rank, col, occ)) \
        {\
            to = SetBit(to, C##rank); \
            nMoves++; \
        }\
    }\
    AddMovesToInfo(info, pc, ksq, to, nMoves); 

/**
 * Adds knight moves to move info. 
 * 
 * @param col Color to move
 */
#define FN_GEN_N_INFO(col) \
    Piece pc = MakePiece(PieceN, col); \
    Bitboard pcs = g->Pieces[pc]; \
    FOR_EACH_BIT(pcs, \
    {\
        Bitboard to = MovesN[sq] & g->Movement & TARGET_SQUARES; \
        AddMovesToInfo(info, pc, sq, to, PopCount(to)); \
    });

/**
 * Adds bishop moves to move info. 
 * 
 * @param col Color to move
 */
#define FN_GEN_B_INFO(col) \
    Piece pc = MakePiece(PieceB, col); \
    Bitboard pcs = g->Pieces[pc]; \
    FOR_EACH_BIT(pcs, \
    {\
        Bitboard to = BAttacks(sq, g->All); \
        to &= g->Movement & TARGET_SQUARES; \
        AddMovesToInfo(info, pc, sq, to, PopCount(to)); \
    });

/**
 * Adds rook moves to move info. 
 * 
 * @param col Color to move
 */
#define FN_GEN_R_INFO(col) \
    Piece pc = MakePiece(PieceR, col); \
    Bitboard pcs = g->Pieces[pc]; \
    FOR_EACH_BIT(pcs, \
    {\
        Bitboard to = RAttacks(sq, g->All); \
        to &= g->Movement & TARGET_SQUARES; \
        AddMovesToInfo(info, pc, sq, to, PopCount(to)); \
    });

/**
 * Adds queen moves to move info. 
 * 
 * @param col Color to move
 */
#define FN_GEN_Q_INFO(col) \
    Piece pc = MakePiece(PieceQ, col); \
    Bitboard pcs = g->Pieces[pc]; \
    FOR_EACH_BIT(pcs, \
    {\
        Bitboard to = RAttacks(sq, g->All) | BAttacks(sq, g->All); \
        to &= g->Movement & TARGET_SQUARES; \
        AddMovesToInfo(info, pc, sq, to, PopCount(to)); \
    });

// movement for each combination of piece and color: 

static inline void GenWPInfo(const Game* g, TARGET_SQUARE_PARAMS, MoveInfo* info) { FN_GEN_P_INFO(ColorW, ShiftN, ShiftNN, Rank2, Rank8); } 
static inline void GenBPInfo(const Game* g, TARGET_SQUARE_PARAMS, MoveInfo* info) { FN_GEN_P_INFO(ColorB, ShiftS, ShiftSS, Rank7, Rank1); } 
static inline void GenWKInfo(const Game* g, MoveInfo* info) { FN_GEN_K_INFO(ColorW, W, 1); } 
static inline void GenBKInfo(const Game* g, MoveInfo* info) { FN_GEN_K_INFO(ColorB, B, 8); } 
static inline void GenWNInfo(const Game* g, TARGET_SQUARE_PARAMS, MoveInfo* info) { FN_GEN_N_INFO(ColorW); } 
static inline void GenBNInfo(const Game* g, TARGET_SQUARE_PARAMS, MoveInfo* info) { FN_GEN_N_INFO(ColorB); } 
static inline void GenWBInfo(const Game* g, TARGET_SQUARE_PARAMS, MoveInfo* info) { FN_GEN_B_INFO(ColorW); } 
static inline void GenBBInfo(const Game* g, TARGET_SQUARE_PARAMS, MoveInfo* info) { FN_GEN_B_INFO(ColorB); } 
static inline void GenWRInfo(const Game* g, TARGET_SQUARE_PARAMS, MoveInfo* info) { FN_GEN_R_INFO(ColorW); } 
static inline void GenBRInfo(const Game* g, TARGET_SQUARE_PARAMS, MoveInfo* info) { FN_GEN_R_INFO(ColorB); } 
static inline void GenWQInfo(const Game* g, TARGET_SQUARE_PARAMS, MoveInfo* info) { FN_GEN_Q_INFO(ColorW); } 
static inline void GenBQInfo(const Game* g, TARGET_SQUARE_PARAMS, MoveInfo* info) { FN_GEN_Q_INFO(ColorB); } 

/**
 * Adds all legal moves for one color to move info. 
 * 
 * @param col Color to move 
 * @param letter Color's letter (W or B)
 */
#define FN_GEN_MOVE_INFO(col, letter) \
{\
    InitMoveInfo(info); \
    Color opp = OppositeColor(col); \
    Bitboard oppP = g->Pieces[MakePiece(PieceP, opp)]; \
    Bitboard oppN = g->Pieces[MakePiece(PieceN, opp)]; \
    Bitboard oppB = g->Pieces[MakePiece(PieceB, opp)]; \
    Bitboard oppR = g->Pieces[MakePiece(PieceR, opp)]; \
    Bitboard oppQ = g->Pieces[MakePiece(PieceQ, opp)]; \
    Bitboard oppK = g->Pieces[MakePiece(PieceK, opp)]; \
    Bitboard target = g->Pieces[MakePiece(PieceK, col)]; \
    Bitboard ksq = LeastSigBit(target); \
    Bitboard colOcc = g->Colors[col]; \
    /* relevant opp sliding pieces for each direction */ \
    Bitboard oppRQ = oppR | oppQ; \
    Bitboard oppBQ = oppB | oppQ; \
    /* if the king were a rook, how far could it move */ \
    Bitboard kRHit  = RAttacks(ksq, g->All); \
    /* used to remove ally pieces (check for pinned pieces) */ \
    Bitboard kRNoCol = ~(kRHit & colOcc); \
    /* check if removing one ally piece reveals an attacker */ \
    Bitboard kRDHit = RAttacks(ksq, g->All & kRNoCol) & oppRQ; \
    /* do the above for bishop*/ \
    Bitboard kBHit  = BAttacks(ksq, g->All); \
    Bitboard kBNoCol = ~(kBHit & colOcc); \
    Bitboard kBDHit = BAttacks(ksq, g->All & kBNoCol) & oppBQ; \
    /* sliding piece checkers*/ \
    Bitboard chkSlide = (kRHit & oppRQ) | (kBHit & oppBQ); \
    /* all checkers (remaining pieces can only be captured or force the king to retreat)*/ \
    Bitboard chk = chkSlide \
               | (oppK & MovesK[ksq]) \
               | (oppN & MovesN[ksq]) \
               | (oppP & AttacksP[col][ksq]); \
    Bitboard dirs[9] = { 0 }; \
    const U8* pinIndex = PinIndex[ksq]; \
    FOR_EACH_BIT(kRDHit | kBDHit, \
    {\
        dirs[pinIndex[sq]] = SlideTo[ksq][sq]; \
    });\
    if (chk == 0) \
    {\
        Gen##letter##PInfo(g, AllBits, dirs, pinIndex, info); \
        Gen##letter##NInfo(g, AllBits, dirs, pinIndex, info); \
        Gen##letter##BInfo(g, AllBits, dirs, pinIndex, info); \
        Gen##letter##RInfo(g, AllBits, dirs, pinIndex, info); \
        Gen##letter##QInfo(g, AllBits, dirs, pinIndex, info); \
        Gen##letter##KInfo(g, info); \
    }\
    else \
    {\
        int nChecks = PopCount(chk); \
        if (nChecks == 1) \
        {\
            Square attacker = LeastSigBit(chk); \
            Bitboard okSquares = SlideTo[ksq][attacker] | Bits[attacker]; \
            Gen##letter##PInfo(g, okSquares, dirs, pinIndex, info); \
            Gen##letter##NInfo(g, okSquares, dirs, pinIndex, info); \
            Gen##letter##BInfo(g, okSquares, dirs, pinIndex, info); \
            Gen##letter##RInfo(g, okSquares, dirs, pinIndex, info); \
            Gen##letter##QInfo(g, okSquares, dirs, pinIndex, info); \
            Gen##letter##KInfo(g, info); \
        }\
        else \
        {\
            Gen##letter##KInfo(g, info); \
        }\
    }\
}

/**
 * Adds all legal moves of the current color to move. 
 * 
 * @param g The game 
 * @param info Move info 
 */
void GenMoveInfo(const Game* g, MoveInfo* info) 
{
    if (g->Turn == ColorW) 
    {
        FN_GEN_MOVE_INFO(ColorW, W); 
    }
    else 
    {
        FN_GEN_MOVE_INFO(ColorB, B); 
    }
}

/**
 * Finds rook-style discovery attackers. 
 * 
 * @param g The game 
 * @param sq Attacked square 
 * @param chkCol Friendly color of the square
 * @param mask Pieces to keep on the board (remove the moving piece)
 * @param add Pieces (considered neutral) to add to the board (add the moving piece)
 * @return Bitboard of attackers 
 */
static inline Bitboard DiscRAttackers(const Game* g, Square sq, Color chkCol, Bitboard mask, Bitboard add) 
{
    Color opp = OppositeColor(chkCol); 

    Bitboard occ = (g->All & mask) | add; 
    Bitboard oppR = g->Pieces[MakePiece(PieceR, opp)]; 
    Bitboard oppQ = g->Pieces[MakePiece(PieceQ, opp)]; 
    Bitboard oppRQ = (oppR | oppQ) & mask; 

    return RAttacks(sq, occ) & oppRQ; 
}

/**
 * Finds bishop-style discovery attackers. 
 * 
 * @param g The game 
 * @param sq Attacked square 
 * @param chkCol Friendly color of the square
 * @param mask Pieces to keep on the board (remove the moving piece)
 * @param add Pieces (considered neutral) to add to the board (add the moving piece)
 * @return Bitboard of attackers 
 */
static inline Bitboard DiscBAttackers(const Game* g, Square sq, Color chkCol, Bitboard mask, Bitboard add) 
{
    Color opp = OppositeColor(chkCol); 

    Bitboard occ = (g->All & mask) | add; 
    Bitboard oppB = g->Pieces[MakePiece(PieceB, opp)]; 
    Bitboard oppQ = g->Pieces[MakePiece(PieceQ, opp)]; 
    Bitboard oppBQ = (oppB | oppQ) & mask; 

    return BAttacks(sq, occ) & oppBQ; 
}

/**
 * Finds discovery attackers. 
 * 
 * @param g The game 
 * @param sq Attacked square 
 * @param chkCol Friendly color of the square
 * @param mask Pieces to keep on the board (remove the moving piece)
 * @param add Pieces (considered neutral) to add to the board (add the moving piece)
 * @param pinIndex Pin type to consider 
 * @return Bitboard of attackers 
 */
static inline Bitboard DiscAttackers(const Game* g, Square sq, Color chkCol, Bitboard mask, Bitboard add, int pinIndex) 
{
    if (pinIndex == NumCheckDirections) 
    {
        return 0; 
    }
    else if (pinIndex <= CheckDirectionRookEnd) 
    {
        return DiscRAttackers(g, sq, chkCol, mask, add); 
    }
    else 
    {
        return DiscBAttackers(g, sq, chkCol, mask, add); 
    }
}

/**
 * Finds discovered attacks on the enemy king. 
 * This is all that's needed for standard moves. 
 * 
 * @param col Color to move 
 */
#define DISC_ATTACKERS(col) (DiscAttackers(g, oppKSquare, OppositeColor(col), InverseBits[from], Bits[sq], pinIndex[from]))

/**
 * Finds direct attacks on the enemy king from a knight. 
 */
#define CHECK_N (MovesN[sq] & oppK)

/**
 * Finds direct attacks on the enemy king from a bishop. 
 */
#define CHECK_B (BAttacks(sq, g->All & InverseBits[from]) & oppK)

/**
 * Finds direct attacks on the enemy king from a rook. 
 */
#define CHECK_R (RAttacks(sq, g->All & InverseBits[from]) & oppK)

/**
 * Finds direct attacks on the enemy king from a queen. 
 */
#define CHECK_Q ((BAttacks(sq, g->All & InverseBits[from]) | RAttacks(sq, g->All & InverseBits[from])) & oppK)

/**
 * Adds pawn moves to a list. 
 * 
 * @param col Color to move 
 * @param proRank Bitboard highlighting promotion squares 
 * @param colLetter Color's letter (W or B) 
 * @param oppLetter Opponent's letter (W or B) 
 */
#define FN_GEN_P_MOVES(col, proRank, colLetter, oppLetter) \
{\
    FOR_EACH_BIT(to & ~proRank, \
    {\
        moves->Moves[moves->Size++] = MakeEnPassantMove( \
            from, \
            sq, \
            /* piece to move */ \
            Piece##colLetter##P, \
            /* piece to capture */ \
            (sq == g->EnPassant) ? Piece##oppLetter##P : PieceAt(&g->Board, sq), \
            /* is en passant */ \
            sq == g->EnPassant, \
            /* is this move a check */ \
            (sq == g->EnPassant) ? (\
                (AttacksP[col][sq] & oppK) | \
                /* discoveries */ \
                DiscAttackers(g, oppKSquare, OppositeColor(col), ~(Bits[from] | Bits[sq - 8 * ColorSign(col)]), Bits[sq], pinIndex[from]) | \
                DiscAttackers(g, oppKSquare, OppositeColor(col), ~(Bits[from] | Bits[sq - 8 * ColorSign(col)]), Bits[sq], pinIndex[sq - 8 * ColorSign(col)]) \
            ) : (\
                (AttacksP[col][sq] & oppK) | DISC_ATTACKERS(col) \
        )); \
    });\
    FOR_EACH_BIT(to & proRank, \
    {\
        Piece at = PieceAt(&g->Board, sq); \
        /* discoveries */ \
        Bitboard disc = DiscAttackers(g, oppKSquare, OppositeColor(col), InverseBits[from], Bits[sq], pinIndex[from]);\
        /* all promotion moves */ \
        moves->Moves[moves->Size++] = MakePromotionMove(from, sq, Piece##colLetter##P, Piece##colLetter##Q, at, (CHECK_Q) | disc); \
        moves->Moves[moves->Size++] = MakePromotionMove(from, sq, Piece##colLetter##P, Piece##colLetter##R, at, (CHECK_R) | disc); \
        moves->Moves[moves->Size++] = MakePromotionMove(from, sq, Piece##colLetter##P, Piece##colLetter##B, at, (CHECK_B) | disc); \
        moves->Moves[moves->Size++] = MakePromotionMove(from, sq, Piece##colLetter##P, Piece##colLetter##N, at, (CHECK_N) | disc); \
    });\
}

static inline void GenWPMoves(const Game* g, Square from, Bitboard to, Square oppKSquare, Bitboard oppK, const U8* pinIndex, MoveList* moves) 
{
    FN_GEN_P_MOVES(ColorW, Rank8, W, B); 
}

static inline void GenBPMoves(const Game* g, Square from, Bitboard to, Square oppKSquare, Bitboard oppK, const U8* pinIndex, MoveList* moves) 
{
    FN_GEN_P_MOVES(ColorB, Rank1, B, W); 
}

/**
 * Adds king moves to a list. 
 * 
 * @param col Color to move 
 * @param letter Color's letter (W or B) 
 * @param rank Starting rank of the king (1 or 8)
 */
#define FN_GEN_K_MOVES(col, letter, rank) \
    /* if any castle flag is enabled, then the king has not moved */ \
    Bitboard castle = to & (((g->Castle & Castle##letter) != 0) * (1ULL << G##rank | 1ULL << C##rank)); \
    to &= ~castle; \
    FOR_EACH_BIT(to, \
    {\
        moves->Moves[moves->Size++] = MakeMove(from, sq, Piece##letter##K, PieceAt(&g->Board, sq), DiscAttackers(g, oppKSquare, OppositeColor(g->Turn), InverseBits[from], Bits[sq], pinIndex[from])); \
    });\
    FOR_EACH_BIT(castle, \
    {\
        switch (sq) \
        {\
            case G##rank: moves->Moves[moves->Size++] = MakeCastleMove(E##rank, G##rank, Piece##letter##K, MoveCastle##letter##K, RAttacks(F##rank, g->All & ~(1ULL << E##rank)) & oppK); break; \
            case C##rank: moves->Moves[moves->Size++] = MakeCastleMove(E##rank, C##rank, Piece##letter##K, MoveCastle##letter##Q, RAttacks(D##rank, g->All & ~(1ULL << E##rank)) & oppK); break; \
            default: break; \
        }\
    });\

static inline void GenWKMoves(const Game* g, Square from, Bitboard to, Square oppKSquare, Bitboard oppK, const U8* pinIndex, MoveList* moves) 
{
    FN_GEN_K_MOVES(ColorW, W, 1); 
}

static inline void GenBKMoves(const Game* g, Square from, Bitboard to, Square oppKSquare, Bitboard oppK, const U8* pinIndex, MoveList* moves) 
{
    FN_GEN_K_MOVES(ColorB, B, 8); 
}

/**
 * Generate normal piece moves. 
 * 
 * @param col Color to play 
 * @param atk Direct attacks on the enemy king from the moved piece 
 */
#define GEN_MOVES(col, atk) \
    FOR_EACH_BIT(to, \
    {\
        moves->Moves[moves->Size++] = MakeMove(from, sq, pc, PieceAt(&g->Board, sq), (atk) | DISC_ATTACKERS(col)); \
    });

/**
 * Adds all moves from a move info into a list. 
 * 
 * @param col Color to move 
 * @param letter Color's letter (W or B) 
 */
#define FN_GEN_MOVES(col, letter) \
{\
    Bitboard oppK = g->Pieces[MakePiece(PieceK, OppositeColor(col))]; \
    Square oppKSquare = LeastSigBit(oppK); \
    /* pin type array for detecting discovery checks */ \
    const U8* pinIndex = PinIndex[oppKSquare]; \
    for (int pcId = 0; pcId < info->NumPieces; pcId++) \
    {\
        Bitboard to = info->Moves[pcId]; \
        Piece pc = info->Pieces[pcId]; \
        PieceType type = TypeOfPiece(pc); \
        Square from = info->From[pcId]; \
        switch (type) \
        {\
            case PieceP: Gen##letter##PMoves(g, from, info->Moves[pcId], oppKSquare, oppK, pinIndex, moves); break; \
            case PieceK: Gen##letter##KMoves(g, from, info->Moves[pcId], oppKSquare, oppK, pinIndex, moves); break; \
            case PieceN: GEN_MOVES(col, CHECK_N); break; \
            case PieceB: GEN_MOVES(col, CHECK_B); break; \
            case PieceR: GEN_MOVES(col, CHECK_R); break; \
            case PieceQ: GEN_MOVES(col, CHECK_Q); break; \
            default: break; \
        }\
    }\
} 

void GenMovesFromInfo(const Game* g, const MoveInfo* info, MoveList* moves) 
{
    if (g->Turn == ColorW) 
    {
        FN_GEN_MOVES(ColorW, W); 
    }
    else 
    {
        FN_GEN_MOVES(ColorB, B); 
    }
}