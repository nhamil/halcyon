/**
 * @file Tune.c
 * @author Nicholas Hamilton 
 * @date 2023-07-17
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Tunes eval weights. 
 */

#include <math.h> 
#include <stdio.h> 

#include <pthread.h> 

#include "Game.h" 
#include "Search.h"
#include "Vector.h" 

typedef struct FenState FenState; 

struct FenState 
{
    Game Board; 
    int NumMoves; 
    double Result; 
    bool Draw; 
};

#define MAX_THREADS 256

static double ThreadErrors[MAX_THREADS]; 

static int NWeights; 
static Vector States[1]; 
static int* Weights; 
static char* Names; 
static int Iteration; 
static int NumThreads; 

typedef char FenStr[FEN_LEN]; 

void LoadFens(const char* filename) 
{
    FenStr buf; 

    CREATE_VEC(States, FenState); 

    MvList* moves = NewMvList(); 

    printf("Loading FENs from %s\n", filename); 
    FILE* fensFile = fopen(filename, "r"); 
    while (fgets(buf, sizeof(buf), fensFile)) 
    {
        if (States->Size % 1000000 == 0) 
        {
            printf("Loaded %d positions\n", (int) States->Size); 
            fflush(stdout); 
        }

        buf[strlen(buf) - 1] = '\0'; 
        char resChar = buf[0]; 
        double result = 0.5; 
        if (resChar == 'w') result = 1.0; 
        else if (resChar == 'b') result = 0.0; 
        else if (resChar == 'd') result = 0.5; 
        else 
        {
            printf("Warning - Unknown result: %c\n", resChar); 
        }

        FenState* state = (FenState*) PushVecEmpty(States); 
        LoadFen(&state->Board, buf + 3); 

        state->Result = result;  
        state->Draw = IsSpecialDraw(&state->Board); 

        MvInfo info; 
        GenMvInfo(&state->Board, &info); 
        state->NumMoves = info.NumMoves; 
    }
    printf("Done - Read %d FENs (%lldmb)\n", 
        (int) States->Size, 
        (long long) (States->Size * States->ElemSize / 1024 / 1024)
    ); 

    FreeMvList(moves); 
    fclose(fensFile); 
}

double Sigmoid(double x) 
{
    return 1.0 / (1.0 + pow(10, -1.0 * x / 400)); 
}

void* EThread(void* data) 
{
    int offset = *(int*) data; 
    ThreadErrors[offset] = 0; 

    for (U64 i = offset; i < States->Size; i += NumThreads) 
    {
        FenState* fen = AtVec(States, i);  
        double result = fen->Result; 

        double add = result - Sigmoid(Evaluate(&fen->Board, fen->NumMoves, fen->Draw, 0)); 
        ThreadErrors[offset] += add * add; 
    }

    return NULL; 
}

double E(void) 
{
    double total = 0; 

    for (int i = 0; i < NWeights; i++) 
    {
        *GetEvalParam(i, NULL) = Weights[i]; 
    }

    pthread_t threads[NumThreads]; 
    long offsets[NumThreads]; 
    for (int i = 0; i < NumThreads; i++) 
    {
        offsets[i] = i; 
        pthread_create(&threads[i], NULL, EThread, &offsets[i]); 
    }

    // for (U64 i = 0; i < States->Size; i++) 
    // {
    //     FenState* fen = AtVec(States, i);  
    //     double result = fen->Result; 

    //     double add = result - Sigmoid(Evaluate(&fen->Board, fen->NumMoves, fen->Draw, 0)); 
    //     total += add * add; 
    // }

    for (int i = 0; i < NumThreads; i++) 
    {
        pthread_join(threads[i], NULL); 
        total += ThreadErrors[i]; 
    }

    return total / States->Size; 
}

void LoadFromPrevOutput(const char* filename) 
{
    FILE* f = fopen(filename, "r"); 

    char name[PARAM_NAME_LEN]; 
    char line[4096]; 
    char* tok; 

    printf("Loading previous weights\n"); 

    while (fgets(line, sizeof(line), f)) 
    {
        int param = -1; 
        tok = strtok(line, " "); 
        while (tok) 
        {
            if (strcmp(tok, "param") == 0) 
            {
                tok = strtok(NULL, " "); 
                param = atoi(tok); 
            }
            else if (strcmp(tok, "to") == 0 && param >= 0) 
            {
                tok = strtok(NULL, " "); 
                *GetEvalParam(param, name) = atoi(tok); 
                // printf("Set %s = %d\n", name, *GetEvalParam(param, NULL)); 
                param = -1; 
            }
            else if (strcmp(tok, "Iteration") == 0) 
            {
                tok = strtok(NULL, " "); 
                Iteration = atoi(tok); 
            }
            tok = strtok(NULL, " "); 
        }
    }

    printf("Done!\n"); 

    fclose(f); 
}

int main(int argc, char** argv) 
{
    if (argc < 3 || argc > 4) 
    {
        printf("Usage: tune <num threads> <fen results file> [resume from file]\n"); 
        return 1; 
    }

    NWeights = GetNumEvalParams(); 
    Weights = calloc(NWeights, sizeof(int)); 
    Names = calloc(NWeights, PARAM_NAME_LEN); 

    NumThreads = atoi(argv[1]); 
    printf("Using %d threads\n", NumThreads); 

    if (argc == 4) 
    {
        LoadFromPrevOutput(argv[3]); 
    }

    for (int i = 0; i < NWeights; i++) 
    {   
//        *GetEvalParam(i, NULL) = 0;  
        Weights[i] = *GetEvalParam(i, Names + PARAM_NAME_LEN * i); 
    }

    LoadFens(argv[2]); 

    double add = 1; 
    double bestE = E(); 
    printf("E(init) = %.10lf\n", bestE); 
    bool improved = true; 
    while (improved) 
    {
        Iteration++; 
        printf("Iteration %d\n", Iteration); 
        improved = false; 
        for (int pi = 0; pi < NWeights; pi++) 
        {
            int start = Weights[pi]; 
            printf("- Checking param %d %s (%d) ", pi, Names + PARAM_NAME_LEN * pi, start); 
            fflush(stdout); 

            Weights[pi] += add; 
            double newE = E(); 
            if (newE < bestE) 
            {
                improved = true; 
            }
            else 
            {
                Weights[pi] -= 2 * add; 
                newE = E(); 
                if (newE < bestE) 
                {
                    improved = true; 
                }
                else 
                {
                    Weights[pi] += add; 
                }
            }

            if (Weights[pi] != start) 
            {
                printf("-- Changed %+d to %d (E = %.10lf to %.10lf)\n", Weights[pi] - start, Weights[pi], bestE, newE); 
                bestE = newE; 
            }
            else 
            {
                printf("-- No improvement (E = %.10lf)\n", bestE); 
            }
        }
    }

    DestroyVec(States); 
    free(Weights); 
}
