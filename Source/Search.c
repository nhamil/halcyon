/**
 * @file Search.c
 * @author Nicholas Hamilton 
 * @date 2023-01-19
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Implements negamax search and various optimizations. 
 */

#include "Search.h" 
#include "Game.h"
#include "Move.h"
#include "MoveGen.h"
#include "Piece.h"
#include "Vector.h"

#include <string.h>
#include <time.h> 

#define CHECK_TIME 100000

typedef struct SearchData SearchData; 

static const int PcValues[] = 
{
    100, 310, 320, 500, 900, 10000, 0
};

static inline bool OutOfTime(const SearchCtx* ctx) 
{
    // need at least depth 1
    return ctx->PV.NumMoves && (ctx->ShouldExit || (ctx->TgtTime >= 0 && clock() > ctx->EndAt)); 
}

static inline clock_t NSecFromNow(int num) 
{
    return clock() + num * CLOCKS_PER_SEC; 
}

static inline bool HandleOutOfTime(SearchCtx* ctx) 
{
    if (ctx->CheckTime++ < CHECK_TIME) return false; 

    ctx->CheckTime = 0; 

    // check for time limit
    if (OutOfTime(ctx)) 
    {
        if (ctx->PV.NumMoves) 
        {
            printf("bestmove "); 
            PrintMoveEnd(ctx->PV.Moves[0], "\n"); 
            fflush(stdout); 
        }
        else 
        {
            // no best move 
            printf("bestmove a1a1\n"); 
            fflush(stdout); 
        }

        pthread_exit(NULL); 
        return true; 
    }

    clock_t curTime = clock(); 
    if (curTime >= ctx->NextMsgAt) 
    {
        U64 nodes = ctx->Board->Nodes; 
        double dur = (double) (curTime - ctx->StartAt) / CLOCKS_PER_SEC; 
        double nps = nodes / dur; 
        printf("info depth %d time %.0f nodes %" PRIu64 " nps %.0f hashfull %" PRIu64 "\n", ctx->Depth+1, dur*1000, nodes, nps, 1000*ctx->TT.Used/ctx->TT.Size);
        fflush(stdout); 
        ctx->NextMsgAt = NSecFromNow(1); 
    }

    return false; 
}

// assumes the move is a capture 
static inline int Mvvlva(SearchCtx* ctx, Move mv) 
{
    Piece pcType = GetNoCol(FromPc(mv)); 
    return 10 * (PcValues[GetNoCol(PcAt(ctx->Board, ToSq(mv)))] - PcValues[pcType]); 
}

static inline int QMoveVal(SearchCtx* ctx, Move mv) 
{
    if (IsCapture(mv)) 
    {
        return Mvvlva(ctx, mv); 
    }

    if (IsPro(mv)) 
    {
        return PcValues[GetNoCol(ProPc(mv))]; 
    }

    if (IsCheck(mv)) 
    {
        return 100000; 
    }

    return -100000; 
}

static inline Move NextQMove(SearchCtx* ctx, U64 start) 
{
    MvList* moves = ctx->Moves; 

    // swap current index with highest priority move 
    U64 bestI = start; 
    int bestVal = Mvvlva(ctx, moves->Moves[start]); 
    for (U64 i = start + 1; i < moves->Size; i++) 
    {
        Move mv = moves->Moves[i]; 
        int val = QMoveVal(ctx, mv); 
        
        if (val > bestVal) 
        {
            bestI = i; 
            bestVal = val; 
        }
    }

    // put best move in first place and return it 
    SwapMvList(moves, start, bestI); 
    return moves->Moves[start]; 
}

static inline int MoveVal(SearchCtx* ctx, Move mv) 
{
    Game* g = ctx->Board; 
    Piece pcType = GetNoCol(FromPc(mv)); 

    int add = IsCheck(mv) * 1000000; 

    // previous pv has highest priority 
    // first move is ply 1, not ply 0
    // so check <= instead of <
    if (ctx->InPV && ctx->Ply <= (S64) ctx->PV.NumMoves) 
    {
        if (mv == ctx->PV.Moves[ctx->Ply - 1]) return 2000000 + add;  
    }

    // capture
    if (IsCapture(mv)) 
    {
        return 100000 + Mvvlva(ctx, mv) + add; 
    }

    bool q = IsQuiet(mv); 
    if (q) 
    {
        // killer move 
        if (ctx->Killer[ctx->Ply][0] == mv) 
        {
            return 100001 + add; 
        }
        if (ctx->Killer[ctx->Ply][1] == mv) 
        {
            return 100000 + add; 
        }

        // history heuristic 
        int hist = ctx->History[g->Turn][pcType][ToSq(mv)]; 
        if (hist > 0) 
        {
            return hist + add; 
        }
    }
    
    // promotion
    if (IsPro(mv)) 
    {
        return PcValues[GetNoCol(ProPc(mv))] + add; 
    }

    // default 
    if (g->Turn == COL_B) 
    {
        return -100000 + PcSq[0][pcType][ToSq(mv)] - PcSq[0][pcType][FromSq(mv)] + add; 
    }
    else // COL_W
    {
        return -100000 + PcSq[0][pcType][RRank(ToSq(mv))] - PcSq[0][pcType][RRank(FromSq(mv))] + add; 
    }
}

