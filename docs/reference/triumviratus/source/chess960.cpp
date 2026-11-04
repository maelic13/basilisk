// Chess960: foundations (stage 1). See chess960.h and docs/audit_8.0/I_CHESS960.md.
#include "chess960.h"
#include "defs.h"

bool g_chess960 = false;
Castling960 g_c960;

// Rook of side `color` on file `file` of the back rank? Returns the square or no_sq.
static int rook_on_file(int color, int file)
{
    const int sq = (color == white ? 56 : 0) + file;   // a1 = 56, a8 = 0 (convenzione del motore)
    const int rook = color == white ? R : r;
    return get_bit(bitboards[rook], sq) ? sq : no_sq;
}

// Outermost rook on the given side (X-FEN): starting from the corner towards the king.
static int outer_rook(int color, bool king_side, int king_file)
{
    if (king_side) {
        for (int f = 7; f > king_file; f--)
            if (rook_on_file(color, f) != no_sq) return rook_on_file(color, f);
    } else {
        for (int f = 0; f < king_file; f++)
            if (rook_on_file(color, f) != no_sq) return rook_on_file(color, f);
    }
    return no_sq;
}

int c960_parse_castling(const char* p, const char* end)
{
    g_c960.rights = 0;
    for (int i = 0; i < 4; i++) g_c960.rook_from[i] = no_sq;
    for (int sq = 0; sq < 64; sq++) g_c960.rights_mask[sq] = 15;

    for (int c = white; c <= black; c++) {
        const U64 kb = bitboards[c == white ? K : k];
        const int base = c == white ? 56 : 0;
        g_c960.king_from[c] = no_sq;
        for (int f = 0; f < 8; f++)
            if (get_bit(kb, base + f)) g_c960.king_from[c] = base + f;
    }

    int dropped = 0;
    for (; p < end && *p != ' '; p++) {
        const char ch = *p;
        if (ch == '-') continue;
        const int color = (ch >= 'A' && ch <= 'Z') ? white : black;
        const int ksq = g_c960.king_from[color];
        if (ksq == no_sq) { dropped++; continue; }
        const int kfile = ksq & 7;
        int rsq = no_sq;
        if (ch == 'K' || ch == 'k')      rsq = outer_rook(color, true, kfile);
        else if (ch == 'Q' || ch == 'q') rsq = outer_rook(color, false, kfile);
        else if ((ch >= 'A' && ch <= 'H') || (ch >= 'a' && ch <= 'h'))
            rsq = rook_on_file(color, (ch | 0x20) - 'a');
        if (rsq == no_sq || (rsq & 7) == kfile) { dropped++; continue; }
        const bool king_side = (rsq & 7) > kfile;
        const int idx = (color == white ? 0 : 2) + (king_side ? 0 : 1);   // wk, wq, bk, bq
        g_c960.rights |= c960_bit(idx);
        g_c960.rook_from[idx] = rsq;
    }

    // Mask: moving the king removes both rights of its colour, moving (or capturing) a castling
    // rook removes its own. With the king on e1/e8 and rooks in the corners it coincides with the old fixed table.
    for (int c = white; c <= black; c++)
        if (g_c960.king_from[c] != no_sq)
            g_c960.rights_mask[g_c960.king_from[c]] &= ~(c == white ? (wk | wq) : (bk | bq));
    for (int i = 0; i < 4; i++)
        if (g_c960.rook_from[i] != no_sq)
            g_c960.rights_mask[g_c960.rook_from[i]] &= ~c960_bit(i);
    return dropped;
}

// ---- Stage 2: connection to the engine ------------------------------------------------------------
extern int castling_rights[64];      // movegen.cpp: castle &= castling_rights[origin/destination]
int castle_rook_sq[4] = {h1, a1, h8, a8};
int castle_king_sq[2] = {e1, e8};

static const int kStdRightsMask[64] = {
     7, 15, 15, 15,  3, 15, 15, 11,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    13, 15, 15, 15, 12, 15, 15, 14
};

void c960_reset_standard()
{
    castle_rook_sq[0] = h1; castle_rook_sq[1] = a1; castle_rook_sq[2] = h8; castle_rook_sq[3] = a8;
    castle_king_sq[0] = e1; castle_king_sq[1] = e8;
    for (int sq = 0; sq < 64; sq++) castling_rights[sq] = kStdRightsMask[sq];
}

void c960_apply_to_engine()
{
    c960_reset_standard();
    for (int i = 0; i < 4; i++)
        if (g_c960.rook_from[i] != no_sq) castle_rook_sq[i] = g_c960.rook_from[i];
    for (int c = 0; c < 2; c++)
        if (g_c960.king_from[c] != no_sq) castle_king_sq[c] = g_c960.king_from[c];
    for (int sq = 0; sq < 64; sq++) castling_rights[sq] = g_c960.rights_mask[sq];
}
