#pragma once 

#include <stdbool.h>
#include <stdint.h> 

#include "Move.h" 
#include "Types.h" 
#include "Vector.h"
#include "Zobrist.h" 

typedef struct Book Book; 
typedef struct BookPos BookPos; 

struct Book 
{
    Vector Entries; // BookPos[]
    U64 NumGames; 
    U64 NumWhite; 
    U64 NumDraw; 
    U64 NumBlack; 
};

struct BookPos 
{
    Zobrist Hash; 
    U64 NumGames; 
    U64 NumWhite; 
    U64 NumDraw; 
    U64 NumBlack; 
};

void CreateBook(Book* b); 

void CreateBookFromFile(Book* b, const char* file); 

void DestroyBook(Book* b);  

void SaveBook(const Book* b, const char* file, U64 minGames); 

void AddBookPos(Book* b, Zobrist key, int white, int draw, int black); 

const BookPos* FindBookPos(const Book* b, Zobrist key); 

void PrintBook(const Book* b, U64 min); 
