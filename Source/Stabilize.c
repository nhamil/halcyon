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

        if (Evaluate(ctx->Board, 0, info.NumMoves, false, 0) == ColSign(ctx->Board->Turn) * BasicQSearch(ctx)) 
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
