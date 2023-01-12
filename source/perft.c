#include <stdio.h> 
#include <time.h> 

#include "bitboard.h" 
#include "castle.h"
#include "game.h" 
#include "move.h"
#include "piece.h"
#include "random.h" 
#include "square.h"
#include "vector.h" 

static int total = 0; 

bool perft(const char *name, const char *fen, int depth, const uint64_t *expected) 
{
    total++; 

    bool ret = true; 

    game g; 
    game_create_fen(&g, fen); 

    printf("%s\n%s\n", name, fen); 
    game_print(&g); 

    int i = -1; 
    while (++i <= depth) 
    {
        clock_t c = clock(); 
        uint64_t total = game_perft(&g, i);
        clock_t c2 = clock(); 

        double time = (double) (c2 - c) / CLOCKS_PER_SEC; 
        printf(", Time: %.2lfms, Speed: %.2lfMnps - %s\n", time * 1000, total / time / 1000 / 1000, expected[i] == total ? "PASS" : "FAIL"); 

        if (expected[i] != total) 
        {
            printf("FAILURE: Expected %lu, difference of %ld\n", expected[i], (int64_t) expected[i] - (int64_t) total); 
            ret = 0; 
            // goto cleanup; 
        }
    }

    printf("DONE\n\n"); 

cleanup: 
    game_destroy(&g); 
    return ret; 
}

uint64_t pos1[] = 
{
    1ULL, 
    20ULL, 
    400ULL, 
    8902ULL, 
    197281ULL, 
    4865609ULL, 
    119060324ULL, 
    3195901860ULL
};

uint64_t pos2[] = 
{
    1ULL, 
    48ULL, 
    2039ULL, 
    97862ULL, 
    4085603ULL, 
    193690690ULL, 
    8031647685ULL
};

uint64_t pos3[] = 
{
    1ULL, 
    14ULL, 
    191ULL, 
    2812ULL, 
    43238ULL, 
    674624ULL, 
    11030083ULL 
};

uint64_t pos4[] = 
{
    1ULL, 
    6ULL, 
    264ULL, 
    9467ULL, 
    422333ULL, 
    15833292ULL, 
    706045033ULL
};

uint64_t pos5[] = 
{
    1ULL, 
    44ULL, 
    1486ULL, 
    62379ULL, 
    2103487ULL, 
    89941194ULL
};

uint64_t pos6[] = 
{
    1ULL, 
    46ULL, 
    2079ULL, 
    89890ULL, 
    3894594ULL, 
    164075551ULL, 
    6923051137ULL
};

int main(void) 
{
    int res = 0; 

    res += perft("Position 1", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ", 6, pos1); 
    res += perft("Position 2", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ", 5, pos2); 
    res += perft("Position 3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - ", 6, pos3); 
    res += perft("Position 4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 6, pos4); 
    res += perft("Position 4.1", "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 6, pos4); 
    res += perft("Position 5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8  ", 5, pos5); 
    res += perft("Position 6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 ", 5, pos6); 

    printf("%d / %d passed\n", res, total); 

    return -(res != total); // 0 if succeed
}