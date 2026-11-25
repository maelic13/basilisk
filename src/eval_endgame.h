#pragma once

// Endgame knowledge: the scale factors and known-endgame overrides that
// apply_endgame() puts on the tapered score (on the lazy path as well as the
// full one), the KPK bitbase, the bare-king drives, the rook-ending scales,
// the tune-build endgame census, and the mate drive the full block ends with.
// Part of the evaluator's single translation unit: included by src/eval.cpp
// alone, which the engine library, the Texel tuner and the oracle build all
// compile as one file. Include it nowhere else.

#include "eval_common.h"

// ===========================================================================
// Step 3.5 - Scale-factor framework + endgame knowledge
//
// All terms below are FROZEN (not Texel-traced): like the existing OCB scaling
// and mate-drive mop-up, they land in the tuner's `rest` term and therefore do
// not affect `--verify` reconstruction. They fire only on rare, near-dead
// material patterns absent from the `bench 13` suite, so the search-node
// fingerprint is unchanged. Correctness is gated by tests/endgames.epd, not by
// SPRT (these positions are too rare for self-play to measure).
// ===========================================================================

static constexpr int SCALE_NORMAL = 64;   // no endgame scaling
static constexpr int SCALE_DRAW   = 0;    // dead-drawn scale factor
// Above normal: a scale that AMPLIFIES, for positions the strong side is known
// to be converting. The Lucena rules need this -- reading them against
// SCALE_NORMAL instead turns a won rook ending into a near-draw.
static constexpr int SCALE_MAX    = 128;
// The floor under the rook-ending rules, and the reason they are usable at
// all. Two independent arguments, one of correctness and one of cost.
//
// Correctness: these are heuristics with no tablebase behind them. Checked
// against Syzygy on 3,000 random KRPKR positions, the unfloored draw rules call
// 1.3% of genuinely WON positions dead draws. A floor means no rule here can
// ever assert a draw -- it can only discount -- so a misfire costs accuracy
// instead of throwing a win.
//
// Cost: unfloored, the reference's most aggressive scales (9-21 of 64) flatten
// the evaluation across large regions of rook-ending search space and cost
// **+79% bench nodes** for KRPKR and +108% for KRPPKRP (BAS-E54). Every floor
// from 24 up avoids that. 24 is the largest discount that keeps a clear
// majority of each rule's effect; it is NOT selected by benching the floors,
// because that curve is chaotic (32 costs +28.5% while 24 and 40 are within
// 9% of baseline) and picking its minimum would be fitting to noise.
static constexpr int SCALE_FLOOR  = 24;
static constexpr int KPK_PAWN_EG  = 206;   // the donor's PawnValueEg, in a won KPK


// Rough eg material values, used only for endgame material classification
// (never for the actual evaluation, which uses the tuned EvalParams tables).
static constexpr int EG_MAT[PIECE_TYPE_NB] = {0, 100, 320, 330, 500, 950, 0};

static constexpr Bitboard EG_DARK_SQUARES = 0xAA55AA55AA55AA55ULL;

// ---- KPK bitbase ----------------------------------------------------------
// Canonical orientation: WHITE is the side with the single pawn, pawn on files
// A-D (mirror the file when probing E-H). Index space 2*64*64*24 = 196608.
// Result is relative to the pawn side: WIN means the pawn side wins.

enum KpkResult : uint8_t { KPK_INVALID = 0, KPK_UNKNOWN = 1, KPK_DRAW = 2, KPK_WIN = 4 };

static uint8_t       g_kpk[2 * 64 * 64 * 24];
static std::once_flag g_kpk_once;

static inline int kpk_pidx(Square psq) {
    // file 0..3, rank 1..6 (RANK_2..RANK_7) -> 0..23
    return file_of(psq) + 4 * (rank_of(psq) - 1);
}

static inline int kpk_index(int stm, Square wk, Square bk, Square psq) {
    return stm + 2 * (int(wk) + 64 * (int(bk) + 64 * kpk_pidx(psq)));
}

static KpkResult kpk_classify_initial(int stm, Square wk, Square bk, Square psq) {
    // Illegal positions.
    if (wk == bk || wk == psq || bk == psq)
        return KPK_INVALID;
    if (KingAttacks[wk] & sq_bb(bk))
        return KPK_INVALID;
    // White (pawn side) to move cannot leave the black king in check from the pawn.
    if (stm == 0 && (PawnAttacks[WHITE][psq] & sq_bb(bk)))
        return KPK_INVALID;

    // White to move: immediate promotion win on the 7th rank.
    if (stm == 0 && rank_of(psq) == RANK_7) {
        Square promo = psq + 8;
        if (wk != promo
            && (KING_DIST[bk][promo] > 1 || (KingAttacks[wk] & sq_bb(promo))))
            return KPK_WIN;
    }

    // Black to move: draw by stalemate or by capturing the lone (undefended) pawn.
    if (stm == 1) {
        Bitboard safe = KingAttacks[bk] & ~(KingAttacks[wk] | PawnAttacks[WHITE][psq]);
        bool can_capture_pawn = (KingAttacks[bk] & sq_bb(psq)) && !(KingAttacks[wk] & sq_bb(psq));
        if (safe == 0 && !can_capture_pawn)
            return KPK_DRAW;          // stalemate
        if (can_capture_pawn)
            return KPK_DRAW;          // king grabs the pawn -> KK
    }

    return KPK_UNKNOWN;
}

static KpkResult kpk_classify(int stm, Square wk, Square bk, Square psq) {
    // White-to-move (stm 0) wants WIN; Black-to-move (stm 1) wants DRAW.
    const KpkResult good = (stm == 0) ? KPK_WIN : KPK_DRAW;
    const KpkResult bad  = (stm == 0) ? KPK_DRAW : KPK_WIN;
    int r = KPK_INVALID;

    Square mover = (stm == 0) ? wk : bk;
    Bitboard kmoves = KingAttacks[mover];
    while (kmoves) {
        Square to = Square(pop_lsb(kmoves));
        if (stm == 0)
            r |= g_kpk[kpk_index(1, to, bk, psq)];
        else
            r |= g_kpk[kpk_index(0, wk, to, psq)];
    }

    if (stm == 0 && rank_of(psq) < RANK_7) {
        Square push = psq + 8;
        if (push != wk && push != bk) {
            r |= g_kpk[kpk_index(1, wk, bk, push)];
            if (rank_of(psq) == RANK_2) {
                Square dbl = push + 8;
                if (dbl != wk && dbl != bk)
                    r |= g_kpk[kpk_index(1, wk, bk, dbl)];
            }
        }
    }

    if (r & good)    return good;
    if (r & KPK_UNKNOWN) return KPK_UNKNOWN;
    return bad;
}

