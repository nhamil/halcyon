#pragma once 

#include <stdbool.h>
#include <stdint.h> 

#include "move.h" 
#include "vector.h"
#include "zobrist.h" 

typedef struct book book; 
typedef struct book_pos book_pos; 

struct book 
{
    vector entries; // book_pos[]
    uint64_t n_games; 
    uint64_t n_white; 
    uint64_t n_draw; 
    uint64_t n_black; 
};

struct book_pos 
{
    zobrist hash; 
    uint64_t n_games; 
    uint64_t n_white; 
    uint64_t n_draw; 
    uint64_t n_black; 
};

void create_book(book *b); 

void create_book_file(book *b, const char *file); 

void destroy_book(book *b);  

void save_book(const book *b, const char *file, uint64_t min_games); 

void add_book_pos(book *b, zobrist key, int white, int draw, int black); 

const book_pos *find_book_pos(const book *b, zobrist key); 

void print_book(const book *b, uint64_t min); 
