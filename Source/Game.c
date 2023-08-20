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
    g->Movement = 0; 
    g->Hash = 0; 
    g->Castle = 0; 
    g->EnPassant = NoSquare; 
    g->Ply = 0; 
    g->Halfmove = 0; 
    g->Turn = ColorW; 
    g->InCheck = false; 
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
    g->Movement = from->Movement; 
    g->Hash = from->Hash; 
    g->Castle = from->Castle; 
    g->EnPassant = from->EnPassant; 
    g->Ply = from->Ply; 
    g->Halfmove = from->Halfmove; 
    g->Turn = from->Turn; 
    g->InCheck = from->InCheck; 
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
    GAME_EQ(Movement); 
    GAME_EQ(Hash); 
    GAME_EQ(Castle); 
    GAME_EQ(EnPassant); 
    // GAME_EQ(Ply); 
    // GAME_EQ(Halfmove); 
    GAME_EQ(Turn); 
    GAME_EQ(InCheck); 
    // GAME_EQ(Nodes); 
    // GAME_EQ(Depth); 
    // for (int i = 0; i < a->Depth; i++) 
    // {
    //     if (a->Hist[i] != b->Hist[i]) return false; 
    // }

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
    fprintf(out, "In check: %s\n", g->InCheck ? "yes" : "no"); 

    fprintf(out, "Special draw: %s, ", IsSpecialDraw(g) ? "yes" : "no");

    fprintf(out, "%s to move\n", g->Turn ? "Black" : "White"); 
}

void PrintGame(const Game* g) 
{
    FilePrintGame(g, stdout); 
}

