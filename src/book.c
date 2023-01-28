#include "book.h"

#include <stdio.h> 
#include <stdlib.h> 

void create_book(book *b) 
{
    CREATE_VEC(&b->entries, book_pos); 
    b->n_games = 0; 
    b->n_white = 0; 
    b->n_draw = 0; 
    b->n_black = 0; 
}

void destroy_book(book *b) 
{
    destroy_vec(&b->entries); 
}

static inline uint64_t read_u64(FILE *file) 
{
    uint64_t out = 0; 
    for (int i = 0; i < 16; i++) 
    {
        out |= ((uint64_t) (getc(file) & 255)) << (i*8); 
    }
    return out; 
}

static inline void write_u64(FILE *file, uint64_t value) 
{
    for (int i = 0; i < 16; i++) 
    {
        putc(value & 255, file); 
        value >>= 8; 
    }
}

void create_book_file(book *b, const char *filename) 
{
    create_book(b); 
    FILE *file = fopen(filename, "rb"); 

    size_t total = read_u64(file); 
    printf("Loading %"PRIu64" entries\n", total); 

    reserve_vec(&b->entries, total); 
    b->entries.size = total; 

    book_pos *pos = (book_pos *) b->entries.data; 

    for (size_t i = 0; i < total; i++) 
    {
        pos[i].hash = read_u64(file); 
        pos[i].n_white = read_u64(file); 
        pos[i].n_draw = read_u64(file); 
        pos[i].n_black = read_u64(file); 
        pos[i].n_games = pos[i].n_white + pos[i].n_draw + pos[i].n_black; 

        b->n_white += pos[i].n_white; 
        b->n_draw += pos[i].n_draw; 
        b->n_black += pos[i].n_black; 
        b->n_games += pos[i].n_games; 
    }

    fclose(file); 
}

void save_book(const book *b, const char *filename, uint64_t min) 
{
    FILE *file = fopen(filename, "wb"); 

    size_t total = 0; 
    for (size_t i = 0; i < b->entries.size; i++) 
    {
        if (AT_VEC_CONST(&b->entries, book_pos, i).n_games >= min) 
        {
            total++; 
        }
    }

    printf("Saving %"PRIu64" entries\n", total); 
    write_u64(file, total); 
    for (size_t i = 0; i < b->entries.size; i++) 
    {
        book_pos pos = AT_VEC_CONST(&b->entries, book_pos, i); 
        if (pos.n_games >= min) 
        {
            write_u64(file, pos.hash); 
            write_u64(file, pos.n_white); 
            write_u64(file, pos.n_draw); 
            write_u64(file, pos.n_black); 
        }
    }

    fclose(file); 
}

static ssize_t find_book_idx(const book *b, zobrist key, bool insert) 
{
    ssize_t lo = 0; 
    ssize_t hi = b->entries.size; 
    ssize_t pos; 

    while (lo <= hi) 
    {
        pos = lo + (hi - lo) / 2; 

        zobrist val = AT_VEC_CONST(&b->entries, zobrist, (size_t) pos); 
        
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
        if (lo >= (ssize_t) b->entries.size) 
        {
            return b->entries.size; 
        }

        zobrist val = AT_VEC_CONST(&b->entries, zobrist, (size_t) lo); 
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

const book_pos *find_book_pos(const book *b, zobrist key) 
{
    ssize_t idx = find_book_idx(b, key, false); 

    if (idx >= 0) 
    {
        return at_vec_const(&b->entries, idx); 
    }
    else 
    {
        return NULL; 
    }
}

void add_book_pos(book *b, zobrist key, int white, int draw, int black) 
{
    book_pos *pos = (book_pos *) find_book_pos(b, key); 

    if (!pos) 
    {
        book_pos tmp = {0}; 
        tmp.hash = key; 
        ssize_t idx = find_book_idx(b, key, true); 
        // printf("Could not find position: "); 
        // print_zb(key); 
        // printf("Inserting at %d\n", (int) idx); 
        INSERT_VEC(&b->entries, book_pos, (ssize_t) idx, tmp); 
        pos = (book_pos *) find_book_pos(b, key); 
    }

    if (!pos) 
    {
        printf("Could not find position "); 
        print_zb_end(key, " even though it should have been allocated\n"); 
        fflush(stdout); 
        exit(1); 
    }

    int total = white + draw + black; 

    b->n_games += total; 
    pos->n_games += total; 

    b->n_white += white; 
    pos->n_white += white; 

    b->n_draw += draw; 
    pos->n_draw += draw; 

    b->n_black += black; 
    pos->n_black += black; 
}

void print_book(const book *b, uint64_t min) 
{
    size_t idx = 1; 
    for (size_t i = 0; i < b->entries.size; i++) 
    {
        book_pos p = AT_VEC_CONST(&b->entries, book_pos, i); 
        
        if (p.n_games < min) 
        {
            continue; 
        }

        printf("%zu. ", idx++); 
        print_zb_end(p.hash, " "); 
        printf("%d/%d/%d (%d)\n", (int) p.n_white, (int) p.n_draw, (int) p.n_black, (int) p.n_games); 
    }
}