static inline Move NextMove(SearchCtx* ctx, U64 start) 
{
    MvList* moves = ctx->Moves; 

    // swap current index with highest priority move 
    U64 bestI = start; 
    int bestVal = MoveVal(ctx, moves->Moves[start]); 
    for (U64 i = start + 1; i < moves->Size; i++) 
    {
        Move mv = moves->Moves[i]; 
        int val = MoveVal(ctx, mv); 
        
        if (val > bestVal) 
        {
            bestI = i; 
            bestVal = val; 
        }
    }

    // put best move in first place and return it 
    SwapMvList(moves, start, bestI); 
    return moves->Moves[start]; 
}

static inline int QSearch(SearchCtx* ctx, int alpha, int beta, int depth) 
{
    Game* g = ctx->Board; 
    MvList* moves = ctx->Moves; 
    U64 start = moves->Size; 

    ctx->NumQNodes++; 

    HandleOutOfTime(ctx); 

    bool draw = IsSpecialDraw(g); 
    if (draw) return -ctx->ColContempt * ColSign(g->Turn); 

    GenMoves(g, moves); 
    U64 numMoves = moves->Size - start; 

    int standPat = ColSign(g->Turn) * Evaluate(g, numMoves, draw, -ctx->ColContempt); 

    // check for beta cutoff
    if (standPat >= beta) 
    {
        PopMvList(moves, start); 
        return beta; 
    }
    
    // check if doing nothing improves score 
    if (standPat > alpha) 
    {
        alpha = standPat; 
    }

    // leaf node 
    if (depth <= 0) 
    {
        ctx->NumQLeaves++; 
        PopMvList(moves, start); 
        return alpha; 
    }

    // search active moves 
    bool foundMove = false; 
    if (!draw) 
    {
        ctx->Ply++; 
        for (U64 i = start; i < moves->Size; i++) 
        {
            Move mv = NextQMove(ctx, i); 

            bool shouldSearch = IsTactical(mv); 
            if (!shouldSearch) continue; 

            // there is 1+ active moves, so not a leaf node 
            foundMove = true; 

            // search move 
            PushMove(g, mv); 
            int score = -QSearch(ctx, -beta, -alpha, depth - 1); 
            PopMove(g, mv); 

            // beta cutoff 
            if (score >= beta) 
            {
                ctx->Ply--; 
                PopMvList(moves, start); 
                return beta; 
            }

            // pv node 
            if (score > alpha) 
            {
                alpha = score; 
            }
        }
        ctx->Ply--; 
    }
    if (!foundMove) ctx->NumQLeaves++; 

    PopMvList(moves, start); 
    return alpha; 
}

static inline void UpdatePV(SearchCtx* ctx, Move mv, int offset) 
{
    PVLine* dst = ctx->Lines + ctx->Ply + offset; 
    PVLine* src = ctx->Lines + ctx->Ply + 1 + offset; 

    dst->Moves[0] = mv; 
    memcpy(dst->Moves + 1, src->Moves, src->NumMoves * sizeof(Move)); 
    dst->NumMoves = src->NumMoves + 1; 
}

static inline void ClearPV(SearchCtx* ctx, int offset) 
{
    ctx->Lines[ctx->Ply + offset].NumMoves = 0; 
}

