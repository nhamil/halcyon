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
#include "piece.h"
#include "random.h" 
#include "search.h" 
#include "square.h"
#include "vector.h" 

#define ENGINE_NAME "Halcyon 0.1"

#define UCI_MAX_INPUT 4096

FILE *log; 
game uci_game; 
search_thread engine; 

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
    stop_search_thread(&engine); 
    load_fen(&uci_game, START_FEN); 
    return true; 
}

bool uci_cmd_position(void) 
{
    vector moves; 
    CREATE_VEC(&moves, move); 

    game *g = &uci_game; 

    const char *token = uci_next_token(); 

    bool success = true; 

    if (uci_equals(token, "startpos")) 
    {
        load_fen(g, START_FEN); 
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
        while (token = uci_next_token()) 
        {
            clear_vec(&moves); 
            gen_moves(g, &moves); 

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

            printf("%s (%s) to %s\n", str_sq(from), str_pc(pc_at_or_wp(g, from)), str_sq(to)); 

            // if promote is still pawn, then it wasn't promoted -> get the real original piece
            if (promote == PC_P) 
            {
                promote = get_no_col(pc_at_or_wp(g, from)); 
            }

            promote = make_pc(promote, g->turn); 

            printf("Piece: %s\n", str_pc(promote)); 

            // make sure the complete move is legal 
            bool found = false; 
            for (size_t i = 0; i < moves.size; i++) 
            {
                move m = AT_VEC(&moves, move, i); 
                print_move(m); 
                if (from_sq(m) == from && to_sq(m) == to && pro_pc(m) == promote) 
                {
                    found = true; 
                    push_move(g, m); 
                }
            }

            // keep previous legal moves, only ignore remaining 
            if (!found) 
            {
                printf("Could not find move '%s'\n", token); 
                goto done; 
            }
        }
    }

done: 
    if (!success) 
    {
        load_fen(g, START_FEN); 
    }

    destroy_vec(&moves); 
    return success; 
}

bool uci_cmd_print(void) 
{
    print_game(&uci_game); 
    return true; 
}

bool uci_cmd_go(void) 
{
    const char *token; 
    int depth = INF_DEPTH, time_ms = INF_TIME; 
    int movetime = INF_TIME; 
    int wtime = INF_TIME, btime = INF_TIME, sidetime = INF_TIME; 

    while (token = uci_next_token()) 
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
            if (uci_game.turn == COL_W) sidetime = wtime; 
        }
        if (uci_equals(token, "btime") && (token = uci_next_token())) 
        {
            btime = atoi(token); 
            if (uci_game.turn == COL_B) sidetime = btime; 
        }
    }

    if (depth > 0) 
    {
        printf("info string Searching with max depth of %d\n", depth); 
    }
    else 
    {
        printf("info string Searching with no depth\n"); 
    }

    if (movetime > 0) 
    {
        time_ms = movetime; 
    }

    if (sidetime > 0) 
    {
        int inv_scale = (int) (10 - 0.05 * uci_game.ply); 
        if (inv_scale < 2) inv_scale = 2; 

        int total_time = sidetime - 1000; 
        if (total_time < 10) total_time = 10; 

        int tgt_time = (int) (total_time / inv_scale); 
        if (tgt_time > 0) 
        {
            time_ms = tgt_time; 
            printf("info string Using %dms out of %dms to think\n", time_ms, sidetime); 
            fflush(stdout); 
        }
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
    init_search_params(&params, &uci_game, depth, time_ms); 

    search(&engine, &params); 
    return true; 
}

bool uci_cmd_stop(void) 
{
    stop_search_thread(&engine); 
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
        if (uci_equals(token, "d")) return uci_cmd_print(); 
        if (uci_equals(token, "go")) return uci_cmd_go(); 
        if (uci_equals(token, "stop")) return uci_cmd_stop(); 
    }

    return false; 
}

int main(void) 
{
    printf("%s\n", ENGINE_NAME); 
    fflush(stdout); 

    // log = fopen("input.txt", "a"); 
    // fprintf(log, "NEW RUN\n"); 

    create_game_fen(&uci_game, START_FEN); 
    create_search_thread(&engine); 

    char input[UCI_MAX_INPUT]; 
    while (true) 
    {
        fflush(stdout); 
        fgets(input, UCI_MAX_INPUT, stdin); 
        // fprintf(log, "%s", input); 
        // fflush(log); 
        input[strlen(input) - 1] = '\0'; 
        if (!uci_parse(input)) printf("Unknown command: '%s'\n", input); 
    }

    // fclose(log); 

    destroy_game(&uci_game); 
    destroy_search_thread(&engine); 
    return 0; 
}