#include <ctype.h>
#include <stdio.h> 
#include <string.h> 

#include "book.h"
#include "game.h" 
#include "vector.h" 

#define RES_NONE 2 
#define RES_DRAW 0
#define RES_W 1 
#define RES_B -1 

typedef struct book_tree book_tree; 

struct book_tree 
{
    zobrist hash; 
    book_tree *left; 
    book_tree *right; 
    uint64_t n_games; 
    uint64_t n_white; 
    uint64_t n_draw; 
    uint64_t n_black; 
};

book_tree *new_tree(zobrist key) 
{
    book_tree *t = calloc(1, sizeof(book_tree)); 
    t->hash = key; 
    return t;   
}

void free_tree(book_tree *t) 
{
    if (t->left) 
    {
        free_tree(t->left); 
    }
    if (t->right) 
    {
        free_tree(t->right); 
    }
    free(t); 
}

void add_tree_node_result(book_tree *t, int score) 
{
    t->n_games++; 
    if (score > 0) 
    {
        t->n_white++; 
    }
    else if (score == 0) 
    {
        t->n_draw++; 
    }
    else 
    {
        t->n_black++; 
    }
}

void add_tree_result(book_tree *t, zobrist key, int score) 
{
    if (key == t->hash) 
    {
        add_tree_node_result(t, score); 
    }
    else 
    {
        if (key < t->hash) // left
        {
            if (t->left) 
            {
                add_tree_result(t->left, key, score); 
            }
            else 
            {
                t->left = new_tree(key); 
                add_tree_node_result(t->left, score); 
            }
        }
        else // right
        {
            if (t->right) 
            {
                add_tree_result(t->right, key, score); 
            }
            else 
            {
                t->right = new_tree(key); 
                add_tree_node_result(t->right, score); 
            }
        }
    }
}

void add_tree_to_book(const book_tree *t, book *b) 
{
    if (t->left) add_tree_to_book(t->left, b); 
    add_book_pos(b, t->hash, t->n_white, t->n_draw, t->n_black); 
    if (t->right) add_tree_to_book(t->right, b); 
}

size_t add_pgns(book_tree *bt, const char *filename) 
{
    FILE *file = fopen(filename, "r"); 
    if (!file) 
    {
        printf("Could not open %s\n", filename); 
        return 0; 
    }

    char buf[2049]; 
    game g; 
    vector moves; 
    size_t pgns = 0, added = 0; 
    int result = RES_NONE; 
    bool in_game = false; 
    printf("Adding PGNs from %s\n", filename); 

    CREATE_VEC(&moves, move); 
    create_game(&g); 

    while ((fscanf(file, "%2048s", buf)) != EOF) 
    {
        // printf("\" %s \"\n", buf); 
        if (strcmp("1.", buf) == 0) 
        {
            pgns++; 

            if (result != RES_NONE) // make sure game has result 
            {
                added++; 
                // printf("Game %zu result is %d\n", pgns, result); 

                for (size_t i = 0; i < 16; i++) 
                {
                    if (i < g.hist.size) 
                    {
                        zobrist hash = AT_VEC(&g.hist, game, i).hash; 
                        add_tree_result(bt, hash, result); 
                    }
                }
            }
            else 
            {
                // printf("Unknown termination for game %zu\n", pgns); 
            }

            // if (pgns - 1 >= 5000) 
            // {
            //     pgns--; 
            //     result = RES_NONE; 
            //     goto cleanup; 
            // }

            printf("Importing game %zu                  \r", pgns); 
            load_fen(&g, START_FEN); 
            in_game = true; 
            result = RES_NONE; 
        }
        else if (in_game && strlen(buf) >= 1)
        {
            if (strcmp("1-0", buf) == 0) 
            {
                // printf("White wins "); 
                in_game = false; 
                result = RES_W; 
            }
            else if (strcmp("0-1", buf) == 0) 
            {
                // printf("Black wins "); 
                in_game = false; 
                result = RES_B; 
            }
            else if (strcmp("1/2-1/2", buf) == 0) 
            {
                // printf("Drawn game "); 
                in_game = false; 
                result = RES_DRAW; 
            }
            else if (isdigit(*buf)) 
            {
                // ignore "#."
            }
            else if (*buf == '*') 
            {
                // unknown termination, ignore 
                in_game = false; 
            }
            else 
            {
                move mv = alg_to_move(&g, buf); 
                if (mv == NO_MOVE) 
                {
                    printf("ERROR Illegal move: '%s'\n", buf); 
                    goto cleanup; 
                }
                else 
                {
                    // printf("%s ", buf); 
                    // print_move_end(mv, " "); 
                    push_move(&g, mv); 
                }
            }
        }
    }

cleanup: 
    printf("Imported %zu out of %zu PGNs  \n", added, pgns); 
    destroy_vec(&moves); 
    destroy_game(&g); 
    fclose(file); 
    return true; 
}

int main(int argc, char **argv) 
{
    if (argc < 4) 
    {
        printf("Usage: opengen <out_file> <min_games> <pgn_file>...\n"); 
        return 1; 
    }

    const char *out_file = argv[1]; 
    const int N = atoi(argv[2]); 

    size_t pgns = 0; 

    book_tree *bt = new_tree(0); 

    for (int i = 3; i < argc; i++) 
    {
        pgns += add_pgns(bt, argv[i]); 
    }

    if (pgns == 0) 
    {
        printf("Could not find any PGNs\n"); 
        return 1; 
    }
    else 
    {
        printf("Building book\n"); 
        book b; 
        create_book(&b); 
        add_tree_to_book(bt, &b); 
        printf("Done!\n"); 
        // print_book(&b, N); 

        printf("Saving positions with %d+ games to %s\n", N, out_file); 
        save_book(&b, out_file, N); 
        printf("Done!\n"); 

        // book b2; 
        // create_book_file(&b2, "book.hc"); 
        // print_book(&b2, N); 
        // destroy_book(&b2); 

        destroy_book(&b); 
    }

    free_tree(bt); 

    return 0; 
}