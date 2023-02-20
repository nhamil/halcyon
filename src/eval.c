#include "game.h" 

int PC_SQ[2][6][64] = 
{
    {
        { // pawn
            0,   0,   0,   0,   0,   0,   0,   0, 
            50,  50,  50,  50,  50,  50,  50,  50, 
            10,  10,  20,  30,  30,  20,  10,  10, 
            5,   5,  10,  25,  25,  10,   5,   5, 
            0,   0,   0,  20,  20,   0,   0,   0, 
            5,  -5, -10,   0,   0, -10,  -5,   5, 
            5,  10,  10, -20, -20,  10,  10,   5, 
            0,   0,   0,   0,   0,   0,   0,   0
        }, 
        { // knight 
            -50, -40, -30, -30, -30, -30, -40, -50, 
            -40, -20,   0,   0,   0,   0, -20, -40, 
            -30,   0,  10,  15,  15,  10,   0, -30, 
            -30,   5,  15,  20,  20,  15,   5, -30, 
            -30,   0,  15,  20,  20,  15,   0, -30, 
            -30,   5,  10,  15,  15,  10,   5, -30, 
            -40, -20,   0,   5,   5,   0, -20, -40, 
            -50, -40, -30, -30, -30, -30, -40, -50 
        }, 
        { // bishop 
            -20, -10, -10, -10, -10, -10, -10, -20, 
            -10,   0,   0,   0,   0,   0,   0, -10, 
            -10,   0,   5,  10,  10,   5,   0, -10, 
            -10,   5,   5,  10,  10,   5,   5, -10, 
            -10,   0,  10,  10,  10,  10,   0, -10, 
            -10,  10,  10,  10,  10,  10,  10, -10, 
            -10,   5,   0,   0,   0,   0,   5, -10, 
            -20, -10, -10, -10, -10, -10, -10, -20 
        }, 
        { // rook
             0,   0,   0,   0,   0,   0,   0,   0,
             5,  10,  10,  10,  10,  10,  10,   5, 
            -5,   0,   0,   0,   0,   0,   0,  -5, 
            -5,   0,   0,   0,   0,   0,   0,  -5, 
            -5,   0,   0,   0,   0,   0,   0,  -5, 
            -5,   0,   0,   0,   0,   0,   0,  -5, 
            -5,   0,   0,   0,   0,   0,   0,  -5, 
             0,   0,   0,   5,   5,   0,   0,   0 
        }, 
        { // queen
            -20, -10, -10,  -5,  -5, -10, -10, -20, 
            -10,   0,   0,   0,   0,   0,   0, -10, 
            -10,   0,   5,   5,   5,   5,   0, -10, 
              0,   0,   5,   5,   5,   5,   0,   0, 
              0,   0,   5,   5,   5,   5,   0,   0, 
            -10,   0,   5,   5,   5,   5,   0, -10, 
            -10,   0,   5,   0,   0,   5,   0, -10, 
            -20, -10, -10,  -5,  -5, -10, -10, -20, 
        }, 
        { // king 
            -30, -40, -40, -50, -50, -40, -40, -30, 
            -30, -40, -40, -50, -50, -40, -40, -30, 
            -30, -40, -40, -50, -50, -40, -40, -30, 
            -30, -40, -40, -50, -50, -40, -40, -30, 
            -20, -30, -30, -40, -40, -30, -30, -20, 
            -10, -20, -20, -20, -20, -20, -20, -10, 
            20,  20,   0,   0,   0,   0,  20,  20, 
            20,  30,  10,   0,   0,  10,  30,  20 
        }
    }, 
    {
        { // pawn
            0,   0,   0,   0,   0,   0,   0,   0, 
          100, 100, 100, 100, 100, 100, 100, 100, 
           40,  40,  40,  40,  40,  40,  40,  40,
           30,  30,  30,  30,  30,  30,  30,  30,  
           20,  20,  20,  20,  20,  20,  20,  20,  
           10,  10,  10,  10,  10,  10,  10,  10, 
            5,   0,   0, -20, -20,   0,   0,   5, 
            0,   0,   0,   0,   0,   0,   0,   0
        }, 
        { // knight 
            -50, -40, -30, -30, -30, -30, -40, -50, 
            -40, -20,   0,   0,   0,   0, -20, -40, 
            -30,   0,  10,  15,  15,  10,   0, -30, 
            -30,   5,  15,  20,  20,  15,   5, -30, 
            -30,   0,  15,  20,  20,  15,   0, -30, 
            -30,   5,  10,  15,  15,  10,   5, -30, 
            -40, -20,   0,   5,   5,   0, -20, -40, 
            -50, -40, -30, -30, -30, -30, -40, -50 
        }, 
        { // bishop 
            -20, -10, -10, -10, -10, -10, -10, -20, 
            -10,   0,   0,   0,   0,   0,   0, -10, 
            -10,   0,   5,  10,  10,   5,   0, -10, 
            -10,   5,   5,  10,  10,   5,   5, -10, 
            -10,   0,  10,  10,  10,  10,   0, -10, 
            -10,  10,  10,  10,  10,  10,  10, -10, 
            -10,   5,   0,   0,   0,   0,   5, -10, 
            -20, -10, -10, -10, -10, -10, -10, -20 
        }, 
        { // rook
            0,   0,   0,   0,   0,   0,   0,   0,
            5,  10,  10,  10,  10,  10,  10,   5, 
           -5,   0,   0,   0,   0,   0,   0,  -5, 
           -5,   0,   0,   0,   0,   0,   0,  -5, 
           -5,   0,   0,   0,   0,   0,   0,  -5, 
           -5,   0,   0,   0,   0,   0,   0,  -5, 
           -5,   0,   0,   0,   0,   0,   0,  -5, 
            0,   0,   0,   5,   5,   0,   0,   0 
        }, 
        { // queen
            -20, -10, -10,  -5,  -5, -10, -10, -20, 
            -10,   0,   0,   0,   0,   0,   0, -10, 
            -10,   0,   5,   5,   5,   5,   0, -10, 
              0,   0,   5,   5,   5,   5,   0,   0, 
              0,   0,   5,   5,   5,   5,   0,   0, 
            -10,   0,   5,   5,   5,   5,   0, -10, 
            -10,   0,   5,   0,   0,   5,   0, -10, 
            -20, -10, -10,  -5,  -5, -10, -10, -20, 
        }, 
        { // king 
            -50, -40, -30, -30, -30, -30, -40, -50, 
            -40, -20,   0,   0,   0,   0, -20, -40, 
            -30,   0,  10,  15,  15,  10,   0, -30, 
            -30,   5,  15,  20,  20,  15,   5, -30, 
            -30,   0,  15,  20,  20,  15,   0, -30, 
            -30,   5,  10,  15,  15,  10,   5, -30, 
            -40, -20,   0,   5,   5,   0, -20, -40, 
            -50, -40, -30, -30, -30, -30, -40, -50 
        }
    }, 
};

