#include <stdio.h> 

#include "bitboard.h" 
#include "castle.h"
#include "game.h" 
#include "move.h"
#include "piece.h"
#include "random.h" 
#include "square.h"
#include "vector.h" 

int main(void) 
{
    game g; 
    game_create_fen(&g, GAME_STARTING_FEN); 

    game_print(&g); 

    game_destroy(&g); 
    return 0; 
}