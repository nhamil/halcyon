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

#include <stdlib.h> 

/**
 * Filter out moves ignoring absolute pins. 
 */
#define TARGET_SQUARES ( allowed & ( ((GetBit(pinDirs[pinIndex[sq]], sq) == 0) * AllBits) | pinDirs[pinIndex[sq]] ) )

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
 * Add all standard moves to a list. 
 * 
 * @param moves Move list 
 * @param pc Piece to move
 * @param from Start square of the piece 
 * @param to Bitboard of all squares the piece can move to 
 */
static inline void PushBitboardMoves(const Game* g, Piece pc, Square from, Bitboard to, MoveList* moves) 
{
    FOR_EACH_BIT(to, 
    {
        PushMoveToList(moves, MakeMove(from, sq, pc, PieceAt(&g->Board, sq))); 
    });
}

static inline void PushPBitboardMoves(const Game* g, Piece pc, Bitboard to, Square moveDelta, Square ep, Piece epPiece, MoveList* moves) 
{
    FOR_EACH_BIT(to, 
    {
        PushMoveToList(moves, MakeEnPassantMove(sq + moveDelta, sq, pc, sq != ep ? PieceAt(&g->Board, sq) : epPiece, sq == ep)); 
    });
}

static inline void PushPromotionBitboardMoves(const Game* g, Piece pc, Bitboard to, Square moveDelta, Piece q, Piece n, Piece r, Piece b, MoveList* moves) 
{
    Piece tgt; 
    FOR_EACH_BIT(to, 
    {
        tgt = PieceAt(&g->Board, sq); 
        PushMoveToList(moves, MakePromotionMove(sq + moveDelta, sq, pc, q, tgt)); 
        PushMoveToList(moves, MakePromotionMove(sq + moveDelta, sq, pc, n, tgt)); 
        PushMoveToList(moves, MakePromotionMove(sq + moveDelta, sq, pc, r, tgt)); 
        PushMoveToList(moves, MakePromotionMove(sq + moveDelta, sq, pc, b, tgt)); 
    });
}

