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

#include "Bitboard.h" 
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

#define MaxUciInput 4096

#define MinUciTT 1 
#define MaxUciTT 16384
#define DefaultUciTT 16

#define MinUciContempt -1000
#define MaxUciContempt  1000
#define DefaultUciContempt  0

static Game* UciGame; 
static SearchContext UciEngine; 

/**
 * @return Next whitespace-delimited token from user input or null
 */
const char* UciNextToken(void) 
{
    return strtok(NULL, " \n"); 
}

/**
 * Checks if two strings are equal. 
 * 
 * @param a First string
 * @param b Second string 
 * @return True if equal, otherwise false 
 */
bool UciEquals(const char* a, const char* b) 
{
    if (!a || !b) return false; 

    return strcmp(a, b) == 0; 
}

/**
 * Prints UCI ID and option information. 
 * 
 * @return True
 */
bool UciCommandUci(void) 
{
    printf("id name %s\n", ENGINE_NAME); 
    printf("id author Nicholas Hamilton\n"); 
    printf("option name Hash type spin default %d min %d max %d\n", DefaultUciTT, MinUciTT, MaxUciTT); 
    printf("option name Contempt type spin default %d min %d max %d\n", DefaultUciContempt, MinUciContempt, MaxUciContempt); 
    printf("uciok\n"); 
    return true; 
}

/**
 * Notifies the user that commands can be sent. 
 * 
 * @return True
 */
bool UciCommandIsReady(void) 
{
    printf("readyok\n"); 
    return true; 
}

/**
 * Loads starting position and resets transposition table. 
 * 
 * @return True
 */
bool UciCommandUciNewGame(void) 
{
    StopSearchContext(&UciEngine); 
    LoadFen(UciGame, StartFen); 
    ResetTTable(&UciEngine.Transpositions); 
    return true; 
}

/**
 * Loads a position, optionally applying moves. 
 * 
 * @return True on success, otherwise false
 */
bool UciCommandPosition(void) 
{
    MoveList* moves = NewMoveList(); 

    Game* g = UciGame; 

    const char* token = UciNextToken(); 

    bool success = true; 

    if (UciEquals(token, "startpos")) 
    {
        LoadFen(g, StartFen); 
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
        char fen[MaxUciInput]; 
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
            ClearMoves(moves); 
            GenMoves(g, moves); 

            PieceType promoteType = PieceP; 

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
                    if (token[4] == 'q') promoteType = PieceQ; 
                    if (token[4] == 'r') promoteType = PieceR; 
                    if (token[4] == 'b') promoteType = PieceB; 
                    if (token[4] == 'n') promoteType = PieceN; 
                }
            }
            else 
            {
                success = false; goto done; 
            }

            Square from = MakeSquare(token[0] - 'a', token[1] - '1'); 
            Square to = MakeSquare(token[2] - 'a', token[3] - '1'); 

            // printf("%s (%s) to %s\n", SquareString(from), PieceString(pcAtOrWP(g, from)), SquareString(to)); 

            // if promote is still pawn, then it wasn't promoted -> get the real original piece
            if (promoteType == PieceP) 
            {
                promoteType = TypeOfPiece(PieceAt(&g->Board, from)); 
            }

            Piece promote = MakePiece(promoteType, g->Turn); 

            // printf("Piece: %s\n", PieceString(promote)); 

            // make sure the complete move is legal 
            bool found = false; 
            for (U64 i = 0; i < moves->Size; i++) 
            {
                Move m = moves->Moves[i]; 
                if (FromSquare(m) == from && ToSquare(m) == to && PromotionPiece(m) == promote) 
                {
                    found = true; 
                    PushMove(g, m); 
                    ClearDepth(g); 
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
        LoadFen(g, StartFen); 
    }

    FreeMoveList(moves); 
    return success; 
}

/**
 * Prints the board. 
 * 
 * @return True
 */
bool UciCommandPrint(void) 
{
    PrintGame(UciGame); 
    return true; 
}

/**
 * Searches the current game state. 
 * 
 * @return True 
 */
bool UciCommandGo(void) 
{
    const char* token; 
    int depth = InfDepth, timeMs = InfTime; 
    int moveTime = InfTime; 
    int wTime = InfTime, bTime = InfTime, sideTime = InfTime; 
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
            if (UciGame->Turn == ColorW) 
            {
                sideTime = wTime; 
                if (sideTime < 0) sideTime = 0; 
            }
        }
        if (UciEquals(token, "btime") && (token = UciNextToken())) 
        {
            bTime = atoi(token); 
            if (UciGame->Turn == ColorB) 
            {
                sideTime = bTime; 
                if (sideTime < 0) sideTime = 0; 
            }
        }
        if (UciEquals(token, "winc") && (token = UciNextToken())) 
        {
            wIncr = atoi(token); 
            if (UciGame->Turn == ColorW) sideIncr = wIncr; 
        }
        if (UciEquals(token, "binc") && (token = UciNextToken())) 
        {
            bIncr = atoi(token); 
            if (UciGame->Turn == ColorB) sideIncr = bIncr; 
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
        CopyGame(pGame, UciGame); 
        MoveList* moves = NewMoveList(); 

        clock_t start = clock(); 

        GenMoves(UciGame, moves); 
        for (U64 i = 0; i < moves->Size; i++) 
        {
            Move mv = moves->Moves[i]; 
            PrintMoveEnd(mv, " - "); 
            PushMove(UciGame, mv); 
            total += Perft(UciGame, perftNum - 1); 
            PopMove(UciGame, mv); 
            printf("\n"); 
        }

        clock_t end = clock(); 
        printf("Total: %lu", total); 

        FreeMoveList(moves); 
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

    if (sideTime > InfTime) 
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
    InitSearchParams(&params, UciGame, depth, timeMs); 

    Search(&UciEngine, &params); 
    return true; 
}

