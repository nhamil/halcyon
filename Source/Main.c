/**
 * @file Main.c
 * @author Nicholas Hamilton 
 * @date 2023-01-11
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Handles UCI communication. 
 */

#include <stdatomic.h> 
#include <stdbool.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <pthread.h> 

#include "BBoard.h" 
#include "Castle.h"
#include "Game.h" 
#include "Move.h"
#include "MoveGen.h"
#include "Piece.h"
#include "Random.h" 
#include "Search.h" 
#include "Square.h"
#include "Vector.h" 

#ifndef ENGINE_NAME 
    #define ENGINE_NAME "Halcyon"
#endif 

#define UCI_MAX_INPUT 4096

#define UCI_MIN_TT 1 
#define UCI_MAX_TT 16384
#define UCI_DEF_TT 16

#define UCI_MIN_CONTEMPT -1000
#define UCI_MAX_CONTEMPT  1000
#define UCI_DEF_CONTEMPT  0

Game* s_UciGame; 
SearchCtx s_Engine; 

const char* UciNextToken(void) 
{
    return strtok(NULL, " \n"); 
}

bool UciEquals(const char* a, const char* b) 
{
    if (!a || !b) return false; 

    return strcmp(a, b) == 0; 
}

bool UciCmdUci(void) 
{
    printf("id name %s\n", ENGINE_NAME); 
    printf("id author Nicholas Hamilton\n"); 
    printf("option name Hash type spin default %d min %d max %d\n", UCI_DEF_TT, UCI_MIN_TT, UCI_MAX_TT); 
    printf("option name Contempt type spin default %d min %d max %d\n", UCI_DEF_CONTEMPT, UCI_MIN_CONTEMPT, UCI_MAX_CONTEMPT); 
    printf("uciok\n"); 
    return true; 
}

bool UciCmdIsReady(void) 
{
    printf("readyok\n"); 
    return true; 
}

bool UciCmdUciNewGame(void) 
{
    StopSearchCtx(&s_Engine); 
    LoadFen(s_UciGame, START_FEN); 
    return true; 
}

