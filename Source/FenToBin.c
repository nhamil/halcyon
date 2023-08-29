/**
 * @file FenToBin.c
 * @author Nicholas Hamilton 
 * @date 2023-08-23
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Prepare datagen FENs + evaluation for training. 
 */

#include <stdio.h> 
#include <string.h> 

#include "Game.h" 
#include "Search.h" 

#define MaxGameLength 1024

typedef struct Sample Sample; 
typedef struct ThreadData ThreadData; 

struct Sample 
{
    Game* State;
    S16 Evaluation; 
    bool IsQuiet; 
};

struct ThreadData 
{
    SearchContext Context[1]; 
    int Count; 
    int Offset; 
};

static Sample Samples[MaxGameLength]; 
static ThreadData* ThreadDatas; 
static int NumThreads = 1; 
static long long UsedPositions = 0; 
static long long TotalPositions = 0;  

int GetNumPiecesOnBoard(const Game* g) 
{
    int total = 0; 
    for (Piece p = 0; p < NumPieces; p++) 
    {
        total += PopCount(g->Pieces[p]); 
    }
    return total; 
}

S16 ClampEval(int eval) 
{
    return eval > INT16_MAX ? INT16_MAX : eval < -INT16_MAX ? -INT16_MAX : eval; 
}

static inline void WriteU64(U64 value, FILE* file) 
{
    for (int i = 0; i < 8; i++) 
    {
        putc(value & 255, file); 
        value >>= 8; 
    }
}

void* CheckIfQuiet(void* threadData) 
{
    ThreadData* data = threadData; 

    for (int i = data->Offset; i < data->Count; i += NumThreads) 
    {
        MoveInfo info; 
        CopyGame(data->Context->State, Samples[i].State); 
        ClearDepth(data->Context->State); 
        GenMoveInfo(data->Context->State, &info); 

        Samples[i].IsQuiet = Evaluate(data->Context->State, 0, info.NumMoves, false, 0) == ColorSign(data->Context->State->Turn) * BasicQSearch(data->Context); 
    }

    return NULL; 
}

void WriteSamples(int result, int count, FILE* file) 
{
    // only add quiet positions: need to check if static eval == qsearch 
    pthread_t threads[NumThreads]; 
    for (int i = 0; i < NumThreads; i++) 
    {
        ThreadDatas[i].Count = count; 
        ThreadDatas[i].Offset = i; 
        pthread_create(&threads[i], NULL, CheckIfQuiet, &ThreadDatas[i]); 
    }
    for (int i = 0; i < NumThreads; i++) 
    {
        pthread_join(threads[i], NULL); 
    }

    for (int i = 0; i < count; i++) 
    {
        TotalPositions++; 

        if (!Samples[i].IsQuiet) continue; 

        Color turn = Samples[i].State->Turn; 
        int relResult = result * ColorSign(turn); 

        UsedPositions++; 
        if (UsedPositions % 100000 == 0) printf("Using %lld positions out of %lld (%.1f%%)\n", UsedPositions, TotalPositions, 100.0 * UsedPositions / TotalPositions); 

        // current player 
        // 8 bytes * 6 types = 48
        for (PieceType type = 0; type < NumPieceTypes; type++) 
        {
            Bitboard b = Samples[i].State->Pieces[MakePiece(type, turn)]; 
            if (turn == ColorB) b = FlipRows(b); 
            WriteU64(b, file); 
        }
        // opponent 
        // + 8 bytes * 6 types = 48 + 48 = 96
        for (PieceType type = 0; type < NumPieceTypes; type++) 
        {
            Bitboard b = Samples[i].State->Pieces[MakePiece(type, OppositeColor(turn))]; 
            if (OppositeColor(turn) == ColorB) b = FlipRows(b); 
            WriteU64(b, file); 
        }
        // + 2 bytes = 96 + 2 = 98
        putc(Samples[i].Evaluation & 255, file); 
        putc((Samples[i].Evaluation >> 8) & 255, file); 
        // + 1 byte = 98 + 1 = 99
        putc(relResult, file); 
        // + 1 byte = 99 + 1 = 100
        putc(Samples[i].State->Turn, file); 
    }
}

int main(int argc, char** argv) 
{
    if (argc != 4) 
    {
        printf("Usage: fen2bin <threads> <fen> <bin>\n"); 
        return 1; 
    }

    NumThreads = atoi(argv[1]); 
    if (NumThreads < 1) 
    {
        printf("Threads must be greater than or equal to 1\n"); 
        return 1; 
    }

    ThreadDatas = calloc(NumThreads, sizeof(ThreadData)); 
    for (int i = 0; i < NumThreads; i++) 
    {
        CreateSearchContext(ThreadDatas[i].Context); 
    }

    FILE* fin = fopen(argv[2], "r"); 
    if (!fin) 
    {
        printf("Could not open FEN file %s\n", argv[1]); 
        return 1; 
    }

    FILE* fout = fopen(argv[3], "wb"); 
    if (!fout) 
    {
        printf("Could not write to bin file %s\n", argv[2]); 
        fclose(fin); 
        return 1; 
    }

    char line[4096]; 
    int ply = 0; 
    int numPieces = -1; 

    for (int i = 0; i < MaxGameLength; i++) 
    {
        Samples[i].State = NewGame(); 
    }

    int iter = 0; 
    while (fgets(line, sizeof(line), fin)) 
    {
        iter++; 

        const char* fen = strtok(line, ";"); 
        const char* move = strtok(NULL, ";") + 5; 
        const char* bestmove = strtok(NULL, ";") + 9; 
        const char* eval = strtok(NULL, ";\n"); 

        if (strcmp(move, bestmove) != 0) 
        {
            continue; 
        }

        LoadFen(Samples[ply].State, fen); 
        // relative to current side to move 
        Samples[ply].Evaluation = ClampEval(atof(eval) * 100); 

        int newNumPieces = GetNumPiecesOnBoard(Samples[ply].State); 

        if (newNumPieces > numPieces) 
        {
            // must be a new game

            // don't write this sample
            // if eval wasn't mate then it must be a draw
            WriteSamples(0, ply, fout); 

            // reset with this sample as first 
            CopyGame(Samples[0].State, Samples[ply].State); 
            Samples[0].Evaluation = Samples[ply].Evaluation; 
            ply = 1; 
        }
        else if (abs(Samples[ply].Evaluation) > 20000) 
        {
            // positive = white wins 
            // when writing, this will be converted to be relative to current side for each position 
            int result = ColorSign(Samples[ply].State->Turn) * ((Samples[ply].Evaluation > 0) * 2 - 1); 

            // ignore last sample: 
            // it found mate so it's not normal static eval 
            WriteSamples(result, ply+1, fout); 
            ply = 0; 
        }
        else if (ply + 1 >= MaxGameLength) 
        {
            // game went too long -> draw 
            WriteSamples(0, ply + 1, fout); 
            ply = 0; 
        }
        else 
        {
            // not done 
            ply++; 
        }

        numPieces = newNumPieces; 
    }

    printf("Using %lld positions out of %lld (%.1f%%)\n", UsedPositions, TotalPositions, 100.0 * UsedPositions / TotalPositions); 
    printf("Done\n"); 

    fclose(fin); 
    fclose(fout); 
    return 0; 
}
