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
#include "square.h"
#include "vector.h" 

#define UCI_MAX_INPUT 4096

const char *uci_next_token(void) 
{
    return strtok(NULL, " \n"); 
}

bool uci_equals(const char *a, const char *b) 
{
    if (!a || !b) return false; 

    return strcmp(a, b) == 0; 
}

bool uci_command_uci(game *g) 
{
    printf("id name chess-engine 0.1\n"); 
    printf("id author Nicholas Hamilton\n"); 
    printf("uciok\n"); 
    return true; 
}

bool uci_command_isready(game *g) 
{
    printf("readyok\n"); 
    return true; 
}

bool uci_command_position(game *g) 
{
    vector moves; 
    CREATE_VEC(&moves, move); 

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

            printf("%s (%s) to %s\n", str_sq(from), str_pc(pc_at(g, from)), str_sq(to)); 

            // if promote is still pawn, then it wasn't promoted -> get the real original piece
            if (promote == PC_P) 
            {
                promote = get_no_col(pc_at(g, from)); 
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

bool uci_command_print(game *g) 
{
    print_game(g); 
    return true; 
}

bool uci_command_go(game *g) 
{
    const char *token; 
    int depth = -1; 

    while (token = uci_next_token()) 
    {
        if (uci_equals(token, "depth") && (token = uci_next_token())) 
        {
            depth = atoi(token); 
        }
    }

    if (depth <= 0) 
    {
        depth = 4; 
    }

    printf("info string Searching with max depth of %d\n", depth); 

    int eval = 0; 
    vector pv; 
    CREATE_VEC(&pv, move); 

    search(g, depth, &pv, &eval); 

    destroy_vec(&pv); 
    return true; 
}

bool uci_parse(game *g, const char *orig_cmd) 
{
    // remove spaces at the beginning
    while (*orig_cmd && *orig_cmd == ' ') orig_cmd++; 

    char cmd[UCI_MAX_INPUT]; 
    strcpy(cmd, orig_cmd); 

    char *token = strtok(cmd, " \n"); 
    if (token) 
    {
        if (uci_equals(token, "quit")) exit(0); 
        if (uci_equals(token, "uci")) return uci_command_uci(g); 
        if (uci_equals(token, "isready")) return uci_command_isready(g); 
        if (uci_equals(token, "position")) return uci_command_position(g); 
        if (uci_equals(token, "d")) return uci_command_print(g); 
        if (uci_equals(token, "go")) return uci_command_go(g); 
    }

    return false; 
}

int main(void) 
{
    printf("chess-engine 0.1\n"); 
    fflush(stdout); 

    game g; 
    create_game_fen(&g, START_FEN); 

    FILE *tmp = fopen("C:\\Users\\Nicholas\\Documents\\Code\\chess-engine\\build\\input.txt", "a"); 
    fprintf(tmp, "NEW RUN\n"); 

    char input[UCI_MAX_INPUT]; 
    while (true) 
    {
        fflush(stdout); 
        fgets(input, UCI_MAX_INPUT, stdin); 
        fprintf(tmp, "%s", input); 
        fflush(tmp); 
        input[strlen(input) - 1] = '\0'; 
        if (!uci_parse(&g, input)) printf("Unknown command: '%s'\n", input); 
    }

    fclose(tmp); 

    destroy_game(&g); 
    return 0; 
}