#include "eval.h"
#include "attacks.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <mutex>
#ifdef BASILISK_TUNE
#include <climits>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#endif

// ---- Texel trace thread-local storage and macros --------------------------
#ifdef TEXEL_TRACE
thread_local EvalTrace g_trace{};

// Precomputed flat base offsets for PST groups indexed by PieceType [0..6].
// PieceType 0 = no piece; valid piece types 1=PAWN .. 6=KING.
static constexpr int PST_MG_BASE[7] = {
    -1,
    eval_param_offset(EPG_PstMgPawn),
    eval_param_offset(EPG_PstMgKnight),
    eval_param_offset(EPG_PstMgBishop),
    eval_param_offset(EPG_PstMgRook),
    eval_param_offset(EPG_PstMgQueen),
    eval_param_offset(EPG_PstMgKing),
};
static constexpr int PST_EG_BASE[7] = {
    -1,
    eval_param_offset(EPG_PstEgPawn),
    eval_param_offset(EPG_PstEgKnight),
    eval_param_offset(EPG_PstEgBishop),
    eval_param_offset(EPG_PstEgRook),
    eval_param_offset(EPG_PstEgQueen),
    eval_param_offset(EPG_PstEgKing),
};

// TR macros: record signed count contribution to mg or eg array.
// Caller must zero g_trace before calling evaluate().
#define TR_MG(group, idx, val) \
    (g_trace.mg[eval_param_offset(EPG_##group) + (idx)] += static_cast<int16_t>(val))
#define TR_EG(group, idx, val) \
    (g_trace.eg[eval_param_offset(EPG_##group) + (idx)] += static_cast<int16_t>(val))
#define TR_BOTH(group, idx, val) \
    do { TR_MG(group, idx, val); TR_EG(group, idx, val); } while(0)
#define TR_PST_MG(pt, sq, val) \
    (g_trace.mg[PST_MG_BASE[pt] + (sq)] += static_cast<int16_t>(val))
#define TR_PST_EG(pt, sq, val) \
    (g_trace.eg[PST_EG_BASE[pt] + (sq)] += static_cast<int16_t>(val))

// Reconstruction: linear tapered score from trace counts and weights.
int reconstruct(const EvalTrace& tr, const EvalParams& w) {
    int mg_dot = 0, eg_dot = 0;
#define X(name, member, len) \
    { const int* wptr = eval_param_cptr(w.member); \
      int base = eval_param_offset(EPG_##name); \
      for (int i = 0; i < (len); i++) { \
          mg_dot += tr.mg[base + i] * wptr[i]; \
          eg_dot += tr.eg[base + i] * wptr[i]; \
      } }
    EVAL_PARAM_LIST(X)
#undef X
    return (mg_dot * tr.phase + eg_dot * (24 - tr.phase)) / 24;
}

#else // !TEXEL_TRACE
#define TR_MG(group, idx, val)   (void)0
#define TR_EG(group, idx, val)   (void)0
#define TR_BOTH(group, idx, val) (void)0
#define TR_PST_MG(pt, sq, val)   (void)0
#define TR_PST_EG(pt, sq, val)   (void)0
#endif // TEXEL_TRACE

// Global evaluation parameters (defaults in EvalParams.h).
// Call init_eval_tables(g_eval_params) again after changing these.
EvalParams g_eval_params;

// Phase weights (frozen; not in EvalParams)
static constexpr int PHASE_W[PIECE_TYPE_NB] = {0, 0, 1, 1, 2, 4, 0};
static constexpr int TOTAL_PHASE = 24;

// Runtime tables baked from g_eval_params in init_eval_tables().
static int MG_TABLE[NCOLORS][PIECE_TYPE_NB][SQUARE_NB];
static int EG_TABLE[NCOLORS][PIECE_TYPE_NB][SQUARE_NB];

static Square forward_square(Color c, Square sq) {
    const int to = int(sq) + (c == WHITE ? 8 : -8);
    return (to >= 0 && to < 64) ? Square(to) : SQ_NONE;
}

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
static constexpr int KNOWN_WIN    = 10000; // static "won, mate is technique" magnitude

// Lazy-eval margin (Step 3.11): if the cheap (material/PST/imbalance/pawns/minor)
// tapered score exceeds this, the expensive positional block is skipped. Tuned
// under a lazy-on-vs-off SPRT; conservative start.
[[maybe_unused]] static constexpr int LAZY_MARGIN  = 700;  // unused under TEXEL_TRACE (lazy eval off there)
// 8.6.6b lazy-audit "did not fire" marker (any value the eval can't produce).
[[maybe_unused]] static constexpr int VALUE_NONE_SENTINEL = INT_MIN;

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
    Square psq = Square(lsb(b.pieces[strong][PAWN]));
    Square wk  = b.king_sq[strong];
    Square bk  = b.king_sq[weak];
    int stm    = (b.side_to_move == strong) ? 0 : 1;

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
static int kbnk_score(const Board& b, Color strong) {
    Color  weak = ~strong;
    Square sk   = b.king_sq[strong];
    Square wksq = b.king_sq[weak];
    Square bsq  = Square(lsb(b.pieces[strong][BISHOP]));

    // The two same-coloured corners as the bishop are a diagonal pair.
    bool dark_bishop = (sq_bb(bsq) & EG_DARK_SQUARES) != 0;
    Square c1, c2;
    if (dark_bishop) { c1 = A1; c2 = H8; }   // dark corners
    else             { c1 = A8; c2 = H1; }   // light corners

    int corner_dist = std::min(KING_DIST[wksq][c1], KING_DIST[wksq][c2]);
    int king_dist   = KING_DIST[sk][wksq];

    int v = KNOWN_WIN + (7 - corner_dist) * 250 + (8 - king_dist) * 30;
    return (strong == WHITE) ? v : -v;
}

// ---- Endgame scaling + knowledge -------------------------------------------
// Receives the white-perspective tapered score and returns it after applying
// known-endgame overrides and draw scaling. Reproduces the previous OCB and
// KNNK behaviour exactly for every position they used to touch.
// Rule-50 eval damping (8.4 retry): drain the static eval toward zero as the
// 50-move counter grows. The old (100 - clock)/100 line retained only 1% at
// clock 99 -- through RFP/null-margin/stand-pat that told the search nearly
// every long-clock position was already drawn. The SF-era (1 - clock/199)
// curve retains ~50% at clock 99. Reopened after the 2026-07-15 canary fix
// (the brittle KBNK conversion gate wrongly reverted this); SPRT gates it.
// Definition lives in eval.h (8.6.2b) so the Texel tuner uses the identical
// curve; see the shared-predicates block there.

static int apply_endgame(const Board& b, int score) {
    auto lone_king = [&](Color c) {
        return b.occupancy[c] == sq_bb(b.king_sq[c]);
    };
    int total_pawns = popcount(b.pieces[WHITE][PAWN] | b.pieces[BLACK][PAWN]);
    bool scaled = false;

    // ---- Step 3.10 guard: the known-endgame rules (KNNK / KPK / KBNK / KBP /
    // no-pawn-<=-minor scaling) all require a lone king or a pawnless side. If
    // neither holds, skip the whole census + known-endgame block and fall through
    // to OCB scaling. Behaviour-identical (none of those rules could fire), but it
    // avoids the per-node 12-popcount census in the opening/middlegame -- the cost
    // behind 3.5's fast-TC SPRT result.
    if (lone_king(WHITE) || lone_king(BLACK)
        || !b.pieces[WHITE][PAWN] || !b.pieces[BLACK][PAWN]) {
        int cnt[NCOLORS][PIECE_TYPE_NB];
        for (int c = 0; c < NCOLORS; c++)
            for (int pt = PAWN; pt <= QUEEN; pt++)
                cnt[c][pt] = popcount(b.pieces[c][pt]);

        auto npm = [&](Color c) {
            return cnt[c][KNIGHT] * EG_MAT[KNIGHT] + cnt[c][BISHOP] * EG_MAT[BISHOP]
                 + cnt[c][ROOK]   * EG_MAT[ROOK]   + cnt[c][QUEEN]  * EG_MAT[QUEEN];
        };

        // ---- KNNK draw -----------------------------------------------------
        // Predicate shared with the tuner (8.6.2b). It recomputes its own
        // popcounts rather than reading the cnt[] census; that census is still
        // needed by the KPK/KBNK/scaling rules below, and this path is already
        // gated behind the lone-king/pawnless guard.
        if (is_knnk_draw(b)) return 0;

        // ---- KPK: exact bitbase classification -----------------------------
        if (total_pawns == 1 && npm(WHITE) == 0 && npm(BLACK) == 0) {
            Color strong = cnt[WHITE][PAWN] ? WHITE : BLACK;
            if (lone_king(~strong) && !kpk_strong_wins(b, strong))
                return 0; // drawn KPK
            // Won KPK keeps its (already winning) tapered score.
        }

        // ---- KBNK: bishop + knight vs bare king is a win; drive to corner --
        for (int s = 0; s < NCOLORS; s++) {
            Color strong = Color(s), weak = ~strong;
            if (lone_king(weak) && !cnt[strong][PAWN] && cnt[strong][KNIGHT] == 1
                && cnt[strong][BISHOP] == 1 && !cnt[strong][ROOK] && !cnt[strong][QUEEN])
                return kbnk_score(b, strong);
        }

        // ---- KBP(s) vs K with the wrong rook-file bishop is a draw ---------
        for (int s = 0; s < NCOLORS; s++) {
            Color strong = Color(s), weak = ~strong;
            if (!lone_king(weak) || cnt[strong][PAWN] == 0 || cnt[strong][BISHOP] == 0)
                continue;
            if (cnt[strong][KNIGHT] || cnt[strong][ROOK] || cnt[strong][QUEEN])
                continue;
            Bitboard pawns = b.pieces[strong][PAWN];
            bool all_a = (pawns & ~BB_FILES[FILE_A]) == 0;
            bool all_h = (pawns & ~BB_FILES[FILE_H]) == 0;
            if (!all_a && !all_h)
                continue;
            Square promo = all_a ? (strong == WHITE ? A8 : A1)
                                 : (strong == WHITE ? H8 : H1);
            bool promo_dark = (sq_bb(promo) & EG_DARK_SQUARES) != 0;
            // Wrong bishop = no bishop controls the promotion-square colour.
            Bitboard right_mask = promo_dark ? EG_DARK_SQUARES : ~EG_DARK_SQUARES;
            bool has_right_bishop = (b.pieces[strong][BISHOP] & right_mask) != 0;
            if (has_right_bishop)
                continue;
            if (KING_DIST[b.king_sq[weak]][promo] <= 1)
                return 0; // defender holds the wrong-corner draw
        }

        // ---- Winning side has no pawns and at most a minor-piece advantage:
        // it cannot force mate (KmK, KmKm, KRKm, ...). Standard SF heuristic.
        {
            int w_tot = npm(WHITE) + cnt[WHITE][PAWN] * EG_MAT[PAWN];
            int b_tot = npm(BLACK) + cnt[BLACK][PAWN] * EG_MAT[PAWN];
            Color strong = (w_tot >= b_tot) ? WHITE : BLACK;
            if (cnt[strong][PAWN] == 0
                && npm(strong) - npm(~strong) <= EG_MAT[BISHOP]) {
                int scale;
                if (npm(strong) < EG_MAT[ROOK])
                    scale = SCALE_DRAW;                   // KmK, KmKm: dead draw
                else
                    scale = (npm(~strong) > 0) ? 4 : 14;  // KRKm etc.: near draw
                score = score * scale / SCALE_NORMAL;
                scaled = true;
            }
        }
    }

    // Opposite-coloured bishops: draw-scale toward 32/48, relaxed to at most
    // neutral by pawn count (8.3: capped -- see ocb_draw_scale in eval.h).
    if (!scaled && is_opposite_coloured_bishops(b)) {   // predicate shared with the tuner (8.6.2b)
        score = score * ocb_draw_scale(total_pawns) / 48;
    }

    return score;
}

// 5.9.6a: empty-board bishop rays. bishop_xray_pawn wants the diagonals a
// bishop would command with nothing in the way, which does not depend on the
// position at all -- so the magic lookup it was doing per bishop per eval is
// pure overhead. Filled by init_eval_tables (after init_attacks).
static Bitboard EVAL_BISHOP_EMPTY[SQUARE_NB];

void init_eval_tables(const EvalParams& p) {
    for (int sq = 0; sq < SQUARE_NB; sq++)
        EVAL_BISHOP_EMPTY[sq] = bishop_attacks(Square(sq), 0ULL);

    for (int pt = PAWN; pt <= KING; pt++) {
        for (int sq = 0; sq < 64; sq++) {
            MG_TABLE[WHITE][pt][sq] = p.mg_val[pt] + p.pst_mg[pt - 1][sq];
            EG_TABLE[WHITE][pt][sq] = p.eg_val[pt] + p.pst_eg[pt - 1][sq];
            // Black: mirror rank
            int msq = sq ^ 56;
            MG_TABLE[BLACK][pt][sq] = p.mg_val[pt] + p.pst_mg[pt - 1][msq];
            EG_TABLE[BLACK][pt][sq] = p.eg_val[pt] + p.pst_eg[pt - 1][msq];
        }
    }
}

Evaluator::Evaluator() {
    clear_pawn_table();
}

void Evaluator::clear_pawn_table() {
    std::memset(pawn_table_, 0, sizeof(pawn_table_));
}

// ---- Pawn structure evaluation (cached) ------------------------------------

void Evaluator::eval_pawns(const Board& b,
                           int& mg_out, int& eg_out,
                           Bitboard passed[NCOLORS],
                           Bitboard attacks[NCOLORS]) {
#ifndef TEXEL_TRACE
    // Cache lookup: skip recomputation if pawn structure matches.
    Key pkey = b.pawn_key;
    PawnEntry& pe = pawn_table_[pkey & (PAWN_TABLE_SIZE - 1)];

    ++pawn_probes;   // 8.7.1(c)
    if (pe.key == pkey) {
        ++pawn_hits;   // 8.7.1(c)
        mg_out = pe.mg;
        eg_out = pe.eg;
        passed[WHITE]  = pe.passed[WHITE];
        passed[BLACK]  = pe.passed[BLACK];
        attacks[WHITE] = pe.attacks[WHITE];
        attacks[BLACK] = pe.attacks[BLACK];
        return;
    }
#endif

    const EvalParams& p = g_eval_params;
    int mg = 0, eg = 0;

    for (int c = 0; c < NCOLORS; c++) {
        Color us   = Color(c);
        Color them = ~us;
        int sign   = (us == WHITE) ? 1 : -1;

        Bitboard our_pawns   = b.pieces[us][PAWN];
        Bitboard their_pawns = b.pieces[them][PAWN];

        // Pawn attack map
        Bitboard pawn_atk = 0;
        Bitboard tmp = our_pawns;
        while (tmp) {
            int sq = pop_lsb(tmp);
            pawn_atk |= PawnAttacks[us][sq];
        }
        attacks[c] = pawn_atk;

        passed[c] = 0;
        tmp = our_pawns;
        while (tmp) {
            int sq = pop_lsb(tmp);
            int f = file_of(Square(sq));
            int r = rank_of(Square(sq));

            // Passed pawn: no enemy pawns in front on same or adjacent files
            if (!(BB_PASSED_PAWN_MASK[us][sq] & their_pawns)) {
                passed[c] |= sq_bb(Square(sq));
                int rel_r = (us == WHITE) ? r : 7 - r;
                mg += sign * p.passed_mg[rel_r]; TR_MG(PassedMg, rel_r, sign);
                eg += sign * p.passed_eg[rel_r]; TR_EG(PassedEg, rel_r, sign);
            }

            Bitboard file_bb = BB_FILES[f];
            Bitboard adj_bb  = BB_ADJACENT_FILES[f];

            // Doubled pawns
            if (more_than_one(our_pawns & file_bb)) {
                mg += sign * p.doubled_mg; TR_MG(DoubledMg, 0, sign);
                eg += sign * p.doubled_eg; TR_EG(DoubledEg, 0, sign);
            }

            // Isolated pawns
            if (!(our_pawns & adj_bb)) {
                mg += sign * p.isolated_mg; TR_MG(IsolatedMg, 0, sign);
                eg += sign * p.isolated_eg; TR_EG(IsolatedEg, 0, sign);
            }

            // Connected pawns (supported by another pawn)
            if (PawnAttacks[them][sq] & our_pawns) {
                mg += sign * p.connected_mg; TR_MG(ConnectedMg, 0, sign);
                eg += sign * p.connected_eg; TR_EG(ConnectedEg, 0, sign);
            }

            // Backward pawn
            bool backward = false;
            if (!(our_pawns & BB_PASSED_PAWN_MASK[them][sq] & adj_bb)) {
                int stop_sq = (us == WHITE) ? sq + 8 : sq - 8;
                if (stop_sq >= 0 && stop_sq < 64) {
                    if (PawnAttacks[us][stop_sq] & their_pawns) {
                        backward = true;
                        mg += sign * p.backward_mg; TR_MG(BackwardMg, 0, sign);
                        eg += sign * p.backward_eg; TR_EG(BackwardEg, 0, sign);
                    }
                }
            }

            // ---- Step 3.4 pawn-structure refinement (pawn-only, seeded 0) ----
            int rel_r = (us == WHITE) ? r : 7 - r;
            // Connected/phalanx bonus by rank (supported by, or side-by-side with, a friendly pawn).
            bool supported = (PawnAttacks[them][sq] & our_pawns) != 0;
            bool phalanx   = (our_pawns & adj_bb & BB_RANKS[r]) != 0;
            if (supported || phalanx) {
                mg += sign * p.connected_rank_mg[rel_r]; TR_MG(ConnectedRankMg, rel_r, sign);
                eg += sign * p.connected_rank_eg[rel_r]; TR_EG(ConnectedRankEg, rel_r, sign);
            }
            // Weak (isolated or backward) pawn on a half-open file (unopposed).
            bool isolated_p = !(our_pawns & adj_bb);
            if ((isolated_p || backward) && !(their_pawns & file_bb)) {
                mg += sign * p.weak_unopposed_mg; TR_MG(WeakUnopposedMg, 0, sign);
                eg += sign * p.weak_unopposed_eg; TR_EG(WeakUnopposedEg, 0, sign);
            }
            // Rammed: own pawn directly blocked by an enemy pawn, on relative rank 5 / 6.
            {
                int ahead = (us == WHITE) ? sq + 8 : sq - 8;
                if (ahead >= 0 && ahead < 64 && (their_pawns & sq_bb(Square(ahead)))) {
                    if (rel_r == 4) {
                        mg += sign * p.blocked_pawn_mg[0]; TR_MG(BlockedPawnMg, 0, sign);
                        eg += sign * p.blocked_pawn_eg[0]; TR_EG(BlockedPawnEg, 0, sign);
                    } else if (rel_r == 5) {
                        mg += sign * p.blocked_pawn_mg[1]; TR_MG(BlockedPawnMg, 1, sign);
                        eg += sign * p.blocked_pawn_eg[1]; TR_EG(BlockedPawnEg, 1, sign);
                    }
                }
            }
        }

        // Pawn majorities (queenside files a-d, kingside e-h) - breakthrough potential.
        {
            const Bitboard qs = BB_FILES[FILE_A] | BB_FILES[FILE_B] | BB_FILES[FILE_C] | BB_FILES[FILE_D];
            const Bitboard ks = BB_FILES[FILE_E] | BB_FILES[FILE_F] | BB_FILES[FILE_G] | BB_FILES[FILE_H];
            if (popcount(our_pawns & qs) > popcount(their_pawns & qs)) {
                mg += sign * p.pawn_majority_mg; TR_MG(PawnMajorityMg, 0, sign);
                eg += sign * p.pawn_majority_eg; TR_EG(PawnMajorityEg, 0, sign);
            }
            if (popcount(our_pawns & ks) > popcount(their_pawns & ks)) {
                mg += sign * p.pawn_majority_mg; TR_MG(PawnMajorityMg, 0, sign);
                eg += sign * p.pawn_majority_eg; TR_EG(PawnMajorityEg, 0, sign);
            }
        }

        // Pawn islands: disconnected own-pawn file groups (Step 3.9, seeded 0).
        {
            int islands = 0;
            bool prev = false;
            for (int f = FILE_A; f <= FILE_H; f++) {
                bool has = (our_pawns & BB_FILES[f]) != 0;
                if (has && !prev) islands++;
                prev = has;
            }
            mg += sign * islands * p.pawn_islands_mg; TR_MG(PawnIslandsMg, 0, sign * islands);
            eg += sign * islands * p.pawn_islands_eg; TR_EG(PawnIslandsEg, 0, sign * islands);
        }
    }

#ifndef TEXEL_TRACE
    pe.key            = pkey;
    pe.mg             = mg;
    pe.eg             = eg;
    pe.passed[WHITE]  = passed[WHITE];
    pe.passed[BLACK]  = passed[BLACK];
    pe.attacks[WHITE] = attacks[WHITE];
    pe.attacks[BLACK] = attacks[BLACK];
#endif

    mg_out = mg;
    eg_out = eg;
}

// ---- Main evaluation -------------------------------------------------------

int Evaluator::evaluate(const Board& b) {
    ++eval_calls;   // 8.7.1(c)
    const EvalParams& p = g_eval_params;
    int mg = 0, eg = 0;
    int phase = 0;

    // ---- Material and PST ----
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        for (int pt = PAWN; pt <= KING; pt++) {
            Bitboard bb = b.pieces[c][pt];
            phase += popcount(bb) * PHASE_W[pt];
            while (bb) {
                int sq = pop_lsb(bb);
                mg += sign * MG_TABLE[c][pt][sq];
                eg += sign * EG_TABLE[c][pt][sq];
                TR_MG(MgVal, pt, sign);
                TR_EG(EgVal, pt, sign);
#ifdef TEXEL_TRACE
                {
                    int pst_sq = (c == WHITE) ? sq : (sq ^ 56);
                    TR_PST_MG(pt, pst_sq, sign);
                    TR_PST_EG(pt, pst_sq, sign);
                }
#endif
            }
        }
    }
    phase = std::min(phase, TOTAL_PHASE);
#ifdef TEXEL_TRACE
    g_trace.phase = phase;
#endif

    // ---- Material imbalance (Step 3.8; SF-style quadratic, seeded inert) ----
    // Linear in its coefficients (feature = piece-count products), so traced
    // normally and fit by the linear tuner in Phase 4. All coefficients seeded 0
    // -> contributes exactly 0 today (bench unchanged, --verify exact). Added
    // equally to mg and eg (a non-tapered material correction).
    {
        int ic[NCOLORS][6];
        for (int c = 0; c < NCOLORS; c++) {
            ic[c][0] = (popcount(b.pieces[c][BISHOP]) >= 2) ? 1 : 0;  // bishop pair
            ic[c][1] = popcount(b.pieces[c][PAWN]);
            ic[c][2] = popcount(b.pieces[c][KNIGHT]);
            ic[c][3] = popcount(b.pieces[c][BISHOP]);
            ic[c][4] = popcount(b.pieces[c][ROOK]);
            ic[c][5] = popcount(b.pieces[c][QUEEN]);
        }
        int imbalance = 0;
        for (int i = 0; i < 6; i++) {
            int lin_cnt = ic[WHITE][i] - ic[BLACK][i];
            imbalance += p.imb_linear[i] * lin_cnt;
            TR_BOTH(ImbLinear, i, lin_cnt);
            for (int j = 0; j <= i; j++) {
                int t = i * (i + 1) / 2 + j;
                int our_cnt   = ic[WHITE][i] * ic[WHITE][j] - ic[BLACK][i] * ic[BLACK][j];
                int their_cnt = ic[WHITE][i] * ic[BLACK][j] - ic[BLACK][i] * ic[WHITE][j];
                imbalance += p.imb_our[t]   * our_cnt;   TR_BOTH(ImbOur,   t, our_cnt);
                imbalance += p.imb_their[t] * their_cnt; TR_BOTH(ImbTheir, t, their_cnt);
            }
        }
        mg += imbalance;
        eg += imbalance;
    }

    // ---- Pawn structure (cached) ----
    Bitboard passed[NCOLORS] = {};
    Bitboard pawn_atk[NCOLORS] = {};
    int pmg = 0, peg = 0;
    eval_pawns(b, pmg, peg, passed, pawn_atk);
    mg += pmg;
    eg += peg;

    // ---- Dynamic passed-pawn terms (non-pawn occupancy; outside pawn hash) --
    for (int c = 0; c < NCOLORS; c++) {
        Color us   = Color(c);
        Color them = ~us;
        int sign   = (us == WHITE) ? 1 : -1;
        Bitboard pp = passed[c];
        while (pp) {
            Square psq = Square(pop_lsb(pp));
            Square stop = forward_square(us, psq);
            if (stop == SQ_NONE || (b.all_occ & sq_bb(stop)))
                continue;

            int rel_r = relative_rank(us, psq);
            mg += sign * (rel_r * p.pass_free_mg); TR_MG(PassFreeMg, 0, sign * rel_r);
            eg += sign * (rel_r * p.pass_free_eg); TR_EG(PassFreeEg, 0, sign * rel_r);

            if (!b.is_attacked_by(stop, b.all_occ, them)) {
                eg += sign * (rel_r * p.pass_safe_eg); TR_EG(PassSafeEg, 0, sign * rel_r);
            }
        }
    }

    // ---- Bishop pair ----
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        if (more_than_one(b.pieces[c][BISHOP])) {
            mg += sign * p.bp_mg; TR_MG(BpMg, 0, sign);
            eg += sign * p.bp_eg; TR_EG(BpEg, 0, sign);
        }
    }

    // ---- Rook bonuses ----
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color us   = Color(c);
        Color them = ~us;
        Bitboard rooks = b.pieces[c][ROOK];
        while (rooks) {
            int sq = pop_lsb(rooks);
            int f = file_of(Square(sq));

            bool no_own_pawn   = !(b.pieces[us][PAWN]   & BB_FILES[f]);
            bool no_their_pawn = !(b.pieces[them][PAWN] & BB_FILES[f]);

            if (no_own_pawn && no_their_pawn) {
                mg += sign * p.rook_open_mg; TR_MG(RookOpenMg, 0, sign);
                eg += sign * p.rook_open_eg; TR_EG(RookOpenEg, 0, sign);
            } else if (no_own_pawn) {
                mg += sign * p.rook_semi_mg; TR_MG(RookSemiMg, 0, sign);
                eg += sign * p.rook_semi_eg; TR_EG(RookSemiEg, 0, sign);
            }

            // Rook on 7th rank (relative)
            if (relative_rank(us, Square(sq)) == RANK_7) {
                mg += sign * p.rook_7th_mg; TR_MG(Rook7thMg, 0, sign);
                eg += sign * p.rook_7th_eg; TR_EG(Rook7thEg, 0, sign);
            }
        }
    }

    // ---- Knight bonuses ----
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color us   = Color(c);
        Color them = ~us;
        Bitboard knights = b.pieces[c][KNIGHT];
        while (knights) {
            int sq = pop_lsb(knights);
            Rank r = relative_rank(us, Square(sq));
            if (r >= RANK_5) {
                if (PawnAttacks[them][sq] & b.pieces[us][PAWN]) {
                    if (!(PawnAttacks[us][sq] & b.pieces[them][PAWN])) {
                        mg += sign * p.knight_outpost_mg; TR_MG(KnightOutpostMg, 0, sign);
                        eg += sign * p.knight_outpost_eg; TR_EG(KnightOutpostEg, 0, sign);
                    }
                }
            }
        }

        // 5.9.2 bishop outpost. The same concept the knight loop above prices,
        // for the piece it was never applied to: a bishop on a protected
        // advanced square no enemy pawn can challenge. Placed here rather than
        // in the attack-map block so lazy eval treats both minors alike --
        // pricing one before the checkpoint and the other after would make the
        // pair's relative value depend on whether the margin fired.
        Bitboard bishops_c = b.pieces[c][BISHOP];
        while (bishops_c) {
            int sq = pop_lsb(bishops_c);
            if (relative_rank(us, Square(sq)) >= RANK_5
                && (PawnAttacks[them][sq] & b.pieces[us][PAWN])
                && !(PawnAttacks[us][sq] & b.pieces[them][PAWN])) {
                mg += sign * p.bishop_outpost_mg; TR_MG(BishopOutpostMg, 0, sign);
                eg += sign * p.bishop_outpost_eg; TR_EG(BishopOutpostEg, 0, sign);
            }
        }
    }

    // ---- Lazy eval checkpoint (Step 3.11; behaviour-changing, SPRT-gated) ---
    // The cheap part (material/PST + imbalance + pawns + dynamic passers + bishop
    // pair + rook/knight bonuses) is complete. If its tapered margin is decisive,
    // skip the expensive attack-map-driven block (mobility, king safety, threats,
    // hanging, shelter, small terms, space, winnable) and finish with endgame
    // scaling -- which still runs, so KBNK/KPK/draw-scaling survive the skip.
    // Disabled under TEXEL_TRACE so the tuner fits the full eval (--verify exact).
#ifndef TEXEL_TRACE
    // 8.6.6b: under the lazy audit the skip does NOT return — the full tail
    // runs for measurement and the final return serves the remembered lazy
    // value, so the audit is behaviour-identical (bench-identical) when on.
    int lazy_served = VALUE_NONE_SENTINEL;
    {
        int lazy = (mg * phase + eg * (TOTAL_PHASE - phase)) / TOTAL_PHASE;
        int lazy_abs = lazy < 0 ? -lazy : lazy;
        if (lazy_abs > LAZY_MARGIN) {
            int score = apply_endgame(b, lazy);
            if (b.halfmove_clock > 0)
                score = damp_rule50(score, b.halfmove_clock);
            const int served = (b.side_to_move == WHITE) ? score : -score;
            if (!diag_lazy)
                return served;
            lazy_served = served;
        }
    }
#endif

    // ---- Attack-map substrate (Step 3.0) -----------------------------------
    // Compute per-color attack maps once and reuse them for mobility, king
    // safety, and hanging-piece detection, replacing per-square is_attacked_by
    // recomputation and the duplicate slider sweep that mobility and king
    // safety each used to perform separately.
    //   attacked_by[c][pt] : union of attacks by all of color c's pieces of pt
    //   attacked[c]        : full union == is_attacked_by(sq, all_occ, c) as bb
    //   attacked2[c]       : squares attacked 2+ times by color c
    // attacked_by[] and attacked2[] are substrate seeded here for later Phase 3
    // steps (threats, king-safety v2, per-count mobility); only attacked[] and
    // the mobility / king-attacker accumulation below feed the current eval.
    Bitboard attacked_by[NCOLORS][PIECE_TYPE_NB] = {};
    Bitboard attacked[NCOLORS]  = {0, 0};
    Bitboard attacked2[NCOLORS] = {0, 0};
    Bitboard king_zone[NCOLORS];
    int attack_units[NCOLORS] = {0, 0};
    int n_attackers[NCOLORS]  = {0, 0};

    for (int c = 0; c < NCOLORS; c++) {
        Square ksq = b.king_sq[c];
        Bitboard kz = KingAttacks[ksq] | sq_bb(ksq);
        kz |= (c == WHITE) ? shift<NORTH>(KingAttacks[ksq])
                           : shift<SOUTH>(KingAttacks[ksq]);
        king_zone[c] = kz;

        // Seed pawn and king attacks. The two pawn capture directions are
        // seeded separately (8.3c): a square attacked by two different pawns
        // belongs in attacked2 -- the threats package (strongly_protected /
        // hanging) and the king-ring/flank code consume it (audit
        // hce_analysis 3; the old "no current consumer" note was stale).
        const Bitboard pawns = b.pieces[Color(c)][PAWN];
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
        Square ksq = b.king_sq[us];
        Bitboard snipers =
            (rook_attacks(ksq, 0ULL)   & (b.pieces[them][ROOK]   | b.pieces[them][QUEEN]))
          | (bishop_attacks(ksq, 0ULL) & (b.pieces[them][BISHOP] | b.pieces[them][QUEEN]));
        Bitboard blockers = 0;
        while (snipers) {
            Square s = Square(pop_lsb(snipers));
            Bitboard between = BB_BETWEEN[ksq][s] & b.all_occ;
            if (between && !more_than_one(between))
                blockers |= between & b.occupancy[us];
        }
        blockers_for_king[c] = blockers;
    }

    // 8.7.7(a): cache each slider's attack set from the sweep so the king-ring
    // and trapped-bishop tests below reuse it instead of recomputing the same
    // magic lookup. Every reuse site uses bishop/rook_attacks(sq, b.all_occ) —
    // the exact value the sweep computes — so this is bench-identical. Only
    // BISHOP/ROOK squares are stored (the only pieces those tests iterate);
    // every such square is written by the sweep before any test reads it, so
    // no zero-init is needed (and none is paid per eval).
    Bitboard slider_att[SQUARE_NB];

    // Single knight/slider sweep: builds the maps and accumulates both mobility
    // (for the moving color) and king-attacker pressure (against the enemy king
    // — a piece of color c presses on the king of color ~c).
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color them = ~Color(c);
        // SF-style mobility area (Step 4.6b): exclude own K/Q, own pawns that
        // are blocked (a piece directly ahead) or on the low ranks (2/3 from
        // our side), squares attacked by enemy pawns, and own pieces pinned to
        // our king. Own minors/rooks are NOT excluded (controlling the square
        // under a friendly piece is real mobility); enemy-occupied squares stay
        // includable (mobility onto/against them counts).
        Bitboard own_pawns = b.pieces[c][PAWN];
        Bitboard low_ranks = (c == WHITE) ? (BB_RANKS[1] | BB_RANKS[2])
                                          : (BB_RANKS[6] | BB_RANKS[5]);
        Bitboard ahead = (c == WHITE) ? shift<SOUTH>(b.all_occ)
                                      : shift<NORTH>(b.all_occ);
        Bitboard blocked_or_low = own_pawns & (ahead | low_ranks);
        Bitboard mob_area = ~(blocked_or_low | b.pieces[c][KING] | b.pieces[c][QUEEN]
                              | pawn_atk[them] | blockers_for_king[c]);
        // 8.7.7(c): hoist the two per-piece switch(pt) out of the mobility
        // inner loop. pt is constant across a whole `while (pcs)` run, so both
        // switches are loop-invariant. A template lambda instantiated once per
        // piece type folds them to compile-time constants — the att magic is
        // picked directly, the mobility table selected at compile time — so the
        // inner loop is branchless. Same values, same accumulation order (all
        // knights, then bishops, rooks, queens) ⇒ bench-identical by
        // construction, and the TR_* trace calls are unchanged so the tuner is
        // exact.
        auto sweep = [&]<PieceType PT>() {
            Bitboard pcs = b.pieces[c][PT];
            while (pcs) {
                int sq = pop_lsb(pcs);
                Bitboard att;
                if      constexpr (PT == KNIGHT) att = KnightAttacks[sq];
                else if constexpr (PT == BISHOP) att = bishop_attacks(Square(sq), b.all_occ);
                else if constexpr (PT == ROOK)   att = rook_attacks(Square(sq), b.all_occ);
                else                             att = queen_attacks(Square(sq), b.all_occ);

                attacked2[c] |= attacked[c] & att;
                attacked[c]  |= att;
                attacked_by[c][PT] |= att;
                if constexpr (PT == BISHOP || PT == ROOK)
                    slider_att[sq] = att;   // 8.7.7(a)

                // Per-count mobility (Step 3.3): one-hot table indexed by the
                // mobility-square count over the SF area (Step 4.6b).
                int mob = popcount(att & mob_area);
                if constexpr (PT == KNIGHT) {
                    mg += sign * p.mob_n_mg[mob]; TR_MG(MobNMg, mob, sign);
                    eg += sign * p.mob_n_eg[mob]; TR_EG(MobNEg, mob, sign);
                } else if constexpr (PT == BISHOP) {
                    mg += sign * p.mob_b_mg[mob]; TR_MG(MobBMg, mob, sign);
                    eg += sign * p.mob_b_eg[mob]; TR_EG(MobBEg, mob, sign);
                } else if constexpr (PT == ROOK) {
                    mg += sign * p.mob_r_mg[mob]; TR_MG(MobRMg, mob, sign);
                    eg += sign * p.mob_r_eg[mob]; TR_EG(MobREg, mob, sign);
                } else {  // QUEEN
                    mg += sign * p.mob_q_mg[mob]; TR_MG(MobQMg, mob, sign);
                    eg += sign * p.mob_q_eg[mob]; TR_EG(MobQEg, mob, sign);
                }

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

    // (blockers_for_king is now computed before the mobility sweep — Step 4.6b.)

    // ---- Pawn threats to enemy pieces ----
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color them = ~Color(c);
        // 5.9.1 safe-square qualifier. Our pawn threats fire wherever a pawn
        // attacks a piece, including from a square an enemy pawn covers, where
        // the threat costs a pawn to execute and is often empty. This adds a
        // separate term for threats delivered from squares no enemy pawn
        // attacks. It layers on top rather than replacing the existing terms, so
        // the fit at 5.9.4 can price the qualifier independently of the threat.
        {
            const Bitboard safe_pawns = b.pieces[c][PAWN] & ~pawn_atk[~Color(c)];
            const Bitboard safe_atk = (c == WHITE)
                ? (((safe_pawns & ~BB_FILES[FILE_A]) << 7) | ((safe_pawns & ~BB_FILES[FILE_H]) << 9))
                : (((safe_pawns & ~BB_FILES[FILE_A]) >> 9) | ((safe_pawns & ~BB_FILES[FILE_H]) >> 7));
            const Bitboard tgt = b.occupancy[them] & ~b.pieces[them][PAWN];
            const int n = popcount(safe_atk & tgt);
            mg += sign * n * p.threat_safe_pawn_mg; TR_MG(ThreatSafePawnMg, 0, sign * n);
            eg += sign * n * p.threat_safe_pawn_eg; TR_EG(ThreatSafePawnEg, 0, sign * n);
        }
        Bitboard threats = pawn_atk[c] & b.occupancy[them];
        while (threats) {
            int sq = pop_lsb(threats);
            PieceType pt = type_of(b.board_sq[sq]);
            switch (pt) {
                case KNIGHT: case BISHOP:
                    mg += sign * p.threat_minor_mg; TR_MG(ThreatMinorMg, 0, sign);
                    eg += sign * p.threat_minor_eg; TR_EG(ThreatMinorEg, 0, sign);
                    break;
                case ROOK:
                    mg += sign * p.threat_rook_mg; TR_MG(ThreatRookMg, 0, sign);
                    eg += sign * p.threat_rook_eg; TR_EG(ThreatRookEg, 0, sign);
                    break;
                case QUEEN:
                    mg += sign * p.threat_queen_mg; TR_MG(ThreatQueenMg, 0, sign);
                    eg += sign * p.threat_queen_eg; TR_EG(ThreatQueenEg, 0, sign);
                    break;
                default: break;
            }
        }
    }

    // ---- Threats package (Step 3.1; SF-style, seeded inert) ----------------
    // Uses the Step 3.0 attack maps. Every weight is seeded 0 (bench identical);
    // the loops compute + trace so the Phase-4.2 fit can activate the group. It
    // coexists with the flat pawn-threat and hang_pen terms above until Phase 4.2
    // swaps the tuned replacement in.
    for (int c = 0; c < NCOLORS; c++) {
        int sign   = (c == WHITE) ? 1 : -1;
        Color us   = Color(c);
        Color them = ~us;

        Bitboard nonpawn_enemies = b.occupancy[them] & ~b.pieces[them][PAWN];
        // Squares the enemy holds firmly: defended by a pawn, or attacked twice by
        // them and not twice by us.
        Bitboard strongly_protected =
            attacked_by[them][PAWN] | (attacked2[them] & ~attacked2[us]);
        Bitboard defended = nonpawn_enemies & strongly_protected;
        // Enemy pieces we attack that are not firmly held.
        Bitboard weak = b.occupancy[them] & ~strongly_protected & attacked[us];

        // Threats by a minor (knight or bishop) on weak or defended targets.
        Bitboard minor_att = attacked_by[us][KNIGHT] | attacked_by[us][BISHOP];
        Bitboard tb = (defended | weak) & minor_att;
        while (tb) {
            PieceType pt = type_of(b.board_sq[pop_lsb(tb)]);
            mg += sign * p.threat_by_minor_mg[pt]; TR_MG(ThreatByMinorMg, pt, sign);
            eg += sign * p.threat_by_minor_eg[pt]; TR_EG(ThreatByMinorEg, pt, sign);
        }

        // Threats by a rook on weak targets.
        tb = weak & attacked_by[us][ROOK];
        while (tb) {
            PieceType pt = type_of(b.board_sq[pop_lsb(tb)]);
            mg += sign * p.threat_by_rook_mg[pt]; TR_MG(ThreatByRookMg, pt, sign);
            eg += sign * p.threat_by_rook_eg[pt]; TR_EG(ThreatByRookEg, pt, sign);
        }

        // Threat by king on a weak target.
        if (weak & attacked_by[us][KING]) {
            mg += sign * p.threat_by_king_mg; TR_MG(ThreatByKingMg, 0, sign);
            eg += sign * p.threat_by_king_eg; TR_EG(ThreatByKingEg, 0, sign);
        }

        // Hanging: weak pieces with no enemy defender, or non-pawns we hit twice.
        Bitboard hanging = weak & (~attacked[them] | (nonpawn_enemies & attacked2[us]));
        int hang_cnt = popcount(hanging);
        mg += sign * hang_cnt * p.threat_hanging_mg; TR_MG(ThreatHangingMg, 0, sign * hang_cnt);
        eg += sign * hang_cnt * p.threat_hanging_eg; TR_EG(ThreatHangingEg, 0, sign * hang_cnt);

        // Weak piece whose only defender is the enemy queen.
        int wq_cnt = popcount(weak & attacked_by[them][QUEEN]);
        mg += sign * wq_cnt * p.weak_queen_prot_mg; TR_MG(WeakQueenProtMg, 0, sign * wq_cnt);
        eg += sign * wq_cnt * p.weak_queen_prot_eg; TR_EG(WeakQueenProtEg, 0, sign * wq_cnt);

        // Restricted: squares the enemy attacks that we also attack and they do not
        // firmly hold (limits their piece mobility).
        int restr_cnt = popcount(attacked[them] & ~strongly_protected & attacked[us]);
        mg += sign * restr_cnt * p.restricted_mg; TR_MG(RestrictedMg, 0, sign * restr_cnt);
        eg += sign * restr_cnt * p.restricted_eg; TR_EG(RestrictedEg, 0, sign * restr_cnt);

        // Threat by a safe pawn push: squares our pawns can (double-)push to that
        // are safe, whose resulting pawn attacks would hit an enemy non-pawn.
        Bitboard empty = ~b.all_occ;
        Bitboard safe  = ~attacked[them] | attacked[us];
        Bitboard push, push_att;
        if (us == WHITE) {
            push  = shift<NORTH>(b.pieces[us][PAWN]) & empty;
            push |= shift<NORTH>(push & BB_RANKS[RANK_3]) & empty;
            push &= ~attacked_by[them][PAWN] & safe;
            push_att = shift<NORTH_WEST>(push) | shift<NORTH_EAST>(push);
        } else {
            push  = shift<SOUTH>(b.pieces[us][PAWN]) & empty;
            push |= shift<SOUTH>(push & BB_RANKS[RANK_6]) & empty;
            push &= ~attacked_by[them][PAWN] & safe;
            push_att = shift<SOUTH_WEST>(push) | shift<SOUTH_EAST>(push);
        }
        int push_cnt = popcount(push_att & nonpawn_enemies);
        mg += sign * push_cnt * p.threat_push_mg; TR_MG(ThreatPushMg, 0, sign * push_cnt);
        eg += sign * push_cnt * p.threat_push_eg; TR_EG(ThreatPushEg, 0, sign * push_cnt);
    }

    // ---- King safety v2 (finalization) -------------------------------------
    // safety_table[units] is a one-hot lookup. Piece-attack pressure
    // (attack_units / n_attackers) was gathered in the attack-map sweep; here we
    // add the Step-3.2 danger inputs (all seeded 0, so bench is unchanged), apply
    // the no-queen relief and open-file penalty, and look up the table. The new
    // unit weights and the existing ks_unit/coord/open-file/no-queen knobs are
    // composite (they shape the index), so only SafetyTable carries a trace; they
    // are tuned by the Phase-4.3 finite-difference path.
    for (int c = 0; c < NCOLORS; c++) {
        int sign   = (c == WHITE) ? 1 : -1;
        Color us   = Color(c);
        Color them = ~us;
        Square ksq = b.king_sq[us];
        File   kf  = file_of(ksq);

        int units = attack_units[c];
        if (n_attackers[c] >= 2) units += p.ks_coord_bonus;
        if (n_attackers[c] == 0) units = 0;

        // King's flank file group (SF-style) and our camp (own half + one rank).
        Bitboard flank_files;
        if (kf <= FILE_C)
            flank_files = BB_FILES[FILE_A] | BB_FILES[FILE_B] | BB_FILES[FILE_C] | BB_FILES[FILE_D];
        else if (kf >= FILE_F)
            flank_files = BB_FILES[FILE_E] | BB_FILES[FILE_F] | BB_FILES[FILE_G] | BB_FILES[FILE_H];
        else
            flank_files = BB_FILES[FILE_C] | BB_FILES[FILE_D] | BB_FILES[FILE_E] | BB_FILES[FILE_F];

        // Safe checks: enemy checking squares it attacks and we do not hold.
        Bitboard safe_sq = ~b.occupancy[them] & ~attacked[us];
        for (int pt : {KNIGHT, BISHOP, ROOK, QUEEN}) {
            Bitboard checks = b.check_squares(PieceType(pt), them)
                            & attacked_by[them][pt] & safe_sq;
            if (checks)
                units += p.ks_safe_check[pt] * popcount(checks);
        }

        // Weak king-ring squares: enemy-attacked, not held twice, no pawn cover.
        Bitboard ring = KingAttacks[ksq];
        Bitboard weak_ring = ring & attacked[them] & ~attacked2[us] & ~attacked_by[us][PAWN];
        units += p.ks_weak_ring * popcount(weak_ring);

        // Enemy mobility pressure around the ring.
        units += p.ks_ring_pressure * popcount(ring & attacked[them]);

        // King-flank attack vs defense within our camp.
        Bitboard camp = (us == WHITE)
            ? (BB_RANKS[RANK_1] | BB_RANKS[RANK_2] | BB_RANKS[RANK_3] | BB_RANKS[RANK_4] | BB_RANKS[RANK_5])
            : (BB_RANKS[RANK_4] | BB_RANKS[RANK_5] | BB_RANKS[RANK_6] | BB_RANKS[RANK_7] | BB_RANKS[RANK_8]);
        Bitboard flank_zone = flank_files & camp;
        int flank_attack  = popcount(flank_zone & attacked[them])
                          + popcount(flank_zone & attacked2[them]);
        int flank_defense = popcount(flank_zone & attacked[us]);
        units += p.ks_flank_attack * flank_attack - p.ks_flank_defense * flank_defense;

        // Pawnless flank.
        if (!(b.pieces[us][PAWN] & flank_files))
            units += p.ks_pawnless_flank;

        // Own pieces pinned in front of the king (a pinned defender doesn't defend).
        units += p.ks_king_blockers * popcount(blockers_for_king[c]);

        // Central king with castling rights gone.
        const int our_castle_mask = (us == WHITE) ? 3 : 12;
        if (kf >= FILE_C && kf <= FILE_F && !(b.castling_rights & our_castle_mask))
            units += p.ks_central_king;

        // Pawn-shelter exposure folded into the danger funnel (open files near king).
        int shelter_danger = 0;
        for (int df = -1; df <= 1; df++) {
            int f = int(kf) + df;
            if (f < FILE_A || f > FILE_H) continue;
            if (!(b.pieces[us][PAWN] & BB_FILES[f])) shelter_danger++;
        }
        units += p.ks_shelter_storm * shelter_danger;

        // No-enemy-queen relief (exposed scalar; was frozen 2/3, retuned since).
        if (!b.pieces[them][QUEEN])
            units = units * p.ks_noqueen_num / p.ks_noqueen_den;

        // Open king file (existing term).
        if (!(b.pieces[us][PAWN] & BB_FILES[kf]))
            units += p.ks_open_file;

        units = std::min(units, 24);
        if (units < 0) units = 0;
        mg -= sign * p.safety_table[units];
        TR_MG(SafetyTable, units, -sign);
    }

    // ---- King pawn shelter -------------------------------------------------
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color us = Color(c);
        Square ksq = b.king_sq[c];
        File   kf  = file_of(ksq);
        Rank   kr  = rank_of(ksq);

        if (kf <= FILE_C || kf >= FILE_F) {
            for (int df = -1; df <= 1; df++) {
                int f = kf + df;
                if (f < FILE_A || f > FILE_H) continue;
                Bitboard file_pawns = b.pieces[us][PAWN] & BB_FILES[f];

                if (us == WHITE) {
                    Bitboard in_front = file_pawns & BB_FORWARD_RANKS[WHITE][kr];
                    if (!in_front) {
                        if (df == 0) { mg -= sign * p.shelter_missing_center; TR_MG(ShelterMissingCenter, 0, -sign); }
                        else         { mg -= sign * p.shelter_missing_flank;  TR_MG(ShelterMissingFlank,  0, -sign); }
                    } else {
                        Rank pawn_rank = rank_of(Square(lsb(in_front)));
                        int dist = pawn_rank - kr;
                        if (dist == 1)      { mg += sign * p.shelter_close1; TR_MG(ShelterClose1, 0, sign); }
                        else if (dist == 2) { mg += sign * p.shelter_close2; TR_MG(ShelterClose2, 0, sign); }
                    }
                } else {
                    Bitboard in_front = file_pawns & BB_FORWARD_RANKS[BLACK][kr];
                    if (!in_front) {
                        if (df == 0) { mg -= sign * p.shelter_missing_center; TR_MG(ShelterMissingCenter, 0, -sign); }
                        else         { mg -= sign * p.shelter_missing_flank;  TR_MG(ShelterMissingFlank,  0, -sign); }
                    } else {
                        Rank pawn_rank = rank_of(Square(msb(in_front)));
                        int dist = kr - pawn_rank;
                        if (dist == 1)      { mg += sign * p.shelter_close1; TR_MG(ShelterClose1, 0, sign); }
                        else if (dist == 2) { mg += sign * p.shelter_close2; TR_MG(ShelterClose2, 0, sign); }
                    }
                }
            }
        }

        Bitboard storm_files = 0;
        for (int df = -1; df <= 1; df++) {
            int f = kf + df;
            if (f >= FILE_A && f <= FILE_H)
                storm_files |= BB_FILES[f];
        }

        Bitboard storm = b.pieces[~us][PAWN] & storm_files;
        while (storm) {
            Square psq = Square(pop_lsb(storm));
            int rel_r = relative_rank(~us, psq);
            if (rel_r >= 3) {
                if (file_of(psq) == kf) {
                    mg -= sign * rel_r * p.storm_weight_kf;  TR_MG(StormWeightKf,  0, -sign * rel_r);
                } else {
                    mg -= sign * rel_r * p.storm_weight_adj; TR_MG(StormWeightAdj, 0, -sign * rel_r);
                }
            }
        }
    }

    // ---- Rook behind passed pawn -------------------------------------------
    // Two independent passes (8.3b): pre-8.3 the enemy-rook penalty was nested
    // inside the friendly-rook loop, so it only fired when WE also had a rook
    // on the passer file (and fired once per friendly rook when stacked).
    // "Behind" stays purely geometric (blockers between rook and pawn are not
    // examined), matching the friendly-rook term. Audit hce_analysis 2.
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color us = Color(c);
        Color them = ~us;

        // Friendly rook behind own passer: reward per rook.
        Bitboard rooks = b.pieces[us][ROOK];
        while (rooks) {
            Square rsq = Square(pop_lsb(rooks));
            int f = file_of(rsq);
            Bitboard file_passers = passed[us] & BB_FILES[f];
            if (!file_passers) continue;
            const bool behind = (us == WHITE)
                ? rank_of(rsq) < rank_of(Square(lsb(file_passers)))
                : rank_of(rsq) > rank_of(Square(msb(file_passers)));
            if (behind) {
                mg += sign * p.rook_behind_passer_mg; TR_MG(RookBehindPasserMg, 0, sign);
                eg += sign * p.rook_behind_passer_eg; TR_EG(RookBehindPasserEg, 0, sign);
            }
        }

        // Enemy rook behind own passer: penalty per enemy rook, independent
        // of any friendly rook on the file.
        Bitboard enemy_rooks = b.pieces[them][ROOK];
        while (enemy_rooks) {
            Square er = Square(pop_lsb(enemy_rooks));
            int f = file_of(er);
            Bitboard file_passers = passed[us] & BB_FILES[f];
            if (!file_passers) continue;
            const bool behind = (us == WHITE)
                ? rank_of(er) < rank_of(Square(lsb(file_passers)))
                : rank_of(er) > rank_of(Square(msb(file_passers)));
            if (behind) {
                mg -= sign * p.enemy_rook_passer_mg; TR_MG(EnemyRookPasserMg, 0, -sign);
                eg -= sign * p.enemy_rook_passer_eg; TR_EG(EnemyRookPasserEg, 0, -sign);
            }
        }
    }

    // ---- Hanging pieces (attacked and not defended) ------------------------
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color us   = Color(c);
        Color them = ~us;
        Bitboard pieces_bb = b.occupancy[us] & ~b.pieces[us][PAWN] & ~b.pieces[us][KING];
        while (pieces_bb) {
            Square sq = Square(pop_lsb(pieces_bb));
            // attacked[c] is the full union, identical to is_attacked_by(sq, all_occ, c):
            // skip if not attacked by them, or if defended by us.
            if (!(attacked[them] & sq_bb(sq))) continue;
            if  (attacked[us]   & sq_bb(sq))   continue;
            PieceType pt = type_of(b.board_sq[sq]);
            mg -= sign * p.hang_pen[pt]; TR_BOTH(HangPen, pt, -sign);
            eg -= sign * p.hang_pen[pt];
        }
    }

    // ---- Passed pawn king proximity + promotion-path safety (endgame) ------
    // The king-proximity term is unchanged; the Step-3.4 promotion-path terms
    // (path-safe / block-defended / king-to-block) are seeded 0 and use the
    // attack maps (so they live here, outside the pawn cache).
    for (int c = 0; c < NCOLORS; c++) {
        Color us = Color(c);
        Color them = ~us;
        int sign = (us == WHITE) ? 1 : -1;
        Bitboard pp = passed[us];
        while (pp) {
            Square psq = Square(pop_lsb(pp));
            int rel_r = (us == WHITE) ? rank_of(psq) : (RANK_8 - rank_of(psq));
            int own_dist = KING_DIST[b.king_sq[us]][psq];
            int opp_dist = KING_DIST[b.king_sq[them]][psq];
            eg += sign * (opp_dist - own_dist) * (p.prox_base + rel_r);
            TR_EG(ProxBase, 0, sign * (opp_dist - own_dist));
            // rel_r * (opp_dist-own_dist) is a frozen data contribution captured in rest

            // Promotion-path safety (Step 3.4, seeded 0).
            Bitboard path = BB_PASSED_PAWN_MASK[us][psq] & BB_FILES[file_of(psq)];
            if (!(path & attacked[them])) {
                eg += sign * rel_r * p.passed_path_safe_eg;
                TR_EG(PassedPathSafeEg, 0, sign * rel_r);
            }
            Square block = forward_square(us, psq);
            if (block != SQ_NONE) {
                if (attacked[us] & sq_bb(block)) {
                    eg += sign * p.passed_block_defended_eg;
                    TR_EG(PassedBlockDefendedEg, 0, sign);
                }
                int own_bd = KING_DIST[b.king_sq[us]][block];
                int opp_bd = KING_DIST[b.king_sq[them]][block];
                eg += sign * (opp_bd - own_bd) * p.passed_king_block_eg;
                TR_EG(PassedKingBlockEg, 0, sign * (opp_bd - own_bd));
            }

        }
    }

    // ---- Small positional terms (Step 3.7; seeded inert) -------------------
    // Uses the Step-3.0 attack maps + king_zone. Every weight is seeded 0 (bench
    // unchanged); traced so the Phase-4.5 fit can activate the batch.
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color us = Color(c), them = ~us;
        Bitboard our_pawns   = b.pieces[us][PAWN];
        Bitboard their_pawns = b.pieces[them][PAWN];
        Bitboard ring_them   = king_zone[them];
        Bitboard advanced = (us == WHITE)
            ? (BB_RANKS[RANK_5] | BB_RANKS[RANK_6] | BB_RANKS[RANK_7])
            : (BB_RANKS[RANK_2] | BB_RANKS[RANK_3] | BB_RANKS[RANK_4]);
        Bitboard outpost_sqs = pawn_atk[us] & ~pawn_atk[them] & advanced;

        // Bishop pair scaled by own pawn count.
        if (more_than_one(b.pieces[us][BISHOP])) {
            int np = popcount(our_pawns);
            mg += sign * np * p.bishop_pair_pawns_mg; TR_MG(BishopPairPawnsMg, 0, sign * np);
            eg += sign * np * p.bishop_pair_pawns_eg; TR_EG(BishopPairPawnsEg, 0, sign * np);
        }

        // Bishops: bad bishop, king-ring pressure, x-ray pawns, long diagonal.
        // (The outpost bonus lives in the cheap block beside the knight's.)
        Bitboard bishops = b.pieces[us][BISHOP];
        while (bishops) {
            Square bsq = Square(pop_lsb(bishops));
            Bitboard cmask = (sq_bb(bsq) & EG_DARK_SQUARES) ? EG_DARK_SQUARES : ~EG_DARK_SQUARES;
            int bp_on_col = popcount(our_pawns & cmask);
            mg += sign * bp_on_col * p.bad_bishop_mg; TR_MG(BadBishopMg, 0, sign * bp_on_col);
            eg += sign * bp_on_col * p.bad_bishop_eg; TR_EG(BadBishopEg, 0, sign * bp_on_col);
            if (slider_att[bsq] & ring_them) {   // 8.7.7(a): was bishop_attacks(bsq, b.all_occ)
                mg += sign * p.minor_king_ring_mg; TR_MG(MinorKingRingMg, 0, sign);
                eg += sign * p.minor_king_ring_eg; TR_EG(MinorKingRingEg, 0, sign);
            }
            // Step 3.9: minor behind a friendly pawn; king-protector distance.
            Square b_ahead = forward_square(us, bsq);
            if (b_ahead != SQ_NONE && (our_pawns & sq_bb(b_ahead))) {
                mg += sign * p.minor_behind_pawn_mg; TR_MG(MinorBehindPawnMg, 0, sign);
                eg += sign * p.minor_behind_pawn_eg; TR_EG(MinorBehindPawnEg, 0, sign);
            }
            int b_kd = KING_DIST[b.king_sq[us]][bsq];
            mg += sign * b_kd * p.king_protector_b_mg; TR_MG(KingProtectorBMg, 0, sign * b_kd);
            eg += sign * b_kd * p.king_protector_b_eg; TR_EG(KingProtectorBEg, 0, sign * b_kd);

            // 5.9.1 bishop x-ray pawns. bad_bishop already prices OUR pawns on
            // the bishop's colour; this prices the ENEMY pawns actually sitting
            // on its diagonals. Empty-board attacks give the rays the bishop
            // would command with nothing in the way, so a pawn counts whether or
            // not something else already blocks it.
            {
                int xr = popcount(EVAL_BISHOP_EMPTY[bsq] & b.pieces[them][PAWN]);
                mg += sign * xr * p.bishop_xray_pawn_mg; TR_MG(BishopXrayPawnMg, 0, sign * xr);
                eg += sign * xr * p.bishop_xray_pawn_eg; TR_EG(BishopXrayPawnEg, 0, sign * xr);
            }

            // 5.9.1 long-diagonal bishop: on a1-h8 or h1-a8 with an unobstructed
            // view of at least one central square. Complements bad_bishop, which
            // counts pawns by colour and says nothing about what the bishop sees.
            if ((sq_bb(bsq) & EVAL_LONG_DIAGONALS)
                && (slider_att[bsq] & EVAL_CENTRE_SQUARES)) {
                mg += sign * p.long_diagonal_bishop_mg; TR_MG(LongDiagonalBishopMg, 0, sign);
                eg += sign * p.long_diagonal_bishop_eg; TR_EG(LongDiagonalBishopEg, 0, sign);
            }
        }

        // Knights: reachable outpost, king-ring pressure.
        Bitboard knights = b.pieces[us][KNIGHT];
        while (knights) {
            Square nsq = Square(pop_lsb(knights));
            int reach = popcount(KnightAttacks[nsq] & outpost_sqs & ~b.occupancy[us]);
            mg += sign * reach * p.reachable_outpost_mg; TR_MG(ReachableOutpostMg, 0, sign * reach);
            eg += sign * reach * p.reachable_outpost_eg; TR_EG(ReachableOutpostEg, 0, sign * reach);
            if (KnightAttacks[nsq] & ring_them) {
                mg += sign * p.minor_king_ring_mg; TR_MG(MinorKingRingMg, 0, sign);
                eg += sign * p.minor_king_ring_eg; TR_EG(MinorKingRingEg, 0, sign);
            }
            // Step 3.9: minor behind a friendly pawn; king-protector distance.
            Square n_ahead = forward_square(us, nsq);
            if (n_ahead != SQ_NONE && (our_pawns & sq_bb(n_ahead))) {
                mg += sign * p.minor_behind_pawn_mg; TR_MG(MinorBehindPawnMg, 0, sign);
                eg += sign * p.minor_behind_pawn_eg; TR_EG(MinorBehindPawnEg, 0, sign);
            }
            int n_kd = KING_DIST[b.king_sq[us]][nsq];
            mg += sign * n_kd * p.king_protector_n_mg; TR_MG(KingProtectorNMg, 0, sign * n_kd);
            eg += sign * n_kd * p.king_protector_n_eg; TR_EG(KingProtectorNEg, 0, sign * n_kd);

            // 5.9.1 bad outpost: the knight_outpost bonus assumes an outpost is
            // permanent, but it is only worth full value if the enemy cannot
            // trade it off. If they still hold a knight, or a bishop on the
            // outpost's own square colour, the post is contestable and the bonus
            // is overstated. This term prices that back down; it fires only
            // where knight_outpost fired, so the two compose rather than
            // double-count.
            if (relative_rank(us, nsq) >= RANK_5
                && (PawnAttacks[them][nsq] & our_pawns)
                && !(PawnAttacks[us][nsq] & b.pieces[them][PAWN])) {
                const Bitboard same_colour =
                    (sq_bb(nsq) & EVAL_DARK_SQUARES) ? EVAL_DARK_SQUARES : ~EVAL_DARK_SQUARES;
                if (b.pieces[them][KNIGHT] || (b.pieces[them][BISHOP] & same_colour)) {
                    mg += sign * p.bad_outpost_mg; TR_MG(BadOutpostMg, 0, sign);
                    eg += sign * p.bad_outpost_eg; TR_EG(BadOutpostEg, 0, sign);
                }
            }

            // 5.9.1 knight on queen: squares this knight can reach, that no
            // enemy pawn covers, from which it would attack the enemy queen.
            // A latent fork threat rather than a present attack — a present one
            // is already priced by threat_by_minor.
            if (b.pieces[them][QUEEN]) {
                const Square qs = Square(lsb(b.pieces[them][QUEEN]));
                Bitboard hops = KnightAttacks[nsq] & ~b.occupancy[us] & ~pawn_atk[them];
                int koq = popcount(hops & KnightAttacks[qs]);
                mg += sign * koq * p.knight_on_queen_mg; TR_MG(KnightOnQueenMg, 0, sign * koq);
                eg += sign * koq * p.knight_on_queen_eg; TR_EG(KnightOnQueenEg, 0, sign * koq);
            }
        }

        // Queen infiltration (Step 3.9): our queen safely deep in the enemy half.
        Bitboard queens_us = b.pieces[us][QUEEN];
        while (queens_us) {
            Square qsq = Square(pop_lsb(queens_us));
            if (relative_rank(us, qsq) >= RANK_5 && !(pawn_atk[them] & sq_bb(qsq))) {
                mg += sign * p.queen_infiltration_mg; TR_MG(QueenInfiltrationMg, 0, sign);
                eg += sign * p.queen_infiltration_eg; TR_EG(QueenInfiltrationEg, 0, sign);
            }
        }

        // 5.9.1 slider on queen: our bishop/rook bearing on the enemy queen
        // THROUGH one blocker. Removing every first blocker from the occupancy
        // and re-casting from the queen's square finds exactly the sliders that
        // are one capture away from bearing on her — a pin-and-skewer motif the
        // evaluator otherwise cannot see, and distinct from threat_by_minor,
        // which needs a present attack.
        if (b.pieces[them][QUEEN] && (b.pieces[us][BISHOP] | b.pieces[us][ROOK])) {
            const Square  qs  = Square(lsb(b.pieces[them][QUEEN]));
            const Bitboard occ = b.all_occ;
            // Each half is skipped when we hold no piece of that kind: the
            // popcount would be zero regardless, so this is exact, not an
            // approximation.
            int soq = 0;
            if (b.pieces[us][BISHOP]) {
                const Bitboard diag_block = bishop_attacks(qs, occ) & occ;
                soq += popcount(bishop_attacks(qs, occ ^ diag_block) & b.pieces[us][BISHOP]);
            }
            if (b.pieces[us][ROOK]) {
                const Bitboard line_block = rook_attacks(qs, occ) & occ;
                soq += popcount(rook_attacks(qs, occ ^ line_block) & b.pieces[us][ROOK]);
            }
            mg += sign * soq * p.slider_on_queen_mg; TR_MG(SliderOnQueenMg, 0, sign * soq);
            eg += sign * soq * p.slider_on_queen_eg; TR_EG(SliderOnQueenEg, 0, sign * soq);
        }

        // 5.9.1 trapped rook: a rook with almost nowhere to go, boxed in on the
        // side its own king sits on. The classic shape after castling rights are
        // lost and the king blocks its own rook in the corner. Mobility alone
        // does not catch it, because a rook can be cheaply immobile without
        // being trapped.
        {
            Bitboard rr = b.pieces[us][ROOK];
            while (rr) {
                const Square rsq = Square(pop_lsb(rr));
                const int mob = popcount(slider_att[rsq] & ~b.occupancy[us]);   // 8.7.7(a)
                if (mob <= 3) {
                    const int kf = file_of(b.king_sq[us]);
                    const int rf = file_of(rsq);
                    const bool king_side_of_rook =
                        (rf < kf && kf <= FILE_D) || (rf > kf && kf >= FILE_E);
                    if (king_side_of_rook) {
                        mg += sign * p.trapped_rook_mg; TR_MG(TrappedRookMg, 0, sign);
                        eg += sign * p.trapped_rook_eg; TR_EG(TrappedRookEg, 0, sign);
                    }
                }
            }
        }

        // Rooks: king-ring pressure, closed file, enemy-queen file.
        Bitboard rooks  = b.pieces[us][ROOK];
        Bitboard q_them = b.pieces[them][QUEEN];
        while (rooks) {
            Square rsq = Square(pop_lsb(rooks));
            int f = file_of(rsq);
            if (slider_att[rsq] & ring_them) {   // 8.7.7(a): was rook_attacks(rsq, b.all_occ)
                mg += sign * p.rook_king_ring_mg; TR_MG(RookKingRingMg, 0, sign);
                eg += sign * p.rook_king_ring_eg; TR_EG(RookKingRingEg, 0, sign);
            }
            if ((our_pawns & BB_FILES[f]) && (their_pawns & BB_FILES[f])) {
                mg += sign * p.rook_closed_mg; TR_MG(RookClosedMg, 0, sign);
                eg += sign * p.rook_closed_eg; TR_EG(RookClosedEg, 0, sign);
            }
            if (q_them & BB_FILES[f]) {
                mg += sign * p.rook_queen_file_mg; TR_MG(RookQueenFileMg, 0, sign);
                eg += sign * p.rook_queen_file_eg; TR_EG(RookQueenFileEg, 0, sign);
            }
        }

        // Connected rooks (one rook defends the other along an open line).
        if (more_than_one(b.pieces[us][ROOK])) {
            Bitboard rks = b.pieces[us][ROOK];
            bool connected = false;
            Bitboard tmp = rks;
            while (tmp) {
                Square r = Square(pop_lsb(tmp));
                if (slider_att[r] & rks & ~sq_bb(r)) { connected = true; break; }   // 8.7.7(a)
            }
            if (connected) {
                mg += sign * p.connected_rooks_mg; TR_MG(ConnectedRooksMg, 0, sign);
                eg += sign * p.connected_rooks_eg; TR_EG(ConnectedRooksEg, 0, sign);
            }
        }

    }

    // ---- Space evaluation (center control in middlegame) -------------------
    {
        const Bitboard center_files =
            BB_FILES[FILE_C] | BB_FILES[FILE_D] | BB_FILES[FILE_E] | BB_FILES[FILE_F];
        const Bitboard white_space_ranks =
            BB_RANKS[RANK_2] | BB_RANKS[RANK_3] | BB_RANKS[RANK_4];
        const Bitboard black_space_ranks =
            BB_RANKS[RANK_5] | BB_RANKS[RANK_6] | BB_RANKS[RANK_7];

        Bitboard wspace = (center_files & white_space_ranks) & ~b.pieces[WHITE][PAWN] & ~pawn_atk[BLACK];
        Bitboard bspace = (center_files & black_space_ranks) & ~b.pieces[BLACK][PAWN] & ~pawn_atk[WHITE];
        int wsp = popcount(wspace), bsp = popcount(bspace);
        int space_count = wsp - bsp;
        mg += space_count * p.space_mg;
        TR_MG(SpaceMg, 0, space_count);

        // ---- Step 3.6 space refinement (traced) ----
        Bitboard wp = b.pieces[WHITE][PAWN];
        Bitboard bp = b.pieces[BLACK][PAWN];

        // Space weighted by own non-pawn piece count (SF weights by piece count).
        int wpc = popcount(b.occupancy[WHITE]) - popcount(wp) - 1;
        int bpc = popcount(b.occupancy[BLACK]) - popcount(bp) - 1;
        int piece_w = wsp * wpc - bsp * bpc;
        mg += piece_w * p.space_piece_mg;
        TR_MG(SpacePieceMg, 0, piece_w);

        // Space weighted by own blocked-pawn count (a piece directly ahead).
        int wblk = popcount(wp & (b.all_occ >> 8));
        int bblk = popcount(bp & (b.all_occ << 8));
        int blocked_w = wsp * wblk - bsp * bblk;
        mg += blocked_w * p.space_blocked_mg;
        TR_MG(SpaceBlockedMg, 0, blocked_w);
    }

    // ---- Trapped bishop detection ------------------------------------------
    if (phase < TOTAL_PHASE / 2) {
        for (int c = 0; c < NCOLORS; c++) {
            int sign = (c == WHITE) ? 1 : -1;
            Bitboard bbs = b.pieces[c][BISHOP];
            while (bbs) {
                Square bsq = Square(pop_lsb(bbs));
                Bitboard moves = slider_att[bsq] & ~b.occupancy[c];   // 8.7.7(a): was bishop_attacks(bsq, b.all_occ)
                if (moves == 0) {
                    mg -= sign * p.trapped_mg; TR_MG(TrappedMg, 0, -sign);
                    eg -= sign * p.trapped_eg; TR_EG(TrappedEg, 0, -sign);
                }
            }
        }
    }

    // ---- Endgame mate-drive (frozen) ---------------------------------------
    if (phase <= 6) {
        int score_approx = (mg * phase + eg * (TOTAL_PHASE - phase)) / TOTAL_PHASE;
        if (std::abs(score_approx) > 200) {
            Color winning = (score_approx > 0) ? WHITE : BLACK;
            Color losing  = ~winning;
            // 8.6: gate the push-to-edge / close-the-kings geometry to
            // bare-king mating signatures. It is wrong in pawn races, rook-
            // and-pawn endings, and king-opposition pawn endings (audit
            // hce_analysis 7). Fire only when the defender has no pawns AND
            // the attacker holds material that can force mate against a lone
            // king (Q, R, bishop pair, or B+N). KBNK/KNNK stay handled by
            // apply_endgame's overrides, so this gate never touches them.
            const bool losing_pawnless = b.pieces[losing][PAWN] == 0;
            const bool win_can_mate =
                   b.pieces[winning][QUEEN] != 0
                || b.pieces[winning][ROOK]  != 0
                || more_than_one(b.pieces[winning][BISHOP])
                || (b.pieces[winning][BISHOP] != 0 && b.pieces[winning][KNIGHT] != 0);
            if (losing_pawnless && win_can_mate) {
                int sign = (winning == WHITE) ? 1 : -1;
                Square wksq = b.king_sq[winning];
                Square lksq = b.king_sq[losing];
                int lk_center = std::max(3 - file_of(lksq), file_of(lksq) - 4)
                              + std::max(3 - rank_of(lksq), rank_of(lksq) - 4);
                int king_dist = KING_DIST[wksq][lksq];
                eg += sign * (5 * lk_center + (14 - king_dist) * 4);
                // Frozen: not traced; captured in rest by the tuner.
            }
        }
    }

    // ---- Tempo bonus -------------------------------------------------------
    {
        int tempo_sign = (b.side_to_move == WHITE) ? 1 : -1;
        mg += tempo_sign * p.tempo;
        TR_MG(Tempo, 0, tempo_sign);
    }

    // ---- Winnable / complexity coupling (Step 3.6) -------------------------
    // Frozen, not traced (sign-preserving nonlinearity); all weights are 0,
    // so the contribution is exactly 0 today (bench unchanged, --verify exact).
    // NOTE (8.3 rider): never actually tuned -- the winnability block is not
    // traced and the generic optimizer only consumes linear trace
    // coefficients, so its gradient was always zero (audit hce_analysis 4).
    // Activating it is PLAN 8.5+/re-tune material. The complexity may never flip the
    // sign of the endgame score.
    {
        Square wk = b.king_sq[WHITE], bk = b.king_sq[BLACK];
        int outflanking = std::abs(file_of(wk) - file_of(bk))
                        - std::abs(rank_of(wk) - rank_of(bk));
        Bitboard all_pawns = b.pieces[WHITE][PAWN] | b.pieces[BLACK][PAWN];
        bool both_flanks =
            (all_pawns & (BB_FILES[FILE_A] | BB_FILES[FILE_B] | BB_FILES[FILE_C])) &&
            (all_pawns & (BB_FILES[FILE_F] | BB_FILES[FILE_G] | BB_FILES[FILE_H]));
        bool infiltration = rank_of(wk) > RANK_4 || rank_of(bk) < RANK_5;
        Bitboard pieces = b.pieces[WHITE][KNIGHT] | b.pieces[WHITE][BISHOP]
                        | b.pieces[WHITE][ROOK]   | b.pieces[WHITE][QUEEN]
                        | b.pieces[BLACK][KNIGHT] | b.pieces[BLACK][BISHOP]
                        | b.pieces[BLACK][ROOK]   | b.pieces[BLACK][QUEEN];
        bool pawn_endgame = (pieces == 0);
        int passed_cnt = popcount(passed[WHITE]) + popcount(passed[BLACK]);
        int total_pawns = popcount(all_pawns);

        int complexity = p.win_outflanking  * outflanking
                       + p.win_both_flanks  * (both_flanks ? 1 : 0)
                       + p.win_infiltration * (infiltration ? 1 : 0)
                       + p.win_pawn_endgame * (pawn_endgame ? 1 : 0)
                       + p.win_passed       * passed_cnt
                       + p.win_total_pawns  * total_pawns
                       - p.win_bias;

        int sgn = (eg > 0) - (eg < 0);
        eg += sgn * std::max(complexity, -std::abs(eg));
    }

    // ---- Taper and return --------------------------------------------------
    int score = (mg * phase + eg * (TOTAL_PHASE - phase)) / TOTAL_PHASE;

    // ---- Endgame scaling + knowledge (Step 3.5) ----------------------------
    score = apply_endgame(b, score);

    if (b.halfmove_clock > 0)
        score = damp_rule50(score, b.halfmove_clock);

    const int full_served = (b.side_to_move == WHITE) ? score : -score;
#ifndef TEXEL_TRACE
    if (lazy_served != VALUE_NONE_SENTINEL) {
        // Lazy audit bookkeeping — the full tail ran only for measurement.
        lazy_fires++;
        const int64_t d = std::llabs((long long)full_served - lazy_served);
        lazy_absdelta_sum += d;
        lazy_absdelta_max  = std::max<int64_t>(lazy_absdelta_max, d);
        if ((full_served > 0) != (lazy_served > 0) && full_served != 0 && lazy_served != 0)
            lazy_sign_flips++;
        if (std::abs(full_served) <= LAZY_MARGIN)
            lazy_margin_crossings++;
        return lazy_served;
    }
#endif
    return full_served;
}