static inline void GenPMoves(const Game* g, Color turn, MoveList* moves) 
{
    Bitboard all = g->All; 
    Bitboard empty = ~all; 
    Bitboard pawns, pros, opp; 
    Square ep = g->EnPassant; 

    if (turn == ColorW) 
    {
        pawns = g->Pieces[PieceWP] & ~Rank7; 
        pros = g->Pieces[PieceWP] & Rank7; 
        opp = g->Colors[ColorB] | Bits[ep]; 

        if (pros) 
        {
            PushPromotionBitboardMoves(
                g, 
                PieceWP, 
                // move all pawns 1 square if that square is empty 
                ShiftN(pros) & empty, 
                SquareS, 
                PieceWQ, 
                PieceWN, 
                PieceWR, 
                PieceWB, 
                moves
            ); 

            PushPromotionBitboardMoves(
                g, 
                PieceWP, 
                // move all pawns NE if that square has an opponent piece 
                ShiftNE(pros) & opp, 
                SquareS + SquareW, 
                PieceWQ, 
                PieceWN, 
                PieceWR, 
                PieceWB, 
                moves
            ); 

            PushPromotionBitboardMoves(
                g, 
                PieceWP, 
                // move all pawns NW if that square has an opponent piece 
                ShiftNW(pros) & opp, 
                SquareS + SquareE, 
                PieceWQ, 
                PieceWN, 
                PieceWR, 
                PieceWB, 
                moves
            ); 
        }

        PushPBitboardMoves(
            g, 
            PieceWP, 
            // move all pawns 1 square if that square is empty 
            ShiftN(pawns) & empty, 
            SquareS, 
            NoSquare, 
            NoPiece, 
            moves
        ); 

        PushPBitboardMoves(
            g, 
            PieceWP, 
            // move all pawns NE if that square has an opponent piece 
            ShiftNE(pawns) & opp, 
            SquareS + SquareW, 
            ep, 
            PieceBP, 
            moves
        ); 

        PushPBitboardMoves(
            g, 
            PieceWP, 
            // move all pawns NW if that square has an opponent piece 
            ShiftNW(pawns) & opp, 
            SquareS + SquareE, 
            ep, 
            PieceBP, 
            moves
        ); 

        PushPBitboardMoves(
            g, 
            PieceWP, 
            // all pawns on 2nd rank and who have no piece directly in front of them
            //   -> move 2 squares if that square is empty 
            ShiftNN(pawns & ShiftS(empty) & Rank2) & empty, 
            SquareS + SquareS, 
            NoSquare, 
            NoPiece, 
            moves
        ); 
    }
    else // ColorB 
    {
        pawns = g->Pieces[PieceBP] & ~Rank2; 
        pros = g->Pieces[PieceBP] & Rank2; 
        opp = g->Colors[ColorW] | Bits[ep];  

        if (pros) 
        {
            PushPromotionBitboardMoves(
                g, 
                PieceBP, 
                // move all pawns 1 square if that square is empty 
                ShiftS(pros) & empty, 
                SquareN, 
                PieceBQ, 
                PieceBN, 
                PieceBR, 
                PieceBB, 
                moves
            ); 

            PushPromotionBitboardMoves(
                g, 
                PieceBP, 
                // move all pawns SE if that square has an opponent piece 
                ShiftSE(pros) & opp, 
                SquareN + SquareW, 
                PieceBQ, 
                PieceBN, 
                PieceBR, 
                PieceBB, 
                moves
            ); 

            PushPromotionBitboardMoves(
                g, 
                PieceBP, 
                // move all pawns SW if that square has an opponent piece 
                ShiftSW(pros) & opp, 
                SquareN + SquareE, 
                PieceBQ, 
                PieceBN, 
                PieceBR, 
                PieceBB, 
                moves
            ); 
        }

        PushPBitboardMoves(
            g, 
            PieceBP, 
            // move all pawns 1 square if that square is empty 
            ShiftS(pawns) & empty, 
            SquareN, 
            NoSquare, 
            NoPiece, 
            moves
        ); 

        PushPBitboardMoves(
            g, 
            PieceBP, 
            // move all pawns SE if that square has an opponent piece 
            ShiftSE(pawns) & opp, 
            SquareN + SquareW, 
            ep, 
            PieceWP, 
            moves
        ); 

        PushPBitboardMoves(
            g, 
            PieceBP, 
            // move all pawns SW if that square has an opponent piece 
            ShiftSW(pawns) & opp, 
            SquareN + SquareE, 
            ep, 
            PieceWP, 
            moves
        ); 

        PushPBitboardMoves(
            g, 
            PieceBP, 
            // all pawns on 2nd rank and who have no piece directly in front of them
            //   -> move 2 squares if that square is empty 
            ShiftSS(pawns & ShiftN(empty) & Rank7) & empty, 
            SquareN + SquareN, 
            NoSquare, 
            NoPiece, 
            moves
        ); 
    }
}