static void kpk_init() {
    // Seed terminal/illegal states.
    for (int pidx = 0; pidx < 24; pidx++) {
        Square psq = make_square(File(pidx & 3), Rank((pidx >> 2) + 1));
        for (int wk = 0; wk < 64; wk++)
            for (int bk = 0; bk < 64; bk++)
                for (int stm = 0; stm < 2; stm++)
                    g_kpk[kpk_index(stm, Square(wk), Square(bk), psq)] =
                        kpk_classify_initial(stm, Square(wk), Square(bk), psq);
    }
    // Iterate to a fixpoint.
    bool changed = true;
    while (changed) {
        changed = false;
        for (int pidx = 0; pidx < 24; pidx++) {
            Square psq = make_square(File(pidx & 3), Rank((pidx >> 2) + 1));
            for (int wk = 0; wk < 64; wk++)
                for (int bk = 0; bk < 64; bk++)
                    for (int stm = 0; stm < 2; stm++) {
                        int idx = kpk_index(stm, Square(wk), Square(bk), psq);
                        if (g_kpk[idx] != KPK_UNKNOWN)
                            continue;
                        KpkResult nr = kpk_classify(stm, Square(wk), Square(bk), psq);
                        if (nr != KPK_UNKNOWN) {
                            g_kpk[idx] = nr;
                            changed = true;
                        }
                    }
        }
    }
}

// True if the pawn side wins K+P vs K, with `strong` the side owning the pawn.
static bool kpk_strong_wins(const Board& b, Color strong) {
    std::call_once(g_kpk_once, kpk_init);
    Color weak = ~strong;
    Square psq = Square(lsb(b.piece_bb(strong, PAWN)));
    Square wk  = b.king_square(strong);
    Square bk  = b.king_square(weak);
    int stm    = (b.turn() == strong) ? 0 : 1;

    // Reorient so the pawn marches toward rank 8 (north).
    if (strong == BLACK) {
        psq = flip_rank(psq);
        wk  = flip_rank(wk);
        bk  = flip_rank(bk);
    }
    // Fold pawn onto files A-D.
    if (file_of(psq) >= FILE_E) {
        psq = flip_file(psq);
        wk  = flip_file(wk);
        bk  = flip_file(bk);
    }
    return g_kpk[kpk_index(stm, wk, bk, psq)] == KPK_WIN;
}

// ---- KBNK: drive the bare king to the bishop-coloured corner ---------------
struct KbnkDriveWeights {
    int base;
    int diagonal;
    int edge;
    int king;
    int knight;
};

// Largest score this vector can ever produce. The extremal legal multipliers
// are diagonal 7, edge 3, king 6 (the kings cannot touch) and knight 7.
static constexpr int kbnk_drive_max(const KbnkDriveWeights& w) {
    return w.base + 7 * w.diagonal + 3 * w.edge + 6 * w.king + 7 * w.knight;
}

// One constexpr vector serves every build and is checked here at compile
// time, so no static KBNK score can enter the mate band. The vector (base 15600, diagonal 1900, no edge pull, king 460, no knight
// pull) converts 98 of 138 held-out KBNK positions; the screen winner with
// diagonal 1750 and king 340 was rejected because on KBNK0061 it forks its own
// bishop and knight and draws a DTZ-52 win.
// Bare-king wins sit in tiers by the winner's strongest piece, so more
// material never scores lower: a promotion to a queen outranks one to a
// rook, and either outranks a knight or bishop. Below the rook tier are the
// minor-piece mates (KBNK and KXK), whose drives span thousands of points.
// The cap is the highest score that is still an evaluation, not a decisive one.
// The tier bases live in eval.h, where the rule-50 damping keeps them.
static constexpr int BARE_KING_CAP        = 31742;
static constexpr int BARE_KING_PAWN_STEP  = 3;
static constexpr KbnkDriveWeights KBNK_DRIVE_DEFAULT{10000, 1900, 0, 460, 0};
static_assert(kbnk_drive_max(KBNK_DRIVE_DEFAULT) + 8 * BARE_KING_PAWN_STEP < BARE_KING_ROOK_TIER,
              "KBNK, a pawn bonus included, must stay below the rook tier");
static_assert(BARE_KING_CAP < tablebaseWinInMaxPly,
              "bare-king scores stay below the tablebase band");
static_assert(KBNK_DRIVE_DEFAULT.base >= KNOWN_WIN,
              "KBNK base must sit at or above KNOWN_WIN so the class outranks material");
static_assert(kbnk_drive_max(KBNK_DRIVE_DEFAULT) < KBNK_STATIC_MATE_FLOOR,
              "compiled KBNK default can reach the mate-score band: a static "
              "evaluation would be ply-adjusted and stored as a mate in the TT");

static constexpr KbnkDriveWeights g_kbnk_drive = KBNK_DRIVE_DEFAULT;