// ---- Tune-only: eval file loader and dumper --------------------------------
#ifdef BASILISK_TUNE

// Write current g_eval_params to stdout in "name index value" format.
// This defines the canonical round-trip format consumed by load_eval_params.
void run_dumpeval() {
    const EvalParams& p = g_eval_params;
#define X(name, member, len) \
    { const int* ptr = eval_param_cptr(p.member); \
      for (int i = 0; i < (len); i++) \
          std::cout << #name << " " << i << " " << ptr[i] << "\n"; }
    EVAL_PARAM_LIST(X)
#undef X
    std::cout.flush();
}

// Load "name index value" lines from filename into p, then rebuild eval tables.
// Unknown names are a hard error; indices out of range are a hard error.
static void load_eval_params(const char* filename, EvalParams& p) {
    std::unordered_map<std::string, std::pair<int*, int>> table;
#define X(name, member, len) \
    table[#name] = {eval_param_ptr(p.member), (len)};
    EVAL_PARAM_LIST(X)
#undef X

    std::ifstream f(filename);
    if (!f)
        throw std::runtime_error(std::string("Cannot open eval file: ") + filename);

    std::string line;
    int lineno = 0;
    while (std::getline(f, line)) {
        ++lineno;
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string name;
        int idx, val;
        if (!(ss >> name >> idx >> val))
            throw std::runtime_error("Bad format at line " + std::to_string(lineno)
                                     + ": '" + line + "'");
        auto it = table.find(name);
        if (it == table.end())
            throw std::runtime_error("Unknown eval param '" + name
                                     + "' at line " + std::to_string(lineno));
        auto [ptr, len] = it->second;
        if (idx < 0 || idx >= len)
            throw std::runtime_error("Index " + std::to_string(idx)
                                     + " out of range for '" + name
                                     + "' (length " + std::to_string(len)
                                     + ") at line " + std::to_string(lineno));
        ptr[idx] = val;
    }
}

// Called at engine startup. If BASILISK_EVAL_FILE is set, loads it and
// rebuilds the MG/EG tables. Pawn caches are always fresh at startup.
void load_eval_file_if_set() {
    const char* path = std::getenv("BASILISK_EVAL_FILE");
    if (!path || path[0] == '\0') return;
    load_eval_params(path, g_eval_params);
    init_eval_tables(g_eval_params);
}

#endif // BASILISK_TUNE
