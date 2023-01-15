#pragma once 

#define CAS_WK 1 
#define CAS_WQ 2 
#define CAS_W 3 

#define CAS_BK 4 
#define CAS_BQ 8 
#define CAS_B 12 
        
#define CAS_ALL 15 
#define CAS_NONE 0 

typedef int castle; 

static const char *cas_str(castle cf) 
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