/**
 * Stops the search if it is running. 
 * 
 * @return True 
 */
bool UciCommandStop(void) 
{
    StopSearchContext(&UciEngine); 
    return true; 
}

/**
 * Sets engine options. 
 * 
 * @return True if option is valid, otherwise false
 */
bool UciCommandSetOption(void) 
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
        if (value > MaxUciTT) value = MaxUciTT; 
        if (value < MinUciTT) value = MinUciTT; 

        DestroyTTable(&UciEngine.Transpositions); 
        CreateTTable(&UciEngine.Transpositions, value); 

        return true; 
    }
    else if (UciEquals(token, "Contempt")) 
    {
        token = UciNextToken(); 
        if (!UciEquals(token, "value")) return false; 
        token = UciNextToken(); 
        if (!token) return false; 

        int value = atoi(token); 
        if (value > MaxUciContempt) value = MaxUciContempt; 
        if (value < MinUciContempt) value = MinUciContempt; 

        UciEngine.Contempt = value; 

        return true; 
    }

    printf("Unknown option: %s\n", token); 
    return true; 
}

/**
 * Prints current board state static evaluation. 
 * 
 * @param qsearch Whether to use quiescence search
 * @return True if options are supported, otherwise false 
 */
bool UciCommandEval(bool qsearch) 
{
    if (qsearch) 
    {
        printf("Quiescence search evaluation is not supported yet\n"); 
        return false; 
    }

    printf("Static Evaluation\n"); 
    PrintGame(UciGame); 

    MoveInfo info; 
    GenMoveInfo(UciGame, &info); 

    bool draw = IsSpecialDraw(UciGame); 

    EvaluateVerbose(UciGame, 0, info.NumMoves, draw, UciEngine.Contempt, true); 

    return true; 
}

/**
 * Print all tunable weights. 
 * 
 * @return True 
 */
bool UciCommandGetTune(void) 
{
    int N = GetNumEvalParams(); 

    printf("%d weights: ", N); 

    for (int i = 0; i < N; i++) 
    {
        printf("%d ", *GetEvalParam(i, NULL)); 
    }
    printf("\n"); 
    fflush(stdout); 

    return true; 
}

/**
 * Sets tunable weights to values given by the user. 
 * 
 * @return True
 */
bool UciCommandSetTune(void) 
{
    int N = GetNumEvalParams(); 
    int total = 0; 

    for (int i = 0; i < N; i++) 
    {
        const char* in = UciNextToken(); 
        if (!in) break; 

        *GetEvalParam(i, NULL) = atoi(in); 
        total++; 
    }

    printf("info string Updated %d eval weights\n", total); 
    fflush(stdout); 

    return true; 
}

/**
 * Used to selfplay data generation. 
 * 
 * @return True 
 */
