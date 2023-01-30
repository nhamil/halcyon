#include <stdio.h> 
#include <stdlib.h> 

#include "book.h"

int main(int argc, char **argv) 
{
    if (argc != 3) 
    {
        printf("Usage: openlist <file> <min_games>\n"); 
        return 1; 
    }

    book b; 
    create_book_file(&b, argv[1]); 

    int min = atoi(argv[2]); 
    print_book(&b, min); 

    destroy_book(&b); 
}