#ifdef BASILISK_TUNE
namespace {

using MaterialCounts = std::array<int, 5>; // P, N, B, R, Q
constexpr int ANY = -1;                    // one or more

bool material_is(const MaterialCounts& actual, const MaterialCounts& pattern) {
    for (size_t i = 0; i < actual.size(); ++i) {
        if (pattern[i] == ANY ? actual[i] < 1 : actual[i] != pattern[i])
            return false;
    }
    return true;
}

void record_endgame_occurrence(const Board& b, EndgameOccurrenceCounters& out) {
    // The largest named family has five non-king pieces. Avoid five popcounts
    // per side on every ordinary middlegame evaluation.
    if (popcount(b.all_pieces()) > 7)
        return;

    ++out.classified;
    MaterialCounts side[NCOLORS];
    for (int c = 0; c < NCOLORS; ++c)
        for (int pt = PAWN; pt <= QUEEN; ++pt)
            side[c][static_cast<size_t>(pt - PAWN)] = popcount(b.piece_bb(c, pt));

    constexpr MaterialCounts BARE      = {0, 0, 0, 0, 0};
    constexpr MaterialCounts KRP       = {1, 0, 0, 1, 0};
    constexpr MaterialCounts KR        = {0, 0, 0, 1, 0};
    constexpr MaterialCounts KB        = {0, 0, 1, 0, 0};
    constexpr MaterialCounts KN        = {0, 1, 0, 0, 0};
    constexpr MaterialCounts KP        = {1, 0, 0, 0, 0};

    for (int orientation = 0; orientation < NCOLORS; ++orientation) {
        const MaterialCounts& strong = side[orientation];
        const MaterialCounts& weak   = side[orientation ^ 1];

        // KPKP is symmetric. Trying both orientations must still count one
        // evaluated position once, not twice.
        if (orientation == 1 && side[WHITE] == side[BLACK])
            break;

        if (material_is(strong, KRP) && material_is(weak, KR))
            ++out.krpkr;
        else if (material_is(strong, KRP) && material_is(weak, KB))
            ++out.krpkb;
        else if (material_is(strong, {2, 0, 0, 1, 0})
                 && material_is(weak, KRP))
            ++out.krppkrp;
        else if (material_is(strong, KP) && material_is(weak, BARE)) {
            ++out.kpk;
            ++out.kpsk;
        } else if (material_is(strong, {ANY, 0, 0, 0, 0})
                   && material_is(weak, BARE))
            ++out.kpsk;
        else if (material_is(strong, KR) && material_is(weak, KP))
            ++out.krkp;
        else if (material_is(strong, KP) && material_is(weak, KP))
            ++out.kpkp;
        else if (material_is(strong, {ANY, 0, 1, 0, 0})
                   && material_is(weak, BARE))
            ++out.kbpsk;
        else if (material_is(strong, {0, 0, 0, 0, 1})
                   && material_is(weak, KP))
            ++out.kqkp;
        else if (material_is(strong, {2, 0, 1, 0, 0})
                   && material_is(weak, KB))
            ++out.kbppkb;
        else if (material_is(strong, {1, 0, 1, 0, 0})
                   && material_is(weak, KB))
            ++out.kbpkb;
        else if (material_is(strong, {1, 0, 1, 0, 0})
                   && material_is(weak, KN))
            ++out.kbpkn;
        else if (material_is(strong, KR) && material_is(weak, KN))
            ++out.krkn;
        else if (material_is(strong, KR) && material_is(weak, KB))
            ++out.krkb;
        else if (material_is(strong, {0, 2, 0, 0, 0})
                   && material_is(weak, KP))
            ++out.knnkp;
        else if (material_is(strong, {0, 2, 0, 0, 0})
                   && material_is(weak, BARE))
            ++out.knnk;
        else if (material_is(strong, {0, 0, 0, 0, 1})
                   && material_is(weak, KR))
            ++out.kqkr;
        else if (material_is(strong, {0, 0, 0, 0, 1})
                   && material_is(weak, {ANY, 0, 0, 1, 0}))
            ++out.kqkrps;
        else if (material_is(strong, {0, 1, 1, 0, 0})
                   && material_is(weak, BARE)) {
            ++out.kbnk;
            ++out.kxk;
        } else if (strong[0] == 0 && material_is(weak, BARE)) {
            // Count actual mating material, not Rarog's coarser test: KBK and
            // KNK are dead draws and must not enter KXK. The bishop pair is
            // tested by SQUARE COLOUR, exactly as apply_endgame's KXK gate
            // does, because two same-coloured bishops -- reachable by
            // promotion -- also cannot force mate. Queen and rook stay in the
            // family census even though the engine deliberately does not
            // override their search-solved mates (BAS-E30, BAS-E34); this
            // instrument measures family frequency, not kxk_score firings.
            const Bitboard bishops = b.piece_bb(Color(orientation), BISHOP);
            const bool bishop_pair =
                (bishops & EG_DARK_SQUARES) && (bishops & ~EG_DARK_SQUARES);
            const bool can_mate = strong[4] > 0 || strong[3] > 0
                               || bishop_pair
                               || (strong[2] > 0 && strong[1] > 0);
            if (can_mate)
                ++out.kxk;
        }
    }
}

} // namespace
#endif

static int kbnk_score(const Board& b, Color strong) {
    Color  weak = ~strong;
    Square sk   = b.king_square(strong);
    Square wksq = b.king_square(weak);
    Square bsq  = Square(lsb(b.piece_bb(strong, BISHOP)));

    // The two same-coloured corners as the bishop are a diagonal pair.
    bool dark_bishop = (sq_bb(bsq) & EG_DARK_SQUARES) != 0;

    // The drive uses Manhattan distance to the nearest correct corner, not
    // Chebyshev, which has large plateaus -- a whole L-shaped band shares one
    // value -- and leaves no gradient to follow over most of the board. It is
    // written as the equivalent bishop-colour diagonal formula. For
    // dark-corner bishops:
    //
    //   14 - min(f+r, 14-f-r) == 7 + abs(7-r-f)
    //
    // and for light-corner bishops the variable part is abs(r-f). The +7 is a
    // class-wide constant retained for exact score and bench identity. The
    // open question is the scale of this drive and its interaction with the
    // other pulls below.
    const int wf = int(file_of(wksq)), wr = int(rank_of(wksq));
    const int diagonal = dark_bishop ? std::abs(7 - wr - wf) : std::abs(wr - wf);

    const int edge_dist = std::min(std::min(wf, 7 - wf), std::min(wr, 7 - wr));
    const int king_dist = KING_DIST[sk][wksq];

    int v = g_kbnk_drive.base
          + diagonal * g_kbnk_drive.diagonal
          + (3  - edge_dist) * g_kbnk_drive.edge
          + (8  - king_dist) * g_kbnk_drive.king;
    if (b.piece_bb(strong, KNIGHT))
        v += (8 - KING_DIST[lsb(b.piece_bb(strong, KNIGHT))][wksq]) * g_kbnk_drive.knight;

    return (strong == WHITE) ? v : -v;
}