bool UciCmdPosition(void) 
{
    MvList* moves = NewMvList(); 

    Game* g = s_UciGame; 

    const char* token = UciNextToken(); 

    bool success = true; 

    if (UciEquals(token, "startpos")) 
    {
        LoadFen(g, START_FEN); 
        token = UciNextToken(); 
    }
    else if (UciEquals(token, "test1")) 
    {
        LoadFen(g, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"); 
        token = UciNextToken(); 
    }
    else if (UciEquals(token, "test2")) 
    {
        LoadFen(g, "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -"); 
        token = UciNextToken(); 
    }
    else if (UciEquals(token, "test3")) 
    {
        LoadFen(g, "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"); 
        token = UciNextToken(); 
    }
    else if (UciEquals(token, "test4")) 
    {
        LoadFen(g, "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1"); 
        token = UciNextToken(); 
    }
    else if (UciEquals(token, "test5")) 
    {
        LoadFen(g, "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"); 
        token = UciNextToken(); 
    }
    else if (UciEquals(token, "fen")) 
    {
        // need to recreate FEN from token list 
        char fen[UCI_MAX_INPUT]; 
        fen[0] = '\0'; 
        while ((token = UciNextToken()) && !UciEquals(token, "moves")) 
        {
            strcpy(fen + strlen(fen), token); 
            U64 len = strlen(fen); 
            fen[len] = ' '; 
            fen[len + 1] = '\0'; 
        }
        // token is NULL or "moves" at this point 

        LoadFen(g, fen); 
    }

    if (UciEquals(token, "moves")) 
    {
        while ((token = UciNextToken())) 
        {
            ClearMvList(moves); 
            GenMoves(g, moves); 

            Piece promote = PC_P; 

            if (strlen(token) == 4) 
            {
                // from -> to no promotion 
                if (!(token[0] >= 'a' && token[0] <= 'h' && token[2] >= 'a' && token[2] <= 'h' && token[1] >= '1' && token[1] <= '8' && token[3] >= '1' && token[3] <= '8')) 
                {
                    success = false; goto done; 
                }
            }
            else if (strlen(token) == 5) 
            {
                // from -> to with promotion 
                if (!(token[0] >= 'a' && token[0] <= 'h' && token[2] >= 'a' && token[2] <= 'h' && token[1] >= '1' && token[1] <= '8' && token[3] >= '1' && token[3] <= '8' && (token[4] == 'q' || token[4] == 'r' || token[4] == 'b' || token[4] == 'n'))) 
                {
                    success = false; goto done; 
                }
                else 
                {
                    if (token[4] == 'q') promote = PC_Q; 
                    if (token[4] == 'r') promote = PC_R; 
                    if (token[4] == 'b') promote = PC_B; 
                    if (token[4] == 'n') promote = PC_N; 
                }
            }
            else 
            {
                success = false; goto done; 
            }

            Square from = MakeSq(token[0] - 'a', token[1] - '1'); 
            Square to = MakeSq(token[2] - 'a', token[3] - '1'); 

            // printf("%s (%s) to %s\n", StrSq(from), StrPc(pcAtOrWP(g, from)), StrSq(to)); 

            // if promote is still pawn, then it wasn't promoted -> get the real original piece
            if (promote == PC_P) 
            {
                promote = GetNoCol(PcAt(g, from)); 
            }

            promote = MakePc(promote, g->Turn); 

            // printf("Piece: %s\n", StrPc(promote)); 

            // make sure the complete move is legal 
            bool found = false; 
            for (U64 i = 0; i < moves->Size; i++) 
            {
                Move m = moves->Moves[i]; 
                if (FromSq(m) == from && ToSq(m) == to && ProPc(m) == promote) 
                {
                    found = true; 
                    PushMove(g, m); 
                    NoDepth(g); 
                }
            }

            // keep previous legal moves, only ignore remaining 
            if (!found) 
            {
                printf("info string Could not find move '%s'\n", token); 
                goto done; 
            }
        }
    }

done: 
    if (!success) 
    {
        LoadFen(g, START_FEN); 
    }

    FreeMvList(moves); 
    return success; 
}

bool UciCmdPrint(void) 
{
    PrintGame(s_UciGame); 
    return true; 
}

bool UciCmdGo(void) 
{
    const char* token; 
    int depth = INF_DEPTH, timeMs = INF_TIME; 
    int moveTime = INF_TIME; 
    int wTime = INF_TIME, bTime = INF_TIME, sideTime = INF_TIME; 
    int wIncr = 0, bIncr = 0, sideIncr = 0; 
    int perftNum = -1; 

    while ((token = UciNextToken())) 
    {
        if (UciEquals(token, "depth") && (token = UciNextToken())) 
        {
            depth = atoi(token); 
        }
        if (UciEquals(token, "movetime") && (token = UciNextToken())) 
        {
            moveTime = atoi(token); 
        }
        if (UciEquals(token, "wtime") && (token = UciNextToken())) 
        {
            wTime = atoi(token); 
            if (s_UciGame->Turn == COL_W) 
            {
                sideTime = wTime; 
                if (sideTime < 0) sideTime = 0; 
            }
        }
        if (UciEquals(token, "btime") && (token = UciNextToken())) 
        {
            bTime = atoi(token); 
            if (s_UciGame->Turn == COL_B) 
            {
                sideTime = bTime; 
                if (sideTime < 0) sideTime = 0; 
            }
        }
        if (UciEquals(token, "winc") && (token = UciNextToken())) 
        {
            wIncr = atoi(token); 
            if (s_UciGame->Turn == COL_W) sideIncr = wIncr; 
        }
        if (UciEquals(token, "binc") && (token = UciNextToken())) 
        {
            bIncr = atoi(token); 
            if (s_UciGame->Turn == COL_B) sideIncr = bIncr; 
        }
        if (UciEquals(token, "perft") && (token = UciNextToken())) 
        {
            perftNum = atoi(token); 
        }
    }

    if (perftNum > 0) 
    {
        U64 total = 0; 

        Game* pGame = NewGame(); 
        CopyGame(pGame, s_UciGame); 
        MvList* moves = NewMvList(); 

        clock_t start = clock(); 

        GenMoves(s_UciGame, moves); 
        for (U64 i = 0; i < moves->Size; i++) 
        {
            Move mv = moves->Moves[i]; 
            PrintMoveEnd(mv, " - "); 
            PushMove(s_UciGame, mv); 
            total += Perft(s_UciGame, perftNum - 1); 
            PopMove(s_UciGame, mv); 
            printf("\n"); 
        }

        clock_t end = clock(); 
        printf("Total: %lu", total); 

        FreeMvList(moves); 
        FreeGame(pGame); 

        double dur = (double) (end - start) / CLOCKS_PER_SEC; 
        double nps = total / dur; 
        printf(", Time: %0.2fs, Speed: %0.3fMnps\n", dur, nps / 1000000.0); 
        return true; 
    }

    if (depth > 0) 
    {
        printf("info string Searching with max depth of %d\n", depth); 
    }
    else 
    {
        printf("info string Searching with no depth\n"); 
    }

    if (sideTime > INF_TIME) 
    {
        int totalTime = sideTime - 2000; 
        if (totalTime < 40) totalTime = 40; 

        int tgtTime = (int) (totalTime / 40 + sideIncr * 3 / 4); 
        if (tgtTime <= 0) tgtTime = 1; 
         
        timeMs = tgtTime; 
        printf("info string Using %dms out of %dms to think\n", timeMs, sideTime); 
        fflush(stdout); 
    }

    if (moveTime > 0) 
    {
        timeMs = moveTime; 
    }

    if (timeMs > 0) 
    {
        printf("info string Searching with max time of %dms\n", timeMs); 
    }
    else 
    {
        printf("info string Searching with no time limit\n"); 
    }
    fflush(stdout); 

    SearchParams params; 
    InitSearchParams(&params, s_UciGame, depth, timeMs); 

    Search(&s_Engine, &params); 
    return true; 
}

bool UciCmdStop(void) 
{
    StopSearchCtx(&s_Engine); 
    return true; 
}

bool UciCmdSetOption(void) 
{
    const char* token = UciNextToken(); 
    if (!UciEquals(token, "name")) return false; 

    token = UciNextToken(); 
    if (UciEquals(token, "Hash")) 
    {
        token = UciNextToken(); 
        if (!UciEquals(token, "value")) return false; 
        token = UciNextToken(); 
        if (!token) return false; 

        int value = atoi(token); 
        if (value > UCI_MAX_TT) value = UCI_MAX_TT; 
        if (value < UCI_MIN_TT) value = UCI_MIN_TT; 

        DestroyTTable(&s_Engine.TT); 
        CreateTTable(&s_Engine.TT, value); 

        return true; 
    }
    else if (UciEquals(token, "Contempt")) 
    {
        token = UciNextToken(); 
        if (!UciEquals(token, "value")) return false; 
        token = UciNextToken(); 
        if (!token) return false; 

        int value = atoi(token); 
        if (value > UCI_MAX_CONTEMPT) value = UCI_MAX_CONTEMPT; 
        if (value < UCI_MIN_CONTEMPT) value = UCI_MIN_CONTEMPT; 

        s_Engine.Contempt = value; 

        return true; 
    }

    printf("Unknown option: %s\n", token); 
    return true; 
}

bool UciCmdEval(bool qsearch) 
{
    if (qsearch) 
    {
        printf("Quiescence search evaluation is not supported yet\n"); 
        return false; 
    }

    printf("Static Evaluation\n"); 
    PrintGame(s_UciGame); 

    MvInfo info; 
    GenMvInfo(s_UciGame, &info); 

    bool draw = IsSpecialDraw(s_UciGame); 

    EvaluateVerbose(s_UciGame, info.NumMoves, draw, s_Engine.Contempt, true); 

    return true; 
}

bool UciCmdGetTune(void) 
{
    int N = GetNumEvalParams(); 

    printf("%d weights: ", N); 

    for (int i = 0; i < N; i++) 
    {
        printf("%d ", *GetEvalParam(i)); 
    }
    printf("\n"); 

    return true; 
}

bool UciParse(const char* origCmd) 
{
    // remove spaces at the beginning
    while (*origCmd && *origCmd == ' ') origCmd++; 

    char cmd[UCI_MAX_INPUT]; 
    strcpy(cmd, origCmd); 

    char* token = strtok(cmd, " \n"); 
    if (token) 
    {
        if (UciEquals(token, "quit")) exit(0); 
        if (UciEquals(token, "ucinewgame")) return UciCmdUciNewGame(); 
        if (UciEquals(token, "uci")) return UciCmdUci(); 
        if (UciEquals(token, "isready")) return UciCmdIsReady(); 
        if (UciEquals(token, "position")) return UciCmdPosition(); 
        if (UciEquals(token, "print")) return UciCmdPrint(); 
        if (UciEquals(token, "go")) return UciCmdGo(); 
        if (UciEquals(token, "stop")) return UciCmdStop(); 
        if (UciEquals(token, "setoption")) return UciCmdSetOption(); 
        if (UciEquals(token, "seval")) return UciCmdEval(false); 
        if (UciEquals(token, "qeval")) return UciCmdEval(true); 
        if (UciEquals(token, "gettune")) return UciCmdGetTune(); 
    }

    return false; 
}

int main(void) 
{
    printf("%s by Nicholas Hamilton\n", ENGINE_NAME); 
    fflush(stdout); 

    s_UciGame = NewGame(); 
    LoadFen(s_UciGame, START_FEN); 
    CreateSearchCtx(&s_Engine); 

    DestroyTTable(&s_Engine.TT); 
    CreateTTable(&s_Engine.TT, UCI_DEF_TT); 
    s_Engine.Contempt = UCI_DEF_CONTEMPT; 

    char input[UCI_MAX_INPUT]; 
    while (true) 
    {
        fflush(stdout); 
        fgets(input, UCI_MAX_INPUT, stdin); 
        input[strlen(input) - 1] = '\0'; 
        if (!UciParse(input)) printf("Unknown command: '%s'\n", input); 
    }

    FreeGame(s_UciGame); 
    DestroySearchCtx(&s_Engine); 
    return 0; 
}