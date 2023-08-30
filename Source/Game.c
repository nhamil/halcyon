/**
 * @file Game.c
 * @author Nicholas Hamilton 
 * @date 2023-01-19
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Implements game logic. 
 */

#include "Game.h" 

#include <stdlib.h> 
#include <string.h> 

#include "Bitboard.h"
#include "Castle.h"
#include "Mailbox.h"
#include "Move.h"
#include "MoveGen.h"
#include "Piece.h"
#include "Square.h"
#include "Zobrist.h"

Game* NewGame(void) 
{
    Game* g = malloc(sizeof(Game)); 
    ResetGame(g); 
    return g; 
}

void FreeGame(Game* g) 
{
    free(g); 
}

void ResetGame(Game* g) 
{
    InitHash(); 
    InitMailbox(&g->Board); 
    memset(g->Pieces, 0, sizeof(g->Pieces)); 
    memset(g->Colors, 0, sizeof(g->Colors)); 
    memset(g->Counts, 0, sizeof(g->Counts)); 
    g->All = 0; 
    g->Hash = 0; 
    g->Castle = 0; 
    g->EnPassant = NoSquare; 
    g->Ply = 0; 
    g->Halfmove = 0; 
    g->Turn = ColorW; 
    g->Nodes = 0; 
    g->Depth = 0; 
}

void CopyGame(Game* g, const Game* from) 
{
    InitHash(); 
    g->Board = from->Board; 
    memcpy(g->Pieces, from->Pieces, sizeof(g->Pieces)); 
    memcpy(g->Colors, from->Colors, sizeof(g->Colors)); 
    memcpy(g->Counts, from->Counts, sizeof(g->Counts)); 
    g->All = from->All; 
    g->Hash = from->Hash; 
    g->Castle = from->Castle; 
    g->EnPassant = from->EnPassant; 
    g->Ply = from->Ply; 
    g->Halfmove = from->Halfmove; 
    g->Turn = from->Turn; 
    g->Nodes = 0; 
    g->Depth = from->Depth; 
    for (int i = 0; i < g->Depth; i++) 
    {
        g->Hist[i] = from->Hist[i]; 
    }
}

/**
 * Checks if a single property of game states are equal. 
 * 
 * @param prop Property to check 
 */
#define GAME_EQ(prop) if (a->prop != b->prop) { printf("Not equal: %s\n", #prop); return false; }

bool EqualsTTableGame(const Game* a, const Game* b) 
{
    if (!MailboxEquals(&a->Board, &b->Board)) { printf("Not equal: Mailbox\n"); return false; } 
    if (memcmp(a->Pieces, b->Pieces, sizeof(a->Pieces) - sizeof(Bitboard)) != 0) { printf("Not equal: Pieces\n"); return false; }// ignore the scratch index
    if (memcmp(a->Colors, b->Colors, sizeof(a->Colors)) != 0) { printf("Not equal: Colors\n"); return false; }
    if (memcmp(a->Counts, b->Counts, sizeof(a->Counts) - sizeof(int)) != 0) { printf("Not equal: Counts\n"); return false; } // ignore the scratch index
    GAME_EQ(All); 
    GAME_EQ(Hash); 
    GAME_EQ(Castle); 
    GAME_EQ(EnPassant); 
    GAME_EQ(Turn); 

    return true; 
}

void FilePrintGame(const Game* g, FILE* out) 
{
    char fen[MaxFenLength]; 
    ToFen(g, fen); 

    fprintf(out, "+---+---+---+---+---+---+---+---+\n"); 
    for (int rank = 7; rank >= 0; rank--) 
    {
        fprintf(out, "| "); 
        for (int file = 0; file < 8; file++) 
        {
            Square sq = MakeSquare(file, rank); 
            fprintf(out, "%s | ", PieceString(PieceAt(&g->Board, sq))); 
        }
        fprintf(out, "%d\n", rank + 1); 
        fprintf(out, "+---+---+---+---+---+---+---+---+\n"); 
    }
    fprintf(out, "  A   B   C   D   E   F   G   H  \n"); 

    for (Color col = ColorW; col < NumColors; col++) 
    {
        for (PieceType pc = 0; pc < NumPieceTypes; pc++) 
        {
            Piece cpc = MakePiece(pc, col); 
            fprintf(out, "%d%s ", g->Counts[cpc], PieceString(cpc)); 
        }
    }
    fprintf(out, "\n"); 

    fprintf(out, "%s\nHash: ", fen); 
    FilePrintHashEnd(g->Hash, "\n", out);

    fprintf(out, "Ply: %d, ", g->Ply); 
    fprintf(out, "Halfmove: %d, ", g->Halfmove); 
    fprintf(out, "Castling: %s\n", CastleString(g->Castle)); 

    fprintf(out, "En passant: %s, ", SquareString(g->EnPassant)); 

    fprintf(out, "Special draw: %s, ", IsSpecialDraw(g) ? "yes" : "no");

    fprintf(out, "%s to move\n", g->Turn ? "Black" : "White"); 
}

