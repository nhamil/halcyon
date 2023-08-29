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

/**
 * How often we should check for being out of time. 
 */
#define CheckTimeThreshold 100000

/**
 * Simplified piece values for move ordering. 
 * This should not be used for static evaluation. 
 */
static const int PieceValues[] = 
{
    100, 310, 320, 500, 900, 10000, 0
};

/**
 * Should we ignore out of time? 
 */
static bool NoHandleTime = false; 

/**
 * Checks if search should stop due to time. 
 * 
 * @param ctx Search context 
 * @return True if out of time
 */
static inline bool IsOutOfTime(const SearchContext* ctx) 
{
    // need at least depth 1
    return ctx->BestLine.NumMoves && (ctx->ShouldExit || (ctx->TargetTimeMs >= 0 && clock() > ctx->EndAt)); 
}

/**
 * @param num Number of seconds 
 * @return What the time will be `num` seconds from now 
 */
static inline clock_t NSecondsFromNow(int num) 
{
    return clock() + num * CLOCKS_PER_SEC; 
}

/**
 * Quit the search if the alloted time has run out. 
 * 
 * If search is out of time, the current best move will be printed and the thread will exit. 
 * 
 * @param ctx Search context 
 * @return True if out of time and thread is exiting
 */
static inline bool HandleOutOfTime(SearchContext* ctx) 
{
    // only check every so often to reduce affect on search speed 
    if (ctx->CheckTime++ < CheckTimeThreshold || NoHandleTime) return false; 

    ctx->CheckTime = 0; 

    // check for time limit
    if (IsOutOfTime(ctx)) 
    {
        if (ctx->BestLine.NumMoves) 
        {
            printf("bestmove "); 
            PrintMoveEnd(ctx->BestLine.Moves[0], "\n"); 
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

    // if next depth is taking too long send occasional updates 
    clock_t curTime = clock(); 
    if (curTime >= ctx->NextMessageAt) 
    {
        U64 nodes = ctx->State->Nodes; 
        double dur = (double) (curTime - ctx->StartAt) / CLOCKS_PER_SEC; 
        double nps = nodes / dur; 
        printf("info depth %d time %.0f nodes %" PRIu64 " nps %.0f hashfull %" PRIu64 "\n", ctx->Depth+1, dur*1000, nodes, nps, 1000*ctx->Transpositions.Used/ctx->Transpositions.Size);
        fflush(stdout); 
        ctx->NextMessageAt = NSecondsFromNow(1); 
    }

    return false; 
}

/**
 * Gets MVV-LVA value for move ordering. 
 * 
 * Moves are assumed to be captures, don't call this function if they are not. 
 * 
 * @param ctx Search context
 * @param mv The move
 * @return MVV-LVA value
 */
static inline int MvvLva(SearchContext* ctx, Move mv) 
{
    (void) ctx; 
    PieceType pcType = TypeOfPiece(FromPiece(mv)); 
    PieceType tgtType = TypeOfPiece(TargetPiece(mv));
    return 10 * (PieceValues[tgtType] - PieceValues[pcType]); 
}

/**
 * Move ordering values for qsearch. 
 * 
 * @param ctx Search context 
 * @param mv The move
 * @param hashMove TT move for the board position
 * @return Move value where higher values should come first 
 */
static inline int QMoveVal(SearchContext* ctx, Move mv, Move hashMove) 
{
    // push quiet moves to the end
    if (IsQuiet(mv)) return -1000000; 

    // try hash move first 
    if (mv == hashMove) return 999999998; 

    int val = 0; 

    // order captures by MVV-LVA 
    if (IsCapture(mv)) 
    {
        val += MvvLva(ctx, mv); 
    }

    // promotions 
    if (IsPromotion(mv)) 
    {
        val += 10 * PieceValues[TypeOfPiece(PromotionPiece(mv))]; 
    }

    // bonus for checks 
    if (IsCheck(mv)) 
    {
        val += 1000; 
    }

    return val; 
}

/**
 * Move ordering values for negamax search. 
 * 
 * @param ctx Search context 
 * @param mv The move
 * @param hashMove TT move for the board position
 * @return Move value where higher values should come first 
 */
static inline int MoveVal(SearchContext* ctx, Move mv, Move hashMove) 
{
    Game* g = ctx->State; 
    PieceType pcType = TypeOfPiece(FromPiece(mv)); 

    int add = IsCheck(mv) * 1000000; 

    // previous pv has highest priority 
    // first move is ply 1, not ply 0
    // so check <= instead of <
    if (ctx->InPV && ctx->Ply <= (S64) ctx->BestLine.NumMoves) 
    {
        if (mv == ctx->BestLine.Moves[ctx->Ply - 1]) return 999999999 + add;  
    }

    if (mv == hashMove) return 999999998; 

    // capture
    if (IsCapture(mv)) 
    {
        return 100000 + MvvLva(ctx, mv) + add; 
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
        int hist = ctx->History[g->Turn][pcType][ToSquare(mv)]; 
        if (hist > 0) 
        {
            return hist + add; 
        }
    }
    
    // promotion
    if (IsPromotion(mv)) 
    {
        return PieceValues[TypeOfPiece(PromotionPiece(mv))] + add; 
    }

    // default 
    if (g->Turn == ColorB) 
    {
        return -100000 + PieceSquare[0][pcType][ToSquare(mv)] - PieceSquare[0][pcType][FromSquare(mv)] + add; 
    }
    else // ColorW
    {
        return -100000 + PieceSquare[0][pcType][FlipRank(ToSquare(mv))] - PieceSquare[0][pcType][FlipRank(FromSquare(mv))] + add; 
    }
}

/**
 * Compute move order values for all moves. 
 * This does not sort moves. 
 * 
 * @param ctx Search context 
 * @param start Start index 
 * @param hashMove TT move 
 * @param moveVal Function for determining move order value 
 * @param values Output for move values
 */
static inline void GetMoveOrder(SearchContext* ctx, U64 start, Move hashMove, int (*moveVal)(SearchContext*,Move,Move), int* values) 
{
    MoveList* moves = ctx->Moves; 

    for (U64 i = start; i < moves->Size; i++) 
    {
        *(values++) = moveVal(ctx, moves->Moves[i], hashMove); 
    }
}

/**
 * Finds the next move with the highest move order value. 
 * Move values should already be computed with `GetMoveOrder`. 
 * 
 * @param ctx Search context 
 * @param start Start index 
 * @param values Input/output for move values 
 * @return 
 */
static inline Move NextMove(SearchContext* ctx, U64 start, int* values) 
{
    MoveList* moves = ctx->Moves; 

    // swap current index with highest priority move 
    U64 bestI = start; 
    int bestVal = values[0]; 
    for (U64 i = start + 1; i < moves->Size; i++) 
    {
        int val = values[i - start]; 
        
        if (val > bestVal) 
        {
            bestI = i; 
            bestVal = val; 
        }
    }

    // put best move in first place and return it 
    SwapMoves(moves, start, bestI); 

    // swap move value too 
    values[bestI - start] = values[0]; 
    values[0] = bestVal; 

    return moves->Moves[start]; 
}

/**
 * Continues search to make positions quiet and then returns board evaluation. 
 * 
 * @param ctx Search context 
 * @param alpha Lower bound score 
 * @param beta Upper bound score 
 * @param depth Remaining depth to search 
 * @return Quiescence search evaluation
 */
static inline int QSearch(SearchContext* ctx, int alpha, int beta, int depth) 
{
    Game* g = ctx->State; 
    MoveList* moves = ctx->Moves; 
    U64 start = moves->Size; 

    ctx->NumQNodes++; 

    HandleOutOfTime(ctx); 

    // no further moves are possible 
    bool draw = IsSpecialDraw(g); 
    if (draw) return -ctx->ColorContempt * ColorSign(g->Turn); 

    GenMoves(g, moves); 
    U64 numMoves = moves->Size - start; 

    int standPat = ColorSign(g->Turn) * Evaluate(g, ctx->Ply, numMoves, draw, -ctx->ColorContempt); 

    // check for beta cutoff
    if (standPat >= beta) 
    {
        PopMovesToSize(moves, start); 
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
        PopMovesToSize(moves, start); 
        return alpha; 
    }

    // check if position has a TT move 
    TTableEntry* entry = FindTTableEntry(&ctx->Transpositions, g->Hash, g); 
    Move hashMove = entry ? entry->Mv : NoMove; 

    // search tactical moves 
    bool foundMove = false; 
    if (!draw) 
    {
        ctx->Ply++; 
        int moveValues[moves->Size - start]; 
        GetMoveOrder(ctx, start, hashMove, QMoveVal, moveValues); 
        for (U64 i = start; i < moves->Size; i++) 
        {
            Move mv = NextMove(ctx, i, moveValues + (i - start)); 

            bool shouldSearch = IsTactical(mv); 
            if (!shouldSearch) continue; 

            // there are 1+ tactical moves, so not a leaf node 
            foundMove = true; 

            // search move 
            PushMove(g, mv); 
            int score = -QSearch(ctx, -beta, -alpha, depth - 1); 
            PopMove(g, mv); 

            // beta cutoff 
            if (score >= beta) 
            {
                ctx->Ply--; 
                PopMovesToSize(moves, start); 
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

    PopMovesToSize(moves, start); 
    return alpha; 
}

/**
 * Adds a new move to the current principal variation. 
 * 
 * @param ctx Search context 
 * @param mv Move to add 
 * @param offset Offset from current ply 
 */
static inline void UpdatePV(SearchContext* ctx, Move mv, int offset) 
{
    PVLine* dst = ctx->Lines + ctx->Ply + offset; 
    PVLine* src = ctx->Lines + ctx->Ply + 1 + offset; 

    dst->Moves[0] = mv; 
    memcpy(dst->Moves + 1, src->Moves, src->NumMoves * sizeof(Move)); 
    dst->NumMoves = src->NumMoves + 1; 
}

/**
 * Clears the principal variation. 
 * 
 * @param ctx Search context 
 * @param offset Offset from current ply 
 */
static inline void ClearPV(SearchContext* ctx, int offset) 
{
    ctx->Lines[ctx->Ply + offset].NumMoves = 0; 
}

/**
 * Handles ordered move looping in negamax. 
 * 
 * @param onMove Actions to perform for each move
 */
#define NEGAMAX_LOOP_MOVES(onMove) \
    /* search moves if there is remaining depth */ \
    /* must reset: */ \
    /* - Ply */ \
    /* - InPV */ \
\
    ctx->Ply++; \
    Move bestMove = NoMove; \
    int score = -MaxScore; \
    bool foundPV = false; \
    bool nodeInPV = ctx->InPV; \
    int moveValues[moves->Size - start]; \
    GetMoveOrder(ctx, start, hashMove, MoveVal, moveValues); \
    for (U64 i = start; i < moves->Size; i++) \
    {\
        Move mv = NextMove(ctx, i, moveValues + (i - start)); \
\
        onMove; \
\
        bool capture = IsCapture(mv); \
        bool pro = IsPromotion(mv); \
        bool check = g->InCheck; \
\
        /* search position after applying move */ \
        PushMove(g, mv); \
        bool givesCheck = g->InCheck; \
\
        /* false if no further searching (and pruning) is needed */ \
        bool fullSearch = true; \
\
        bool lmr = true; \
        lmr &= !capture; \
        lmr &= !pro; \
        lmr &= !check; \
        lmr &= !givesCheck; \
        lmr &= i - start >= 4; \
        lmr &= depth > 3; \
        int lmrAmt = lmr * 2; \
\
        if (!check && !givesCheck && foundPV) /* principal variation search */ \
        {\
            /* check if the move is at all better than current best */ \
            score = -Negamax_(ctx, -alpha - 1, -alpha, depth - 1 - lmrAmt); \
\
            if (score <= alpha || score >= beta) \
            {\
                fullSearch = false; \
            }\
        }\
        else if (lmr) /* late move reduction */ \
        {\
            score = -Negamax_(ctx, -alpha - 1, -alpha, depth - 1 - lmrAmt); \
        \
            if (score <= alpha || score >= beta) \
            {\
                fullSearch = false; \
            }\
        }\
\
        if (fullSearch) \
        {\
            score = -Negamax_(ctx, -beta, -alpha, depth - 1); \
        }\
        \
        PopMove(g, mv); \
\
        /* beta cutoff */ \
        if (score >= beta) \
        {\
            if (IsQuiet(mv)) \
            {\
                /* killer move heuristic */ \
                if (ctx->Killer[ctx->Ply][0] != mv) \
                {\
                    ctx->Killer[ctx->Ply][1] = ctx->Killer[ctx->Ply][0]; \
                    ctx->Killer[ctx->Ply][0] = mv; \
                }\
\
                /* history heuristic */ \
                ctx->History[g->Turn][TypeOfPiece(FromPiece(mv))][ToSquare(mv)] = depth * depth; \
            }\
\
            alpha = beta; \
            bestMove = mv; \
            break; \
        }\
\
        /* improves score: pv node */ \
        if (score > alpha) \
        {\
            foundPV = true; \
            alpha = score; \
            UpdatePV(ctx, mv, -1); /* ply is incremented right now: -1 offset */ \
            bestMove = mv; \
        }\
\
        /* only way to be in previous PV is to be the leftmost node */ \
        ctx->InPV = false; \
    } \
    ctx->Ply--; \
    ctx->InPV = nodeInPV; 

/**
 * Negamax for non-root nodes. 
 * Call `Negamax` instead. 
 * 
 * @param ctx Search context
 * @param alpha Lower bound
 * @param beta Upper bound 
 * @param depth Remaining depth 
 * @return Evaluation
 */
static inline int Negamax_(SearchContext* ctx, int alpha, int beta, int depth) 
{
    // init node 
    ClearPV(ctx, 0); 
    ctx->NumNodes++; 

    // break out if search should end 
    HandleOutOfTime(ctx); 

    Game* g = ctx->State; 
    MoveList* moves = ctx->Moves; 
    U64 start = moves->Size; 

    int alphaOrig = alpha; 

    // 3-fold repetition etc 
    bool draw = IsSpecialDraw(g); 
    if (draw) 
    {
        return -ctx->ColorContempt * ColorSign(g->Turn); 
    }

    // collect all moves from current position 
    // these moves must be cleared before returning
    GenMoves(g, moves); 
    U64 numMoves = moves->Size - start; 

    // end of search or end of game 
    if (depth <= 0 || numMoves == 0 || draw) 
    {
        // no move found for node so this is a leaf node
        ClearPV(ctx, 0); 
        ctx->NumLeaves++; 

        PopMovesToSize(moves, start); 
        return QSearch(ctx, alpha, beta, 16); 
    }

    TTableEntry* entry = FindTTableEntry(&ctx->Transpositions, g->Hash, g); 
    Move hashMove = entry ? entry->Mv : NoMove; 

    if (!ctx->InPV) 
    {
        if (entry && entry->Depth >= depth) 
        {
            if (entry->Type == PVNode) 
            {
                PopMovesToSize(moves, start); 
                return entry->Score; 
            }
            else if (entry->Type == FailHigh) 
            {
                // alpha = max(entry, alpha)
                if (entry->Score > alpha) alpha = entry->Score; 
            }
            else if (entry->Type == FailLow) 
            {
                // beta = min(entry, beta) 
                if (entry->Score < beta) beta = entry->Score; 
            }

            if (alpha >= beta) 
            {
                PopMovesToSize(moves, start); 
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
        if (depth >= 1 + R && !g->InCheck && !EitherSideKP(g)) 
        {
            ctx->Ply++; 
            ctx->NullMove = false; 
            bool pv = ctx->InPV; 
            ctx->InPV = false; 

            PushNullMove(g); 
            // check if full search would have beta cutoff 
            int score = -Negamax_(ctx, -beta, -beta + 1, depth - 1 - R); 
            PopNullMove(g); 

            if (score >= beta) 
            {
                ctx->NullMove = true; 
                ctx->Ply--; 

                ClearPV(ctx, 0); 
                PopMovesToSize(moves, start); 
                return beta; 
            }

            ctx->InPV = pv; 
            ctx->NullMove = true; 
            ctx->Ply--; 
        }
    }

    NEGAMAX_LOOP_MOVES(); 

    int ttType = PVNode; 
    if (alpha <= alphaOrig) 
    {
        ttType = FailLow; 
    }
    else if (alpha >= beta) 
    {
        ttType = FailHigh; 
    }

    UpdateTTable(&ctx->Transpositions, g->Hash, ttType, alpha, depth, bestMove, g); 

    PopMovesToSize(moves, start); 

    return alpha; 
}

/**
 * Negamax for root nodes. 
 * 
 * @param ctx Search context
 * @param alpha Lower bound
 * @param beta Upper bound 
 * @param depth Remaining depth 
 * @return Evaluation
 */
static inline int Negamax(SearchContext* ctx, int alpha, int beta, int depth) 
{
    // reset data on root node 
    ctx->NumNodes = 0; 
    ctx->NumLeaves = 0; 
    ctx->NumQNodes = 0; 
    ctx->NumQLeaves = 0; 
    ctx->CheckTime = 0; 
    memset(ctx->Killer, 0, sizeof(ctx->Killer)); 
    memset(ctx->History, 0, sizeof(ctx->History)); 
    ctx->NullMove = true; 
    ctx->InPV = true; 

    // init node 
    ClearPV(ctx, 0); 
    ctx->NumNodes++; 

    // break out if search should end 
    HandleOutOfTime(ctx); 

    Game* g = ctx->State; 
    MoveList* moves = ctx->Moves; 
    U64 start = moves->Size; 

    int alphaOrig = alpha; 

    // collect all moves from current position 
    // these moves must be cleared before returning
    GenMoves(g, moves); 
    U64 numMoves = moves->Size - start; 

    // end of search or end of game 
    if (depth <= 0 || numMoves == 0) 
    {
        // no move found for node so this is a leaf node
        ClearPV(ctx, 0); 
        ctx->NumLeaves++; 

        PopMovesToSize(moves, start); 
        return QSearch(ctx, alpha, beta, 16); 
    }

    TTableEntry* entry = FindTTableEntry(&ctx->Transpositions, g->Hash, g); 
    Move hashMove = entry ? entry->Mv : NoMove; 

    clock_t curTime = clock(); 
    bool printCurMove = curTime >= ctx->CurMoveAt; 

    NEGAMAX_LOOP_MOVES(
        if (printCurMove)
        {
            printf("info currmove "); 
            PrintMoveEnd(mv, " currmovenumber "); 
            printf("%d\n", (int) (i-start+1)); 
            fflush(stdout); 
        }
    );

    int ttType = PVNode; 
    if (alpha <= alphaOrig) 
    {
        ttType = FailLow; 
    }
    else if (alpha >= beta) 
    {
        ttType = FailHigh; 
    }
    UpdateTTable(&ctx->Transpositions, g->Hash, ttType, alpha, depth, bestMove, g); 

    PopMovesToSize(moves, start); 
    return alpha; 
}

/**
 * Updates the best principal variation and prints info to stdout. 
 * 
 * @param ctx Search context 
 * @param searchStart Start time of search
 * @param start Start of current depth's search
 * @param end When did current depth's search end 
 * @param depth Current search depth target 
 * @param eval Negamax evaluation
 * @param line Best PV for current search
 */
static inline void UpdateSearch(SearchContext* ctx, clock_t searchStart, clock_t start, clock_t end, int depth, int eval, const PVLine* line) 
{
    (void) start; 

    U64 nodes = ctx->State->Nodes;

    float startDuration = (end - searchStart); 
    if (startDuration <= 0) startDuration = 1; 
    float nps = nodes / (startDuration / CLOCKS_PER_SEC); 
    startDuration /= CLOCKS_PER_SEC * 0.001; 

    ctx->BestLine = *line; 
    ctx->Nodes = nodes; 
    ctx->Nps = (U64) nps; 
    ctx->Depth = depth; 
    ctx->Eval = eval; 

    if (IsMateScore(eval)) 
    {
        int matePly = 100000 - abs(eval); 
        int plies = matePly;
        printf("info depth %d seldepth %" PRIu64 " multipv 1 score mate %d time %.0f nodes %" PRIu64 " nps %.0f hashfull %.0f pv ", 
            depth, 
            ctx->BestLine.NumMoves, 
            (eval > 0 ? 1 : -1) * (plies/2), 
            startDuration, 
            nodes, 
            nps, 
            1000.0f * ctx->Transpositions.Used / ctx->Transpositions.Size); 
    }
    else 
    {
        printf("info depth %d seldepth %" PRIu64 " multipv 1 score cp %d time %.0f nodes %" PRIu64 " nps %.0f hashfull %.0f pv ", 
            depth, 
            ctx->BestLine.NumMoves, 
            eval, 
            startDuration, 
            nodes, 
            nps, 
            1000.0f * ctx->Transpositions.Used / ctx->Transpositions.Size); 
    }
    // print PV for mate and normal eval 
    for (U64 i = 0; i < ctx->BestLine.NumMoves; i++) 
    {
        PrintMoveEnd(ctx->BestLine.Moves[i], " "); 
    }
    printf("\n"); 
    fflush(stdout); 
}

/**
 * Performs the search using iterative deepening and aspiration windows. 
 * 
 * @param ctx Search context 
 */
void RunSearch(SearchContext* ctx) 
{
    clock_t start, end, searchStart = clock(); 

    int tgtDepth = ctx->TargetDepth; 
    if (tgtDepth < 0) tgtDepth = INT_MAX; 
    if (tgtDepth > MaxDepth) tgtDepth = MaxDepth; 

    // there is no last eval for depth 1
    start = clock(); 
    int eval = Negamax(ctx, -MaxScore, MaxScore, 1); 
    end = clock(); 
    UpdateSearch(ctx, searchStart, start, end, 1, eval, &ctx->Lines[0]); 

    // iterative deepening
    for (int depth = 2; depth <= tgtDepth; depth++) 
    {
        ClearMoves(ctx->Moves); 
        int last = eval; 

        // aspiration windows 
        int a[] = { last - 30, last - 130, last - 530, -MaxScore }; 
        int b[] = { last + 30, last + 130, last + 530,  MaxScore }; 
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

/**
 * Entry point for search thread. 
 * 
 * @param data Pointer to the search context 
 * @return Null
 */
static void* StartPThreadSearch(void* data) 
{
    SearchContext* ctx = data; 
    RunSearch(ctx); 

    return NULL; 
}

void CreateSearchContext(SearchContext* ctx) 
{
    memset(ctx, 0, sizeof(SearchContext)); 

    // board should always be initialized
    ctx->State = NewGame(); 
    ctx->Moves = NewMoveList(); 
    ctx->Contempt = 0; 
    CreateTTable(&ctx->Transpositions, 1); 

    pthread_mutex_init(&ctx->Lock, NULL); 
} 

void DestroySearchContext(SearchContext* ctx) 
{
    StopSearchContext(ctx); 
    FreeGame(ctx->State); 
    FreeMoveList(ctx->Moves); 
    DestroyTTable(&ctx->Transpositions); 
    pthread_mutex_destroy(&ctx->Lock); 
}

void StopSearchContext(SearchContext* ctx) 
{
    if (ctx->Running) 
    {
        ctx->ShouldExit = true; 
        pthread_join(ctx->Thread, NULL); 

        ClearMoves(ctx->Moves); 
        // ResetTTable(&ctx->TT); 
        ctx->Running = false; 
    }

    ctx->ShouldExit = false; 
    fflush(stdout); 
}

void WaitForSearchContext(SearchContext* ctx) 
{
    if (ctx->Running) 
    {
        pthread_join(ctx->Thread, NULL); 
    }
}

void Search(SearchContext* ctx, SearchParams* params) 
{
    StopSearchContext(ctx); 

    // prepare for search 

    ctx->BestLine.NumMoves = 0; 
    ctx->Nodes = 0; 
    ctx->Nps = 0; 
    ctx->Depth = 0; 
    ctx->Eval = 0; 
    ctx->Running = true; 
    ctx->StartAt = clock(); 
    ctx->CurMoveAt = NSecondsFromNow(2); 
    ctx->NextMessageAt = NSecondsFromNow(1); 

    ClearMoves(ctx->Moves); 
    ctx->Ply = 0; 
    for (int i = 0; i < MaxDepth; i++) 
    {
        ctx->Lines[i].NumMoves = 0; 
    }

    ctx->TargetDepth = params->Depth; 
    ctx->TargetTimeMs = params->TimeMs; 
    ctx->ShouldExit = false; 
    if (ctx->TargetTimeMs >= 0) 
    {
        ctx->EndAt = clock() + ctx->TargetTimeMs * CLOCKS_PER_SEC / 1000; 
    }

    // assign the current board state to the search context 
    CopyGame(ctx->State, params->Board); 
    ClearDepth(ctx->State); 

    ctx->StartColor = ctx->State->Turn; 
    ctx->ColorContempt = ColorSign(ctx->StartColor) * ctx->Contempt; 

    // run search on another thread 
    pthread_create(&ctx->Thread, NULL, StartPThreadSearch, ctx); 
}

int BasicQSearch(SearchContext* ctx) 
{
    ctx->BestLine.NumMoves = 0; 
    ctx->Ply = 0; 
    NoHandleTime = true; 
    return QSearch(ctx, -MaxScore, MaxScore, 16); 
} 