// ---- KXK: mate a bare king with Q, R, bishop pair, or B+N ------------------
// The old generic drive was `5*lk_center + 4*(14 - king_dist)` -- **5 and 4
// centipawns per step**, an order of magnitude below the 100-500cp futility and
// razoring margins, so the search pruned the king walk before it could pay off.
// That is the BAS-E29 defect that held KBNK at 13% conversion.
//
// It is why KBB-K converts 3/12 while the far harder KBNK now converts 54.5%.
// KQ-K and KR-K survive the defect only because their mates sit inside the
// search horizon (BAS-E30 measured both at 100/100) -- they never needed the
// gradient, which is exactly why the defect stayed invisible here.
//
// An override rather than a bonus added to `eg`, for the same reason KBNK is
// one: a gradient large enough to beat the pruning margins spans thousands of
// centipawns, which would swamp material if it were added to a normal score.
// As an override the whole class sits above KNOWN_WIN and the drive only orders
// moves *within* it. Dropping below mating material (KBB-K -> KB-K) leaves this
// function entirely and collapses to the dead-draw scale, so the large weights
// cannot bribe the engine into a losing simplification. Non-pawn material is
// carried in the score so that KQ-K still outranks KR-K.
static int kxk_score(const Board& b, Color strong) {
    const Color  weak = ~strong;
    const Square wksq = b.king_square(weak);
    const Square sk   = b.king_square(strong);

    const int npm = popcount(b.piece_bb(strong, KNIGHT)) * EG_MAT[KNIGHT]
                  + popcount(b.piece_bb(strong, BISHOP)) * EG_MAT[BISHOP]
                  + popcount(b.piece_bb(strong, ROOK))   * EG_MAT[ROOK]
                  + popcount(b.piece_bb(strong, QUEEN))  * EG_MAT[QUEEN];

    // Only minor-piece mates reach here; the caller sends Q and R elsewhere.
    // That is what makes these weights safe: the attacker's material cannot
    // vary within the class, exactly as in KBNK, so a drive spanning
    // thousands of centipawns can never outrank a piece. Edge first, then
    // corner, then close the kings -- the order the technique is executed in.
    // Every weight clears the razoring margin so the walk survives pruning,
    // which is the whole point of the step.
    constexpr int w_edge   = 900;
    constexpr int w_corner = 250;
    constexpr int w_king   = 300;

    const int wf = int(file_of(wksq)), wr = int(rank_of(wksq));
    const int edge_dist = std::min(std::min(wf, 7 - wf), std::min(wr, 7 - wr));
    // Unlike KBNK, Q/R/BB mate in ALL FOUR corners, so the nearest one is the
    // target; for a corner set that symmetric, Manhattan distance collapses to
    // this closed form. Manhattan rather than Chebyshev because Chebyshev has
    // wide plateaus -- whole L-shaped bands share a value, leaving no gradient
    // to follow over most of the board.
    const int corner_md = std::min(wf, 7 - wf) + std::min(wr, 7 - wr);
    const int king_dist = KING_DIST[sk][wksq];

    const int v = KNOWN_WIN + npm
                + (3 - edge_dist) * w_edge
                + (6 - corner_md) * w_corner
                + (8 - king_dist) * w_king;
    return (strong == WHITE) ? v : -v;
}

// ---- KQ-K and KR-K: the top tiers' score -----------------------------------
// The donor's Endgame<KXK> (classical Stockfish, sf_16's endgame.cpp): the
// winner's material, a push of the bare king to the edge and a pull of the
// kings together. Its two tables are in the donor's internal units and are
// converted at 100 to the donor's endgame pawn (208).
//
// It replaced the winner's ordinary score, which made the fitted PSTs and
// mobility tables these mates' only gradient. Those rows are overrides that
// a Texel fit leaves out, so every refit reshaped the gradient blind, and
// C.2.8's run 2 left KR-K unconverted at 40,000 to 60,000 nodes (the C.2.8
// registration's Amendment 2). Nothing here reads g_eval_params.
static constexpr int KXK_DONOR_PAWN_EG = 208;

constexpr int kxk_push_to_edge(Square s) {
    const int r = int(rank_of(s)), f = int(file_of(s));
    const int rd = r < 4 ? r : 7 - r, fd = f < 4 ? f : 7 - f;
    return 90 - (7 * fd * fd / 2 + 7 * rd * rd / 2);  // 28 in the centre, 90 in a corner
}

constexpr int kxk_push_close(int king_distance) { return 140 - 20 * king_distance; }

// The drive's whole span, both tables at their extremes, stays under one pawn
// (87 cp), so it orders positions of equal material and never outranks one.
static_assert((kxk_push_to_edge(Square(0)) + kxk_push_close(1)            // a1, kings touching
               - kxk_push_to_edge(Square(27)) - kxk_push_close(7))       // d4, kings apart
                  * 100 / KXK_DONOR_PAWN_EG
              < EG_MAT[PAWN],
              "the KQ-K/KR-K drive must span less than a pawn");

// The winner's score above its tier, White-independent (strong's view).
static int top_tier_score(const Board& b, Color strong) {
    const Color weak = ~strong;
    const int material = popcount(b.piece_bb(strong, PAWN))   * EG_MAT[PAWN]
                       + popcount(b.piece_bb(strong, KNIGHT)) * EG_MAT[KNIGHT]
                       + popcount(b.piece_bb(strong, BISHOP)) * EG_MAT[BISHOP]
                       + popcount(b.piece_bb(strong, ROOK))   * EG_MAT[ROOK]
                       + popcount(b.piece_bb(strong, QUEEN))  * EG_MAT[QUEEN];
    const int drive = (kxk_push_to_edge(b.king_square(weak))
                       + kxk_push_close(KING_DIST[b.king_square(strong)][b.king_square(weak)]))
                    * 100 / KXK_DONOR_PAWN_EG;
    return material + drive;
}

