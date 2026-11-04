// Origin: this file descends from BBC (Bit Board Chess) by Maksim Korzh, https://github.com/maksimKorzh/bbc,
// released under the GNU General Public License v3; the project started from BBC in 2024 and has since rewritten
// and extended much of it. Triumviratus is distributed under the GNU GPL v3 (see COPYING).
#include "defs.h"
#include "magic.h"

// Zobrist keys (rewritten 28/10/2026). The inherited keys joined the low 16 bits of four consecutive outputs of a
// 32-bit xorshift generator: every key was the same linear map of a 32-bit state, so all 849 keys lay in a subspace of
// dimension 32 and so did the key of every position. Two positions in one hash bucket then shared their key with
// probability about 2^-10 at 256 MB (measured on 8.0: 2,700-3,600 false hash hits per search at 1.5M nodes, a share
// with depth >= 8 and some near the root). The keys now come from a 64-bit counter passed through a strong mixing
// function (the splitmix64 finaliser), with a seed of ours; their GF(2) rank is 64. BAKED 28/10/2026: non-regression
// SPRT at 5+0.05 noHT Hash 64 +4.2 +- 3.9 over 8,000 games (LLR +2.5 of 2.94; pairs lost on time left out), the old
// generator removed (random.cpp is empty). Regression test: tools/regress_re4.py.
static U64 zobrist_next(U64 &counter)
{
    U64 z = (counter += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

void init_random_keys()
{
    U64 counter = 0x5452494F4D564952ULL;   // "TRIOMVIR" in ASCII
    for (int piece = P; piece <= k; piece++)
        for (int square = 0; square < 64; square++)
            piece_keys[piece][square] = zobrist_next(counter);
    for (int square = 0; square < 64; square++)
        enpassant_keys[square] = zobrist_next(counter);
    for (int index = 0; index < 16; index++)
        castle_keys[index] = zobrist_next(counter);
    side_key = zobrist_next(counter);
}

U64 generate_hash_key()
{
    U64 final_key = 0ULL;
    U64 bitboard;

    for (int piece = P; piece <= k; piece++)
    {
        bitboard = bitboards[piece];

        while (bitboard)
        {
            int square = get_ls1b_index(bitboard);
            final_key ^= piece_keys[piece][square];
            pop_bit(bitboard, square);
        }
    }

    if (enpassant != no_sq)
        final_key ^= enpassant_keys[enpassant];

    final_key ^= castle_keys[castle];

    if (side == black) final_key ^= side_key;

    return final_key;
}
