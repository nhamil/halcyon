/**
 * @file Tune.c
 * @author Nicholas Hamilton 
 * @date 2023-07-17
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Stabilizes a list of FEN positions. 
 */

#include <math.h> 
#include <stdio.h> 

#include "Game.h" 
#include "Search.h"
#include "Vector.h" 

// static inline int QuickQSearch(Game* g, MvList* moves, int alpha, int beta, int depth) 
// {
//     U64 start = moves->Size; 

//     bool draw = IsSpecialDraw(g); 
//     if (draw) return 0; 

//     GenMoves(g, moves); 
//     U64 numMoves = moves->Size - start; 

//     int standPat = ColSign(g->Turn) * Evaluate(g, numMoves, draw, -ctx->ColContempt); 

//     // check for beta cutoff
//     if (standPat >= beta) 
//     {
//         PopMvList(moves, start); 
//         return beta; 
//     }
    
//     // check if doing nothing improves score 
//     if (standPat > alpha) 
//     {
//         alpha = standPat; 
//     }

//     // leaf node 
//     if (depth <= 0) 
//     {
//         ctx->NumQLeaves++; 
//         PopMvList(moves, start); 
//         return alpha; 
//     }

//     TTableEntry* entry = FindTTableEntry(&ctx->TT, g->Hash, g); 
//     Move hashMove = entry ? entry->Mv : NO_MOVE; 

//     // search active moves 
//     bool foundMove = false; 
//     if (!draw) 
//     {
//         ctx->Ply++; 
//         int moveValues[moves->Size - start]; 
//         GetMoveOrder(ctx, start, hashMove, QMoveVal, moveValues); 
//         for (U64 i = start; i < moves->Size; i++) 
//         {
//             Move mv = NextMove(ctx, i, moveValues + (i - start)); 

//             bool shouldSearch = IsTactical(mv); 
//             if (!shouldSearch) continue; 

//             // there is 1+ active moves, so not a leaf node 
//             foundMove = true; 

//             // search move 
//             PushMove(g, mv); 
//             int score = -QSearch(ctx, -beta, -alpha, depth - 1); 
//             PopMove(g, mv); 

//             // beta cutoff 
//             if (score >= beta) 
//             {
//                 ctx->Ply--; 
//                 PopMvList(moves, start); 
//                 return beta; 
//             }

//             // pv node 
//             if (score > alpha) 
//             {
//                 alpha = score; 
//             }
//         }
//         ctx->Ply--; 
//     }
//     if (!foundMove) ctx->NumQLeaves++; 

//     PopMvList(moves, start); 
//     return alpha; 
// }

void LoadFens(const char* filename, const char* outfile) 
{
    char buf[FEN_LEN]; 
    int numGames = 0; 

    SearchCtx ctx[1]; 
    CreateSearchCtx(ctx); 

    printf("Loading FENs from %s and saving non-tactical positions to %s\n", filename, outfile); 
    FILE* fensFile = fopen(filename, "r"); 
    FILE* outFile = fopen(outfile, "w"); 

    U64 tactical = 0, quiet = 0; 

    while (fgets(buf, sizeof(buf), fensFile)) 
    {
        if (++numGames % 1000 == 0) printf("Loaded %d positions (%"PRIu64" tactical, %"PRIu64" quiet)\n", numGames, tactical, quiet); 
        buf[strlen(buf) - 1] = '\0'; 
        
        LoadFen(ctx->Board, buf + 3); 
        MvInfo info; 
        GenMvInfo(ctx->Board, &info); 

        if (Evaluate(ctx->Board, info.NumMoves, false, 0) == ColSign(ctx->Board->Turn) * BasicQSearch(ctx)) 
        {
            fprintf(outFile, "%s\n", buf); 
            quiet++; 
        }
        else 
        {
            // printf("Position is tactical: \n"); 
            // PrintGame(ctx->Board); 
            tactical++; 
        }
    }
    printf("Done - Read %d FENs\n", numGames); 

    DestroySearchCtx(ctx); 

    fclose(outFile); 
    fclose(fensFile); 
}

int main(int argc, char** argv) 
{
    if (argc != 3) 
    {
        printf("Usage: stabilize <fen results file> <fen output file>\n"); 
        return 1; 
    }

    LoadFens(argv[1], argv[2]); 
}