// ---- Rook-ending draw scaling: KRPKR and KRPPKRP ---------------------------
//
// These are SCALING functions, not verdicts. They do not claim a position is
// drawn; they shrink a material advantage the defender is known to be able to
// hold. BAS-E32 measured the defect: on the drawn subset of the 25k holdout we
// predict 0.802 in KRP-KR and 0.762 in KRPP-KRP where the truth is 0.5, while
// the symmetric classes KRP-KRP and KPP-KPP are already accurate. The failure
// is specifically the up-a-pawn case -- the evaluation sees +1 pawn and has no
// notion that Philidor exists.
//
// All squares are taken from the strong side's perspective via
// relative_square, so the rules below read as if the strong side were White.

// Passed for `c`, tested against the enemy pawns directly rather than the
// eval_pawns passed[] bitboard: apply_endgame also runs on the LAZY path,
// where that bitboard has never been computed.
static bool eg_pawn_passed(Color c, Square sq, Bitboard enemy_pawns) {
    const int f = int(file_of(sq));
    Bitboard mask = BB_FILES[f];
    if (f > int(FILE_A)) mask |= BB_FILES[f - 1];
    if (f < int(FILE_H)) mask |= BB_FILES[f + 1];
    const Rank rr = relative_rank(c, sq);
    for (Bitboard e = enemy_pawns & mask; e; ) {
        const Square es = Square(pop_lsb(e));
        if (relative_rank(c, es) > rr)
            return false;
    }
    return true;
}

// Indexed by the relative rank of the more advanced strong pawn. Ranks 1, 7
// and 8 are unreachable for a blockaded non-passer and are left at 0, which
// the caller treats as "do not scale" rather than as a dead draw.
static constexpr int KRPPKRP_SCALE[RANK_NB] = { 0, 9, 10, 14, 21, 44, 0, 0 };

static int krppkrp_scale(const Board& b, Color strong) {
    const Color weak = ~strong;
    const Bitboard strong_pawns = b.piece_bb(strong, PAWN);
    const Square p1 = Square(lsb(strong_pawns));
    const Square p2 = Square(msb(strong_pawns));

    // A passed pawn changes the ending completely; the defender's blockade
    // argument does not apply and the material edge is real.
    if (eg_pawn_passed(strong, p1, b.piece_bb(weak, PAWN))
        || eg_pawn_passed(strong, p2, b.piece_bb(weak, PAWN)))
        return -1;

    const Square wk = b.king_square(weak);
    const int r = int(std::max(relative_rank(strong, p1), relative_rank(strong, p2)));

    // The defending king must be blockading: within one file of both pawns and
    // in front of the more advanced one.
    if (std::abs(int(file_of(wk)) - int(file_of(p1))) <= 1
        && std::abs(int(file_of(wk)) - int(file_of(p2))) <= 1
        && int(relative_rank(strong, wk)) > r) {
        const int scale = KRPPKRP_SCALE[r];
        return scale ? scale : -1;
    }
    return -1;
}

// `rule`, when given, names the draw rule that fired (1 to 5 in the order
// below) or 0: what the hard-verdict instrument reads.
static int krpkr_scale(const Board& b, Color strong, int* rule = nullptr) {
    if (rule)
        *rule = 0;
    const Color weak = ~strong;
    const Square wk = relative_square(strong, b.king_square(strong));
    const Square bk = relative_square(strong, b.king_square(weak));
    const Square wr = relative_square(strong, Square(lsb(b.piece_bb(strong, ROOK))));
    const Square br = relative_square(strong, Square(lsb(b.piece_bb(weak, ROOK))));
    const Square wp = relative_square(strong, Square(lsb(b.piece_bb(strong, PAWN))));

    const int f = int(file_of(wp));
    const int r = int(rank_of(wp));
    const Square queening = make_square(File(f), RANK_8);
    // A tempo is worth a whole rank in these races, so every rule below that
    // compares king distances has to know who is to move.
    const int tempo = (b.turn() == strong) ? 1 : 0;

    const auto dist  = [](Square a, Square c) { return KING_DIST[a][c]; };
    const auto fdist = [](Square a, Square c) {
        return std::abs(int(file_of(a)) - int(file_of(c)));
    };

    // Philidor: pawn not yet on the sixth, defending king on the queening
    // square, defending rook cutting along the sixth rank.
    if (r <= int(RANK_5) && dist(bk, queening) <= 1 && wk <= H5
        && (rank_of(br) == RANK_6
            || (r <= int(RANK_3) && rank_of(wr) != RANK_6))) {
        if (rule)
            *rule = 1;
        return SCALE_DRAW;
    }

    // Pawn on the sixth, defender in front, checking from behind.
    if (r == int(RANK_6) && dist(bk, queening) <= 1
        && int(rank_of(wk)) + tempo <= int(RANK_6)
        && (rank_of(br) == RANK_1 || (!tempo && fdist(br, wp) >= 3))) {
        if (rule)
            *rule = 2;
        return SCALE_DRAW;
    }

    // Defending king ON the queening square with the rook checking from the
    // first rank: the attacker cannot both shield and advance.
    if (r >= int(RANK_6) && bk == queening && rank_of(br) == RANK_1
        && (!tempo || dist(wk, wp) >= 2)) {
        if (rule)
            *rule = 3;
        return SCALE_DRAW;
    }

    // The rook-pawn special case: pawn a7, rook a8, defending king boxed on
    // the short side. The attacking rook is entombed in front of its own pawn.
    if (wp == A7 && wr == A8 && (bk == H7 || bk == G7)
        && file_of(br) == FILE_A
        && (int(rank_of(br)) <= int(RANK_3) || int(file_of(wk)) >= int(FILE_D)
            || int(rank_of(wk)) <= int(RANK_5))) {
        if (rule)
            *rule = 4;
        return SCALE_DRAW;
    }

    // Defending king blockading the pawn with the attacking king too far away
    // to dislodge it.
    if (r <= int(RANK_5) && bk == wp + NORTH
        && dist(wk, wp) - tempo >= 2 && dist(wk, br) - tempo >= 2) {
        if (rule)
            *rule = 5;
        return SCALE_DRAW;
    }

    // Lucena-side: pawn on the seventh with its own rook behind it and the
    // attacking king close enough. Not a draw -- scale toward the full value,
    // less the king's distance from the queening square.
    if (r == int(RANK_7) && f != int(FILE_A) && int(file_of(wr)) == f
        && wr != queening
        && dist(wk, queening) < dist(bk, queening) - 2 + tempo
        && dist(wk, queening) < dist(bk, wr) + tempo)
        return SCALE_MAX - 2 * dist(wk, queening);

    // The same idea with the pawn further back.
    if (f != int(FILE_A) && int(file_of(wr)) == f && wr < wp
        && dist(wk, queening) < dist(bk, queening) - 2 + tempo
        && dist(wk, Square(wp + NORTH)) < dist(bk, Square(wp + NORTH)) - 2 + tempo
        && (dist(bk, wr) + tempo >= 3
            || (dist(wk, queening) < dist(bk, wr) + tempo
                && dist(wk, Square(wp + NORTH)) < dist(bk, wr) + tempo))) {
        const int v = SCALE_MAX - 8 * dist(wp, queening) - 2 * dist(wk, queening);
        return v > 0 ? v : 1;
    }

    // Pawn still low and the defending king somewhere in its path: drawish,
    // and more so the further away the attacking king is.
    if (r <= int(RANK_4) && bk > wp) {
        if (file_of(bk) == file_of(wp))
            return 10;
        if (fdist(bk, wp) == 1 && dist(wk, bk) > 2)
            return 24 - 2 * dist(wk, bk);
    }
    return -1;
}

