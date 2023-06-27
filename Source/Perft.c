#include <math.h> 
#include <stdio.h> 
#include <time.h> 

#include "BBoard.h" 
#include "Castle.h"
#include "Game.h" 
#include "Move.h"
#include "Piece.h"
#include "Random.h" 
#include "Square.h"
#include "Vector.h" 

static int s_Total = 0; 

typedef struct PerftData PerftData; 

struct PerftData
{
    const char* Fen; 
    const U64 Expected[10]; 
};

#define MIN_SPEED_TIME 0.1

struct PerftSpeed 
{
    double Fast; 
    double Slow; 
    double Total; 
    int N; 
} speed = 
{
    .Fast = -INFINITY, 
    .Slow = INFINITY, 
    .Total = 0, 
    .N = 0
}; 

bool RunPerft(const char* name, const char* fen, const U64* expected) 
{
    s_Total++; 

    bool ret = true; 

    Game* g = NewGame(); 
    LoadFen(g, fen); 

    printf("%s\n%s\n", name, fen); 
    PrintGame(g); 

    int i = -1; 
    while (expected[++i] > 0) 
    {
        U64 e = expected[i]; 

        clock_t c = clock(); 
        U64 total = Perft(g, i);
        clock_t c2 = clock(); 

        double time = (double) (c2 - c) / CLOCKS_PER_SEC; 
        double nps = total / time; 
        printf(", Time: %.2lfms, Speed: %.2lfMnps - %s\n", time * 1000, nps / 1000000, e == total ? "PASS" : "FAIL"); 

        if (time >= MIN_SPEED_TIME) 
        {
            speed.N++; 
            speed.Total += nps; 
            if (nps > speed.Fast) speed.Fast = nps; 
            if (nps < speed.Slow) speed.Slow = nps; 
        }

        if (e != total) 
        {
            printf("FAILURE: Expected %lu, difference of %ld\n", e, (S64) e - (S64) total); 
            ret = 0; 
            // goto cleanup; 
        }

        fflush(stdout); 
    }

    printf("DONE\n\n"); 

cleanup: 
    FreeGame(g); 
    return ret; 
}

