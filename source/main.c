#include <stdatomic.h> 
#include <stdbool.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <pthread.h> 

#include "bboard.h" 
#include "castle.h"
#include "game.h" 
#include "move.h"
#include "piece.h"
#include "random.h" 
#include "square.h"
#include "vector.h" 

#define UCI_MAX_INPUT 4096

const char *uci_next(void) 
{
    return strtok(NULL, " \n"); 
}

bool uci_eq(const char *a, const char *b) 
{
    if (!a || !b) return false; 

    return strcmp(a, b) == 0; 
}

bool uci_cmd_uci(game *g) 
{
    (void) g; 
    printf("id name chess-engine 0.1\n"); 
    printf("id author Nicholas Hamilton\n"); 
    printf("uciok\n"); 
    return true; 
}

bool uci_cmd_isready(game *g) 
{
    (void) g; 
    printf("readyok\n"); 
    return true; 
}

bool uci_cmd_position(game *g) 
{
    vector moves;  
    CREATE_VEC(&moves, move); 

    const char *tok = uci_next(); 

    bool success = true; 

    if (uci_eq(tok, "startpos")) 
    {
        load_fen_game(g, START_FEN); 
        tok = uci_next(); 
    }
    else if (uci_eq(tok, "fen")) 
    {
        // need to recreate FEN from tok list 
        char fen[UCI_MAX_INPUT]; 
        fen[0] = '\0'; 
        while ((tok = uci_next()) && !uci_eq(tok, "moves")) 
        {
            strcpy(fen + strlen(fen), tok); 
            size_t len = strlen(fen); 
            fen[len] = ' '; 
            fen[len + 1] = '\0'; 
        }
        // tok is NULL or "moves" at this point 

        load_fen_game(g, fen); 
    }

    if (uci_eq(tok, "moves")) 
    {
        while ((tok = uci_next())) 
        {
            clear_vec(&moves); 
            gen_mvs(g, &moves); 

            piece pro = PC_P; 

            if (strlen(tok) == 4) 
            {
                // from -> to no promotion 
                if (!(tok[0] >= 'a' && tok[0] <= 'h' && tok[2] >= 'a' && tok[2] <= 'h' && tok[1] >= '1' && tok[1] <= '8' && tok[3] >= '1' && tok[3] <= '8')) 
                {
                    success = false; goto done; 
                }
            }
            else if (strlen(tok) == 5) 
            {
                // from -> to with promotion 
                if (!(tok[0] >= 'a' && tok[0] <= 'h' && tok[2] >= 'a' && tok[2] <= 'h' && tok[1] >= '1' && tok[1] <= '8' && tok[3] >= '1' && tok[3] <= '8' && (tok[4] == 'q' || tok[4] == 'r' || tok[4] == 'b' || tok[4] == 'n'))) 
                {
                    success = false; goto done; 
                }
                else 
                {
                    if (tok[4] == 'q') pro = PC_Q; 
                    if (tok[4] == 'r') pro = PC_R; 
                    if (tok[4] == 'b') pro = PC_B; 
                    if (tok[4] == 'n') pro = PC_N; 
                }
            }
            else 
            {
                success = false; goto done; 
            }

            square from = make_sq(tok[0] - 'a', tok[1] - '1'); 
            square to = make_sq(tok[2] - 'a', tok[3] - '1'); 

            printf("%s (%s) to %s\n", sq_str(from), pc_str(pc_at(g, from)), sq_str(to)); 

            // if pro is still pawn, then it wasn't promoted -> get the real original piece
            if (pro == PC_P) 
            {
                pro = no_col_pc(pc_at(g, from)); 
            }

            pro = make_pc(pro, g->turn); 

            printf("Piece: %s\n", pc_str(pro)); 

            // make sure the complete move is legal 
            bool found = false; 
            for (size_t i = 0; i < moves.size; i++) 
            {
                move m = AT_VEC(&moves, move, i); 
                print_mv(m); 
                if (from_sq(m) == from && to_sq(m) == to && promoted(m) == pro) 
                {
                    found = true; 
                    push_mv(g, m); 
                }
            }

            // keep previous legal moves, only ignore remaining 
            if (!found) 
            {
                printf("Could not find move '%s'\n", tok); 
                goto done; 
            }
        }
    }

done: 
    if (!success) 
    {
        load_fen_game(g, START_FEN); 
    }

    destroy_vec(&moves); 
    return success; 
}

bool uci_cmd_print(game *g) 
{
    print_game(g); 
    return true; 
}

bool uci_cmd_go(game *g) 
{
    const char *tok; 
    int depth = -1; 

    while ((tok = uci_next())) 
    {
        if (uci_eq(tok, "depth") && (tok = uci_next())) 
        {
            depth = atoi(tok); 
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

    char *tok = strtok(cmd, " \n"); 
    if (tok) 
    {
        if (uci_eq(tok, "quit")) exit(0); 
        if (uci_eq(tok, "uci")) return uci_cmd_uci(g); 
        if (uci_eq(tok, "isready")) return uci_cmd_isready(g); 
        if (uci_eq(tok, "position")) return uci_cmd_position(g); 
        if (uci_eq(tok, "d")) return uci_cmd_print(g); 
        if (uci_eq(tok, "go")) return uci_cmd_go(g); 
    }

    return false; 
}

int main(void) 
{
    printf("chess-engine 0.1\n"); 
    fflush(stdout); 

    game g; 
    create_fen_game(&g, START_FEN); 

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