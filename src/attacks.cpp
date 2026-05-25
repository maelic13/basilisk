#include "attacks.h"
#include <cstring>
#include <cassert>
#include <algorithm>

#if defined(USE_PEXT)
#  include <immintrin.h>
#endif

Bitboard KnightAttacks[SQUARE_NB];
Bitboard KingAttacks[SQUARE_NB];
Bitboard PawnAttacks[NCOLORS][SQUARE_NB];

// ---- Magic bitboard structures ---------------------------------------------
struct MagicEntry {
    Bitboard  mask;
    uint64_t  magic;
    int       shift;
    Bitboard* attacks;
};

static MagicEntry RookMagics[SQUARE_NB];
static MagicEntry BishopMagics[SQUARE_NB];

// Flat attack tables (rook needs up to 4096 per square = 102400 total;
// bishop needs up to 512 per square = 5248 total)
static Bitboard RookTable[102400];
static Bitboard BishopTable[5248];

#if !defined(USE_PEXT)
// ---- splitmix64 PRNG -------------------------------------------------------
static uint64_t sm64_state;
static uint64_t sm64_next() {
    uint64_t z = (sm64_state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}
static uint64_t sparse64() { return sm64_next() & sm64_next() & sm64_next(); }
#endif

// ---- Slow attack generators (for magic finding) ----------------------------
static Bitboard rook_mask(Square sq) {
    Bitboard mask = 0;
    int r = rank_of(sq), f = file_of(sq);
    for (int i = r+1; i <= 6; i++) mask |= 1ULL << (i*8+f);
    for (int i = r-1; i >= 1; i--) mask |= 1ULL << (i*8+f);
    for (int i = f+1; i <= 6; i++) mask |= 1ULL << (r*8+i);
    for (int i = f-1; i >= 1; i--) mask |= 1ULL << (r*8+i);
    return mask;
}

static Bitboard bishop_mask(Square sq) {
    Bitboard mask = 0;
    int r = rank_of(sq), f = file_of(sq);
    for (int i=1; r+i<=6 && f+i<=6; i++) mask |= 1ULL << ((r+i)*8+(f+i));
    for (int i=1; r+i<=6 && f-i>=1; i++) mask |= 1ULL << ((r+i)*8+(f-i));
    for (int i=1; r-i>=1 && f+i<=6; i++) mask |= 1ULL << ((r-i)*8+(f+i));
    for (int i=1; r-i>=1 && f-i>=1; i++) mask |= 1ULL << ((r-i)*8+(f-i));
    return mask;
}

static Bitboard rook_attacks_slow(Square sq, Bitboard occ) {
    Bitboard att = 0;
    int r = rank_of(sq), f = file_of(sq);
    for (int i=r+1; i<8; i++) { att |= 1ULL<<(i*8+f); if (occ>>( i*8+f)&1) break; }
    for (int i=r-1; i>=0; i--){ att |= 1ULL<<(i*8+f); if (occ>>(i*8+f)&1) break; }
    for (int i=f+1; i<8; i++) { att |= 1ULL<<(r*8+i); if (occ>>(r*8+i)&1) break; }
    for (int i=f-1; i>=0; i--){ att |= 1ULL<<(r*8+i); if (occ>>(r*8+i)&1) break; }
    return att;
}

static Bitboard bishop_attacks_slow(Square sq, Bitboard occ) {
    Bitboard att = 0;
    int r = rank_of(sq), f = file_of(sq);
    for (int i=1; r+i<8&&f+i<8; i++) { att|=1ULL<<((r+i)*8+(f+i)); if(occ>>((r+i)*8+(f+i))&1) break; }
    for (int i=1; r+i<8&&f-i>=0;i++) { att|=1ULL<<((r+i)*8+(f-i)); if(occ>>((r+i)*8+(f-i))&1) break; }
    for (int i=1; r-i>=0&&f+i<8; i++) { att|=1ULL<<((r-i)*8+(f+i)); if(occ>>((r-i)*8+(f+i))&1) break; }
    for (int i=1; r-i>=0&&f-i>=0;i++) { att|=1ULL<<((r-i)*8+(f-i)); if(occ>>((r-i)*8+(f-i))&1) break; }
    return att;
}

#if !defined(USE_PEXT)
// ---- Magic finding ---------------------------------------------------------
static bool try_magic(Bitboard mask, uint64_t magic, int shift, bool bishop, Square sq,
                      Bitboard* table, int tableSize) {
    std::fill(table, table + tableSize, Bitboard(0));
    Bitboard occ = 0;
    // Carry-Rippler enumeration of all subsets of mask
    do {
        int idx = (int)((occ * magic) >> shift);
        Bitboard att = bishop ? bishop_attacks_slow(sq, occ) : rook_attacks_slow(sq, occ);
        if (table[idx] == 0)
            table[idx] = att;
        else if (table[idx] != att)
            return false; // destructive collision
        occ = (occ - mask) & mask;
    } while (occ);
    return true;
}

static void find_magic(Square sq, bool bishop, MagicEntry& entry, Bitboard* table) {
    Bitboard mask = bishop ? bishop_mask(sq) : rook_mask(sq);
    int n = popcount(mask);
    int shift = 64 - n;
    int tableSize = 1 << n;

    entry.mask  = mask;
    entry.shift = shift;

    Bitboard tmp[4096] = {};

    for (int attempt = 0; attempt < 100000000; attempt++) {
        uint64_t magic = sparse64();
        if (popcount((mask * magic) & 0xFF00000000000000ULL) < 6)
            continue;
        if (try_magic(mask, magic, shift, bishop, sq, tmp, tableSize)) {
            entry.magic   = magic;
            entry.attacks = table;
            std::memcpy(table, tmp, sizeof(Bitboard) * (size_t)tableSize);
            return;
        }
    }
    assert(false && "Failed to find magic number");
}
#endif

#if defined(USE_PEXT)
static void init_pext_table(Square sq, bool bishop, MagicEntry& entry, Bitboard* table) {
    Bitboard mask = bishop ? bishop_mask(sq) : rook_mask(sq);

    entry.mask    = mask;
    entry.magic   = 0;
    entry.shift   = 0;
    entry.attacks = table;

    Bitboard occ = 0;
    do {
        size_t idx = static_cast<size_t>(_pext_u64(occ, mask));
        table[idx] = bishop ? bishop_attacks_slow(sq, occ) : rook_attacks_slow(sq, occ);
        occ = (occ - mask) & mask;
    } while (occ);
}
#endif

// ---- Public attack functions -----------------------------------------------
Bitboard bishop_attacks(Square sq, Bitboard occ) {
    const MagicEntry& e = BishopMagics[sq];
#if defined(USE_PEXT)
    return e.attacks[_pext_u64(occ, e.mask)];
#else
    return e.attacks[((occ & e.mask) * e.magic) >> e.shift];
#endif
}

Bitboard rook_attacks(Square sq, Bitboard occ) {
    const MagicEntry& e = RookMagics[sq];
#if defined(USE_PEXT)
    return e.attacks[_pext_u64(occ, e.mask)];
#else
    return e.attacks[((occ & e.mask) * e.magic) >> e.shift];
#endif
}

// ---- init_attacks ----------------------------------------------------------
void init_attacks() {
#if !defined(USE_PEXT)
    sm64_state = 1234567890123ULL;
#endif

    // Knight attacks
    for (int s = 0; s < 64; s++) {
        int r = s >> 3, f = s & 7;
        Bitboard att = 0;
        const int dr[] = {-2,-2,-1,-1, 1, 1, 2, 2};
        const int df[] = {-1, 1,-2, 2,-2, 2,-1, 1};
        for (int i = 0; i < 8; i++) {
            int nr = r + dr[i], nf = f + df[i];
            if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8)
                att |= 1ULL << (nr * 8 + nf);
        }
        KnightAttacks[s] = att;
    }

    // King attacks
    for (int s = 0; s < 64; s++) {
        int r = s >> 3, f = s & 7;
        Bitboard att = 0;
        for (int dr = -1; dr <= 1; dr++)
            for (int df = -1; df <= 1; df++) {
                if (!dr && !df) continue;
                int nr = r + dr, nf = f + df;
                if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8)
                    att |= 1ULL << (nr * 8 + nf);
            }
        KingAttacks[s] = att;
    }

    // Pawn attacks
    for (int s = 0; s < 64; s++) {
        int r = s >> 3, f = s & 7;
        PawnAttacks[WHITE][s] = 0;
        PawnAttacks[BLACK][s] = 0;
        if (r < 7) {
            if (f > 0) PawnAttacks[WHITE][s] |= 1ULL << (s + 7);
            if (f < 7) PawnAttacks[WHITE][s] |= 1ULL << (s + 9);
        }
        if (r > 0) {
            if (f > 0) PawnAttacks[BLACK][s] |= 1ULL << (s - 9);
            if (f < 7) PawnAttacks[BLACK][s] |= 1ULL << (s - 7);
        }
    }

    // Bishop magic tables — compute offsets
    int offset = 0;
    for (int s = 0; s < 64; s++) {
#if defined(USE_PEXT)
        init_pext_table(Square(s), true, BishopMagics[s], BishopTable + offset);
#else
        find_magic(Square(s), true, BishopMagics[s], BishopTable + offset);
#endif
        offset += 1 << popcount(bishop_mask(Square(s)));
    }

    // Rook magic tables
    offset = 0;
    for (int s = 0; s < 64; s++) {
#if defined(USE_PEXT)
        init_pext_table(Square(s), false, RookMagics[s], RookTable + offset);
#else
        find_magic(Square(s), false, RookMagics[s], RookTable + offset);
#endif
        offset += 1 << popcount(rook_mask(Square(s)));
    }
}