void PushMove(Game* g, Move mv) 
{
    g->Nodes++; 

    // store anything that the move doesn't store 
    MoveHist* hist = g->Hist + g->Depth++; 
    hist->Halfmove = g->Halfmove; 
    hist->EnPassant = g->EnPassant; 
    hist->Castle = g->Castle; 
    hist->InCheck = g->InCheck; 
    hist->Hash = g->Hash; 

    // swap color and reset en passant hash
    g->Hash ^= HashColor(); 
    g->Hash ^= HashEnPassant(g->EnPassant); 

    Color col = g->Turn; 
    Color opp = OppositeColor(col); 

    Piece pc = FromPiece(mv); 
    Piece pro = PromotionPiece(mv); 
    Piece tgt = TargetPiece(mv); 

    Square src = FromSquare(mv); 
    Square dst = ToSquare(mv); 

    bool ep = IsEnPassant(mv); 
    int casIndex = CastleIndex(mv); 

    Bitboard srcPos = Bits[src]; 
    Bitboard dstPos = Bits[dst]; 

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
        Square rm = g->EnPassant - 8 * ColorSign(g->Turn); 
        Bitboard rmPos = Bits[rm]; 

        // move pawn from source to destination
        // remove enemy piece 

        g->Colors[col] ^= srcPos ^ dstPos; 
        g->Colors[opp] ^= rmPos; 

        g->Pieces[pc] ^= srcPos ^ dstPos; 
        g->Pieces[tgt] ^= rmPos; 
        
        g->Hash ^= HashSquarePiece(src, pc); 
        g->Hash ^= HashSquarePiece(dst, pc); 
        g->Hash ^= HashSquarePiece(rm, tgt); 

        ClearPieceAt(&g->Board, src); 
        ClearPieceAt(&g->Board, rm); 
        SetPieceAt(&g->Board, dst, pc); 

        // enemy piece has been captured 
        g->Counts[tgt]--; 

        // after en passant, a new en passant square is not possible 
        g->EnPassant = NoSquare; 
    }
    else if (casIndex) // castling 
    {
        // MoveCastleBits* arrays handle removing king and rook as well as placing them on new squares  

        g->Colors[col] ^= MoveCastleBitsAll[casIndex]; 

        g->Pieces[pc] ^= MoveCastleBitsK[casIndex]; 
        g->Pieces[MakePiece(PieceR, col)] ^= MoveCastleBitsR[casIndex]; 

        g->Hash ^= HashSquarePiece(MoveCastleSquareK[casIndex][0], MoveCastlePieceK[casIndex]); 
        g->Hash ^= HashSquarePiece(MoveCastleSquareK[casIndex][1], MoveCastlePieceK[casIndex]); 
        g->Hash ^= HashSquarePiece(MoveCastleSquareR[casIndex][0], MoveCastlePieceR[casIndex]); 
        g->Hash ^= HashSquarePiece(MoveCastleSquareR[casIndex][1], MoveCastlePieceR[casIndex]); 

        ClearPieceAt(&g->Board, MoveCastleSquareK[casIndex][0]); 
        ClearPieceAt(&g->Board, MoveCastleSquareR[casIndex][0]); 
        SetPieceAt(&g->Board, MoveCastleSquareK[casIndex][1], pc); 
        SetPieceAt(&g->Board, MoveCastleSquareR[casIndex][1], MakePiece(PieceR, col)); 

        // en passant square is not possible after castling 
        g->EnPassant = NoSquare; 
    }
    else if (pc != pro) // promotion
    {
        // remove old piece 
        // place promoted piece 
        // remove captured piece if it exists 
        // target is scratch index if there is no capture so this is okay 

        g->Colors[col] ^= srcPos ^ dstPos; 
        g->Colors[opp] ^= (tgt != NoPiece) * dstPos; 

        g->Hash ^= HashSquarePiece(src, pc); 
        g->Hash ^= HashSquarePiece(dst, pro); 
        g->Hash ^= HashSquarePiece(dst, tgt); 

        g->Pieces[pc] ^= srcPos; 
        g->Pieces[pro] ^= dstPos; 
        g->Pieces[tgt] ^= dstPos; 
        
        ClearPieceAt(&g->Board, src); 
        SetPieceAt(&g->Board, dst, pro); 

        g->Counts[tgt]--; 
        g->Counts[pc]--; 
        g->Counts[pro]++; 

        // en passant is not possible after promoting a pawn
        g->EnPassant = NoSquare; 
    }
    else // standard move 
    {
        // remove old piece and place it on new square 
        // remove captured piece if it exists 
        // target is scratch index if there is no capture so this is okay 

        g->Colors[col] ^= srcPos ^ dstPos; 
        g->Colors[opp] ^= (tgt != NoPiece) * dstPos; 

        g->Hash ^= HashSquarePiece(src, pc); 
        g->Hash ^= HashSquarePiece(dst, pc); 
        g->Hash ^= HashSquarePiece(dst, tgt); 

        g->Pieces[pc] ^= srcPos ^ dstPos; 
        g->Pieces[tgt] ^= dstPos; 
        
        ClearPieceAt(&g->Board, src); 
        SetPieceAt(&g->Board, dst, pc); 

        g->Counts[tgt]--; 

        // en passant is possible if a pawn moved two squares 
        if (TypeOfPiece(pc) == PieceP && abs((int) src - (int) dst) == 16) 
        {
            g->EnPassant = ColorSign(g->Turn) * 8 + src; 
        }
        else 
        {
            g->EnPassant = NoSquare; 
        }
    }

    // remove castling rights if: 
    // - from square is initial king or rook position (it isn't there anymore or it moved)
    // - to square is initial king or rook position (it isn't there anymore or it was captured)
    g->Castle &= ~(MoveCastleRemove[src] | MoveCastleRemove[dst]); 

    // update occupants 
    g->All = g->Colors[ColorW] | g->Colors[ColorB]; 

    // re-apply en passant hash 
    g->Hash ^= HashEnPassant(g->EnPassant); 

    // apply diff of castle flag hash 
    g->Hash ^= HashCastleFlags(g->Castle ^ hist->Castle); 

    // for next movement: 
    //   can move to any square without own pieces 
    g->Movement = ~g->Colors[opp]; 
    g->Ply++; 
    g->Turn = opp; 
    g->InCheck = IsCheck(mv); 

    VALIDATE_GAME_MOVE(g, mv, "pushing"); 
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
    g->InCheck = hist->InCheck; 
    // no need to recalculate hash 
    g->Hash = hist->Hash; 

    Piece pc = FromPiece(mv); 
    Piece pro = PromotionPiece(mv); 
    Piece tgt = TargetPiece(mv); 

    Square src = FromSquare(mv); 
    Square dst = ToSquare(mv); 

    bool ep = IsEnPassant(mv); 
    int casIndex = CastleIndex(mv); 

    Bitboard srcPos = Bits[src]; 
    Bitboard dstPos = Bits[dst]; 

    if (ep) // en passant 
    {
        // piece did not move to the same square as it captured 
        Bitboard rmPos = Bits[g->EnPassant - 8 * ColorSign(g->Turn)]; 

        // move pawn from destination to source
        // add enemy piece 

        g->Colors[col] ^= srcPos ^ dstPos; 
        g->Colors[opp] ^= rmPos; 

        g->Pieces[pc] ^= srcPos ^ dstPos; 
        g->Pieces[tgt] ^= rmPos; 
        
        SetPieceAt(&g->Board, src, pc); 
        SetPieceAt(&g->Board, g->EnPassant - 8 * ColorSign(g->Turn), tgt); 
        ClearPieceAt(&g->Board, dst); 

        // enemy piece is uncaptured 
        g->Counts[tgt]++; 
    }
    else if (casIndex) // castling 
    {
        // MoveCastleBits* arrays handle removing king and rook as well as placing them on old squares  

        g->Colors[col] ^= MoveCastleBitsAll[casIndex]; 

        g->Pieces[pc] ^= MoveCastleBitsK[casIndex]; 
        g->Pieces[MakePiece(PieceR, col)] ^= MoveCastleBitsR[casIndex]; 

        ClearPieceAt(&g->Board, MoveCastleSquareK[casIndex][1]); 
        ClearPieceAt(&g->Board, MoveCastleSquareR[casIndex][1]); 
        SetPieceAt(&g->Board, MoveCastleSquareK[casIndex][0], pc); 
        SetPieceAt(&g->Board, MoveCastleSquareR[casIndex][0], MakePiece(PieceR, col)); 
    }
    else if (pc != pro) // promotion 
    {
        // add old piece 
        // remove promoted piece 
        // add enemy piece if there was a capture  
        // target is scratch index if there is no capture so this is okay 

        g->Colors[col] ^= srcPos ^ dstPos; 
        g->Colors[opp] ^= (tgt != NoPiece) * dstPos; 

        g->Pieces[pc] ^= srcPos; 
        g->Pieces[pro] ^= dstPos; 
        g->Pieces[tgt] ^= dstPos; 
        
        SetPieceAt(&g->Board, src, pc); 
        SetPieceAt(&g->Board, dst, tgt); 

        g->Counts[tgt]++; 
        g->Counts[pc]++; 
        g->Counts[pro]--; 
    }
    else // standard move 
    {
        // add old piece and remove it from new square 
        // add captured piece if it exists 
        // target is scratch index if there is no capture so this is okay 

        g->Colors[col] ^= srcPos ^ dstPos; 
        g->Colors[opp] ^= (tgt != NoPiece) * dstPos; 

        g->Pieces[pc] ^= srcPos ^ dstPos; 
        g->Pieces[tgt] ^= dstPos; 
        
        SetPieceAt(&g->Board, src, pc); 
        SetPieceAt(&g->Board, dst, tgt); 

        g->Counts[tgt]++; 
    }

    // update occupants 
    g->All = g->Colors[ColorW] | g->Colors[ColorB]; 

    // for previous movement: 
    //   can move to any square without own pieces 
    g->Movement = ~g->Colors[col]; 

    VALIDATE_GAME_MOVE(g, mv, "popping"); 
}