static inline int Negamax(SearchCtx* ctx, int alpha, int beta, int depth) 
{
    // reset data on root node 
    if (ctx->Ply == 0) 
    {
        ctx->NumNodes = 0; 
        ctx->NumLeaves = 0; 
        ctx->NumQNodes = 0; 
        ctx->NumQLeaves = 0; 
        ctx->CheckTime = 0; 
        memset(ctx->Killer, 0, sizeof(ctx->Killer)); 
        memset(ctx->History, 0, sizeof(ctx->History)); 
        ctx->NullMove = true; 
        ctx->InPV = true; 
    }

    // init node 
    ClearPV(ctx, 0); 
    ctx->NumNodes++; 

    // break out if search should end 
    HandleOutOfTime(ctx); 

    Game* g = ctx->Board; 
    MvList* moves = ctx->Moves; 
    U64 start = moves->Size; 

    int alphaOrig = alpha; 

    // 3-fold repetition etc 
    bool draw = IsSpecialDraw(g); 
    if (draw && ctx->Ply > 0) 
    {
        return -ctx->ColContempt * ColSign(g->Turn); 
    }

    // collect all moves from current position 
    // these moves must be cleared before returning
    GenMoves(g, moves); 
    U64 numMoves = moves->Size - start; 

    // end of search or end of game 
    if (depth <= 0 || numMoves == 0 || (draw && ctx->Ply > 0)) 
    {
        // no move found for node so this is a leaf node
        ClearPV(ctx, 0); 
        ctx->NumLeaves++; 

        PopMvList(moves, start); 
        return QSearch(ctx, alpha, beta, 16); 
    }

    if (!ctx->InPV && ctx->Ply > 0) 
    {
        TTableEntry* entry = FindTTableEntry(&ctx->TT, g->Hash, depth, g); 
        if (entry) 
        {
            if (entry->Type == PV_NODE) 
            {
                PopMvList(moves, start); 
                return entry->Score; 
            }
            else if (entry->Type == FAIL_HIGH) 
            {
                // alpha = max(entry, alpha)
                if (entry->Score > alpha) alpha = entry->Score; 
            }
            else if (entry->Type == FAIL_LOW) 
            {
                // beta = min(entry, beta) 
                if (entry->Score < beta) beta = entry->Score; 
            }

            if (alpha >= beta) 
            {
                PopMvList(moves, start); 
                return beta; 
            }
        }
    }

    if (ctx->NullMove) 
    {
        static const int R = 2; 
        // don't do null move if: 
        // - depth is too shallow 
        // - in check 
        // - either side has only pawns 
        if (depth >= 1 + R && !g->InCheck && !AnySideKP(g)) 
        {
            ctx->Ply++; 
            ctx->NullMove = false; 
            bool pv = ctx->InPV; 
            ctx->InPV = false; 

            PushNullMove(g); 
            // check if full search would have beta cutoff 
            int score = -Negamax(ctx, -beta, -beta + 1, depth - 1 - R); 
            PopNullMove(g); 

            if (score >= beta) 
            {
                ctx->NullMove = true; 
                ctx->Ply--; 

                ClearPV(ctx, 0); 
                PopMvList(moves, start); 
                return beta; 
            }

            ctx->InPV = pv; 
            ctx->NullMove = true; 
            ctx->Ply--; 
        }
    }

    // search moves if there is remaining depth 
    // must reset: 
    // - Ply 
    // - InPV

    ctx->Ply++; 
    int score = -EVAL_MAX; 
    bool foundPV = false; 
    bool nodeInPV = ctx->InPV; 
    for (U64 i = start; i < moves->Size; i++) 
    {
        Move mv = NextMove(ctx, i); 

        if (ctx->Ply == 1)
        {
            clock_t curTime = clock(); 
            if (curTime >= ctx->CurMoveAt)
            {
                printf("info currmove "); 
                PrintMoveEnd(mv, " currmovenumber "); 
                printf("%d\n", (int) (i-start+1)); 
                fflush(stdout); 
            }
            
        }

        bool capture = IsCapture(mv); 
        bool pro = IsPro(mv); 
        bool check = g->InCheck; 

        // search position after applying move 
        PushMove(g, mv); 
        bool givesCheck = g->InCheck; 

        // false if no further searching (and pruning) is needed
        bool fullSearch = true; 

        bool lmr = true; 
        lmr &= !capture; 
        lmr &= !pro; 
        lmr &= !check; 
        lmr &= !givesCheck; 
        lmr &= i - start >= 4;
        lmr &= depth > 3; 
        int lmrAmt = lmr * 2; 

        if (!check && !givesCheck && foundPV) // principal variation search 
        {
            // check if the move is at all better than current best 
            score = -Negamax(ctx, -alpha - 1, -alpha, depth - 1 - lmrAmt); 

            if (score <= alpha || score >= beta) 
            {
                fullSearch = false; 
            }
        }
        else if (lmr) // late move reduction
        {
            score = -Negamax(ctx, -alpha - 1, -alpha, depth - 1 - lmrAmt); 
        
            if (score <= alpha || score >= beta) 
            {
                fullSearch = false; 
            }
        }

        if (fullSearch) 
        {
            score = -Negamax(ctx, -beta, -alpha, depth - 1); 
        }
        
        PopMove(g, mv); 

        // beta cutoff 
        if (score >= beta) 
        {
            if (IsQuiet(mv)) 
            {
                // killer move heuristic 
                if (ctx->Killer[ctx->Ply][0] != mv) 
                {
                    ctx->Killer[ctx->Ply][1] = ctx->Killer[ctx->Ply][0]; 
                    ctx->Killer[ctx->Ply][0] = mv; 
                }

                // history heuristic 
                ctx->History[g->Turn][GetNoCol(FromPc(mv))][ToSq(mv)] = depth * depth; 
            }

            // ctx->Ply--; 
            // ctx->InPV = nodeInPV; 
            // PopMvList(moves, start); 
            // return beta; 
            alpha = beta; 
            break; 
        }

        // improves score: pv node 
        if (score > alpha) 
        {
            foundPV = true; 
            alpha = score; 
            UpdatePV(ctx, mv, -1); // ply is incremented right now: -1 offset
        }

        // only way to be in previous PV is to be the leftmost node 
        ctx->InPV = false; 
    }
    ctx->Ply--; 
    ctx->InPV = nodeInPV; 

    int ttType = PV_NODE; 
    if (alpha <= alphaOrig) 
    {
        ttType = FAIL_LOW; 
    }
    else if (alpha >= beta) 
    {
        ttType = FAIL_HIGH; 
    }

    UpdateTTable(&ctx->TT, g->Hash, ttType, alpha, depth, g); 

    PopMvList(moves, start); 

    return alpha; 
}