static inline void GenPCaptureMoves(const Game* g, Color turn, MoveList* moves) 
{
    Bitboard all = g->All; 
    Bitboard empty = ~all; 
    Bitboard pawns, pros, opp; 
    Square ep = g->EnPassant; 

    if (turn == ColorW) 
    {
        pawns = g->Pieces[PieceWP] & ~Rank7; 
        pros = g->Pieces[PieceWP] & Rank7; 
        opp = g->Colors[ColorB] | Bits[ep]; 

        if (pros) 
        {
            PushPromotionBitboardMoves(
                g, 
                PieceWP, 
                // move all pawns 1 square if that square is empty 
                ShiftN(pros) & empty, 
                SquareS, 
                PieceWQ, 
                PieceWN, 
                PieceWR, 
                PieceWB, 
                moves
            ); 

            PushPromotionBitboardMoves(
                g, 
                PieceWP, 
                // move all pawns NE if that square has an opponent piece 
                ShiftNE(pros) & opp, 
                SquareS + SquareW, 
                PieceWQ, 
                PieceWN, 
                PieceWR, 
                PieceWB, 
                moves
            ); 

            PushPromotionBitboardMoves(
                g, 
                PieceWP, 
                // move all pawns NW if that square has an opponent piece 
                ShiftNW(pros) & opp, 
                SquareS + SquareE, 
                PieceWQ, 
                PieceWN, 
                PieceWR, 
                PieceWB, 
                moves
            ); 
        }

        PushPBitboardMoves(
            g, 
            PieceWP, 
            // move all pawns NE if that square has an opponent piece 
            ShiftNE(pawns) & opp, 
            SquareS + SquareW, 
            ep, 
            PieceBP, 
            moves
        ); 

        PushPBitboardMoves(
            g, 
            PieceWP, 
            // move all pawns NW if that square has an opponent piece 
            ShiftNW(pawns) & opp, 
            SquareS + SquareE, 
            ep, 
            PieceBP, 
            moves
        ); 
    }
    else // ColorB 
    {
        pawns = g->Pieces[PieceBP] & ~Rank2; 
        pros = g->Pieces[PieceBP] & Rank2; 
        opp = g->Colors[ColorW] | Bits[ep];  

        if (pros) 
        {
            PushPromotionBitboardMoves(
                g, 
                PieceBP, 
                // move all pawns 1 square if that square is empty 
                ShiftS(pros) & empty, 
                SquareN, 
                PieceBQ, 
                PieceBN, 
                PieceBR, 
                PieceBB, 
                moves
            ); 

            PushPromotionBitboardMoves(
                g, 
                PieceBP, 
                // move all pawns SE if that square has an opponent piece 
                ShiftSE(pros) & opp, 
                SquareN + SquareW, 
                PieceBQ, 
                PieceBN, 
                PieceBR, 
                PieceBB, 
                moves
            ); 

            PushPromotionBitboardMoves(
                g, 
                PieceBP, 
                // move all pawns SW if that square has an opponent piece 
                ShiftSW(pros) & opp, 
                SquareN + SquareE, 
                PieceBQ, 
                PieceBN, 
                PieceBR, 
                PieceBB, 
                moves
            ); 
        }

        PushPBitboardMoves(
            g, 
            PieceBP, 
            // move all pawns SE if that square has an opponent piece 
            ShiftSE(pawns) & opp, 
            SquareN + SquareW, 
            ep, 
            PieceWP, 
            moves
        ); 

        PushPBitboardMoves(
            g, 
            PieceBP, 
            // move all pawns SW if that square has an opponent piece 
            ShiftSW(pawns) & opp, 
            SquareN + SquareE, 
            ep, 
            PieceWP, 
            moves
        ); 
    }
}

static inline void GenPQuietMoves(const Game* g, Color turn, MoveList* moves) 
{
    Bitboard all = g->All; 
    Bitboard empty = ~all; 
    Bitboard pawns; 
    Square ep = g->EnPassant; 

    if (turn == ColorW) 
    {
        pawns = g->Pieces[PieceWP] & ~Rank7; 

        PushPBitboardMoves(
            g, 
            PieceWP, 
            // move all pawns 1 square if that square is empty 
            ShiftN(pawns) & empty, 
            SquareS, 
            NoSquare, 
            NoPiece, 
            moves
        ); 

        PushPBitboardMoves(
            g, 
            PieceWP, 
            // all pawns on 2nd rank and who have no piece directly in front of them
            //   -> move 2 squares if that square is empty 
            ShiftNN(pawns & ShiftS(empty) & Rank2) & empty, 
            SquareS + SquareS, 
            NoSquare, 
            NoPiece, 
            moves
        ); 
    }
    else // ColorB 
    {
        pawns = g->Pieces[PieceBP] & ~Rank2; 

        PushPBitboardMoves(
            g, 
            PieceBP, 
            // move all pawns 1 square if that square is empty 
            ShiftS(pawns) & empty, 
            SquareN, 
            NoSquare, 
            NoPiece, 
            moves
        ); 

        PushPBitboardMoves(
            g, 
            PieceBP, 
            // all pawns on 2nd rank and who have no piece directly in front of them
            //   -> move 2 squares if that square is empty 
            ShiftSS(pawns & ShiftN(empty) & Rank7) & empty, 
            SquareN + SquareN, 
            NoSquare, 
            NoPiece, 
            moves
        ); 
    }
}

