/**
 * @file Game.h
 * @author Nicholas Hamilton 
 * @date 2023-01-12
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Handles a single instance of a chess game, with support for undoing moves 
 * and playing null moves. 
 */

#pragma once 

#include <inttypes.h>
#include <limits.h> 
#include <stdbool.h>
#include <stddef.h> 
#include <stdint.h>
#include <string.h> 

#include "Bitboard.h"
#include "Castle.h" 
#include "Magic.h" 
#include "Mailbox.h"
#include "Move.h" 
#include "Piece.h"
#include "Square.h"
#include "Vector.h"
#include "Zobrist.h"

/**
 * An instance of a game state. 
 */
typedef struct Game Game; 

/**
 * Stores previous board state that is not stored in moves. 
 */
typedef struct MoveHist MoveHist; 

/**
 * How deep to search for repeated positions.
 */
#define DrawDepth 4

/**
 * Maximum depth to search.
 */
#define MaxDepth 256 

/**
 * Maximum possible score.
 */
#define MaxScore (INT_MAX - 10000)

/**
 * Initial starting position. 
 */
#define StartFen "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

/**
 * Length guaranteed to fit a FEN output. 
 */
#define MaxFenLength 128 

struct MoveHist 
{
    int Halfmove; 
    Square EnPassant; 
    CastleFlags Castle; 
    bool InCheck;
    Zobrist Hash; 
};

struct Game 
{
    MoveHist Hist[MaxDepth + DrawDepth*2]; 

    Mailbox Board; 

    Bitboard Pieces[NumPieces + 1]; // extra bitboard for NoPiece
    Bitboard Colors[NumColors]; 
    Bitboard All; 
    Bitboard Movement; 
    int Counts[NumPieces + 1]; 

    Zobrist Hash; 
    CastleFlags Castle; 
    Square EnPassant; 

    int Ply; 
    int Halfmove; 
    Color Turn; 
    bool InCheck; 
    int Depth; 

    U64 Nodes; 
};

/**
 * Allocates and resets a new game. 
 * You should still call `LoadFen`. 
 * 
 * @return New game 
 */
Game* NewGame(void); 

/**
 * Deallocates a game state. 
 * 
 * @param g The game 
 */
void FreeGame(Game* g); 

/**
 * Resets a game to an empty board. 
 * This does not set the board to the initial position. 
 * 
 * @param g The game
 */
void ResetGame(Game* g); 

/**
 * Clones another game state. 
 * 
 * @param g The game 
 * @param from The game to make a copy of 
 */
void CopyGame(Game* g, const Game* from); 

/**
 * Checks for equality on the parts of game state that are relevant for the 
 * transposition table. 
 * 
 * @param a The game 
 * @param b Other game 
 * @return True if equal, false otherwise
 */
bool EqualsTTableGame(const Game* a, const Game* b); 

/**
 * Resets a game and loads a FEN board state. 
 * 
 * To set a game to the initial position, use `LoadFen(game, StartFen)`. 
 * 
 * @param g The game 
 * @param fen FEN to load 
 */
void LoadFen(Game* g, const char* fen); 

/**
 * Export the current game state to FEN. 
 * 
 * @param g The game 
 * @param out Buffer to write to
 */
void ToFen(const Game* g, char* out); 

/**
 * Makes one move. Does not check for legality. 
 * 
 * @param g The game 
 * @param m The move 
 */
void PushMove(Game* g, Move m); 

/**
 * Undoes one move. Does not check for legality or that it is the last move played. 
 * 
 * @param g The game 
 * @param m The move 
 */
void PopMove(Game* g, Move m); 

/**
 * Waits one turn. 
 * 
 * @param g 
 */
void PushNullMove(Game* g); 

/**
 * Undoes a waiting move. 
 * 
 * @param g 
 */
void PopNullMove(Game* g); 

/**
 * Prints the game state to a file. 
 * 
 * @param g The game 
 * @param out File to write to
 */
void FilePrintGame(const Game* g, FILE* out); 

/**
 * Prints the game to stdout. 
 * 
 * @param g The game 
 */
void PrintGame(const Game* g); 

/**
 * Runs perft on the current game state. 
 * 
 * @param g The game 
 * @param depth Depth to run perft 
 * @return Total number of leaf nodes
 */
U64 Perft(Game* g, int depth); 