static inline void eval_w_pc_sq(const game *g, piece pc, int *mg, int *eg) 
{
    bboard pcs = rrow(g->pieces[pc]); 
    FOR_EACH_BIT(pcs, 
    {
        *mg += PC_SQ[0][pc][sq]; 
        *eg += PC_SQ[1][pc][sq]; 
    });
}

static inline void eval_b_pc_sq(const game *g, piece pc, int *mg, int *eg) 
{
    bboard pcs = g->pieces[make_pc(pc, COL_B)]; 
    FOR_EACH_BIT(pcs, 
    {
        *mg -= PC_SQ[0][pc][sq]; 
        *eg -= PC_SQ[1][pc][sq]; 
    });
}

// bool is_special_draw(const game *g) 
// {
//     // check for repitition 
//     for (size_t i = 1; i <= 8; i += 2)
//     {
//         if (i >= g->hist.size) break; 

//         zobrist hash = ((const game *) at_vec_const(&g->hist, g->hist.size - i - 1))->hash; 
//         if (g->hash == hash || (g->hash ^ col_zb()) == hash) 
//         {
//             return true; 
//         }
//     }

//     return false; 
// }

int evaluate(const game *g, int n_moves, bool draw) 
{
    if (draw) return 0; 

    if (n_moves == 0) 
    {
        if (g->in_check) 
        {
            // lower value the farther out the mate is (prioritize faster mates)
            return (100000 - g->ply) * (-1 + 2 * g->turn); 
        }
        else 
        {
            // stalemate
            return 0; 
        }
    }

    int wp = popcnt(g->pieces[PC_WP]); 
    int bp = popcnt(g->pieces[PC_BP]); 
    int wn = popcnt(g->pieces[PC_WN]); 
    int bn = popcnt(g->pieces[PC_BN]); 
    int wb = popcnt(g->pieces[PC_WB]); 
    int bb = popcnt(g->pieces[PC_BB]); 
    int wr = popcnt(g->pieces[PC_WR]); 
    int br = popcnt(g->pieces[PC_BR]); 
    int wq = popcnt(g->pieces[PC_WQ]); 
    int bq = popcnt(g->pieces[PC_BQ]); 
    int wk = popcnt(g->pieces[PC_WK]); 
    int bk = popcnt(g->pieces[PC_BK]); 

    int eval = 0; 
    int mg = 0; 
    int eg = 0; 

    eval += 100 * (wp - bp); 
    eval += 310 * (wn - bn); 
    eval += 320 * (wb - bb); 
    eval += 500 * (wr - br); 
    eval += 975 * (wq - bq); 
    eval += 10000 * (wk - bk); 

    // bishop pair 
    eval += 15 * ((wb >= 2) - (bb >= 2)); 

    eval_w_pc_sq(g, PC_P, &mg, &eg); 
    eval_b_pc_sq(g, PC_P, &mg, &eg); 
    eval_w_pc_sq(g, PC_N, &mg, &eg); 
    eval_b_pc_sq(g, PC_N, &mg, &eg); 
    eval_w_pc_sq(g, PC_B, &mg, &eg); 
    eval_b_pc_sq(g, PC_B, &mg, &eg); 
    eval_w_pc_sq(g, PC_R, &mg, &eg); 
    eval_b_pc_sq(g, PC_R, &mg, &eg); 
    eval_w_pc_sq(g, PC_Q, &mg, &eg); 
    eval_b_pc_sq(g, PC_Q, &mg, &eg); 
    eval_w_pc_sq(g, PC_K, &mg, &eg); 
    eval_b_pc_sq(g, PC_K, &mg, &eg); 

    // int p = 0 * (wp - bp);  
    int n = 1 * (wn + bn); 
    int b = 1 * (wb + bb); 
    int r = 2 * (wr + br); 
    // quick way to ignore extra queens 
    int q = 4 * ((wq > 0) + (bq > 0)); 

    // between 0 and (4+4+8+16)=32
    int phase = 32 - (n + b + r + q); 
    phase = (phase >= 0) * phase; 
    // printf("phase %d mg %d eg %d eval %d\n", phase, mg, eg, (mg * (32 - phase) + eg * phase) / 32); 
    
    return eval + (mg * (32 - phase) + eg * phase) / 32; 
}