static inline void GenKCaptureMoves(const Game* g, Color turn, Bitboard allowed, MoveList* moves) 
{
    Bitboard to; 
    Piece pc = MakePiece(PieceK, turn); 

    FOR_EACH_BIT(g->Pieces[pc], 
    {
        to = MovesK[sq] & allowed; 
        PushBitboardMoves(g, pc, sq, to, moves); 
    });
}

static inline void GenKMoves(const Game* g, Color turn, Bitboard allowed, MoveList* moves) 
{
    Bitboard to; 
    Piece pc = MakePiece(PieceK, turn); 

    FOR_EACH_BIT(g->Pieces[pc], 
    {
        to = MovesK[sq] & allowed; 
        PushBitboardMoves(g, pc, sq, to, moves); 
    });

    if (turn == ColorW) 
    {
        if ((g->Castle & CastleWK) && (g->All & EmptyWK) == 0) PushMoveToList(moves, MakeCastleMove(E1, G1, PieceWK, MoveCastleWK)); 
        if ((g->Castle & CastleWQ) && (g->All & EmptyWQ) == 0) PushMoveToList(moves, MakeCastleMove(E1, C1, PieceWK, MoveCastleWQ)); 
    }
    else 
    {
        if ((g->Castle & CastleBK) && (g->All & EmptyBK) == 0) PushMoveToList(moves, MakeCastleMove(E8, G8, PieceBK, MoveCastleBK)); 
        if ((g->Castle & CastleBQ) && (g->All & EmptyBQ) == 0) PushMoveToList(moves, MakeCastleMove(E8, C8, PieceBK, MoveCastleBQ)); 
    }
}

static inline void GenNMoves(const Game* g, Color turn, Bitboard allowed, Bitboard* pinDirs, const U8* pinIndex, MoveList* moves) 
{
    Bitboard to; 
    Piece pc = MakePiece(PieceN, turn); 

    FOR_EACH_BIT(g->Pieces[pc], 
    {
        to = MovesN[sq] & TARGET_SQUARES; 
        PushBitboardMoves(g, pc, sq, to, moves); 
    });
}

static inline void GenBMoves(const Game* g, Color turn, Bitboard allowed, Bitboard* pinDirs, const U8* pinIndex, MoveList* moves) 
{
    Bitboard to; 
    Piece pc = MakePiece(PieceB, turn); 
    
    FOR_EACH_BIT(g->Pieces[pc], 
    {
        to = BAttacks(sq, g->All) & TARGET_SQUARES; 
        PushBitboardMoves(g, pc, sq, to, moves); 
    });
}

static inline void GenRMoves(const Game* g, Color turn, Bitboard allowed, Bitboard* pinDirs, const U8* pinIndex, MoveList* moves) 
{
    Bitboard to; 
    Piece pc = MakePiece(PieceR, turn); 
    
    FOR_EACH_BIT(g->Pieces[pc], 
    {
        to = RAttacks(sq, g->All) & TARGET_SQUARES; 
        PushBitboardMoves(g, pc, sq, to, moves); 
    });
}

static inline void GenQMoves(const Game* g, Color turn, Bitboard allowed, Bitboard* pinDirs, const U8* pinIndex, MoveList* moves) 
{
    Bitboard to; 
    Piece pc = MakePiece(PieceQ, turn); 
    
    FOR_EACH_BIT(g->Pieces[pc], 
    {
        to  = RAttacks(sq, g->All); 
        to |= BAttacks(sq, g->All); 
        to &= TARGET_SQUARES; 
        PushBitboardMoves(g, pc, sq, to, moves); 
    });
}