/**
 * Checks for various draw conditions: 
 * - 50-move rule 
 * - repetition 
 * - insufficient material 
 * 
 * One notable exception is stalemate which is determined in `Evaluate`.
 * 
 * @param g The game 
 * @return True if there is a special type of draw, false otherwise
 */
bool IsSpecialDraw(const Game* g);

/**
 * Gets static evaluation for the current game state. 
 * 
 * @param g The game 
 * @param ply What ply from the position the search started in 
 * @param nMoves Number of available moves for the current player 
 * @param draw Is the game a draw 
 * @param contempt Contempt factor 
 * @param verbose Should eval be printed to stdout
 * @return Static evaluation
 */
int EvaluateVerbose(const Game* g, int ply, int nMoves, bool draw, int contempt, bool verbose); 

/**
 * Gets static evaluation for the current game state. 
 * 
 * @param g The game 
 * @param ply What ply from the position the search started in 
 * @param nMoves Number of available moves for the current player 
 * @param draw Is the game a draw 
 * @param contempt Contempt factor 
 * @return Static evaluation
 */
static inline int Evaluate(const Game* g, int ply, int nMoves, bool draw, int contempt) 
{
    return EvaluateVerbose(g, ply, nMoves, draw, contempt, false); 
} 

/**
 * Checks game state to make sure everything is consistent and correct. 
 * 
 * @param g The game 
 * @return True if valid, false otherwise 
 */
bool ValidateGame(const Game* g); 

#ifdef VALIDATION 
    /**
     * Checks that making a move doesn't cause errors. 
     * 
     * @param g The game 
     * @param mv The move 
     * @param action Type of move 
     */
    #define VALIDATE_GAME_MOVE(g, mv, action) \
        if (!ValidateGame(g)) \
        {\
            printf("info string ERROR validation of %s move ", action); \
            PrintMoveEnd(mv, " failed\n"); \
            exit(1); \
        }
#else 
    #define VALIDATE_GAME_MOVE(g, mv, action)
#endif 

/**
 * Checks if either side only has a king and 0+ pawns. 
 * 
 * @param g The game 
 * @return True if either side is king+pawns, false otherwise 
 */
static inline bool EitherSideKP(const Game* g) 
{
    bool wKP = g->Colors[ColorW] == (g->Pieces[PieceWK] | g->Pieces[PieceWP]); 
    bool bKP = g->Colors[ColorB] == (g->Pieces[PieceBK] | g->Pieces[PieceBP]); 
    return wKP | bKP;  
}

/**
 * Removes move history to increase max depth. 
 * Do not use PopMove on previous moves after calling this. 
 * 
 * @param g The game
 */
static inline void ClearDepth(Game* g) 
{
    int copyAmt = DrawDepth * 2; 
    if (g->Depth < copyAmt) copyAmt = g->Depth; 

    if (copyAmt > 0) 
    {
        int copyStart = g->Depth - copyAmt; 

        for (int i = 0; i < copyAmt; i++) 
        {
            g->Hist[i] = g->Hist[copyStart + i]; 
        }
    }

    g->Depth = copyAmt; 
}

/**
 * Gets all valid squares for a rook-style sliding piece. 
 * This includes the first occupant in each direction even if it's the same color. 
 * 
 * @param sq Current square of the piece 
 * @param occ All pieces on the board 
 * @return Bitboard containing all valid squares to move to
 */
static inline Bitboard RAttacks(Square sq, Bitboard occ) 
{
    return MagicRSlide[sq][((MagicRMask[sq] & occ) * MagicR[sq]) >> MagicRShift[sq]];
}

/**
 * Gets all valid squares for a bishop-style sliding piece. 
 * This includes the first occupant in each direction even if it's the same color. 
 * 
 * @param sq Current square of the piece 
 * @param occ All pieces on the board 
 * @return Bitboard containing all valid squares to move to
 */
static inline Bitboard BAttacks(Square sq, Bitboard occ) 
{
    return MagicBSlide[sq][((MagicBMask[sq] & occ) * MagicB[sq]) >> MagicBShift[sq]];
}

/**
 * Gets a measure of how threatened an area of the board is by the other color. 
 * 
 * @param g The game 
 * @param region The region of the board to check 
 * @param chkCol Friendly color 
 * @return How strongly the region is threatened weighted by piece type
 */
