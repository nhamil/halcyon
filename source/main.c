#include <stdbool.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 

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

bool uci_command_position(game *g) 
{
    vector moves; 
    VECTOR_CREATE_TYPE(&moves, move); 

    const char *token = uci_next_token(); 

    bool success = true; 

    if (uci_equals(token, "startpos")) 
    {
        game_load_fen(g, GAME_STARTING_FEN); 
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

        game_load_fen(g, fen); 
    }

    if (uci_equals(token, "moves")) 
    {
        while (token = uci_next_token()) 
        {
            vector_clear(&moves); 
            game_generate_moves(g, &moves); 

            piece promote = PIECE_P; 

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
                    if (token[4] == 'q') promote = PIECE_Q; 
                    if (token[4] == 'r') promote = PIECE_R; 
                    if (token[4] == 'b') promote = PIECE_B; 
                    if (token[4] == 'n') promote = PIECE_N; 
                }
            }
            else 
            {
                success = false; goto done; 
            }

            square from = square_make(token[0] - 'a', token[1] - '1'); 
            square to = square_make(token[2] - 'a', token[3] - '1'); 

            // if promote is still pawn, then it wasn't promoted -> get the real original piece
            if (promote == PIECE_P) 
            {
                promote == piece_get_colorless(game_piece_at(g, from)); 
            }

            promote = piece_make_colored(promote, g->turn); 

            // make sure the complete move is legal 
            bool found = false; 
            for (size_t i = 0; i < moves.size; i++) 
            {
                move m = VECTOR_AT_TYPE(&moves, move, i); 
                if (move_get_from_square(m) == from && move_get_to_square(m) == to && move_get_promotion_piece(m) == promote) 
                {
                    found = true; 
                    game_push_move(g, m); 
                }
            }

            // keep previous legal moves, only ignore remaining 
            if (!found) goto done; 
        }
    }

done: 
    if (!success) 
    {
        game_load_fen(g, GAME_STARTING_FEN); 
    }

    vector_destroy(&moves); 
    return success; 
}

bool uci_command_print(game *g) 
{
    game_print(g); 
    return true; 
}

bool uci_command_go(game *g) 
{
    return false; 
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
    game_create_fen(&g, GAME_STARTING_FEN); 

    char input[UCI_MAX_INPUT]; 
    while (true) 
    {
        fflush(stdout); 
        fgets(input, UCI_MAX_INPUT, stdin); 
        input[strlen(input) - 1] = '\0'; 
        if (!uci_parse(&g, input)) printf("Unknown command: '%s'\n", input); 
    }

    game_destroy(&g); 
    return 0; 
}