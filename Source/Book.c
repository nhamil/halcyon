/**
 * @file Book.c
 * @author Nicholas Hamilton 
 * @date 2023-01-28
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Implementation of Book
 */

#include "Book.h"

#include <stdio.h> 
#include <stdlib.h> 

#include "Zobrist.h"

void CreateBook(Book* b) 
{
    CREATE_VEC(&b->Entries, BookPos); 
    b->NumGames = 0; 
    b->NumWhite = 0; 
    b->NumDraw = 0; 
    b->NumBlack = 0; 
}

void DestroyBook(Book* b) 
{
    DestroyVec(&b->Entries); 
}

static inline U64 ReadU64(FILE* file) 
{
    U64 out = 0; 
    for (int i = 0; i < 8; i++) 
    {
        out |= ((U64) (getc(file) & 255)) << (i*8); 
    }
    return out; 
}

static inline void WriteU64(FILE* file, U64 value) 
{
    for (int i = 0; i < 8; i++) 
    {
        putc(value & 255, file); 
        value >>= 8; 
    }
}

void CreateBookFromFile(Book* b, const char* filename) 
{
    CreateBook(b); 
    FILE* file = fopen(filename, "rb"); 

    U64 seed = ReadU64(file); 
    if (seed != ZB_SEED) 
    {
        printf("Book uses wrong seed: %s (%016" PRIx64 "\n", filename, seed); 
        goto cleanup; 
    }

    U64 total = ReadU64(file); 
    printf("Loading %" PRIu64 " Entries\n", total); 

    ReserveVec(&b->Entries, total); 
    b->Entries.Size = total; 

    BookPos* pos = (BookPos*) b->Entries.Data; 

    for (U64 i = 0; i < total; i++) 
    {
        pos[i].Hash = ReadU64(file); 
        pos[i].NumWhite = ReadU64(file); 
        pos[i].NumDraw = ReadU64(file); 
        pos[i].NumBlack = ReadU64(file); 
        pos[i].NumGames = pos[i].NumWhite + pos[i].NumDraw + pos[i].NumBlack; 

        b->NumWhite += pos[i].NumWhite; 
        b->NumDraw += pos[i].NumDraw; 
        b->NumBlack += pos[i].NumBlack; 
        b->NumGames += pos[i].NumGames; 
    }

cleanup: 
    fclose(file); 
}

void SaveBook(const Book* b, const char* filename, U64 min) 
{
    FILE* file = fopen(filename, "wb"); 

    U64 total = 0; 
    for (U64 i = 0; i < b->Entries.Size; i++) 
    {
        if (AT_VEC_CONST(&b->Entries, BookPos, i).NumGames >= min) 
        {
            total++; 
        }
    }

    WriteU64(file, ZB_SEED); 

    printf("Saving %" PRIu64 " Entries\n", total); 
    WriteU64(file, total); 
    for (U64 i = 0; i < b->Entries.Size; i++) 
    {
        BookPos pos = AT_VEC_CONST(&b->Entries, BookPos, i); 
        if (pos.NumGames >= min) 
        {
            WriteU64(file, pos.Hash); 
            WriteU64(file, pos.NumWhite); 
            WriteU64(file, pos.NumDraw); 
            WriteU64(file, pos.NumBlack); 
        }
    }

    fclose(file); 
}

static S64 FindBookIdx(const Book* b, Zobrist key, bool insert) 
{
    S64 lo = 0; 
    S64 hi = b->Entries.Size; 
    S64 pos; 

    while (lo <= hi) 
    {
        pos = lo + (hi - lo) / 2; 

        Zobrist val = AT_VEC_CONST(&b->Entries, Zobrist, pos); 
        
        if (key == val) 
        {
            return pos; 
        }
        else if (key < val) 
        {
            hi = pos - 1; 
        }
        else 
        {
            lo = pos + 1; 
        }
    }

    if (insert) 
    {
        if (lo >= (S64) b->Entries.Size) 
        {
            return b->Entries.Size; 
        }

        Zobrist val = AT_VEC_CONST(&b->Entries, Zobrist, lo); 
        if (key > val) 
        {
            return lo + 1; 
        }
        else 
        {
            return lo; 
        }
    }

    return -1; 
}

const BookPos* FindBookPos(const Book* b, Zobrist key) 
{
    S64 idx = FindBookIdx(b, key, false); 

    if (idx >= 0) 
    {
        return AtVecConst(&b->Entries, idx); 
    }
    else 
    {
        return NULL; 
    }
}

void AddBookPos(Book* b, Zobrist key, int white, int draw, int black) 
{
    BookPos* pos = (BookPos*) FindBookPos(b, key); 

    if (!pos) 
    {
        BookPos tmp = {0}; 
        tmp.Hash = key; 
        S64 idx = FindBookIdx(b, key, true); 
        // printf("Could not find position: "); 
        // PrintZb(key); 
        // printf("Inserting at %d\n", (int) idx); 
        INSERT_VEC(&b->Entries, BookPos, (S64) idx, tmp); 
        pos = (BookPos*) FindBookPos(b, key); 
    }

    if (!pos) 
    {
        printf("Could not find position "); 
        PrintZbEnd(key, " even though it should have been allocated\n"); 
        fflush(stdout); 
        exit(1); 
    }

    int total = white + draw + black; 

    b->NumGames += total; 
    pos->NumGames += total; 

    b->NumWhite += white; 
    pos->NumWhite += white; 

    b->NumDraw += draw; 
    pos->NumDraw += draw; 

    b->NumBlack += black; 
    pos->NumBlack += black; 
}

void PrintBook(const Book* b, U64 min) 
{
    U64 idx = 1; 
    for (U64 i = 0; i < b->Entries.Size; i++) 
    {
        BookPos p = AT_VEC_CONST(&b->Entries, BookPos, i); 
        
        if (p.NumGames < min) 
        {
            continue; 
        }

        printf("%zu. ", idx++); 
        PrintZbEnd(p.Hash, " "); 
        printf("%d/%d/%d (%d)\n", (int) p.NumWhite, (int) p.NumDraw, (int) p.NumBlack, (int) p.NumGames); 
    }
}