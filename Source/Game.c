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

#include "BBoard.h"
#include "Castle.h"
#include "MBox.h"
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
    InitZb(); 
    InitMBox(&g->Mailbox); 
    memset(g->Pieces, 0, sizeof(g->Pieces)); 
    memset(g->Colors, 0, sizeof(g->Colors)); 
    memset(g->Counts, 0, sizeof(g->Counts)); 
    g->All = 0; 
    g->Movement = 0; 
    g->Hash = 0; 
    g->Castle = 0; 
    g->EP = NO_SQ; 
    g->Ply = 0; 
    g->Halfmove = 0; 
    g->Turn = COL_W; 
    g->InCheck = false; 
    g->Nodes = 0; 
    g->Depth = 0; 
}

void CopyGame(Game* g, const Game* from) 
{
    InitZb(); 
    g->Mailbox = from->Mailbox; 
    memcpy(g->Pieces, from->Pieces, sizeof(g->Pieces)); 
    memcpy(g->Colors, from->Colors, sizeof(g->Colors)); 
    memcpy(g->Counts, from->Counts, sizeof(g->Counts)); 
    g->All = from->All; 
    g->Movement = from->Movement; 
    g->Hash = from->Hash; 
    g->Castle = from->Castle; 
    g->EP = from->EP; 
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

#define GAME_EQ(prop) if (a->prop != b->prop) { printf("Not equal: %s\n", #prop); return false; }

bool EqualsTTableGame(const Game* a, const Game* b) 
{
    if (!EqualsMBox(&a->Mailbox, &b->Mailbox)) { printf("Not equal: Mailbox\n"); return false; } 
    if (memcmp(a->Pieces, b->Pieces, sizeof(a->Pieces) - sizeof(BBoard)) != 0) { printf("Not equal: Pieces\n"); return false; }
    if (memcmp(a->Colors, b->Colors, sizeof(a->Colors)) != 0) { printf("Not equal: Colors\n"); return false; }
    if (memcmp(a->Counts, b->Counts, sizeof(a->Counts) - sizeof(int)) != 0) { printf("Not equal: Counts\n"); return false; }
    GAME_EQ(All); 
    GAME_EQ(Movement); 
    GAME_EQ(Hash); 
    GAME_EQ(Castle); 
    GAME_EQ(EP); 
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
    char fen[FEN_LEN]; 
    ToFen(g, fen); 

    fprintf(out, "+---+---+---+---+---+---+---+---+\n"); 
    for (int rank = 7; rank >= 0; rank--) 
    {
        fprintf(out, "| "); 
        for (int file = 0; file < 8; file++) 
        {
            Square sq = MakeSq(file, rank); 
            fprintf(out, "%s | ", StrPc(PcAt(g, sq))); 
        }
        fprintf(out, "%d\n", rank + 1); 
        fprintf(out, "+---+---+---+---+---+---+---+---+\n"); 
    }
    fprintf(out, "  A   B   C   D   E   F   G   H  \n"); 

    for (Color col = COL_W; col < NUM_COLS; col++) 
    {
        for (Piece pc = PC_P; pc < NUM_PC_TYPES; pc++) 
        {
            Piece cpc = MakePc(pc, col); 
            fprintf(out, "%d%s ", g->Counts[cpc], StrPc(cpc)); 
        }
    }
    fprintf(out, "\n"); 

    fprintf(out, "%s\nHash: ", fen); 
    FilePrintZbEnd(g->Hash, "\n", out);

    fprintf(out, "Ply: %d, ", g->Ply); 
    fprintf(out, "Halfmove: %d, ", g->Halfmove); 
    fprintf(out, "Castling: %s\n", StrCastle(g->Castle)); 

    fprintf(out, "En passant: %s, ", StrSq(g->EP)); 
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

    MoveHist* hist = g->Hist + g->Depth++; 
    hist->Halfmove = g->Halfmove; 
    hist->EP = g->EP; 
    hist->Castle = g->Castle; 
    hist->InCheck = g->InCheck; 

    
    hist->Hash = g->Hash; 

    Color col = g->Turn; 
    Color opp = OppCol(col); 

    g->Hash ^= ColZb(); 
    g->Hash ^= EPZb(g->EP); 

    Piece pc = FromPc(mv); 
    Piece pro = ProPc(mv); 
    Piece tgt = TgtPc(mv); 

    Square src = FromSq(mv); 
    Square dst = ToSq(mv); 

    bool ep = IsEP(mv); 
    int casIdx = CastleIdx(mv); 

    BBoard srcPos = BB[src]; 
    BBoard dstPos = BB[dst]; 

    if (IsCapture(mv) || GetNoCol(pc) == PC_P) 
    {
        g->Halfmove = 0; 
    }
    else 
    {
        g->Halfmove++; 
    }

    if (ep) // en passant 
    {
        Square rm = g->EP - 8 * ColSign(g->Turn); 
        BBoard rmPos = BB[rm]; 

        g->Colors[col] ^= srcPos ^ dstPos; 
        g->Colors[opp] ^= rmPos; 

        g->Hash ^= SqPcZb(src, pc); 
        g->Hash ^= SqPcZb(dst, pc); 
        g->Hash ^= SqPcZb(rm, tgt); 

        g->Pieces[pc] ^= srcPos ^ dstPos; 
        g->Pieces[tgt] ^= rmPos; 
        
        ClearMBox(&g->Mailbox, src); 
        ClearMBox(&g->Mailbox, rm); 
        SetMBox(&g->Mailbox, dst, pc); 

        g->Counts[tgt]--; 
        g->EP = NO_SQ; 
    }
    else if (casIdx) // castling 
    {
        g->Colors[col] ^= MoveCastleBBAll[casIdx]; 

        g->Hash ^= SqPcZb(MoveCastleSqK[casIdx][0], MoveCastlePcK[casIdx]); 
        g->Hash ^= SqPcZb(MoveCastleSqK[casIdx][1], MoveCastlePcK[casIdx]); 
        g->Hash ^= SqPcZb(MoveCastleSqR[casIdx][0], MoveCastlePcR[casIdx]); 
        g->Hash ^= SqPcZb(MoveCastleSqR[casIdx][1], MoveCastlePcR[casIdx]); 

        g->Pieces[pc] ^= MoveCastleBBK[casIdx]; 
        g->Pieces[MakePc(PC_R, col)] ^= MoveCastleBBR[casIdx]; 

        ClearMBox(&g->Mailbox, MoveCastleSqK[casIdx][0]); 
        ClearMBox(&g->Mailbox, MoveCastleSqR[casIdx][0]); 
        SetMBox(&g->Mailbox, MoveCastleSqK[casIdx][1], pc); 
        SetMBox(&g->Mailbox, MoveCastleSqR[casIdx][1], MakePc(PC_R, col)); 

        g->EP = NO_SQ; 
    }
    else if (pc != pro) // promotion
    {
        g->Colors[col] ^= srcPos ^ dstPos; 
        g->Colors[opp] ^= (tgt != NO_PC) * dstPos; 

        g->Hash ^= SqPcZb(src, pc); 
        g->Hash ^= SqPcZb(dst, pro); 
        g->Hash ^= SqPcZb(dst, tgt); 

        g->Pieces[pc] ^= srcPos; 
        g->Pieces[pro] ^= dstPos; 
        g->Pieces[tgt] ^= dstPos; 
        
        ClearMBox(&g->Mailbox, src); 
        SetMBox(&g->Mailbox, dst, pro); 

        g->Counts[tgt]--; 
        g->Counts[pc]--; 
        g->Counts[pro]++; 
        g->EP = NO_SQ; 
    }
    else // standard move 
    {
        g->Colors[col] ^= srcPos ^ dstPos; 
        g->Colors[opp] ^= (tgt != NO_PC) * dstPos; 

        g->Hash ^= SqPcZb(src, pc); 
        g->Hash ^= SqPcZb(dst, pc); 
        g->Hash ^= SqPcZb(dst, tgt); 

        g->Pieces[pc] ^= srcPos ^ dstPos; 
        g->Pieces[tgt] ^= dstPos; 
        
        ClearMBox(&g->Mailbox, src); 
        SetMBox(&g->Mailbox, dst, pc); 

        g->Counts[tgt]--; 
        if (GetNoCol(pc) == PC_P && abs(src - dst) == 16) 
        {
            g->EP = ColSign(g->Turn) * 8 + src; 
        }
        else 
        {
            g->EP = NO_SQ; 
        }
    }

    g->Castle &= ~(MoveCastleRm[src] | MoveCastleRm[dst]); 
    g->All = g->Colors[COL_W] | g->Colors[COL_B]; 

    g->Hash ^= EPZb(g->EP); 
    g->Hash ^= CastleZb(g->Castle ^ hist->Castle); 

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
    Color col = OppCol(opp); 

    g->Ply--; 
    g->Turn = col; 

    MoveHist* hist = g->Hist + --g->Depth; 
    g->Halfmove = hist->Halfmove; 
    g->EP = hist->EP; 
    g->Castle = hist->Castle; 
    g->InCheck = hist->InCheck; 
    g->Hash = hist->Hash; 

    Piece pc = FromPc(mv); 
    Piece pro = ProPc(mv); 
    Piece tgt = TgtPc(mv); 

    Square src = FromSq(mv); 
    Square dst = ToSq(mv); 

    bool ep = IsEP(mv); 
    int casIdx = CastleIdx(mv); 

    BBoard srcPos = BB[src]; 
    BBoard dstPos = BB[dst]; 

    if (ep) 
    {
        BBoard rmPos = BB[g->EP - 8 * ColSign(g->Turn)]; 

        g->Colors[col] ^= srcPos ^ dstPos; 
        g->Colors[opp] ^= rmPos; 

        g->Pieces[pc] ^= srcPos ^ dstPos; 
        g->Pieces[tgt] ^= rmPos; 
        
        SetMBox(&g->Mailbox, src, pc); 
        SetMBox(&g->Mailbox, g->EP - 8 * ColSign(g->Turn), tgt); 
        ClearMBox(&g->Mailbox, dst); 

        g->Counts[tgt]++; 
    }
    else if (casIdx) 
    {
        g->Colors[col] ^= MoveCastleBBAll[casIdx]; 

        g->Pieces[pc] ^= MoveCastleBBK[casIdx]; 
        g->Pieces[MakePc(PC_R, col)] ^= MoveCastleBBR[casIdx]; 

        ClearMBox(&g->Mailbox, MoveCastleSqK[casIdx][1]); 
        ClearMBox(&g->Mailbox, MoveCastleSqR[casIdx][1]); 
        SetMBox(&g->Mailbox, MoveCastleSqK[casIdx][0], pc); 
        SetMBox(&g->Mailbox, MoveCastleSqR[casIdx][0], MakePc(PC_R, col)); 
    }
    else if (pc != pro) 
    {
        g->Colors[col] ^= srcPos ^ dstPos; 
        g->Colors[opp] ^= (tgt != NO_PC) * dstPos; 

        g->Pieces[pc] ^= srcPos; 
        g->Pieces[pro] ^= dstPos; 
        g->Pieces[tgt] ^= dstPos; 
        
        SetMBox(&g->Mailbox, src, pc); 
        SetMBox(&g->Mailbox, dst, tgt); 

        g->Counts[tgt]++; 
        g->Counts[pc]++; 
        g->Counts[pro]--; 
    }
    else 
    {
        g->Colors[col] ^= srcPos ^ dstPos; 
        g->Colors[opp] ^= (tgt != NO_PC) * dstPos; 

        g->Pieces[pc] ^= srcPos ^ dstPos; 
        g->Pieces[tgt] ^= dstPos; 
        
        SetMBox(&g->Mailbox, src, pc); 
        SetMBox(&g->Mailbox, dst, tgt); 

        g->Counts[tgt]++; 
    }

    g->All = g->Colors[COL_W] | g->Colors[COL_B]; 
    g->Movement = ~g->Colors[col]; 

    VALIDATE_GAME_MOVE(g, mv, "popping"); 
}

void PushNullMove(Game* g) 
{
    g->Nodes++; 

    MoveHist* hist = g->Hist + g->Depth++; 
    hist->Halfmove = g->Halfmove; 
    hist->EP = g->EP; 
    hist->Castle = g->Castle; 
    hist->InCheck = g->InCheck; 
    hist->Hash = g->Hash; 

    g->Hash ^= ColZb(); 
    g->Hash ^= EPZb(g->EP); 

    Color col = g->Turn; 
    Color opp = OppCol(col); 

    g->EP = NO_SQ; 
    g->Hash ^= EPZb(g->EP); 

    // for next movement: 
    //   can move to any square without own pieces 
    g->Movement = ~g->Colors[opp]; 

    g->Ply++; 
    g->Turn = opp; 

    g->InCheck = IsAttacked(g, Lsb(g->Pieces[MakePc(PC_K, opp)]), opp); 

    VALIDATE_GAME_MOVE(g, 0, "pushing null"); 
}

void PopNullMove(Game* g) 
{
    Color opp = g->Turn; 
    Color col = OppCol(opp); 

    g->Ply--; 
    g->Turn = col; 

    MoveHist* hist = g->Hist + --g->Depth; 
    g->Halfmove = hist->Halfmove; 
    g->EP = hist->EP; 
    g->Castle = hist->Castle; 
    g->InCheck = hist->InCheck; 
    g->Hash = hist->Hash; 

    g->All = g->Colors[COL_W] | g->Colors[COL_B]; 
    g->Movement = ~g->Colors[col]; 

    VALIDATE_GAME_MOVE(g, 0, "popping null"); 
}

bool IsSpecialDraw(const Game* g) 
{
    // 50-move rule 
    if (g->Halfmove > 99) return true; 

    for (int i = 0; i < DRAW_DEPTH; i++) 
    {
        // (g->Depth-1) is previous ply (opponent made the move)
        // so our first depth to check should be (g->Depth-2)
        int depth = (g->Depth-2) - i*2; 

        if (depth >= 0) 
        {
            if (g->Hash == g->Hist[depth].Hash) return true; 
            if ((g->Hash ^ ColZb()) == g->Hist[depth].Hash) return true; 
        }
        else 
        {
            break; 
        }
    }

    return false; 
}

bool ValidateGame(const Game* g) 
{
    bool valid = true; 

    // piece counts 
    for (Piece pc = 0; pc < NUM_PC; pc++) 
    {
        if (g->Counts[pc] != Popcnt(g->Pieces[pc])) 
        {
            printf("info string ERROR %s count is %d but should be %d\n", StrPc(pc), g->Counts[pc], Popcnt(g->Pieces[pc])); 
            valid = false; 
        }
    }

    // exactly 1 king 
    {
        for (Color col = COL_W; col < NUM_COLS; col++) 
        {
            if (g->Counts[MakePc(PC_K, col)] != 1) 
            {
                printf("info string ERROR %s does not have exactly 1 king: %d\n", col ? "white" : "black", g->Counts[MakePc(PC_K, col)]); 
                valid = false; 
            }
        }
    }

    // bitboards 
    {
        BBoard all = 0, col[2] = { 0, 0 }; 
        for (Piece pc = 0; pc < NUM_PC; pc++) 
        {
            all ^= g->Pieces[pc]; 
            col[GetCol(pc)] ^= g->Pieces[pc]; 
        }

        if (all != g->All) { printf("info string ERROR piece bitboards don't match accumulated board\n"); valid = false; } 
        if (col[COL_W] != g->Colors[COL_W]) { printf("info string ERROR piece bitboards don't match accumulated white board\n"); valid = false; } 
        if (col[COL_B] != g->Colors[COL_B]) { printf("info string ERROR piece bitboards don't match accumulated black board\n"); valid = false; } 
    }

    // check 
    {
        Square ksq = Lsb(g->Pieces[MakePc(PC_K, g->Turn)]); 
        bool actuallyInCheck = IsAttacked(g, ksq, g->Turn); 
        if (actuallyInCheck ^ g->InCheck) 
        {
            printf("info string ERROR check flag is incorrect: %d instead of %d\n", g->InCheck, actuallyInCheck); 
            printf("info string       king on %s\n", StrSq(ksq)); 
            valid = false; 
        }
    }

    // pieces 
    {
        for (Square sq = 0; sq < NUM_SQ; sq++) 
        {
            Piece pc = PcAt(g, sq); 
            if (pc > NO_SQ) 
            {
                printf("info string ERROR mailbox piece is invalid: %d at %s\n", pc, StrSq(sq)); 
                valid = false; 
            }
            else if (pc == NO_PC) 
            {
                if (GetBit(g->All, sq)) 
                {
                    printf("info string ERROR mailbox has no piece but should have one: %s\n", StrSq(sq)); 
                    valid = false; 
                }
            }
            else // valid piece 
            {
                if (!GetBit(g->Pieces[pc], sq)) 
                {
                    printf("info string ERROR mailbox has %s but should be empty: %s\n", StrPc(pc), StrSq(sq)); 
                    valid = false; 
                }
            }
        }
    }

    // castling 
    {
        if (g->Castle & CASTLE_WK) 
        {
            if (PcAt(g, E1) != PC_WK || PcAt(g, H1) != PC_WR) 
            {
                printf("info string ERROR white O-O flag is set but no longer possible\n"); 
                valid = false; 
            }
        }
        if (g->Castle & CASTLE_WQ) 
        {
            if (PcAt(g, E1) != PC_WK || PcAt(g, A1) != PC_WR) 
            {
                printf("info string ERROR white O-O-O flag is set but no longer possible\n"); 
                valid = false; 
            }
        }
        if (g->Castle & CASTLE_BK) 
        {
            if (PcAt(g, E8) != PC_BK || PcAt(g, H8) != PC_BR) 
            {
                printf("info string ERROR black O-O flag is set but no longer possible\n"); 
                valid = false; 
            }
        }
        if (g->Castle & CASTLE_BQ) 
        {
            if (PcAt(g, E8) != PC_BK || PcAt(g, A8) != PC_BR) 
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
            hash ^= ColZb(); 
        }

        // castling 
        hash ^= CastleZb(g->Castle); 

        // ep 
        hash ^= EPZb(g->EP); 

        // pieces 
        for (Piece pc = 0; pc < NUM_PC; pc++) 
        {
            FOR_EACH_BIT(g->Pieces[pc], 
            {
                hash ^= SqPcZb(sq, pc); 
            });
        }

        if (hash != g->Hash) 
        {
            printf("info string ERROR zobrist hashing is invalid: ");
            PrintZbEnd(g->Hash, " instead of "); 
            PrintZbEnd(hash, ", remainder: ");
            FindPrintZb(hash ^ g->Hash);  
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

static inline U64 PerftInternal(Game* g, MvList* moves, int depth) 
{
    U64 total = 0, start = moves->Size, size; 

    if (depth == 1) 
    {
        MvInfo info; 
        GenMvInfo(g, &info); 
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

        PopMvList(moves, size); 
    }
    else 
    {
        total = 1; 
    }

    return total; 
}

U64 Perft(Game* g, int depth) 
{
    MvList* moves = NewMvList(); 

    U64 total = PerftInternal(g, moves, depth); 
    printf("Depth: %d, Total: %" PRIu64 "", depth, total); 

    FreeMvList(moves); 
    return total; 
}