static inline Bitboard GetPinInfo(const Game* g, Color col, Square ksq, const U8* pinIndex, Bitboard dirs[9]) 
{
    Color opp = OppositeColor(col); 
    Bitboard oppP = g->Pieces[MakePiece(PieceP, opp)]; 
    Bitboard oppN = g->Pieces[MakePiece(PieceN, opp)]; 
    Bitboard oppB = g->Pieces[MakePiece(PieceB, opp)]; 
    Bitboard oppR = g->Pieces[MakePiece(PieceR, opp)]; 
    Bitboard oppQ = g->Pieces[MakePiece(PieceQ, opp)]; 
    Bitboard oppK = g->Pieces[MakePiece(PieceK, opp)]; 
    Bitboard colOcc = g->Colors[col]; 
    /* relevant opp sliding pieces for each direction */ 
    Bitboard oppRQ = oppR | oppQ; 
    Bitboard oppBQ = oppB | oppQ; 
    /* if the king were a rook, how far could it move */ 
    Bitboard kRHit  = RAttacks(ksq, g->All); 
    /* used to remove ally pieces (check for pinned pieces) */ 
    Bitboard kRNoCol = ~(kRHit & colOcc); 
    /* check if removing one ally piece reveals an attacker */ 
    Bitboard kRDHit = RAttacks(ksq, g->All & kRNoCol) & oppRQ; 
    /* do the above for bishop */ 
    Bitboard kBHit  = BAttacks(ksq, g->All); 
    Bitboard kBNoCol = ~(kBHit & colOcc); 
    Bitboard kBDHit = BAttacks(ksq, g->All & kBNoCol) & oppBQ; 
    /* sliding piece checkers */ 
    Bitboard chkSlide = (kRHit & oppRQ) | (kBHit & oppBQ); 
    
    FOR_EACH_BIT(kRDHit | kBDHit, 
    {
        dirs[pinIndex[sq]] = SlideTo[ksq][sq]; 
    });

    /* all checkers (remaining pieces can only be captured or force the king to retreat) */ 
    return chkSlide 
         | (oppK & MovesK[ksq]) 
         | (oppN & MovesN[ksq]) 
         | (oppP & AttacksP[col][ksq]); 
}

void GenMoves(const Game* g, MoveList* moves) 
{
    ClearMoves(moves); 

    Color col = g->Turn; 
    Bitboard target = g->Pieces[MakePiece(PieceK, col)]; 
    Bitboard ksq = LeastSigBit(target); 
    Bitboard allowed = ~g->Colors[col]; 
    const U8* pinIndex = PinIndex[ksq]; 
    Bitboard dirs[9] = { 0 }; 

    Bitboard chk = GetPinInfo(g, col, ksq, pinIndex, dirs); 

    if (chk == NoBits) 
    {
        GenPMoves(g, col, moves); 
        GenNMoves(g, col, allowed, dirs, pinIndex, moves); 
        GenBMoves(g, col, allowed, dirs, pinIndex, moves); 
        GenRMoves(g, col, allowed, dirs, pinIndex, moves); 
        GenQMoves(g, col, allowed, dirs, pinIndex, moves); 
        GenKMoves(g, col, allowed, moves); 
    }
    else 
    {
        int nChecks = PopCount(chk); 
        if (nChecks == 1) 
        {
            Square attacker = LeastSigBit(chk); 
            Bitboard okSquares = allowed & (SlideTo[ksq][attacker] | Bits[attacker]); 
            GenPMoves(g, col, moves); 
            GenNMoves(g, col, okSquares, dirs, pinIndex, moves); 
            GenBMoves(g, col, okSquares, dirs, pinIndex, moves); 
            GenRMoves(g, col, okSquares, dirs, pinIndex, moves); 
            GenQMoves(g, col, okSquares, dirs, pinIndex, moves); 
            GenKMoves(g, col, allowed, moves); 
        }
        else 
        {
            GenKMoves(g, col, allowed, moves); 
        }
    }
}

