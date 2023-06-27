#pragma once 

#define CASTLE_WK 1 
#define CASTLE_WQ 2 
#define CASTLE_W 3 

#define CASTLE_BK 4 
#define CASTLE_BQ 8 
#define CASTLE_B 12 

#define CASTLE_ALL 15 
#define CASTLE_NONE 0 

typedef int CastleFlags; 

static const char* StrCastle(CastleFlags cf) 
{
    static const char* Str[] = 
    {
        "(none)", 
        "K", 
        "Q", 
        "KQ", 
        "k", 
        "Kk", 
        "Qk", 
        "KQk", 
        "q", 
        "Kq", 
        "Qq", 
        "KQq", 
        "kq", 
        "Kkq", 
        "Qkq", 
        "KQkq", 
    };
    return Str[cf]; 
}