void PrintGame(const Game* g) 
{
    FilePrintGame(g, stdout); 
}

/**
 * Checks if a square is attacked. 
 * Applies the mask and then adds any extra bits. 
 * 
 * @param g The game 
 * @param sq The square 
 * @param chkCol Friendly color
 * @param mask Remove pieces from being considered 
 * @param add Add blockers to occupants
 * @return True if the square is attacked, otherwise false
 */
static inline bool IsAttackedMaskAdd(const Game* g, Square sq, Color chkCol, Bitboard mask, Bitboard add) 
{
    Color col = chkCol; 
    Color opp = OppositeColor(col); 

    Bitboard occ = (g->All & mask) | add; 

    // b,r,q are not used directly 
    // applying mask to the aggregate bitboards is 1 fewer "&" 

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
 * @param mask ONLY remove pieces from the aggregate bitboard, 
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

bool TryPushMove(Game* g, Move mv) 
{
    bool legal = IsLegal(g, mv); 
    if (legal) PushMove(g, mv); 
    return legal; 
}

bool IsLegal(const Game* g, Move mv) 
{
    Color col = g->Turn; 

    Piece pc = FromPiece(mv); 

    Square src = FromSquare(mv); 
    Square dst = ToSquare(mv); 

    Bitboard srcBits = Bits[src]; 
    Bitboard dstBits = Bits[dst]; 

    bool ep = IsEnPassant(mv); 
    int casIndex = CastleIndex(mv); 

    if (casIndex != MoveCastleNone) 
    {
        switch (casIndex) 
        {
            case MoveCastleWK: 
                return !IsAttackedColMask(g, E1, col, ~srcBits) && 
                       !IsAttackedColMask(g, F1, col, ~srcBits) && 
                       !IsAttackedColMask(g, G1, col, ~srcBits); 
            case MoveCastleWQ: 
                return !IsAttackedColMask(g, E1, col, ~srcBits) && 
                       !IsAttackedColMask(g, D1, col, ~srcBits) && 
                       !IsAttackedColMask(g, C1, col, ~srcBits); 
            case MoveCastleBK: 
                return !IsAttackedColMask(g, E8, col, ~srcBits) && 
                       !IsAttackedColMask(g, F8, col, ~srcBits) && 
                       !IsAttackedColMask(g, G8, col, ~srcBits); 
            case MoveCastleBQ: 
                return !IsAttackedColMask(g, E8, col, ~srcBits) && 
                       !IsAttackedColMask(g, D8, col, ~srcBits) && 
                       !IsAttackedColMask(g, C8, col, ~srcBits); 
            default: 
                return true; 
        }
    }
    else if (TypeOfPiece(pc) == PieceK) 
    {
        return !IsAttackedColMask(g, dst, col, ~srcBits); 
    }
    else if (ep) 
    {
        Square rm = g->EnPassant + SquareS * ColorSign(col); 
        Bitboard rmBits = Bits[rm]; 

        Square ksq = LeastSigBit(g->Pieces[MakePiece(PieceK, col)]); 
        return !IsAttackedMaskAdd(g, ksq, col, ~(srcBits | rmBits), dstBits); 
    }
    else if (TypeOfPiece(pc) == PieceP)
    {
        Square ksq = LeastSigBit(g->Pieces[MakePiece(PieceK, col)]); 
        return !IsAttackedMaskAdd(g, ksq, col, ~(srcBits | dstBits), dstBits); 
    }
    else 
    {
        return true; 
    }
}

void PushMove(Game* g, Move mv) 
{
    g->Nodes++; 

    // store anything that the move doesn't store 
    MoveHist* hist = g->Hist + g->Depth++; 
    hist->Halfmove = g->Halfmove; 
    hist->EnPassant = g->EnPassant; 
    hist->Castle = g->Castle; 

    Color col = g->Turn; 
    Color opp = OppositeColor(col); 

    Piece pc = FromPiece(mv); 
    Piece pro = PromotionPiece(mv); 
    Piece tgt = TargetPiece(mv); 

    Square src = FromSquare(mv); 
    Square dst = ToSquare(mv); 

    Bitboard srcBits = Bits[src]; 
    Bitboard dstBits = Bits[dst]; 

    bool ep = IsEnPassant(mv); 
    int casIndex = CastleIndex(mv); 

    // take piece off the board and place (promoted) piece 
    ClearPieceAt(&g->Board, src); 
    SetPieceAt(&g->Board, dst, pro); 
    g->Colors[col] ^= srcBits ^ dstBits; 

    if (pc == pro) // moving piece  
    {
        g->Pieces[pc] ^= srcBits ^ dstBits; 
    }
    else // promotion  
    {
        g->Pieces[pc] ^= srcBits; 
        g->Pieces[pro] ^= dstBits; 
        g->Counts[pc]--; 
        g->Counts[pro]++; 
    }

    // 50-move rule logic 
    if (IsCapture(mv) || TypeOfPiece(pc) == PieceP) 
    {
        g->Halfmove = 0; 
    }
    else 
    {
        g->Halfmove++; 
    }

    if (ep) // en passant 
    {
        Square rm = g->EnPassant + SquareS * ColorSign(col); 
        Bitboard rmBits = Bits[rm]; 

        ClearPieceAt(&g->Board, rm); 
        g->Colors[opp] ^= rmBits; 
        g->Pieces[tgt] ^= rmBits; 
        g->Counts[tgt]--; 
    }
    else if (tgt != NoPiece) // capture 
    {
        // target is already removed from mailbox 
        g->Colors[opp] ^= dstBits; 
        g->Pieces[tgt] ^= dstBits; 
        g->Counts[tgt]--; 
    }
    else if (casIndex != MoveCastleNone) // castle
    {
        Piece r = MakePiece(PieceR, col); 

        // king is updated but rook still needs to be 
        g->Colors[col] ^= MoveCastleBitsR[casIndex]; 
        g->Pieces[r] ^= MoveCastleBitsR[casIndex]; 

        ClearPieceAt(&g->Board, MoveCastleSquareR[casIndex][0]); 
        SetPieceAt(&g->Board, MoveCastleSquareR[casIndex][1], r); 
    }
    
    // en passant is possible if a pawn moved two squares 
    if (TypeOfPiece(pc) == PieceP && abs((int) src - (int) dst) == 2 * SquareN) 
    {
        g->EnPassant = ColorSign(col) * SquareN + src; 
    }
    else 
    {
        g->EnPassant = NoSquare; 
    }

    // remove castling rights if: 
    // - from square is initial king or rook position (it isn't there anymore or it moved)
    // - to square is initial king or rook position (it isn't there anymore or it was captured)
    g->Castle &= ~(MoveCastleRemove[src] | MoveCastleRemove[dst]); 

    // update occupants 
    g->All = g->Colors[ColorW] | g->Colors[ColorB]; 

    g->Ply++; 
    g->Turn = opp; 
}

void PopMove(Game* g, Move mv) 
{
    Color opp = g->Turn; 
    Color col = OppositeColor(opp); 

    g->Ply--; 
    g->Turn = col; 

    // return anything that the move doesn't store 
    MoveHist* hist = g->Hist + --g->Depth; 
    g->Halfmove = hist->Halfmove; 
    g->EnPassant = hist->EnPassant; 
    g->Castle = hist->Castle; 

    Piece pc = FromPiece(mv); 
    Piece pro = PromotionPiece(mv); 
    Piece tgt = TargetPiece(mv); 

    Square src = FromSquare(mv); 
    Square dst = ToSquare(mv); 

    Bitboard srcBits = Bits[src]; 
    Bitboard dstBits = Bits[dst]; 

    bool ep = IsEnPassant(mv); 
    int casIndex = CastleIndex(mv); 

    // take piece off the board and place (promoted) piece 
    SetPieceAt(&g->Board, src, pc); 
    SetPieceAt(&g->Board, dst, tgt); 
    g->Colors[col] ^= srcBits ^ dstBits; 

    if (pc == pro) // moving piece  
    {
        g->Pieces[pc] ^= srcBits ^ dstBits; 
    }
    else // promotion  
    {
        g->Pieces[pc] ^= srcBits; 
        g->Pieces[pro] ^= dstBits; 
        g->Counts[pc]++; 
        g->Counts[pro]--; 
    }

    if (ep) // en passant 
    {
        Square rm = g->EnPassant + SquareS * ColorSign(col); 
        Bitboard rmBits = Bits[rm]; 

        // we assumed the capture wasn't EP, so dest square needs to be fixed
        SetPieceAt(&g->Board, rm, tgt); 
        ClearPieceAt(&g->Board, dst); 

        g->Colors[opp] ^= rmBits; 
        g->Pieces[tgt] ^= rmBits; 
        g->Counts[tgt]++; 
    }
    else if (tgt != NoPiece) // capture 
    {
        // target is already added to mailbox 
        g->Colors[opp] ^= dstBits; 
        g->Pieces[tgt] ^= dstBits; 
        g->Counts[tgt]++; 
    }
    else if (casIndex != MoveCastleNone) // castle
    {
        Piece r = MakePiece(PieceR, col); 

        // king is updated but rook still needs to be 
        g->Colors[col] ^= MoveCastleBitsR[casIndex]; 
        g->Pieces[r] ^= MoveCastleBitsR[casIndex]; 

        ClearPieceAt(&g->Board, MoveCastleSquareR[casIndex][1]); 
        SetPieceAt(&g->Board, MoveCastleSquareR[casIndex][0], r); 
    }
    
    // update occupants 
    g->All = g->Colors[ColorW] | g->Colors[ColorB]; 
}

void PushNullMove(Game* g) 
{
    g->Nodes++; 

    printf("TODO null move\n"); 
    exit(1); 
}

void PopNullMove(Game* g) 
{
    printf("TODO null move\n"); 
    exit(1); 
}

bool IsSpecialDraw(const Game* g) 
{
    // 50-move rule 
    if (g->Halfmove > 99) return true; 

    for (int i = 0; i < DrawDepth; i++) 
    {
        // (g->Depth-1) is previous ply (opponent made the move)
        // so our first depth to check should be (g->Depth-2)
        int depth = (g->Depth-2) - i*2; 

        if (depth >= 0) 
        {
            if (g->Hash == g->Hist[depth].Hash) return true; 
            if ((g->Hash ^ HashColor()) == g->Hist[depth].Hash) return true; 
        }
        else 
        {
            break; 
        }
    }

    // insufficient material (guaranteed)
    if (g->Pieces[PieceWK] == g->Colors[ColorW]) // white only has king
    {
        // KN vs K
        if ((g->Pieces[PieceBK] | g->Pieces[PieceBN]) == g->Colors[ColorB] && g->Counts[PieceBN] <= 1) return true; 
        // KB vs K
        if ((g->Pieces[PieceBK] | g->Pieces[PieceBB]) == g->Colors[ColorB] && g->Counts[PieceBB] <= 1) return true;  
    }
    else if (g->Pieces[PieceBK] == g->Colors[ColorB]) // black only has king 
    {
        // KN vs K
        if ((g->Pieces[PieceWK] | g->Pieces[PieceWN]) == g->Colors[ColorW] && g->Counts[PieceWN] <= 1) return true; 
        // KB vs K
        if ((g->Pieces[PieceWK] | g->Pieces[PieceWB]) == g->Colors[ColorW] && g->Counts[PieceWB] <= 1) return true;  
    }

    return false; 
}

bool ValidateGame(const Game* g) 
{
    bool valid = true; 

    // piece counts 
    for (Piece pc = 0; pc < NumPieces; pc++) 
    {
        if (g->Counts[pc] != PopCount(g->Pieces[pc])) 
        {
            printf("info string ERROR %s count is %d but should be %d\n", PieceString(pc), g->Counts[pc], PopCount(g->Pieces[pc])); 
            valid = false; 
        }
    }

    // exactly 1 king 
    {
        for (Color col = ColorW; col < NumColors; col++) 
        {
            if (g->Counts[MakePiece(PieceK, col)] != 1) 
            {
                printf("info string ERROR %s does not have exactly 1 king: %d\n", col ? "white" : "black", g->Counts[MakePiece(PieceK, col)]); 
                valid = false; 
            }
        }
    }

    // bitboards 
    {
        Bitboard all = 0, col[2] = { 0, 0 }; 
        for (Piece pc = 0; pc < NumPieces; pc++) 
        {
            all ^= g->Pieces[pc]; 
            col[ColorOfPiece(pc)] ^= g->Pieces[pc]; 
        }

        if (all != g->All) { printf("info string ERROR piece bitboards don't match accumulated board\n"); valid = false; } 
        if (col[ColorW] != g->Colors[ColorW]) { printf("info string ERROR piece bitboards don't match accumulated white board\n"); valid = false; } 
        if (col[ColorB] != g->Colors[ColorB]) { printf("info string ERROR piece bitboards don't match accumulated black board\n"); valid = false; } 
    }

    // pieces 
    {
        for (Square sq = 0; sq < NumSquares; sq++) 
        {
            Piece pc = PieceAt(&g->Board, sq); 
            if (pc > NoPiece) 
            {
                printf("info string ERROR mailbox piece is invalid: %u at %s\n", pc, SquareString(sq)); 
                valid = false; 
            }
            else if (pc == NoPiece) 
            {
                if (GetBit(g->All, sq)) 
                {
                    printf("info string ERROR mailbox has no piece but should have one: %s\n", SquareString(sq)); 
                    valid = false; 
                }
            }
            else // valid piece 
            {
                if (!GetBit(g->Pieces[pc], sq)) 
                {
                    printf("info string ERROR mailbox has %s but should be empty: %s\n", PieceString(pc), SquareString(sq)); 
                    valid = false; 
                }
            }
        }
    }

    // castling 
    {
        if (g->Castle & CastleWK) 
        {
            if (PieceAt(&g->Board, E1) != PieceWK || PieceAt(&g->Board, H1) != PieceWR) 
            {
                printf("info string ERROR white O-O flag is set but no longer possible\n"); 
                valid = false; 
            }
        }
        if (g->Castle & CastleWQ) 
        {
            if (PieceAt(&g->Board, E1) != PieceWK || PieceAt(&g->Board, A1) != PieceWR) 
            {
                printf("info string ERROR white O-O-O flag is set but no longer possible\n"); 
                valid = false; 
            }
        }
        if (g->Castle & CastleBK) 
        {
            if (PieceAt(&g->Board, E8) != PieceBK || PieceAt(&g->Board, H8) != PieceBR) 
            {
                printf("info string ERROR black O-O flag is set but no longer possible\n"); 
                valid = false; 
            }
        }
        if (g->Castle & CastleBQ) 
        {
            if (PieceAt(&g->Board, E8) != PieceBK || PieceAt(&g->Board, A8) != PieceBR) 
            {
                printf("info string ERROR black O-O-O flag is set but no longer possible\n"); 
                valid = false; 
            }
        }
    }

    // hash 
    {
        Zobrist hash = 0; 

        // color 
        if (g->Turn) 
        {
            hash ^= HashColor(); 
        }

        // castling 
        hash ^= HashCastleFlags(g->Castle); 

        // ep 
        hash ^= HashEnPassant(g->EnPassant); 

        // pieces 
        for (Piece pc = 0; pc < NumPieces; pc++) 
        {
            FOR_EACH_BIT(g->Pieces[pc], 
            {
                hash ^= HashSquarePiece(sq, pc); 
            });
        }

        if (hash != g->Hash) 
        {
            printf("info string ERROR zobrist hashing is invalid: ");
            PrintHashEnd(g->Hash, " instead of "); 
            PrintHashEnd(hash, ", remainder: ");
            FindPrintHash(hash ^ g->Hash);  
            valid = false;  
        }
    }

    if (!valid) 
    {
        printf("info string ERROR game state is not valid, exitting\n"); 
        PrintGame(g); 
    }

    return valid; 
}

static inline U64 PerftInternal(Game* g, int depth) 
{
    if (depth == 1)
    {
        MoveList moves; 
        GenMoves(g, &moves); 
        U64 total = 0; 

        for (U64 i = 0; i < moves.Size; i++) 
        {
            total += IsLegal(g, moves.Moves[i]); 
        }
        return total; 
    }
    else if (depth > 1) 
    {
        MoveList moves; 
        GenMoves(g, &moves); 
        U64 total = 0; 

        for (U64 i = 0; i < moves.Size; i++) 
        {
            Move mv = moves.Moves[i]; 
            if (TryPushMove(g, mv)) 
            {
                total += PerftInternal(g, depth - 1); 
                PopMove(g, mv); 
            }
        }
        return total; 
    }
    else 
    {
        return 1; 
    }
}

U64 Perft(Game* g, int depth) 
{
    U64 total = PerftInternal(g, depth); 
    printf("Depth: %d, Total: %" PRIu64 "", depth, total); 
    return total; 
}