// Returns a scale out of SCALE_NORMAL, or -1 for "this is not one of ours".
static int rook_ending_scale(const Board& b) {
    // Exactly one rook a side and nothing else but kings and pawns.
    if (popcount(b.piece_bb(WHITE, ROOK)) != 1 || popcount(b.piece_bb(BLACK, ROOK)) != 1)
        return -1;
    const int wp = popcount(b.piece_bb(WHITE, PAWN));
    const int bp = popcount(b.piece_bb(BLACK, PAWN));
    const Color strong = (wp >= bp) ? WHITE : BLACK;
    const int sp = std::max(wp, bp), dp = std::min(wp, bp);
    int scale = -1;
    if (sp == 1 && dp == 0)      scale = krpkr_scale(b, strong);
    else if (sp == 2 && dp == 1) scale = krppkrp_scale(b, strong);
    // Bound the discount. See SCALE_FLOOR: unfloored these rules both throw
    // wins the tablebase says are won and wreck the search's node counts.
    if (scale >= 0 && scale < SCALE_FLOOR) scale = SCALE_FLOOR;
    return scale;
}

int krpkr_scale_unfloored(const Board& b, Color strong, int& draw_rule) {
    return krpkr_scale(b, strong, &draw_rule);
}

int krppkrp_scale_unfloored(const Board& b, Color strong) {
    return krppkrp_scale(b, strong);
}

// ---- Endgame scaling + knowledge -------------------------------------------
// Receives the white-perspective tapered score and returns it after applying
// known-endgame overrides and draw scaling.
// Rule-50 eval damping: drain the static eval toward zero as the 50-move
// counter grows. A (100 - clock)/100 line retains only 1% at clock 99 --
// through RFP/null-margin/stand-pat that tells the search nearly every
// long-clock position is already drawn. The SF-era (1 - clock/199) curve
// retains ~50% at clock 99.
// Definition lives in eval.h so the Texel tuner uses the identical
// curve; see the shared-predicates block there.

// What the layer did to its score: left it, scaled it, or replaced it.
enum EndgameLayerKind : int { LAYER_SILENT, LAYER_SCALED, LAYER_OVERRIDE };