void GenCaptureMoves(const Game* g, MoveList* moves) 
{
    ClearMoves(moves); 

    Color col = g->Turn; 
    Bitboard target = g->Pieces[MakePiece(PieceK, col)]; 
    Bitboard ksq = LeastSigBit(target); 
    // must land on an enemy piece 
    Bitboard allowed = g->Colors[OppositeColor(col)]; 
    const U8* pinIndex = PinIndex[ksq]; 
    Bitboard dirs[9] = { 0 }; 

    Bitboard chk = GetPinInfo(g, col, ksq, pinIndex, dirs); 

    if (chk == NoBits) 
    {
        GenPCaptureMoves(g, col, moves); 
        GenNMoves(g, col, allowed, dirs, pinIndex, moves); 
        GenBMoves(g, col, allowed, dirs, pinIndex, moves); 
        GenRMoves(g, col, allowed, dirs, pinIndex, moves); 
        GenQMoves(g, col, allowed, dirs, pinIndex, moves); 
        GenKCaptureMoves(g, col, allowed, moves); 
    }
    else 
    {
        int nChecks = PopCount(chk); 
        if (nChecks == 1) 
        {
            Square attacker = LeastSigBit(chk); 
            Bitboard okSquares = allowed & (SlideTo[ksq][attacker] | Bits[attacker]); 
            GenPCaptureMoves(g, col, moves); 
            GenNMoves(g, col, okSquares, dirs, pinIndex, moves); 
            GenBMoves(g, col, okSquares, dirs, pinIndex, moves); 
            GenRMoves(g, col, okSquares, dirs, pinIndex, moves); 
            GenQMoves(g, col, okSquares, dirs, pinIndex, moves); 
            GenKCaptureMoves(g, col, allowed, moves); 
        }
        else 
        {
            GenKCaptureMoves(g, col, allowed, moves); 
        }
    }
}

void GenQuietMoves(const Game* g, MoveList* moves) 
{
    ClearMoves(moves); 

    Color col = g->Turn; 
    Bitboard target = g->Pieces[MakePiece(PieceK, col)]; 
    Bitboard ksq = LeastSigBit(target); 
    // must land on an empty square  
    Bitboard allowed = ~g->All; 
    const U8* pinIndex = PinIndex[ksq]; 
    Bitboard dirs[9] = { 0 }; 

    Bitboard chk = GetPinInfo(g, col, ksq, pinIndex, dirs); 

    if (chk == NoBits) 
    {
        GenPQuietMoves(g, col, moves); 
        GenNMoves(g, col, allowed, dirs, pinIndex, moves); 
        GenBMoves(g, col, allowed, dirs, pinIndex, moves); 
        GenRMoves(g, col, allowed, dirs, pinIndex, moves); 
        GenQMoves(g, col, allowed, dirs, pinIndex, moves); 
        GenKMoves(g, col, allowed, moves); 
    }
    else 
    {
        int nChecks = PopCount(chk); 
        if (nChecks == 1) 
        {
            Square attacker = LeastSigBit(chk); 
            Bitboard okSquares = allowed & (SlideTo[ksq][attacker] | Bits[attacker]); 
            GenPQuietMoves(g, col, moves); 
            GenNMoves(g, col, okSquares, dirs, pinIndex, moves); 
            GenBMoves(g, col, okSquares, dirs, pinIndex, moves); 
            GenRMoves(g, col, okSquares, dirs, pinIndex, moves); 
            GenQMoves(g, col, okSquares, dirs, pinIndex, moves); 
            GenKMoves(g, col, allowed, moves); 
        }
        else 
        {
            GenKMoves(g, col, allowed, moves); 
        }
    }
}