bool UciCommandDataGen(void) 
{
    // in case we were searching before this command 
    StopSearchContext(&UciEngine); 

    bool badData = true; 

    const char* token; 
    
    token = UciNextToken(); 
    if (!token) goto badParse; 
    const char* file = token; 

    token = UciNextToken(); 
    if (!token) goto badParse; 
    U64 numPositions = (U64) atoll(token); 
    if (numPositions < 1) 
    {
        printf("Positions must be greater than 0\n"); 
        return true; 
    }
    
    token = UciNextToken(); 
    if (!token) goto badParse; 
    int depth = (int) atoi(token); 
    if (depth < 1) 
    {
        printf("Depth must be greater than 0\n"); 
        return true; 
    }

    token = UciNextToken(); 
    if (!token) goto badParse; 
    int timeMs = (int) atoi(token); 
    if (timeMs < 1) 
    {
        printf("Time must be greater than 0\n"); 
        return true; 
    }

    token = UciNextToken(); 
    if (!token) goto badParse; 
    int bestMovePly = (int) atoi(token); 
    if (bestMovePly < 0) 
    {
        printf("Best move ply must be greater than or equal to 0\n"); 
        return true; 
    }

    token = UciNextToken(); 
    if (!token) goto badParse; 
    int maxPly = (int) atoi(token); 
    if (maxPly < 1) 
    {
        printf("Max ply must be greater than 0\n"); 
        return true; 
    }

    printf("positions %llu depth %d ms %d bestmove %d maxply %d\n", 
        (unsigned long long) numPositions, 
        depth, 
        timeMs, 
        bestMovePly, 
        maxPly
    );

    badData = false; 
badParse: 
    if (badData) 
    {
        printf("Usage: datagen <output file> <num positions> <target depth> <max ms> <bestmove ply> <max ply>\n"); 
        return true; 
    }

    FILE* out = fopen(file, "a"); 
    if (!out) 
    {
        printf("Could not open file\n"); 
        return true; 
    }

    char fen[MaxFenLength]; 

    Game* g = NewGame(); 
    LoadFen(g, StartFen); 

    MoveList* moves = NewMoveList(); 
    Random r; 
    InitRandom(&r, (U64) clock()); 

    UciEngine.Eval = 0; 
    for (U64 i = 0; i < numPositions; i++) 
    {
        ClearMoves(moves); 
        GenMoves(g, moves); 
        if (g->Ply > maxPly || moves->Size == 0 || IsMateScore(UciEngine.Eval) || IsSpecialDraw(g)) 
        {
            printf("Resetting position maxply=%d nomovse=%d matescore=%d draw=%d\n", 
                g->Ply > maxPly, 
                moves->Size == 0, 
                IsMateScore(UciEngine.Eval), 
                IsSpecialDraw(g)
            ); 
            LoadFen(g, StartFen); 
            ClearMoves(moves); 
            GenMoves(g, moves); 
            UciEngine.Eval = 0; 
        }

        ToFen(g, fen); 

        SearchParams params; 
        InitSearchParams(&params, g, depth, timeMs); 

        Search(&UciEngine, &params); 
        WaitForSearchContext(&UciEngine); 

        Move mv = moves->Moves[NextU32(&r) % moves->Size]; 
        if (g->Ply >= bestMovePly) 
        {
            mv = UciEngine.BestLine.Moves[0]; 
        }
        PushMove(g, mv); 

        fprintf(out, "%s ; move ", fen); 
        FilePrintMoveEnd(mv, " ; bestmove ", out); 
        FilePrintMoveEnd(UciEngine.BestLine.Moves[0], " ; ", out); 
        fprintf(out, "%.2f/%d \n", UciEngine.Eval / 100.0, UciEngine.Depth); 
        fflush(out); 
    }

    fclose(out); 
    FreeGame(g); 
    FreeMoveList(moves); 

    return true; 
}

/**
 * Parses a user command. 
 * 
 * @param origCommand The user input 
 * @return True if command recognized, otherwise false 
 */
bool UciParse(const char* origCommand) 
{
    // remove spaces at the beginning
    while (*origCommand && *origCommand == ' ') origCommand++; 

    char cmd[MaxUciInput]; 
    strcpy(cmd, origCommand); 

    char* token = strtok(cmd, " \n"); 
    if (token) 
    {
        if (UciEquals(token, "quit")) exit(0); 
        if (UciEquals(token, "ucinewgame")) return UciCommandUciNewGame(); 
        if (UciEquals(token, "uci")) return UciCommandUci(); 
        if (UciEquals(token, "isready")) return UciCommandIsReady(); 
        if (UciEquals(token, "position")) return UciCommandPosition(); 
        if (UciEquals(token, "print")) return UciCommandPrint(); 
        if (UciEquals(token, "go")) return UciCommandGo(); 
        if (UciEquals(token, "stop")) return UciCommandStop(); 
        if (UciEquals(token, "setoption")) return UciCommandSetOption(); 
        if (UciEquals(token, "seval")) return UciCommandEval(false); 
        if (UciEquals(token, "qeval")) return UciCommandEval(true); 
        if (UciEquals(token, "gettune")) return UciCommandGetTune(); 
        if (UciEquals(token, "settune")) return UciCommandSetTune(); 
        if (UciEquals(token, "datagen")) return UciCommandDataGen(); 
    }

    return false; 
}

/**
 * Entry point of the program. 
 * 
 * @return 0
 */
int main(void) 
{
    printf("%s by Nicholas Hamilton\n", ENGINE_NAME); 
    fflush(stdout); 

    UciGame = NewGame(); 
    LoadFen(UciGame, StartFen); 
    CreateSearchContext(&UciEngine); 

    DestroyTTable(&UciEngine.Transpositions); 
    CreateTTable(&UciEngine.Transpositions, DefaultUciTT); 
    UciEngine.Contempt = DefaultUciContempt; 

    char input[MaxUciInput]; 
    while (true) 
    {
        fflush(stdout); 
        fgets(input, MaxUciInput, stdin); 
        if (input[strlen(input) - 1] == '\n') input[strlen(input) - 1] = '\0'; 
        if (!UciParse(input)) printf("Unknown command: '%s'\n", input); 
    }

    FreeGame(UciGame); 
    DestroySearchContext(&UciEngine); 
    return 0; 
}