// `draw_verdict` is set when the 0 returned is one of the draw verdicts below
// (KNNK, the drawn KPK, the wrong-corner KBP-K, a pawnless side that cannot
// mate): a classification, which the search adds no correction to. `kind`
// says what the layer did. OcbStep runs the opposite-coloured-bishop scale;
// the rebuilt evaluation leaves it to the classical form's general factors.
// The rebuilt evaluation also skips the scaling steps on a score of exactly
// 0 (ScaleZero false): their strong side is Black at a tie whatever the
// colours, and a scaled 0 there would send a position and its mirror down
// different paths. The overrides do not read the score.
template<bool OcbStep, bool ScaleZero = true>
static int endgame_layer(const Board& b, int score, bool& draw_verdict, int& kind) {
    auto lone_king = [&](Color c) {
        return b.occupancy_bb(c) == sq_bb(b.king_square(c));
    };
    int total_pawns = popcount(b.piece_bb(WHITE, PAWN) | b.piece_bb(BLACK, PAWN));
    bool scaled = false;
    kind = LAYER_SILENT;
    TR_ENDGAME_STEP(EG_UNCHANGED, 1, 1);

    // ---- Step 3.10 guard: the known-endgame rules (KNNK / KPK / KBNK / KBP /
    // no-pawn-<=-minor scaling) all require a lone king or a pawnless side. If
    // neither holds, skip the whole census + known-endgame block and fall through
    // to OCB scaling. Behaviour-identical (none of those rules could fire), but it
    // avoids the per-node 12-popcount census in the opening/middlegame -- the cost
    // behind 3.5's fast-TC SPRT result.
    if (lone_king(WHITE) || lone_king(BLACK)
        || !b.piece_bb(WHITE, PAWN) || !b.piece_bb(BLACK, PAWN)) {
        int cnt[NCOLORS][PIECE_TYPE_NB];
        for (int c = 0; c < NCOLORS; c++)
            for (int pt = PAWN; pt <= QUEEN; pt++)
                cnt[c][pt] = popcount(b.piece_bb(c, pt));

        auto npm = [&](Color c) {
            return cnt[c][KNIGHT] * EG_MAT[KNIGHT] + cnt[c][BISHOP] * EG_MAT[BISHOP]
                 + cnt[c][ROOK]   * EG_MAT[ROOK]   + cnt[c][QUEEN]  * EG_MAT[QUEEN];
        };

        // ---- KNNK draw -----------------------------------------------------
        // Predicate shared with the tuner. It recomputes its own
        // popcounts rather than reading the cnt[] census; that census is still
        // needed by the KPK/KBNK/scaling rules below, and this path is already
        // gated behind the lone-king/pawnless guard.
        if (is_knnk_draw(b)) {
            draw_verdict = true;
            TR_ENDGAME_STEP(EG_OVERRIDE, 0, 1);
            kind = LAYER_OVERRIDE;
            return 0;
        }

        // ---- KPK: exact bitbase classification -----------------------------
        // A won KPK scores as a known win plus the donor's endgame pawn value
        // and the pawn's rank (Stockfish's Endgame<KPK>). Its tapered score
        // could favour the bare king, and the pawnless scaler then scaled the
        // win to 0.
        if (total_pawns == 1 && npm(WHITE) == 0 && npm(BLACK) == 0) {
            Color strong = cnt[WHITE][PAWN] ? WHITE : BLACK;
            if (lone_king(~strong)) {
                TR_ENDGAME_STEP(EG_OVERRIDE, 0, 1);
            kind = LAYER_OVERRIDE;
                if (!kpk_strong_wins(b, strong)) {
                    draw_verdict = true;
                    return 0; // drawn KPK
                }
                const Square pawn = Square(lsb(b.piece_bb(strong, PAWN)));
                const int v = KNOWN_WIN + KPK_PAWN_EG + int(relative_rank(strong, pawn));
                return (strong == WHITE) ? v : -v;
            }
        }

        // ---- Bare king against a queen or rook: the top tiers -------------
        // A tier holds top_tier_score above its base: the winner's frozen
        // material and the donor's KXK drive, which the search converts these
        // mates with. What the tiers add is the order: a queen outranks a
        // rook, and both outrank every minor-piece mate, so a promotion to a
        // queen always beats one to a knight or bishop, and shedding never
        // gains.
        for (int s = 0; s < NCOLORS; s++) {
            const Color strong = Color(s), weak = ~strong;
            if (!lone_king(weak) || (!cnt[strong][QUEEN] && !cnt[strong][ROOK]))
                continue;
            TR_ENDGAME_STEP(EG_OVERRIDE, 0, 1);
            kind = LAYER_OVERRIDE;
            const int above = top_tier_score(b, strong);
            const int v = cnt[strong][QUEEN]
                ? BARE_KING_QUEEN_TIER + std::min(above, BARE_KING_CAP - BARE_KING_QUEEN_TIER)
                : BARE_KING_ROOK_TIER + std::min(above, BARE_KING_QUEEN_TIER - 1 - BARE_KING_ROOK_TIER);
            return (strong == WHITE) ? v : -v;
        }

        // The minor-piece mates below keep their drives, take the same pawn
        // bonus, and stay under the rook tier.
        auto minor_tier = [&](int drive_score, Color strong) {
            TR_ENDGAME_STEP(EG_OVERRIDE, 0, 1);
            kind = LAYER_OVERRIDE;
            const int v = std::min(std::abs(drive_score) + cnt[strong][PAWN] * BARE_KING_PAWN_STEP,
                                   BARE_KING_ROOK_TIER - 1);
            return (strong == WHITE) ? v : -v;
        };

        // ---- KBNK: bishop + knight vs bare king is a win; drive to corner --
        for (int s = 0; s < NCOLORS; s++) {
            Color strong = Color(s), weak = ~strong;
            if (lone_king(weak) && cnt[strong][KNIGHT] == 1
                && cnt[strong][BISHOP] == 1 && !cnt[strong][ROOK] && !cnt[strong][QUEEN])
                return minor_tier(kbnk_score(b, strong), strong);
        }

        // ---- KXK: any other bare-king mate ---------------------------------
        // Must follow KBNK, which keeps its own colour-bound corner logic.
        // Two same-coloured bishops cannot mate, so the pair is tested by
        // square colour rather than by count.
        for (int s = 0; s < NCOLORS; s++) {
            Color strong = Color(s), weak = ~strong;
            if (!lone_king(weak))
                continue;
            const Bitboard bb = b.piece_bb(strong, BISHOP);
            const bool bishop_pair = (bb & EG_DARK_SQUARES) && (bb & ~EG_DARK_SQUARES);
            if (bishop_pair || (cnt[strong][BISHOP] && cnt[strong][KNIGHT]))
                return minor_tier(kxk_score(b, strong), strong);
        }

        // ---- KBP(s) vs K with the wrong rook-file bishop is a draw ---------
        for (int s = 0; s < NCOLORS; s++) {
            Color strong = Color(s), weak = ~strong;
            if (!lone_king(weak) || cnt[strong][PAWN] == 0 || cnt[strong][BISHOP] == 0)
                continue;
            if (cnt[strong][KNIGHT] || cnt[strong][ROOK] || cnt[strong][QUEEN])
                continue;
            Bitboard pawns = b.piece_bb(strong, PAWN);
            bool all_a = (pawns & ~BB_FILES[FILE_A]) == 0;
            bool all_h = (pawns & ~BB_FILES[FILE_H]) == 0;
            if (!all_a && !all_h)
                continue;
            Square promo = all_a ? (strong == WHITE ? A8 : A1)
                                 : (strong == WHITE ? H8 : H1);
            bool promo_dark = (sq_bb(promo) & EG_DARK_SQUARES) != 0;
            // Wrong bishop = no bishop controls the promotion-square colour.
            Bitboard right_mask = promo_dark ? EG_DARK_SQUARES : ~EG_DARK_SQUARES;
            bool has_right_bishop = (b.piece_bb(strong, BISHOP) & right_mask) != 0;
            if (has_right_bishop)
                continue;
            if (KING_DIST[b.king_square(weak)][promo] <= 1) {
                draw_verdict = true;
                TR_ENDGAME_STEP(EG_OVERRIDE, 0, 1);
            kind = LAYER_OVERRIDE;
                return 0; // defender holds the wrong-corner draw
            }
        }

        // ---- Winning side has no pawns and at most a minor-piece advantage:
        // it cannot force mate (KmK, KmKm, KRKm, ...), in Stockfish's form
        // (pawnless_scale). The winning side is the one the score favours, as
        // in Stockfish; by material totals, equal totals always made it White.
        //
        // Its 0 is a draw verdict only when neither side has a pawn. Where the
        // other side has pawns, the side the score favours cannot win, but the
        // other often can (its pawn runs): the tables call 2.6% to 7.9% of such
        // positions wins for it, and a verdict would take the search's
        // correction away from them. The score stays 0, an estimate.
        if (ScaleZero || score != 0) {
            const Color strong = (score > 0) ? WHITE : BLACK;
            const int scale = pawnless_scale(b, strong);
            if (scale != SCALE_NORMAL) {
                score = score * scale / SCALE_NORMAL;
                scaled = true;
                TR_ENDGAME_STEP(EG_SCALED, scale, SCALE_NORMAL);
                kind = LAYER_SCALED;
                draw_verdict = scale == 0 && !b.piece_bb(~strong, PAWN);
            }
        }
    }

    // ---- Rook endings: KRPKR / KRPPKRP draw scaling -----------------------
    // KRPKR reaches here through the census block above (the defender is
    // pawnless); KRPPKRP does NOT, because both sides have pawns, so the test
    // lives out here. The gate is a few bitboard ORs rather than a popcount
    // census: this runs on every node, and BAS-E34 measured what a census on
    // the middlegame path costs (+20.5% bench nodes).
    if (!scaled && (ScaleZero || score != 0)) {
        const Bitboard minors_queens =
              b.piece_bb(WHITE, KNIGHT) | b.piece_bb(WHITE, BISHOP) | b.piece_bb(WHITE, QUEEN)
            | b.piece_bb(BLACK, KNIGHT) | b.piece_bb(BLACK, BISHOP) | b.piece_bb(BLACK, QUEEN);
        if (!minors_queens) {
            const int scale = rook_ending_scale(b);
            if (scale >= 0) {
                score = score * scale / SCALE_NORMAL;
                scaled = true;
                TR_ENDGAME_STEP(EG_SCALED, scale, SCALE_NORMAL);
                kind = LAYER_SCALED;
            }
        }
    }

    // Opposite-coloured bishops: draw-scale toward 32/48, relaxed to at most
    // neutral by pawn count (capped -- see ocb_draw_scale in eval.h).
    if (OcbStep && !scaled && is_opposite_coloured_bishops(b)) {   // predicate shared with the tuner
        score = score * ocb_draw_scale(total_pawns) / 48;
        TR_ENDGAME_STEP(EG_SCALED, ocb_draw_scale(total_pawns), 48);
        kind = LAYER_SCALED;
    }

    return score;
}

