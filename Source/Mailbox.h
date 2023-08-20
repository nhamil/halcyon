/**
 * @file Mailbox.h
 * @author Nicholas Hamilton 
 * @date 2023-02-20
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Defines mailbox, used for quick piece queries. 
 */

#pragma once 

#include <stdbool.h> 
#include <stdint.h> 
#include <stdio.h> 
#include <string.h> 

#include "Piece.h" 
#include "Square.h"
#include "Types.h" 

/**
 * Stores piece information for every square. 
 */
typedef struct Mailbox Mailbox; 

struct Mailbox 
{
    U64 Squares[4]; // 16 squares per U64 
};

/**
 * Initializes a mailbox to no pieces on the board. It does not need to be deinitialized. 
 * 
 * @param m The mailbox 
 */
static inline void InitMailbox(Mailbox* m) 
{
    memset(m, NoPiece << 4 | NoPiece, sizeof(Mailbox)); 
}

/**
 * Make a copy of another mailbox. 
 * 
 * @param m The mailbox 
 * @param from The mailbox to copy
 */
static inline void InitMailboxCopy(Mailbox* m, const Mailbox* from) 
{
    *m = *from; 
}

/**
 * Check if two mailboxes are the same. 
 * 
 * @param a 1st mailbox
 * @param b 2nd mailbox
 * @return True if equal, false otherise 
 */
static inline bool MailboxEquals(const Mailbox* a, const Mailbox* b) 
{
    if (a->Squares[0] != b->Squares[0]) return false; 
    if (a->Squares[1] != b->Squares[1]) return false; 
    if (a->Squares[2] != b->Squares[2]) return false; 
    if (a->Squares[3] != b->Squares[3]) return false; 
    return true; 
}

/**
 * Gets the piece found at the specified square. 
 * 
 * @param m The mailbox 
 * @param sq Target square 
 * @return Piece on the square. 
 */
static inline Piece PieceAt(const Mailbox* m, Square sq) 
{
    return (m->Squares[sq / 16] >> (sq & 0xF) * 4) & 0xF; 
}

/**
 * Checks if there is any piece on the specified square. 
 * 
 * @param m The mailbox 
 * @param sq Target square 
 * @return True if there is a piece, otherwise false 
 */
static inline bool IsOccupied(const Mailbox* m, Square sq) 
{
    return PieceAt(m, sq) != NoPiece; 
}

/**
 * Places a piece on the specified square. 
 * 
 * @param m The mailbox 
 * @param sq Target square 
 * @param pc Piece to place 
 */
static inline void SetPieceAt(Mailbox* m, Square sq, Piece pc) 
{
    // clear and set 
    m->Squares[sq / 16] &= ~(0xFULL << (sq & 0xF) * 4); 
    m->Squares[sq / 16] |= ((U64) pc) << (sq & 0xF) * 4; 
}

/**
 * Removes a piece from the specified square. 
 * 
 * @param m The mailbox 
 * @param sq Target square 
 */
static inline void ClearPieceAt(Mailbox* m, Square sq) 
{
    SetPieceAt(m, sq, NoPiece); 
}

/**
 * Prints a mailbox to `stdout`. 
 * 
 * @param m The mailbox 
 * @param end String to print after printing the mailbox
 */
static inline void PrintMailbox(const Mailbox* m) 
{
    for (int rank = 7; rank >= 0; rank--) 
    {
        for (int file = 0; file < 8; file++) 
        {
            Piece pc = PieceAt(m, MakeSquare(file, rank)); 
            if (pc != NoPiece) 
            {
                printf("%s ", PieceString(pc)); 
            }
            else 
            {
                printf("  "); 
            }
        }
        printf("\n"); 
    }
}
