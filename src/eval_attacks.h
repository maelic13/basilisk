#pragma once

// The attack producer: the inputs every family after the lazy checkpoint
// reads, computed once. The knight and slider sweep is one pass that builds
// the maps, counts mobility and counts king attackers together.
// Part of the evaluator's single translation unit: included by src/eval.cpp
// alone, which the engine library, the Texel tuner and the oracle build all
// compile as one file. Include it nowhere else.

#include "eval_mobility.h"

// The sweep's per-square slider attacks: written for every bishop and rook
// square before anything reads it, and not cleared. It is kept apart from
// AttackInfo because it is indexed by a runtime square: inside the struct, that
// one variable index keeps the compiler from holding any of the struct's other
// fields in registers.
struct SliderAttacks {
    Bitboard att[SQUARE_NB];
    Bitboard& operator[](int sq) { return att[sq]; }
    const Bitboard& operator[](int sq) const { return att[sq]; }
};

// Filled by fill_attack_info() after the lazy checkpoint and only read after
// it. Lives on the evaluator's stack, as does the SliderAttacks it points to.
struct AttackInfo {
    Bitboard attacked_by[NCOLORS][PIECE_TYPE_NB];  // attacks of colour c's pieces of type pt
    Bitboard attacked[NCOLORS];                    // union of attacked_by[c]
    Bitboard attacked2[NCOLORS];                   // squares attacked twice or more
    Bitboard king_zone[NCOLORS];                   // king ring, king square, a rank forward
    Bitboard blockers_for_king[NCOLORS];           // own pieces pinned to own king
    Bitboard mob_area[NCOLORS];                    // the mobility area
    SliderAttacks* slider_att;                     // each bishop's and rook's attacks
    Bitboard pawn_atk[NCOLORS];                    // from the pawn cache
    Bitboard passed[NCOLORS];                      // from the pawn cache
    int      attack_units[NCOLORS];                // pressure on colour c's king
    int      n_attackers[NCOLORS];                 // pieces attacking colour c's king zone
    EvalScore mobility;                            // the sweep's mobility sum
};