void PushNullMove(Game* g) 
{
    g->Nodes++; 

    MoveHist* hist = g->Hist + g->Depth++; 
    hist->Halfmove = g->Halfmove; 
    hist->EnPassant = g->EnPassant; 
    hist->Castle = g->Castle; 
    hist->InCheck = g->InCheck; 
    hist->Hash = g->Hash; 

    Color col = g->Turn; 
    Color opp = OppositeColor(col); 

    g->Hash ^= HashColor(); 

    // opponent should not be able to en passant just because we skipped a turn 
    g->Hash ^= HashEnPassant(g->EnPassant); 
    g->EnPassant = NoSquare; 
    g->Hash ^= HashEnPassant(g->EnPassant); 

    // for next movement: 
    //   can move to any square without own pieces 
    g->Movement = ~g->Colors[opp]; 

    g->Ply++; 
    g->Turn = opp; 

    g->InCheck = IsAttacked(g, LeastSigBit(g->Pieces[MakePiece(PieceK, opp)]), opp); 

    VALIDATE_GAME_MOVE(g, 0, "pushing null"); 
}

void PopNullMove(Game* g) 
{
    Color opp = g->Turn; 
    Color col = OppositeColor(opp); 

    g->Ply--; 
    g->Turn = col; 

    MoveHist* hist = g->Hist + --g->Depth; 
    g->Halfmove = hist->Halfmove; 
    g->EnPassant = hist->EnPassant; 
    g->Castle = hist->Castle; 
    g->InCheck = hist->InCheck; 
    g->Hash = hist->Hash; 

    g->All = g->Colors[ColorW] | g->Colors[ColorB]; 
    g->Movement = ~g->Colors[col]; 

    VALIDATE_GAME_MOVE(g, 0, "popping null"); 
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

    // check 
    {
        Square ksq = LeastSigBit(g->Pieces[MakePiece(PieceK, g->Turn)]); 
        bool actuallyInCheck = IsAttacked(g, ksq, g->Turn); 
        if (actuallyInCheck ^ g->InCheck) 
        {
            printf("info string ERROR check flag is incorrect: %d instead of %d\n", g->InCheck, actuallyInCheck); 
            printf("info string       king on %s\n", SquareString(ksq)); 
            valid = false; 
        }
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

static inline U64 PerftInternal(Game* g, MoveList* moves, int depth) 
{
    U64 total = 0, start = moves->Size, size; 

    if (depth == 1) 
    {
        MoveInfo info; 
        GenMoveInfo(g, &info); 
        total += info.NumMoves; 
    }
    else if (depth > 1) 
    {
        GenMoves(g, moves); 
        size = moves->Size; 

        // PrintGame(g); 

        for (U64 i = start; i < size; i++) 
        {
            Move mv = moves->Moves[i]; 
            PushMove(g, mv); 
            total += PerftInternal(g, moves, depth - 1); 
            PopMove(g, mv); 
        }

        PopMovesToSize(moves, size); 
    }
    else 
    {
        total = 1; 
    }

    return total; 
}

U64 Perft(Game* g, int depth) 
{
    MoveList* moves = NewMoveList(); 

    U64 total = PerftInternal(g, moves, depth); 
    printf("Depth: %d, Total: %" PRIu64 "", depth, total); 

    FreeMoveList(moves); 
    return total; 
}
