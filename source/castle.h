#pragma once 

#define CASTLE_WK 1 
#define CASTLE_WQ 2 
#define CASTLE_W 3 

#define CASTLE_BK 4 
#define CASTLE_BQ 8 
#define CASTLE_B 12 

#define CASTLE_ALL 15 
#define CASTLE_NONE 0 

typedef int castle_flags; 

static const char *castle_string(castle_flags cf) 
{
    static const char *STR[] = 
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
    return STR[cf]; 
}