static void fill_attack_info(const Board& b, const PawnEntry& pawn_entry, AttackInfo& ai) {
    const EvalParams& p = g_eval_params;

    // ---- Attack-map substrate (Step 3.0) -----------------------------------
    // Compute per-color attack maps once and reuse them for mobility, king
    // safety, and hanging-piece detection, replacing per-square is_attacked_by
    // recomputation and the duplicate slider sweep that mobility and king
    // safety each used to perform separately.
    //   attacked_by[c][pt] : union of attacks by all of color c's pieces of pt
    //   attacked[c]        : full union == is_attacked_by(sq, all_occ, c) as bb
    //   attacked2[c]       : squares attacked 2+ times by color c
    //
    // The sweep builds the maps in separate local arrays and hands them to
    // `ai` once, at the end. It indexes them by colour at run time, and a
    // store through a run-time index into one struct may, for the compiler,
    // land in any of its fields: every such store in the sweep would reload
    // the king zones, the mobility area and the mobility sum. Separate arrays
    // cannot alias one another, as in the single body this was split from.
    Bitboard attacked_by[NCOLORS][PIECE_TYPE_NB] = {};
    Bitboard attacked[NCOLORS]  = {0, 0};
    Bitboard attacked2[NCOLORS] = {0, 0};
    Bitboard king_zone[NCOLORS];
    int attack_units[NCOLORS] = {0, 0};
    int n_attackers[NCOLORS]  = {0, 0};
    const Bitboard pawn_atk[NCOLORS] = {pawn_entry.attacks[WHITE], pawn_entry.attacks[BLACK]};
    Bitboard mob_area[NCOLORS];
    EvalScore mobility;

    for (int c = 0; c < NCOLORS; c++) {
        Square ksq = b.king_square(c);
        Bitboard kz = KingAttacks[ksq] | sq_bb(ksq);
        kz |= (c == WHITE) ? shift<NORTH>(KingAttacks[ksq])
                           : shift<SOUTH>(KingAttacks[ksq]);
        king_zone[c] = kz;

        // Seed pawn and king attacks. The two pawn capture directions are
        // seeded separately: a square attacked by two different pawns
        // belongs in attacked2 -- the threats package (strongly_protected /
        // hanging) and the king-ring/flank code consume it (audit
        // hce_analysis 3; the old "no current consumer" note was stale).
        const Bitboard pawns = b.piece_bb(Color(c), PAWN);
        const Bitboard pl = (c == WHITE) ? shift<NORTH_WEST>(pawns) : shift<SOUTH_WEST>(pawns);
        const Bitboard pr = (c == WHITE) ? shift<NORTH_EAST>(pawns) : shift<SOUTH_EAST>(pawns);
        attacked2[c] |= (pl & pr) | (attacked[c] & (pl | pr));
        attacked[c]  |= pl | pr;
        attacked_by[c][PAWN] = pl | pr;

        Bitboard katt = KingAttacks[ksq];
        attacked2[c] |= attacked[c] & katt;
        attacked[c]  |= katt;
        attacked_by[c][KING] = katt;
    }

    // ---- Blockers for king (own pieces pinned in front of their king) ------
    // Computed BEFORE the sweep (Step 4.6b) because the SF mobility area below
    // excludes pinned pieces. Also consumed by king safety v2 (3.2). For each
    // king, an enemy slider aligned with it whose ray holds exactly one piece
    // pins that piece (a pinned defender does not really defend / move freely).
    Bitboard blockers_for_king[NCOLORS] = {0, 0};
    for (int c = 0; c < NCOLORS; c++) {
        Color us = Color(c), them = ~us;
        Square ksq = b.king_square(us);
        Bitboard snipers =
            (rook_attacks(ksq, 0ULL)   & (b.piece_bb(them, ROOK)   | b.piece_bb(them, QUEEN)))
          | (bishop_attacks(ksq, 0ULL) & (b.piece_bb(them, BISHOP) | b.piece_bb(them, QUEEN)));
        Bitboard blockers = 0;
        while (snipers) {
            Square s = Square(pop_lsb(snipers));
            Bitboard between = BB_BETWEEN[ksq][s] & b.all_pieces();
            if (between && !more_than_one(between))
                blockers |= between & b.occupancy_bb(us);
        }
        blockers_for_king[c] = blockers;
    }

    // Cache each slider's attack set from the sweep so the king-ring
    // and trapped-bishop tests below reuse it instead of recomputing the same
    // magic lookup. Every reuse site uses bishop/rook_attacks(sq, b.all_pieces()) —
    // the exact value the sweep computes — so this is bench-identical. Only
    // BISHOP/ROOK squares are stored (the only pieces those tests iterate);
    // every such square is written by the sweep before any test reads it, so
    // no zero-init is needed (and none is paid per eval).
    SliderAttacks& slider_att = *ai.slider_att;

    // Single knight/slider sweep: builds the maps and accumulates both mobility
    // (for the moving color) and king-attacker pressure (against the enemy king
    // — a piece of color c presses on the king of color ~c).
    for (int c = 0; c < NCOLORS; c++) {
        [[maybe_unused]] int sign = (c == WHITE) ? 1 : -1;
        Color them = ~Color(c);
        // SF-style mobility area (Step 4.6b): exclude own K/Q, own pawns that
        // are blocked (a piece directly ahead) or on the low ranks (2/3 from
        // our side), squares attacked by enemy pawns, and own pieces pinned to
        // our king. Own minors/rooks are NOT excluded (controlling the square
        // under a friendly piece is real mobility); enemy-occupied squares stay
        // includable (mobility onto/against them counts).
        Bitboard own_pawns = b.piece_bb(c, PAWN);
        Bitboard low_ranks = (c == WHITE) ? (BB_RANKS[1] | BB_RANKS[2])
                                          : (BB_RANKS[6] | BB_RANKS[5]);
        Bitboard ahead = (c == WHITE) ? shift<SOUTH>(b.all_pieces())
                                      : shift<NORTH>(b.all_pieces());
        Bitboard blocked_or_low = own_pawns & (ahead | low_ranks);
        mob_area[c] = ~(blocked_or_low | b.piece_bb(c, KING) | b.piece_bb(c, QUEEN)
                        | pawn_atk[them] | blockers_for_king[c]);
        const Bitboard area = mob_area[c];
        // Hoist the two per-piece switch(pt) out of the mobility
        // inner loop. pt is constant across a whole `while (pcs)` run, so both
        // switches are loop-invariant. A template lambda instantiated once per
        // piece type folds them to compile-time constants — the att magic is
        // picked directly, the mobility table selected at compile time — so the
        // inner loop is branchless. Same values, same accumulation order (all
        // knights, then bishops, rooks, queens) ⇒ bench-identical by
        // construction, and the TR_* trace calls are unchanged so the tuner is
        // exact.
        auto sweep = [&]<PieceType PT>() {
            Bitboard pcs = b.piece_bb(c, PT);
            while (pcs) {
                int sq = pop_lsb(pcs);
                Bitboard att;
                if      constexpr (PT == KNIGHT) att = KnightAttacks[sq];
                else if constexpr (PT == BISHOP) att = bishop_attacks(Square(sq), b.all_pieces());
                else if constexpr (PT == ROOK)   att = rook_attacks(Square(sq), b.all_pieces());
                else                             att = queen_attacks(Square(sq), b.all_pieces());

                attacked2[c] |= attacked[c] & att;
                attacked[c]  |= att;
                attacked_by[c][PT] |= att;
                if constexpr (PT == BISHOP || PT == ROOK)
                    slider_att[sq] = att;

                // Per-count mobility over the mobility area.
                mobility_term<PT>(p, popcount(att & area), sign, mobility);

                Bitboard zone_hits = att & king_zone[them];
                if (zone_hits) {
                    n_attackers[them]++;
                    attack_units[them] += p.ks_unit[PT] + popcount(zone_hits) / 2;
                }
            }
        };
        sweep.template operator()<KNIGHT>();
        sweep.template operator()<BISHOP>();
        sweep.template operator()<ROOK>();
        sweep.template operator()<QUEEN>();
    }

    std::memcpy(ai.attacked_by, attacked_by, sizeof(attacked_by));
    for (int c = 0; c < NCOLORS; c++) {
        ai.attacked[c]          = attacked[c];
        ai.attacked2[c]         = attacked2[c];
        ai.king_zone[c]         = king_zone[c];
        ai.blockers_for_king[c] = blockers_for_king[c];
        ai.mob_area[c]          = mob_area[c];
        ai.pawn_atk[c]          = pawn_atk[c];
        ai.passed[c]            = pawn_entry.passed[c];
        ai.attack_units[c]      = attack_units[c];
        ai.n_attackers[c]       = n_attackers[c];
    }
    ai.mobility = mobility;
}