static inline int GetAttackUnits(const Game* g, Bitboard region, Color chkCol) 
{
    Color col = chkCol; 
    Color opp = OppositeColor(col); 

    Bitboard occ = g->All; 

    int units = 0; 

    Bitboard pawns = g->Pieces[MakePiece(PieceP, opp)];  
    if (opp == ColorW) 
    {
        pawns = ShiftNW(pawns) | ShiftNE(pawns); 
    }
    else 
    {
        pawns = ShiftSW(pawns) | ShiftSE(pawns); 
    }
    units += PopCount(pawns & region); 

    FOR_EACH_BIT(g->Pieces[MakePiece(PieceN, opp)], 
    {
        units += 2 * PopCount(MovesN[sq] & region);
    });

    FOR_EACH_BIT(g->Pieces[MakePiece(PieceB, opp)], 
    {
        units += 2 * PopCount(BAttacks(sq, occ) & region); 
    });

    FOR_EACH_BIT(g->Pieces[MakePiece(PieceR, opp)], 
    {
        units += 3 * PopCount(RAttacks(sq, occ) & region);
    });

    FOR_EACH_BIT(g->Pieces[MakePiece(PieceQ, opp)], 
    {
        units += 5 * PopCount((BAttacks(sq, occ) | RAttacks(sq, occ)) & region);
    });

    return units; 
}

/**
 * Finds all enemy pieces that attack a square. 
 * 
 * @param g The game 
 * @param sq Square to find attackers of 
 * @param chkCol Friendly color 
 * @return Bitboard highlighting all enemy attackers 
 */
static inline Bitboard GetAttackers(const Game* g, Square sq, Color chkCol) 
{
    Color col = chkCol; 
    Color opp = OppositeColor(col); 

    Bitboard occ = g->All; 

    Bitboard oppP = g->Pieces[MakePiece(PieceP, opp)]; 
    Bitboard oppN = g->Pieces[MakePiece(PieceN, opp)]; 
    Bitboard oppB = g->Pieces[MakePiece(PieceB, opp)]; 
    Bitboard oppR = g->Pieces[MakePiece(PieceR, opp)]; 
    Bitboard oppQ = g->Pieces[MakePiece(PieceQ, opp)]; 
    Bitboard oppK = g->Pieces[MakePiece(PieceK, opp)]; 

    Bitboard oppRQ = (oppR | oppQ); 
    Bitboard oppBQ = (oppB | oppQ); 

    Bitboard chk = (RAttacks(sq, occ) & oppRQ) 
               | (BAttacks(sq, occ) & oppBQ) 
               | (oppK & MovesK[sq]) 
               | (oppN & MovesN[sq]) 
               | (oppP & AttacksP[col][sq]);     

    return chk; 
}

/**
 * Determines if there are any attackers of the square. 
 * 
 * @param g The game 
 * @param sq Square to find attackers of 
 * @param chkCol Friendly color
 * @return True if the square is attacked, false otherwise 
 */
static inline bool IsAttacked(const Game* g, Square sq, Color chkCol) 
{
    return GetAttackers(g, sq, chkCol) != 0; 
}

/**
 * Bonus based on a passed pawn's rank. 
 */
extern int PassedPawnValues[]; 

/**
 * Bonus for connected rooks. 
 */
extern int ConnectedRooks; 

/**
 * Bonus for controlling an open file. 
 */
extern int OpenFile; 

/**
 * Penalty for enemy pieces that can attack the area around the king. 
 */
extern int AttackUnitValues[64]; 

/**
 * Bonus for various pawn structures. 
 */
extern int PawnStructureValues[4]; 

/**
 * Piece-square tables: [phase][piece][square]. 
 * For readability these are from black's perspective. 
 * White's square ranks must be flipped. 
 */
extern int PieceSquare[2][NumPieceTypes][NumSquares]; 

/**
 * Does not include king as this should not be parameterized. 
 */
extern int PieceTypeValues[5]; 

/**
 * Bonus for having the bishop pair. 
 */
extern int BishopPair; 

/**
 * Maximum length of a parameter name. 
 */
#define ParamNameLength 512

/**
 * Gets a pointer to a tunable parameter. 
 * 
 * @param index Parameter index
 * @param name Optional buffer to store the parameter name and relative index 
 * @return Pointer to the parameter 
 */
int* GetEvalParam(int index, char* outName); 

/**
 * @return Total number of tunable parameters 
 */
int GetNumEvalParams(void); 