PerftData s_Tests[] = 
{
    // https://www.chessprogramming.org/Perft_Results
    { .Fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", { 1ULL, 20ULL, 400ULL, 8902ULL, 197281ULL, 4865609ULL, 119060324ULL, 3195901860ULL, 0 } }, 
    { .Fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", { 1ULL, 48ULL, 2039ULL, 97862ULL, 4085603ULL, 193690690ULL, 8031647685ULL, 0 } }, 
    { .Fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", { 1ULL, 14ULL, 191ULL, 2812ULL, 43238ULL, 674624ULL, 11030083ULL, 0 } }, 
    { .Fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", { 1ULL, 6ULL, 264ULL, 9467ULL, 422333ULL, 15833292ULL, 706045033ULL, 0 } }, 
    { .Fen = "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", { 1ULL, 6ULL, 264ULL, 9467ULL, 422333ULL, 15833292ULL, 706045033ULL, 0 } }, 
    { .Fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", { 1ULL, 44ULL, 1486ULL, 62379ULL, 2103487ULL, 89941194ULL, 0 } }, 
    { .Fen = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", { 1ULL, 46ULL, 2079ULL, 89890ULL, 3894594ULL, 164075551ULL, 6923051137ULL, 0 } }, 
    { .Fen = "4k3/8/8/8/8/8/8/4K2R w K - 0 1 ", .Expected = { 1ULL, 15ULL, 66ULL, 1197ULL, 7059ULL, 133987ULL, 764643ULL, 0 } }, 
    // http://www.rocechess.ch/perftsuite.zip
    { .Fen = "4k3/8/8/8/8/8/8/R3K3 w Q - 0 1 ", .Expected = { 1ULL, 16ULL, 71ULL, 1287ULL, 7626ULL, 145232ULL, 846648ULL, 0 } }, 
    { .Fen = "4k2r/8/8/8/8/8/8/4K3 w k - 0 1 ", .Expected = { 1ULL, 5ULL, 75ULL, 459ULL, 8290ULL, 47635ULL, 899442ULL, 0 } }, 
    { .Fen = "r3k3/8/8/8/8/8/8/4K3 w q - 0 1 ", .Expected = { 1ULL, 5ULL, 80ULL, 493ULL, 8897ULL, 52710ULL, 1001523ULL, 0 } }, 
    { .Fen = "4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1 ", .Expected = { 1ULL, 26ULL, 112ULL, 3189ULL, 17945ULL, 532933ULL, 2788982ULL, 0 } }, 
    { .Fen = "r3k2r/8/8/8/8/8/8/4K3 w kq - 0 1 ", .Expected = { 1ULL, 5ULL, 130ULL, 782ULL, 22180ULL, 118882ULL, 3517770ULL, 0 } }, 
    { .Fen = "8/8/8/8/8/8/6k1/4K2R w K - 0 1 ", .Expected = { 1ULL, 12ULL, 38ULL, 564ULL, 2219ULL, 37735ULL, 185867ULL, 0 } }, 
    { .Fen = "8/8/8/8/8/8/1k6/R3K3 w Q - 0 1 ", .Expected = { 1ULL, 15ULL, 65ULL, 1018ULL, 4573ULL, 80619ULL, 413018ULL, 0 } }, 
    { .Fen = "4k2r/6K1/8/8/8/8/8/8 w k - 0 1 ", .Expected = { 1ULL, 3ULL, 32ULL, 134ULL, 2073ULL, 10485ULL, 179869ULL, 0 } }, 
    { .Fen = "r3k3/1K6/8/8/8/8/8/8 w q - 0 1 ", .Expected = { 1ULL, 4ULL, 49ULL, 243ULL, 3991ULL, 20780ULL, 367724ULL, 0 } }, 
    { .Fen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1 ", .Expected = { 1ULL, 26ULL, 568ULL, 13744ULL, 314346ULL, 7594526ULL, 179862938ULL, 0 } }, 
    { .Fen = "r3k2r/8/8/8/8/8/8/1R2K2R w Kkq - 0 1 ", .Expected = { 1ULL, 25ULL, 567ULL, 14095ULL, 328965ULL, 8153719ULL, 195629489ULL, 0 } }, 
    { .Fen = "r3k2r/8/8/8/8/8/8/2R1K2R w Kkq - 0 1 ", .Expected = { 1ULL, 25ULL, 548ULL, 13502ULL, 312835ULL, 7736373ULL, 184411439ULL, 0 } }, 
    { .Fen = "r3k2r/8/8/8/8/8/8/R3K1R1 w Qkq - 0 1 ", .Expected = { 1ULL, 25ULL, 547ULL, 13579ULL, 316214ULL, 7878456ULL, 189224276ULL, 0 } }, 
    { .Fen = "1r2k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1 ", .Expected = { 1ULL, 26ULL, 583ULL, 14252ULL, 334705ULL, 8198901ULL, 198328929ULL, 0 } }, 
    { .Fen = "2r1k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1 ", .Expected = { 1ULL, 25ULL, 560ULL, 13592ULL, 317324ULL, 7710115ULL, 185959088ULL, 0 } }, 
    { .Fen = "r3k1r1/8/8/8/8/8/8/R3K2R w KQq - 0 1 ", .Expected = { 1ULL, 25ULL, 560ULL, 13607ULL, 320792ULL, 7848606ULL, 190755813ULL, 0 } }, 
    { .Fen = "4k3/8/8/8/8/8/8/4K2R b K - 0 1 ", .Expected = { 1ULL, 5ULL, 75ULL, 459ULL, 8290ULL, 47635ULL, 899442ULL, 0 } }, 
    { .Fen = "4k3/8/8/8/8/8/8/R3K3 b Q - 0 1 ", .Expected = { 1ULL, 5ULL, 80ULL, 493ULL, 8897ULL, 52710ULL, 1001523ULL, 0 } }, 
    { .Fen = "4k2r/8/8/8/8/8/8/4K3 b k - 0 1 ", .Expected = { 1ULL, 15ULL, 66ULL, 1197ULL, 7059ULL, 133987ULL, 764643ULL, 0 } }, 
    { .Fen = "r3k3/8/8/8/8/8/8/4K3 b q - 0 1 ", .Expected = { 1ULL, 16ULL, 71ULL, 1287ULL, 7626ULL, 145232ULL, 846648ULL, 0 } }, 
    { .Fen = "4k3/8/8/8/8/8/8/R3K2R b KQ - 0 1 ", .Expected = { 1ULL, 5ULL, 130ULL, 782ULL, 22180ULL, 118882ULL, 3517770ULL, 0 } }, 
    { .Fen = "r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1 ", .Expected = { 1ULL, 26ULL, 112ULL, 3189ULL, 17945ULL, 532933ULL, 2788982ULL, 0 } }, 
    { .Fen = "8/8/8/8/8/8/6k1/4K2R b K - 0 1 ", .Expected = { 1ULL, 3ULL, 32ULL, 134ULL, 2073ULL, 10485ULL, 179869ULL, 0 } }, 
    { .Fen = "8/8/8/8/8/8/1k6/R3K3 b Q - 0 1 ", .Expected = { 1ULL, 4ULL, 49ULL, 243ULL, 3991ULL, 20780ULL, 367724ULL, 0 } }, 
    { .Fen = "4k2r/6K1/8/8/8/8/8/8 b k - 0 1 ", .Expected = { 1ULL, 12ULL, 38ULL, 564ULL, 2219ULL, 37735ULL, 185867ULL, 0 } }, 
    { .Fen = "r3k3/1K6/8/8/8/8/8/8 b q - 0 1 ", .Expected = { 1ULL, 15ULL, 65ULL, 1018ULL, 4573ULL, 80619ULL, 413018ULL, 0 } }, 
    { .Fen = "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1 ", .Expected = { 1ULL, 26ULL, 568ULL, 13744ULL, 314346ULL, 7594526ULL, 179862938ULL, 0 } }, 
    { .Fen = "r3k2r/8/8/8/8/8/8/1R2K2R b Kkq - 0 1 ", .Expected = { 1ULL, 26ULL, 583ULL, 14252ULL, 334705ULL, 8198901ULL, 198328929ULL, 0 } }, 
    { .Fen = "r3k2r/8/8/8/8/8/8/2R1K2R b Kkq - 0 1 ", .Expected = { 1ULL, 25ULL, 560ULL, 13592ULL, 317324ULL, 7710115ULL, 185959088ULL, 0 } }, 
    { .Fen = "r3k2r/8/8/8/8/8/8/R3K1R1 b Qkq - 0 1 ", .Expected = { 1ULL, 25ULL, 560ULL, 13607ULL, 320792ULL, 7848606ULL, 190755813ULL, 0 } }, 
    { .Fen = "1r2k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1 ", .Expected = { 1ULL, 25ULL, 567ULL, 14095ULL, 328965ULL, 8153719ULL, 195629489ULL, 0 } }, 
    { .Fen = "2r1k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1 ", .Expected = { 1ULL, 25ULL, 548ULL, 13502ULL, 312835ULL, 7736373ULL, 184411439ULL, 0 } }, 
    { .Fen = "r3k1r1/8/8/8/8/8/8/R3K2R b KQq - 0 1 ", .Expected = { 1ULL, 25ULL, 547ULL, 13579ULL, 316214ULL, 7878456ULL, 189224276ULL, 0 } }, 
    { .Fen = "8/1n4N1/2k5/8/8/5K2/1N4n1/8 w - - 0 1 ", .Expected = { 1ULL, 14ULL, 195ULL, 2760ULL, 38675ULL, 570726ULL, 8107539ULL, 0 } }, 
    { .Fen = "8/1k6/8/5N2/8/4n3/8/2K5 w - - 0 1 ", .Expected = { 1ULL, 11ULL, 156ULL, 1636ULL, 20534ULL, 223507ULL, 2594412ULL, 0 } }, 
    { .Fen = "8/8/4k3/3Nn3/3nN3/4K3/8/8 w - - 0 1 ", .Expected = { 1ULL, 19ULL, 289ULL, 4442ULL, 73584ULL, 1198299ULL, 19870403ULL, 0 } }, 
    { .Fen = "K7/8/2n5/1n6/8/8/8/k6N w - - 0 1 ", .Expected = { 1ULL, 3ULL, 51ULL, 345ULL, 5301ULL, 38348ULL, 588695ULL, 0 } }, 
    { .Fen = "k7/8/2N5/1N6/8/8/8/K6n w - - 0 1 ", .Expected = { 1ULL, 17ULL, 54ULL, 835ULL, 5910ULL, 92250ULL, 688780ULL, 0 } }, 
    { .Fen = "8/1n4N1/2k5/8/8/5K2/1N4n1/8 b - - 0 1 ", .Expected = { 1ULL, 15ULL, 193ULL, 2816ULL, 40039ULL, 582642ULL, 8503277ULL, 0 } }, 
    { .Fen = "8/1k6/8/5N2/8/4n3/8/2K5 b - - 0 1 ", .Expected = { 1ULL, 16ULL, 180ULL, 2290ULL, 24640ULL, 288141ULL, 3147566ULL, 0 } }, 
    { .Fen = "8/8/3K4/3Nn3/3nN3/4k3/8/8 b - - 0 1 ", .Expected = { 1ULL, 4ULL, 68ULL, 1118ULL, 16199ULL, 281190ULL, 4405103ULL, 0 } }, 
    { .Fen = "K7/8/2n5/1n6/8/8/8/k6N b - - 0 1 ", .Expected = { 1ULL, 17ULL, 54ULL, 835ULL, 5910ULL, 92250ULL, 688780ULL, 0 } }, 
    { .Fen = "k7/8/2N5/1N6/8/8/8/K6n b - - 0 1 ", .Expected = { 1ULL, 3ULL, 51ULL, 345ULL, 5301ULL, 38348ULL, 588695ULL, 0 } }, 
    { .Fen = "B6b/8/8/8/2K5/4k3/8/b6B w - - 0 1 ", .Expected = { 1ULL, 17ULL, 278ULL, 4607ULL, 76778ULL, 1320507ULL, 22823890ULL, 0 } }, 
    { .Fen = "8/8/1B6/7b/7k/8/2B1b3/7K w - - 0 1 ", .Expected = { 1ULL, 21ULL, 316ULL, 5744ULL, 93338ULL, 1713368ULL, 28861171ULL, 0 } }, 
    { .Fen = "k7/B7/1B6/1B6/8/8/8/K6b w - - 0 1 ", .Expected = { 1ULL, 21ULL, 144ULL, 3242ULL, 32955ULL, 787524ULL, 7881673ULL, 0 } }, 
    { .Fen = "K7/b7/1b6/1b6/8/8/8/k6B w - - 0 1 ", .Expected = { 1ULL, 7ULL, 143ULL, 1416ULL, 31787ULL, 310862ULL, 7382896ULL, 0 } }, 
    { .Fen = "B6b/8/8/8/2K5/5k2/8/b6B b - - 0 1 ", .Expected = { 1ULL, 6ULL, 106ULL, 1829ULL, 31151ULL, 530585ULL, 9250746ULL, 0 } }, 
    { .Fen = "8/8/1B6/7b/7k/8/2B1b3/7K b - - 0 1 ", .Expected = { 1ULL, 17ULL, 309ULL, 5133ULL, 93603ULL, 1591064ULL, 29027891ULL, 0 } }, 
    { .Fen = "k7/B7/1B6/1B6/8/8/8/K6b b - - 0 1 ", .Expected = { 1ULL, 7ULL, 143ULL, 1416ULL, 31787ULL, 310862ULL, 7382896ULL, 0 } }, 
    { .Fen = "K7/b7/1b6/1b6/8/8/8/k6B b - - 0 1 ", .Expected = { 1ULL, 21ULL, 144ULL, 3242ULL, 32955ULL, 787524ULL, 7881673ULL, 0 } }, 
    { .Fen = "7k/RR6/8/8/8/8/rr6/7K w - - 0 1 ", .Expected = { 1ULL, 19ULL, 275ULL, 5300ULL, 104342ULL, 2161211ULL, 44956585ULL, 0 } }, 
    { .Fen = "R6r/8/8/2K5/5k2/8/8/r6R w - - 0 1 ", .Expected = { 1ULL, 36ULL, 1027ULL, 29215ULL, 771461ULL, 20506480ULL, 525169084ULL, 0 } }, 
    { .Fen = "7k/RR6/8/8/8/8/rr6/7K b - - 0 1 ", .Expected = { 1ULL, 19ULL, 275ULL, 5300ULL, 104342ULL, 2161211ULL, 44956585ULL, 0 } }, 
    { .Fen = "R6r/8/8/2K5/5k2/8/8/r6R b - - 0 1 ", .Expected = { 1ULL, 36ULL, 1027ULL, 29227ULL, 771368ULL, 20521342ULL, 524966748ULL, 0 } }, 
    { .Fen = "6kq/8/8/8/8/8/8/7K w - - 0 1 ", .Expected = { 1ULL, 2ULL, 36ULL, 143ULL, 3637ULL, 14893ULL, 391507ULL, 0 } }, 
    { .Fen = "6KQ/8/8/8/8/8/8/7k b - - 0 1 ", .Expected = { 1ULL, 2ULL, 36ULL, 143ULL, 3637ULL, 14893ULL, 391507ULL, 0 } }, 
    { .Fen = "K7/8/8/3Q4/4q3/8/8/7k w - - 0 1 ", .Expected = { 1ULL, 6ULL, 35ULL, 495ULL, 8349ULL, 166741ULL, 3370175ULL, 0 } }, 
    { .Fen = "6qk/8/8/8/8/8/8/7K b - - 0 1 ", .Expected = { 1ULL, 22ULL, 43ULL, 1015ULL, 4167ULL, 105749ULL, 419369ULL, 0 } }, 
    { .Fen = "6KQ/8/8/8/8/8/8/7k b - - 0 1 ", .Expected = { 1ULL, 2ULL, 36ULL, 143ULL, 3637ULL, 14893ULL, 391507ULL, 0 } }, 
    { .Fen = "K7/8/8/3Q4/4q3/8/8/7k b - - 0 1 ", .Expected = { 1ULL, 6ULL, 35ULL, 495ULL, 8349ULL, 166741ULL, 3370175ULL, 0 } }, 
    { .Fen = "8/8/8/8/8/K7/P7/k7 w - - 0 1 ", .Expected = { 1ULL, 3ULL, 7ULL, 43ULL, 199ULL, 1347ULL, 6249ULL, 0 } }, 
    { .Fen = "8/8/8/8/8/7K/7P/7k w - - 0 1 ", .Expected = { 1ULL, 3ULL, 7ULL, 43ULL, 199ULL, 1347ULL, 6249ULL, 0 } }, 
    { .Fen = "K7/p7/k7/8/8/8/8/8 w - - 0 1 ", .Expected = { 1ULL, 1ULL, 3ULL, 12ULL, 80ULL, 342ULL, 2343ULL, 0 } }, 
    { .Fen = "7K/7p/7k/8/8/8/8/8 w - - 0 1 ", .Expected = { 1ULL, 1ULL, 3ULL, 12ULL, 80ULL, 342ULL, 2343ULL, 0 } }, 
    { .Fen = "8/2k1p3/3pP3/3P2K1/8/8/8/8 w - - 0 1 ", .Expected = { 1ULL, 7ULL, 35ULL, 210ULL, 1091ULL, 7028ULL, 34834ULL, 0 } }, 
    { .Fen = "8/8/8/8/8/K7/P7/k7 b - - 0 1 ", .Expected = { 1ULL, 1ULL, 3ULL, 12ULL, 80ULL, 342ULL, 2343ULL, 0 } }, 
    { .Fen = "8/8/8/8/8/7K/7P/7k b - - 0 1 ", .Expected = { 1ULL, 1ULL, 3ULL, 12ULL, 80ULL, 342ULL, 2343ULL, 0 } }, 
    { .Fen = "K7/p7/k7/8/8/8/8/8 b - - 0 1 ", .Expected = { 1ULL, 3ULL, 7ULL, 43ULL, 199ULL, 1347ULL, 6249ULL, 0 } }, 
    { .Fen = "7K/7p/7k/8/8/8/8/8 b - - 0 1 ", .Expected = { 1ULL, 3ULL, 7ULL, 43ULL, 199ULL, 1347ULL, 6249ULL, 0 } }, 
    { .Fen = "8/2k1p3/3pP3/3P2K1/8/8/8/8 b - - 0 1 ", .Expected = { 1ULL, 5ULL, 35ULL, 182ULL, 1091ULL, 5408ULL, 34822ULL, 0 } }, 
    { .Fen = "8/8/8/8/8/4k3/4P3/4K3 w - - 0 1 ", .Expected = { 1ULL, 2ULL, 8ULL, 44ULL, 282ULL, 1814ULL, 11848ULL, 0 } }, 
    { .Fen = "4k3/4p3/4K3/8/8/8/8/8 b - - 0 1 ", .Expected = { 1ULL, 2ULL, 8ULL, 44ULL, 282ULL, 1814ULL, 11848ULL, 0 } }, 
    { .Fen = "8/8/7k/7p/7P/7K/8/8 w - - 0 1 ", .Expected = { 1ULL, 3ULL, 9ULL, 57ULL, 360ULL, 1969ULL, 10724ULL, 0 } }, 
    { .Fen = "8/8/k7/p7/P7/K7/8/8 w - - 0 1 ", .Expected = { 1ULL, 3ULL, 9ULL, 57ULL, 360ULL, 1969ULL, 10724ULL, 0 } }, 
    { .Fen = "8/8/3k4/3p4/3P4/3K4/8/8 w - - 0 1 ", .Expected = { 1ULL, 5ULL, 25ULL, 180ULL, 1294ULL, 8296ULL, 53138ULL, 0 } }, 
    { .Fen = "8/3k4/3p4/8/3P4/3K4/8/8 w - - 0 1 ", .Expected = { 1ULL, 8ULL, 61ULL, 483ULL, 3213ULL, 23599ULL, 157093ULL, 0 } }, 
    { .Fen = "8/8/3k4/3p4/8/3P4/3K4/8 w - - 0 1 ", .Expected = { 1ULL, 8ULL, 61ULL, 411ULL, 3213ULL, 21637ULL, 158065ULL, 0 } }, 
    { .Fen = "k7/8/3p4/8/3P4/8/8/7K w - - 0 1 ", .Expected = { 1ULL, 4ULL, 15ULL, 90ULL, 534ULL, 3450ULL, 20960ULL, 0 } }, 
    { .Fen = "8/8/7k/7p/7P/7K/8/8 b - - 0 1 ", .Expected = { 1ULL, 3ULL, 9ULL, 57ULL, 360ULL, 1969ULL, 10724ULL, 0 } }, 
    { .Fen = "8/8/k7/p7/P7/K7/8/8 b - - 0 1 ", .Expected = { 1ULL, 3ULL, 9ULL, 57ULL, 360ULL, 1969ULL, 10724ULL, 0 } }, 
    { .Fen = "8/8/3k4/3p4/3P4/3K4/8/8 b - - 0 1 ", .Expected = { 1ULL, 5ULL, 25ULL, 180ULL, 1294ULL, 8296ULL, 53138ULL, 0 } }, 
    { .Fen = "8/3k4/3p4/8/3P4/3K4/8/8 b - - 0 1 ", .Expected = { 1ULL, 8ULL, 61ULL, 411ULL, 3213ULL, 21637ULL, 158065ULL, 0 } }, 
    { .Fen = "8/8/3k4/3p4/8/3P4/3K4/8 b - - 0 1 ", .Expected = { 1ULL, 8ULL, 61ULL, 483ULL, 3213ULL, 23599ULL, 157093ULL, 0 } }, 
    { .Fen = "k7/8/3p4/8/3P4/8/8/7K b - - 0 1 ", .Expected = { 1ULL, 4ULL, 15ULL, 89ULL, 537ULL, 3309ULL, 21104ULL, 0 } }, 
    { .Fen = "7k/3p4/8/8/3P4/8/8/K7 w - - 0 1 ", .Expected = { 1ULL, 4ULL, 19ULL, 117ULL, 720ULL, 4661ULL, 32191ULL, 0 } }, 
    { .Fen = "7k/8/8/3p4/8/8/3P4/K7 w - - 0 1 ", .Expected = { 1ULL, 5ULL, 19ULL, 116ULL, 716ULL, 4786ULL, 30980ULL, 0 } }, 
    { .Fen = "k7/8/8/7p/6P1/8/8/K7 w - - 0 1 ", .Expected = { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL, 0 } }, 
    { .Fen = "k7/8/7p/8/8/6P1/8/K7 w - - 0 1 ", .Expected = { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL, 0 } }, 
    { .Fen = "k7/8/8/6p1/7P/8/8/K7 w - - 0 1 ", .Expected = { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL, 0 } }, 
    { .Fen = "k7/8/6p1/8/8/7P/8/K7 w - - 0 1 ", .Expected = { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL, 0 } }, 
    { .Fen = "k7/8/8/3p4/4p3/8/8/7K w - - 0 1 ", .Expected = { 1ULL, 3ULL, 15ULL, 84ULL, 573ULL, 3013ULL, 22886ULL, 0 } }, 
    { .Fen = "k7/8/3p4/8/8/4P3/8/7K w - - 0 1 ", .Expected = { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4271ULL, 28662ULL, 0 } }, 
    { .Fen = "7k/3p4/8/8/3P4/8/8/K7 b - - 0 1 ", .Expected = { 1ULL, 5ULL, 19ULL, 117ULL, 720ULL, 5014ULL, 32167ULL, 0 } }, 
    { .Fen = "7k/8/8/3p4/8/8/3P4/K7 b - - 0 1 ", .Expected = { 1ULL, 4ULL, 19ULL, 117ULL, 712ULL, 4658ULL, 30749ULL, 0 } }, 
    { .Fen = "k7/8/8/7p/6P1/8/8/K7 b - - 0 1 ", .Expected = { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL, 0 } }, 
    { .Fen = "k7/8/7p/8/8/6P1/8/K7 b - - 0 1 ", .Expected = { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL, 0 } }, 
    { .Fen = "k7/8/8/6p1/7P/8/8/K7 b - - 0 1 ", .Expected = { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL, 0 } }, 
    { .Fen = "k7/8/6p1/8/8/7P/8/K7 b - - 0 1 ", .Expected = { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL, 0 } }, 
    { .Fen = "k7/8/8/3p4/4p3/8/8/7K b - - 0 1 ", .Expected = { 1ULL, 5ULL, 15ULL, 102ULL, 569ULL, 4337ULL, 22579ULL, 0 } }, 
    { .Fen = "k7/8/3p4/8/8/4P3/8/7K b - - 0 1 ", .Expected = { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4271ULL, 28662ULL, 0 } }, 
    { .Fen = "7k/8/8/p7/1P6/8/8/7K w - - 0 1 ", .Expected = { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL, 0 } }, 
    { .Fen = "7k/8/p7/8/8/1P6/8/7K w - - 0 1 ", .Expected = { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL, 0 } }, 
    { .Fen = "7k/8/8/1p6/P7/8/8/7K w - - 0 1 ", .Expected = { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL, 0 } }, 
    { .Fen = "7k/8/1p6/8/8/P7/8/7K w - - 0 1 ", .Expected = { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL, 0 } }, 
    { .Fen = "k7/7p/8/8/8/8/6P1/K7 w - - 0 1 ", .Expected = { 1ULL, 5ULL, 25ULL, 161ULL, 1035ULL, 7574ULL, 55338ULL, 0 } }, 
    { .Fen = "k7/6p1/8/8/8/8/7P/K7 w - - 0 1 ", .Expected = { 1ULL, 5ULL, 25ULL, 161ULL, 1035ULL, 7574ULL, 55338ULL, 0 } }, 
    { .Fen = "3k4/3pp3/8/8/8/8/3PP3/3K4 w - - 0 1 ", .Expected = { 1ULL, 7ULL, 49ULL, 378ULL, 2902ULL, 24122ULL, 199002ULL, 0 } }, 
    { .Fen = "7k/8/8/p7/1P6/8/8/7K b - - 0 1 ", .Expected = { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL, 0 } }, 
    { .Fen = "7k/8/p7/8/8/1P6/8/7K b - - 0 1 ", .Expected = { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL, 0 } }, 
    { .Fen = "7k/8/8/1p6/P7/8/8/7K b - - 0 1 ", .Expected = { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL, 0 } }, 
    { .Fen = "7k/8/1p6/8/8/P7/8/7K b - - 0 1 ", .Expected = { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL, 0 } }, 
    { .Fen = "k7/7p/8/8/8/8/6P1/K7 b - - 0 1 ", .Expected = { 1ULL, 5ULL, 25ULL, 161ULL, 1035ULL, 7574ULL, 55338ULL, 0 } }, 
    { .Fen = "k7/6p1/8/8/8/8/7P/K7 b - - 0 1 ", .Expected = { 1ULL, 5ULL, 25ULL, 161ULL, 1035ULL, 7574ULL, 55338ULL, 0 } }, 
    { .Fen = "3k4/3pp3/8/8/8/8/3PP3/3K4 b - - 0 1 ", .Expected = { 1ULL, 7ULL, 49ULL, 378ULL, 2902ULL, 24122ULL, 199002ULL, 0 } }, 
    { .Fen = "8/Pk6/8/8/8/8/6Kp/8 w - - 0 1 ", .Expected = { 1ULL, 11ULL, 97ULL, 887ULL, 8048ULL, 90606ULL, 1030499ULL, 0 } }, 
    { .Fen = "n1n5/1Pk5/8/8/8/8/5Kp1/5N1N w - - 0 1 ", .Expected = { 1ULL, 24ULL, 421ULL, 7421ULL, 124608ULL, 2193768ULL, 37665329ULL, 0 } }, 
    { .Fen = "8/PPPk4/8/8/8/8/4Kppp/8 w - - 0 1 ", .Expected = { 1ULL, 18ULL, 270ULL, 4699ULL, 79355ULL, 1533145ULL, 28859283ULL, 0 } }, 
    { .Fen = "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1 ", .Expected = { 1ULL, 24ULL, 496ULL, 9483ULL, 182838ULL, 3605103ULL, 71179139ULL, 0 } }, 
    { .Fen = "8/Pk6/8/8/8/8/6Kp/8 b - - 0 1 ", .Expected = { 1ULL, 11ULL, 97ULL, 887ULL, 8048ULL, 90606ULL, 1030499ULL, 0 } }, 
    { .Fen = "n1n5/1Pk5/8/8/8/8/5Kp1/5N1N b - - 0 1 ", .Expected = { 1ULL, 24ULL, 421ULL, 7421ULL, 124608ULL, 2193768ULL, 37665329ULL, 0 } }, 
    { .Fen = "8/PPPk4/8/8/8/8/4Kppp/8 b - - 0 1 ", .Expected = { 1ULL, 18ULL, 270ULL, 4699ULL, 79355ULL, 1533145ULL, 28859283ULL, 0 } }, 
    { .Fen = "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1 ", .Expected = { 1ULL, 24ULL, 496ULL, 9483ULL, 182838ULL, 3605103ULL, 71179139ULL, 0 } }, 
};

int main(void) 
{
    int res = 0; 
    int N = sizeof(s_Tests) / sizeof(s_Tests[0]); 

    for (int i = 0; i < N; i++) 
    {
        char name[64]; 
        snprintf(name, 64, "Position %d / %d", i+1, N); 

        res += RunPerft(name, s_Tests[i].Fen, s_Tests[i].Expected); 
    }

    printf("%d / %d passed\n", res, s_Total); 
    printf("\nSpeed (>%.1fs)\n", MIN_SPEED_TIME); 
    printf("Min: %.2fMnps\n", speed.Slow / 1000000); 
    printf("Max: %.2fMnps\n", speed.Fast / 1000000); 
    printf("Mean: %.2fMnps\n", speed.Total / speed.N / 1000000); 

    return -(res != s_Total); // 0 if succeed
}