static inline void UpdateSearch(SearchCtx* ctx, clock_t searchStart, clock_t start, clock_t end, int depth, int eval, const PVLine* line) 
{
    (void) start; 

    U64 nodes = ctx->Board->Nodes;// ctx->NumLeaves; 

    float startDuration = (end - searchStart); 
    if (startDuration <= 0) startDuration = 1; 
    float nps = nodes / (startDuration / CLOCKS_PER_SEC); 
    startDuration /= CLOCKS_PER_SEC * 0.001; 

    ctx->PV = *line; 
    ctx->Nodes = nodes; 
    ctx->Nps = (U64) nps; 
    ctx->Depth = depth; 
    ctx->Eval = eval; 

    if (eval > 90000 || eval < -90000) 
    {
        int matePly = 100000 - abs(eval); 
        int plies = matePly - ctx->Board->Ply + 1; 
        printf("info depth %d seldepth %zu multipv 1 score mate %d time %.0f nodes %" PRIu64 " nps %.0f hashfull %.0f pv ", 
            depth, 
            ctx->PV.NumMoves, 
            (eval > 0 ? 1 : -1) * (plies/2), 
            startDuration, 
            nodes, 
            nps, 
            1000.0f * ctx->TT.Used / ctx->TT.Size); 
    }
    else 
    {
        printf("info depth %d seldepth %zu multipv 1 score cp %d time %.0f nodes %" PRIu64 " nps %.0f hashfull %.0f pv ", 
            depth, 
            ctx->PV.NumMoves, 
            eval, 
            startDuration, 
            nodes, 
            nps, 
            1000.0f * ctx->TT.Used / ctx->TT.Size); 
    }
    for (U64 i = 0; i < ctx->PV.NumMoves; i++) 
    {
        PrintMoveEnd(ctx->PV.Moves[i], " "); 
    }
    printf("\n"); 
    // printf("info string TT pct %.1f hits %" PRIu64 " coll %" PRIu64 " srch %" PRIu64 " hitpct %.1f\n", 
    //     100.0f * ctx->TT.Used / ctx->TT.Size, 
    //     ctx->TT.Hits, 
    //     ctx->TT.Collisions, 
    //     ctx->TT.Searches, 
    //     100.0f * ctx->TT.Hits / ctx->TT.Searches
    // );
    // printf("info string nodes %zu leaves %zu mbf %.2f qpct %.0f\n", ctx->NumNodes, ctx->NumLeaves, (double) ctx->NumNodes / (ctx->NumNodes - ctx->NumLeaves), 100.0 * ctx->NumQNodes / (ctx->NumNodes + ctx->NumQNodes)); 
    fflush(stdout); 
}