// The accepted evaluation's layer, its opposite-bishop step included.
[[maybe_unused]] static int apply_endgame(const Board& b, int score, bool& draw_verdict) {
    int kind;
    return endgame_layer<true>(b, score, draw_verdict, kind);
}

// The mate drive's endgame addition for a White-view score in centipawns,
// at the piece-count phase; 0 where its gate is shut.
static int mate_drive_eg(const Board& b, int score_approx, int phase) {
    int eg = 0;
    // ---- Endgame mate-drive (frozen) ---------------------------------------
    if (phase <= 6) {
        if (std::abs(score_approx) > 200) {
            Color winning = (score_approx > 0) ? WHITE : BLACK;
            Color losing  = ~winning;
            // Gate the push-to-edge / close-the-kings geometry to
            // bare-king mating signatures. It is wrong in pawn races, rook-
            // and-pawn endings, and king-opposition pawn endings (audit
            // hce_analysis 7). Fire only when the defender has no pawns AND
            // the attacker holds material that can force mate against a lone
            // king (Q, R, bishop pair, or B+N). KBNK/KNNK stay handled by
            // apply_endgame's overrides, so this gate never touches them.
            const bool losing_pawnless = b.piece_bb(losing, PAWN) == 0;
            const bool win_can_mate =
                   b.piece_bb(winning, QUEEN) != 0
                || b.piece_bb(winning, ROOK)  != 0
                || more_than_one(b.piece_bb(winning, BISHOP))
                || (b.piece_bb(winning, BISHOP) != 0 && b.piece_bb(winning, KNIGHT) != 0);
            if (losing_pawnless && win_can_mate) {
                int sign = (winning == WHITE) ? 1 : -1;
                Square wksq = b.king_square(winning);
                Square lksq = b.king_square(losing);
                int lk_center = std::max(3 - file_of(lksq), file_of(lksq) - 4)
                              + std::max(3 - rank_of(lksq), rank_of(lksq) - 4);
                int king_dist = KING_DIST[wksq][lksq];
                eg += sign * (5 * lk_center + (14 - king_dist) * 4);
                TR_REST_EG(MATE_DRIVE, sign * (5 * lk_center + (14 - king_dist) * 4));
                // Frozen: not traced; captured in rest by the tuner.
                // Bare-king positions do not reach this -- they are
                // overridden by kxk_score in apply_endgame. What is left here
                // is the case the gate's comment does not describe: the
                // defender is PAWNLESS but still has material (KQ-KR and
                // friends). That is a heuristic, not a forced mate, so a
                // margin-beating weight would make the engine prefer chasing
                // the king to converting. Those classes need their own drives;
                // the 5 and 4 stay until then.
            }
        }
    }
    return eg;
}

// The mate drive reads the running sum after every family and before the
// tempo; what it returns is added to the endgame half.
[[maybe_unused]] static EvalScore mate_drive(const Board& b, const EvalScore& sum, int phase) {
    if (phase > 6)
        return {0, 0};
    return {0, mate_drive_eg(b, taper(sum, phase), phase)};
}
