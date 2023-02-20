#include <stdatomic.h> 
#include <stdbool.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <pthread.h> 

#include "bitboard.h" 
#include "castle.h"
#include "game.h" 
#include "move.h"
#include "movegen.h"
#include "piece.h"
#include "random.h" 
#include "search.h" 
#include "square.h"
#include "vector.h" 

#ifndef ENGINE_NAME 
    #define ENGINE_NAME "Halcyon"
#endif 

#define UCI_MAX_INPUT 4096

game *uci_game; 
search_ctx engine; 

const char *uci_next_token(void) 
{
    return strtok(NULL, " \n"); 
}

bool uci_equals(const char *a, const char *b) 
{
    if (!a || !b) return false; 

    return strcmp(a, b) == 0; 
}

bool uci_cmd_uci(void) 
{
    printf("id name %s\n", ENGINE_NAME); 
    printf("id author Nicholas Hamilton\n"); 
    printf("uciok\n"); 
    return true; 
}

bool uci_cmd_isready(void) 
{
    printf("readyok\n"); 
    return true; 
}

bool uci_cmd_ucinewgame(void) 
{
    stop_search_ctx(&engine); 
    load_fen(uci_game, START_FEN); 
    return true; 
}

bool uci_cmd_position(void) 
{
    mvlist *moves = new_mvlist(); 

    game *g = uci_game; 

    const char *token = uci_next_token(); 

    bool success = true; 

    if (uci_equals(token, "startpos")) 
    {
        load_fen(g, START_FEN); 
        token = uci_next_token(); 
    }
    else if (uci_equals(token, "test1")) 
    {
        load_fen(g, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"); 
        token = uci_next_token(); 
    }
    else if (uci_equals(token, "test2")) 
    {
        load_fen(g, "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -"); 
        token = uci_next_token(); 
    }
    else if (uci_equals(token, "test3")) 
    {
        load_fen(g, "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"); 
        token = uci_next_token(); 
    }
    else if (uci_equals(token, "test4")) 
    {
        load_fen(g, "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1"); 
        token = uci_next_token(); 
    }
    else if (uci_equals(token, "test5")) 
    {
        load_fen(g, "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"); 
        token = uci_next_token(); 
    }
    else if (uci_equals(token, "fen")) 
    {
        // need to recreate FEN from token list 
        char fen[UCI_MAX_INPUT]; 
        fen[0] = '\0'; 
        while ((token = uci_next_token()) && !uci_equals(token, "moves")) 
        {
            strcpy(fen + strlen(fen), token); 
            size_t len = strlen(fen); 
            fen[len] = ' '; 
            fen[len + 1] = '\0'; 
        }
        // token is NULL or "moves" at this point 

        load_fen(g, fen); 
    }

    if (uci_equals(token, "moves")) 
    {
        while ((token = uci_next_token())) 
        {
            clear_mvlist(moves); 
            gen_moves(g, moves); 

            piece promote = PC_P; 

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

            square from = make_sq(token[0] - 'a', token[1] - '1'); 
            square to = make_sq(token[2] - 'a', token[3] - '1'); 

            // printf("%s (%s) to %s\n", str_sq(from), str_pc(pc_at_or_wp(g, from)), str_sq(to)); 

            // if promote is still pawn, then it wasn't promoted -> get the real original piece
            if (promote == PC_P) 
            {
                promote = get_no_col(pc_at(g, from)); 
            }

            promote = make_pc(promote, g->turn); 

            // printf("Piece: %s\n", str_pc(promote)); 

            // make sure the complete move is legal 
            bool found = false; 
            for (size_t i = 0; i < moves->size; i++) 
            {
                move m = moves->moves[i]; 
                if (from_sq(m) == from && to_sq(m) == to && pro_pc(m) == promote) 
                {
                    found = true; 
                    push_move(g, m); 
                    no_depth(g); 
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
        load_fen(g, START_FEN); 
    }

    free_mvlist(moves); 
    return success; 
}

bool uci_cmd_print(void) 
{
    print_game(uci_game); 
    return true; 
}

bool uci_cmd_go(void) 
{
    const char *token; 
    int depth = INF_DEPTH, time_ms = INF_TIME; 
    int movetime = INF_TIME; 
    int wtime = INF_TIME, btime = INF_TIME, sidetime = INF_TIME; 
    int winc = 0, binc = 0, sideinc = 0; 
    int perft_num = -1; 

    while ((token = uci_next_token())) 
    {
        if (uci_equals(token, "depth") && (token = uci_next_token())) 
        {
            depth = atoi(token); 
        }
        if (uci_equals(token, "movetime") && (token = uci_next_token())) 
        {
            movetime = atoi(token); 
        }
        if (uci_equals(token, "wtime") && (token = uci_next_token())) 
        {
            wtime = atoi(token); 
            if (uci_game->turn == COL_W) 
            {
                sidetime = wtime; 
                if (sidetime < 0) sidetime = 0; 
            }
        }
        if (uci_equals(token, "btime") && (token = uci_next_token())) 
        {
            btime = atoi(token); 
            if (uci_game->turn == COL_B) 
            {
                sidetime = btime; 
                if (sidetime < 0) sidetime = 0; 
            }
        }
        if (uci_equals(token, "winc") && (token = uci_next_token())) 
        {
            winc = atoi(token); 
            if (uci_game->turn == COL_W) sideinc = winc; 
        }
        if (uci_equals(token, "binc") && (token = uci_next_token())) 
        {
            binc = atoi(token); 
            if (uci_game->turn == COL_B) sideinc = binc; 
        }
        if (uci_equals(token, "perft") && (token = uci_next_token())) 
        {
            perft_num = atoi(token); 
        }
    }

    if (perft_num > 0) 
    {
        uint64_t total = 0; 

        game *pgame = new_game(); 
        copy_game(pgame, uci_game); 
        mvlist *moves = new_mvlist(); 

        clock_t start = clock(); 

        gen_moves(uci_game, moves); 
        for (size_t i = 0; i < moves->size; i++) 
        {
            move mv = moves->moves[i]; 
            print_move_end(mv, " - "); 
            push_move(uci_game, mv); 
            total += perft(uci_game, perft_num - 1); 
            pop_move(uci_game, mv); 
            printf("\n"); 
        }

        clock_t end = clock(); 
        printf("Total: %lu", total); 

        free_mvlist(moves); 
        free_game(pgame); 

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

    if (sidetime > INF_TIME) 
    {
        int total_time = sidetime - 2000; 
        if (total_time < 40) total_time = 40; 

        int tgt_time = (int) (total_time / 40 + sideinc * 3 / 4); 
        if (tgt_time <= 0) tgt_time = 1; 
         
        time_ms = tgt_time; 
        printf("info string Using %dms out of %dms to think\n", time_ms, sidetime); 
        fflush(stdout); 
    }

    if (time_ms > 20000) 
    {
        // limit to <= 20sec if time remaining is given 
        time_ms = 20000; 
    }

    if (movetime > 0) 
    {
        time_ms = movetime; 
    }

    if (time_ms > 0) 
    {
        printf("info string Searching with max time of %dms\n", time_ms); 
    }
    else 
    {
        printf("info string Searching with no time limit\n"); 
    }
    fflush(stdout); 

    search_params params; 
    init_search_params(&params, uci_game, depth, time_ms); 

    search(&engine, &params); 
    return true; 
}

bool uci_cmd_stop(void) 
{
    stop_search_ctx(&engine); 
    return true; 
}

bool uci_parse(const char *orig_cmd) 
{
    // remove spaces at the beginning
    while (*orig_cmd && *orig_cmd == ' ') orig_cmd++; 

    char cmd[UCI_MAX_INPUT]; 
    strcpy(cmd, orig_cmd); 

    char *token = strtok(cmd, " \n"); 
    if (token) 
    {
        if (uci_equals(token, "quit")) exit(0); 
        if (uci_equals(token, "ucinewgame")) return uci_cmd_ucinewgame(); 
        if (uci_equals(token, "uci")) return uci_cmd_uci(); 
        if (uci_equals(token, "isready")) return uci_cmd_isready(); 
        if (uci_equals(token, "position")) return uci_cmd_position(); 
        if (uci_equals(token, "print")) return uci_cmd_print(); 
        if (uci_equals(token, "go")) return uci_cmd_go(); 
        if (uci_equals(token, "stop")) return uci_cmd_stop(); 
    }

    return false; 
}

int main(void) 
{
    printf("%s by Nicholas Hamilton\n", ENGINE_NAME); 
    fflush(stdout); 

    uci_game = new_game(); 
    load_fen(uci_game, START_FEN); 
    create_search_ctx(&engine); 

    char input[UCI_MAX_INPUT]; 
    while (true) 
    {
        fflush(stdout); 
        fgets(input, UCI_MAX_INPUT, stdin); 
        input[strlen(input) - 1] = '\0'; 
        if (!uci_parse(input)) printf("Unknown command: '%s'\n", input); 
    }

    free_game(uci_game); 
    destroy_search_ctx(&engine); 
    return 0; 
}