void RunSearch(SearchCtx* ctx) 
{
    clock_t start, end, searchStart = clock(); 

    int tgtDepth = ctx->TgtDepth; 
    if (tgtDepth < 0) tgtDepth = INT_MAX; 
    if (tgtDepth > MAX_DEPTH) tgtDepth = MAX_DEPTH; 

    start = clock(); 
    int eval = Negamax(ctx, -EVAL_MAX, EVAL_MAX, 1); 
    end = clock(); 
    UpdateSearch(ctx, searchStart, start, end, 1, eval, &ctx->Lines[0]); 

    for (int depth = 2; depth <= tgtDepth; depth++) 
    {
        ClearMvList(ctx->Moves); 
        int last = eval; 

        int a[] = { last - 30, last - 130, last - 530, -EVAL_MAX }; 
        int b[] = { last + 30, last + 130, last + 530, EVAL_MAX }; 
        int ai = 0; 
        int bi = 0; 

        start = clock(); 
        while (true) 
        {
            eval = Negamax(ctx, a[ai], b[bi], depth); 

            if (eval <= a[ai]) 
            {
                ai++; 
            }
            else if (eval >= b[bi]) 
            {
                bi++; 
            }
            else 
            {
                break; 
            }
        }
        end = clock(); 
        
        UpdateSearch(ctx, searchStart, start, end, depth, eval, &ctx->Lines[0]); 
    }

    ctx->Running = false; 

    printf("bestmove "); 
    PrintMoveEnd(ctx->Lines[0].Moves[0], "\n"); 
    fflush(stdout); 
}

static void* StartPThreadSearch(void* data) 
{
    SearchCtx* ctx = data; 
    RunSearch(ctx); 

    return NULL; 
}

void CreateSearchCtx(SearchCtx* ctx) 
{
    memset(ctx, 0, sizeof(SearchCtx)); 

    // board should always be initialized
    ctx->Board = NewGame(); 
    ctx->Moves = NewMvList(); 
    ctx->Contempt = 50; 
    CreateTTable(&ctx->TT, 1); 

    pthread_mutex_init(&ctx->Lock, NULL); 
} 

void DestroySearchCtx(SearchCtx* ctx) 
{
    StopSearchCtx(ctx); 
    FreeGame(ctx->Board); 
    FreeMvList(ctx->Moves); 
    DestroyTTable(&ctx->TT); 
    pthread_mutex_destroy(&ctx->Lock); 
}

void StopSearchCtx(SearchCtx* ctx) 
{
    if (ctx->Running) 
    {
        ctx->ShouldExit = true; 
        pthread_join(ctx->Thread, NULL); 

        ClearMvList(ctx->Moves); 
        // ResetTTable(&ctx->TT); 
        ctx->Running = false; 
    }

    ctx->ShouldExit = false; 
    fflush(stdout); 
}

void Search(SearchCtx* ctx, SearchParams* params) 
{
    StopSearchCtx(ctx); 

    ctx->PV.NumMoves = 0; 
    ctx->Nodes = 0; 
    ctx->Nps = 0; 
    ctx->Depth = 0; 
    ctx->Eval = 0; 
    ctx->Running = true; 
    ctx->StartAt = clock(); 
    ctx->CurMoveAt = NSecFromNow(2); 
    ctx->NextMsgAt = NSecFromNow(1); 

    ClearMvList(ctx->Moves); 
    ctx->Ply = 0; 
    for (int i = 0; i < MAX_DEPTH; i++) 
    {
        ctx->Lines[i].NumMoves = 0; 
    }

    ctx->TgtDepth = params->Depth; 
    ctx->TgtTime = params->TimeMs; 
    ctx->ShouldExit = false; 
    if (ctx->TgtTime >= 0) 
    {
        ctx->EndAt = clock() + ctx->TgtTime * CLOCKS_PER_SEC / 1000; 
    }

    CopyGame(ctx->Board, params->Board); 
    NoDepth(ctx->Board); 

    ctx->StartCol = ctx->Board->Turn; 
    ctx->ColContempt = ColSign(ctx->StartCol) * ctx->Contempt; 

    pthread_create(&ctx->Thread, NULL, StartPThreadSearch, ctx); 
}
