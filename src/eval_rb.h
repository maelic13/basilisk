#pragma once

// The rebuilt evaluation: the classical form's families in its own order,
// then Basilisk's endgame layer, the conversion to centipawns, the tempo and
// the bounds (analysis/c31_rebuild_2026-11-25.md §6 and its Amendment 1).
// Part of the evaluator's single translation unit: included by src/eval.cpp
// alone. Include it nowhere else.

#include "eval_rb_material.h"
#include "eval_rb_threats.h"

namespace rb {

// The one conversion from the form's units to centipawns: cp = v * S / 1024,
// truncated toward zero. A gauge, fixed once (§7); not a fitted value.
inline constexpr int CONVERSION_S = EVAL_REBUILD_CONVERSION_S;

inline int to_centipawns(int v) { return v * CONVERSION_S / 1024; }

// The lazy exits' structure constants: the families after an exit are not
// evaluated once half the halves' sum exceeds its threshold plus a 64th of
// the non-pawn material.
inline constexpr int LAZY_THRESHOLD_1 = 1400;
inline constexpr int LAZY_THRESHOLD_2 = 1300;

inline bool lazy_skip(Score s, int threshold, int npm) {
    return std::abs(s.mg + s.eg) / 2 > threshold + npm / 64;
}

// winnable()'s complexity: how much of the halves the attacking side keeps.
// Its constants are structure constants (§8).
struct Winnable {
    Score adjusted;
    Score complexity;   // the change to each half
    Color strong;
    int raw;            // the complexity itself, which no slot moves
};

inline Winnable winnable(const Board& b, const PawnEntry& e, Score score) {
    const Square wk = b.king_square(WHITE), bk = b.king_square(BLACK);
    const int outflanking = file_distance(wk, bk) - rank_distance(wk, bk);
    const Bitboard pawns = pieces(b, PAWN);
    const bool both_flanks = (pawns & QUEEN_SIDE) && (pawns & KING_SIDE);
    const bool almost_unwinnable = outflanking < 0 && !both_flanks;
    const bool infiltration = rank_of(wk) > RANK_4 || rank_of(bk) < RANK_5;

    const int complexity = 9 * popcount(e.passed[WHITE] | e.passed[BLACK])
                         + 12 * popcount(pawns)
                         + 9 * outflanking
                         + 21 * both_flanks
                         + 24 * infiltration
                         + 51 * !non_pawn_material(b)
                         - 43 * almost_unwinnable
                         - 110;

    auto sign = [](int x) { return (x > 0) - (x < 0); };
    const int u = sign(score.mg) * std::clamp(complexity + 50, -std::abs(score.mg), 0);
    const int v = sign(score.eg) * std::max(complexity, -std::abs(score.eg));
    const Score adjusted{score.mg + u, score.eg + v};
    return {adjusted, {u, v}, adjusted.eg > 0 ? WHITE : BLACK, complexity};
}

// The general scale-factor heuristics for the strong side.
inline int general_scale_factor(const Board& b, const PawnEntry& e, Color strong) {
    const Color weak = ~strong;
    if (is_opposite_coloured_bishops(b)) {
        if (non_pawn_material(b, WHITE) == NPM_BISHOP && non_pawn_material(b, BLACK) == NPM_BISHOP)
            return 18 + 4 * popcount(e.passed[strong]);
        return 22 + 3 * popcount(b.occupancy_bb(strong));
    }
    const Bitboard strong_pawns = b.piece_bb(strong, PAWN);
    if (non_pawn_material(b, WHITE) == NPM_ROOK && non_pawn_material(b, BLACK) == NPM_ROOK
        && count(b, strong, PAWN) - count(b, weak, PAWN) <= 1
        && bool(KING_SIDE & strong_pawns) != bool(QUEEN_SIDE & strong_pawns)
        && (KingAttacks[b.king_square(weak)] & b.piece_bb(weak, PAWN)))
        return 36;
    if (popcount(pieces(b, QUEEN)) == 1) {
        const Color minors = count(b, WHITE, QUEEN) == 1 ? BLACK : WHITE;
        return 37 + 3 * (count(b, minors, BISHOP) + count(b, minors, KNIGHT));
    }
    return std::min(64, 36 + 7 * count(b, strong, PAWN));
}

// Basilisk's own phase by piece count, 0 to 24, which the mate drive reads.
inline int piece_count_phase(const Board& b) {
    int phase = 0;
    for (int pt = PAWN; pt <= KING; pt++)
        phase += popcount(pieces(b, PieceType(pt))) * PHASE_W[pt];
    return std::min(phase, TOTAL_PHASE);
}

inline void record_pair(int (&out)[2], Score s) {
    out[0] = s.mg;
    out[1] = s.eg;
}

// The taper with an endgame scale factor of 64ths, in the reference's
// integer arithmetic.
inline int taper(Score s, int ph, int sf) {
    return (s.mg * ph + s.eg * (PHASE_MIDGAME - ph) * sf / 64) / PHASE_MIDGAME;
}

// The evaluator's counters an evaluation adds to; counted in the Tune and
// Diag flavours only (DIAG_COUNT).
struct Counters {
    int64_t& clamps;
    int64_t& pawn_probes;
    int64_t& pawn_hits;
};

// One evaluation's working state. Rec builds also fill an EvalRebuildRecord
// with every intermediate the parity test compares; the served value is the
// same in both. Exits = false takes neither lazy exit: the full function,
// which the lazy audit compares with what an exit served.
template<bool Rec, bool Exits = true>
class Evaluation {
public:
    Evaluation(const Board& board, PawnEntry (&pawn_table)[PAWN_TABLE_SIZE],
               ImbalanceEntry (&imbalance_table)[1 << IMBALANCE_TABLE_BITS],
               int& imbalance_generation, int tables_generation, Counters counters,
               EvalRebuildRecord* record)
        : b(board), pawn_table_(pawn_table), imbalance_table_(imbalance_table),
          imbalance_generation_(imbalance_generation), tables_generation_(tables_generation),
          counters_(counters), rec(record) {}

    // The value from the side to move's view, in centipawns.
    int value(bool& verdict);

    // After value(): the lazy exit taken (0 none, 1 or 2), the sum the
    // families gave (with Exits = false, every family's) and the non-pawn
    // material the exits' thresholds read.
    int exit_taken() const { return exit_; }
    Score sum() const { return sum_; }
    int npm() const { return npm_; }

private:
    const Board& b;
    PawnEntry (&pawn_table_)[PAWN_TABLE_SIZE];
    ImbalanceEntry (&imbalance_table_)[1 << IMBALANCE_TABLE_BITS];
    int& imbalance_generation_;
    int tables_generation_;
    Counters counters_;
    EvalRebuildRecord* rec;
    int exit_ = 0;
    Score sum_{};
    int npm_ = 0;
};

template<bool Rec, bool Exits>
int Evaluation<Rec, Exits>::value(bool& verdict) {
    verdict = false;
    const EvalParams& p = g_eval_params;

    // ---- The start: material, tables and imbalance -----------------------------
    const Score psq = material(b);
    const int imb = imbalance(b, imbalance_table_, imbalance_generation_, tables_generation_);
    Score score = psq + Score{imb, imb};
    const int ph = game_phase(b);

    // ---- Pawns -----------------------------------------------------------------
    PawnEntry& pe = pawn_probe(b, pawn_table_, counters_.pawn_probes, counters_.pawn_hits);
    const Score pawns{pe.score[WHITE][0] - pe.score[BLACK][0], pe.score[WHITE][1] - pe.score[BLACK][1]};
    // The matched ablation (EvalAblationBit): a masked family is computed and
    // recorded, and enters no sum, the lazy exits' included. Material, the
    // tables, the imbalance, the tempo and the endgame layer are never masked.
    if (!eval_ablated(EVAL_ABLATE_PAWNS))
        score += pawns;

    if constexpr (Rec) {
        record_pair(rec->psq, psq);
        rec->imbalance[0] = rec->imbalance[1] = imb;
        rec->ph = ph;
        record_pair(rec->family[EVAL_ABLATE_PAWNS], pawns);
    }

    // ---- First lazy exit ---------------------------------------------------------
    // A Rec or traced build evaluates past both exits, so its record holds
    // every family, and serves what a release build serves. A traced build
    // keeps the trace as it stood at the exit taken.
    constexpr bool Full = Rec || TRACED;
#ifdef TEXEL_TRACE
    g_trace.phase = piece_count_phase(b);
    auto snapshot = [&] {
        g_lazy_trace = g_trace;
        for (int s = 0; s < REST_SOURCE_NB; s++) {
            g_rest.lazy_mg[s] = g_rest.mg[s];
            g_rest.lazy_eg[s] = g_rest.eg[s];
        }
    };
#endif
    const int npm = non_pawn_material(b);
    int lazy = Exits && lazy_skip(score, LAZY_THRESHOLD_1, npm) ? 1 : 0;
#ifdef TEXEL_TRACE
    if (lazy)
        snapshot();
#endif
    if (Full || !lazy) {
        // ---- Pieces and mobility: the attack maps ----------------------------
        Maps m;
        m.blockers_for_king[WHITE] = slider_blockers(b, b.occupancy_bb(BLACK), b.king_square(WHITE));
        m.blockers_for_king[BLACK] = slider_blockers(b, b.occupancy_bb(WHITE), b.king_square(BLACK));
        init_maps<WHITE>(b, pe, m);
        init_maps<BLACK>(b, pe, m);
        const Score knights = piece_terms<WHITE, KNIGHT>(b, pe, m) - piece_terms<BLACK, KNIGHT>(b, pe, m);
        const Score bishops = piece_terms<WHITE, BISHOP>(b, pe, m) - piece_terms<BLACK, BISHOP>(b, pe, m);
        const Score rooks   = piece_terms<WHITE, ROOK  >(b, pe, m) - piece_terms<BLACK, ROOK  >(b, pe, m);
        const Score queens  = piece_terms<WHITE, QUEEN >(b, pe, m) - piece_terms<BLACK, QUEEN >(b, pe, m);
        const Score piece_sum = knights + bishops + rooks + queens;
        const Score mobility = m.mobility[WHITE] - m.mobility[BLACK];
        if (!lazy) {
            if (!eval_ablated(EVAL_ABLATE_PIECES))
                score += piece_sum;
            if (!eval_ablated(EVAL_ABLATE_MOBILITY))
                score += mobility;
        }

        // ---- King safety ------------------------------------------------------
        const Score king = king_terms<WHITE>(b, pe, m) - king_terms<BLACK>(b, pe, m);
        if (!lazy && !eval_ablated(EVAL_ABLATE_KING))
            score += king;

        // ---- Passed pawns, then the second lazy exit ---------------------------
        const Score passed = passer_terms<WHITE>(b, pe, m) - passer_terms<BLACK>(b, pe, m);
        if (!lazy) {
            if (!eval_ablated(EVAL_ABLATE_PASSERS))
                score += passed;
            if (Exits && lazy_skip(score, LAZY_THRESHOLD_2, npm)) {
                lazy = 2;
#ifdef TEXEL_TRACE
                snapshot();
#endif
            }
        }

        // ---- Threats and space ---------------------------------------------------
        if (Full || !lazy) {
            const Score threats = threat_terms<WHITE>(b, m) - threat_terms<BLACK>(b, m);
            const Score space = space_terms<WHITE>(b, pe, m) - space_terms<BLACK>(b, pe, m);
            if (!lazy) {
                if (!eval_ablated(EVAL_ABLATE_THREATS))
                    score += threats;
                if (!eval_ablated(EVAL_ABLATE_SPACE))
                    score += space;
            }
            if constexpr (Rec) {
                record_pair(rec->family[EVAL_ABLATE_THREATS], threats);
                record_pair(rec->family[EVAL_ABLATE_SPACE], space);
            }
        }

        if constexpr (Rec) {
            record_pair(rec->piece_type[0], knights);
            record_pair(rec->piece_type[1], bishops);
            record_pair(rec->piece_type[2], rooks);
            record_pair(rec->piece_type[3], queens);
            record_pair(rec->family[EVAL_ABLATE_PIECES], piece_sum);
            record_pair(rec->family[EVAL_ABLATE_MOBILITY], mobility);
            record_pair(rec->family[EVAL_ABLATE_KING], king);
            record_pair(rec->family[EVAL_ABLATE_PASSERS], passed);
        }
    }

    // ---- Winnable: the complexity adjustment and the strong side -------------------
    const Winnable w = winnable(b, pe, score);
    const int general_sf = general_scale_factor(b, pe, w.strong);

    // ---- The endgame layer and the scale factor (§6.8) -----------------------------
    // The layer sees the converted taper at a factor of 64 with the mate
    // drive's addition, tapered by the piece-count phase. Where it overrides
    // or scales, its result stands; where it is silent, the general factor
    // scales the endgame half.
    const int t = to_centipawns(taper(w.adjusted, ph, 64));
    const int phase24 = piece_count_phase(b);
    const int drive = mate_drive_eg(b, t, phase24) * (TOTAL_PHASE - phase24) / TOTAL_PHASE;
    int kind;
    const int layered = endgame_layer<false, false>(b, t + drive, verdict, kind);
    const int v = taper(w.adjusted, ph, general_sf);
    const int white = kind != LAYER_SILENT ? layered : to_centipawns(v) + drive;

    if constexpr (Rec) {
        rec->lazy = lazy;
        record_pair(rec->complexity, w.complexity);
        record_pair(rec->adjusted, w.adjusted);
        rec->strong = int(w.strong);
        rec->general_sf = general_sf;
        rec->sf = kind == LAYER_SILENT ? general_sf : -1;
        rec->v = v;
        rec->layer = kind;
        rec->drove = drive != 0;
        rec->layer_input = t + drive;
        rec->verdict = verdict;
    }

    // ---- The side to move, the tempo and the bounds ------------------------------------
    // An override is the layer's own value, and a draw verdict is a 0: no
    // tempo, no clamp.
    const int stm = b.turn() == WHITE ? white : -white;
    int served = stm;
    if (kind != LAYER_OVERRIDE && !verdict) {
        served = stm + p.tempo * CONVERSION_S / 1024;
        if (std::abs(served) >= KNOWN_WIN) {
            DIAG_COUNT(++counters_.clamps);
            served = served > 0 ? KNOWN_WIN - 1 : -(KNOWN_WIN - 1);
        }
    }
#ifdef TEXEL_TRACE
    g_rest.lazy = lazy;
    g_rest.sum_mg = score.mg;
    g_rest.sum_eg = score.eg;
    g_rest.complexity = w.raw;
    g_rest.ph = ph;
    g_rest.general_sf = general_sf;
    g_rest.t = t;
    g_rest.drive = drive;
    g_rest.white = white;
    g_rest.step = g_endgame_step;
    g_rest.step.verdict = verdict;
    g_rest.served = served;
#endif
    exit_ = lazy;
    sum_ = score;
    npm_ = npm;
    return served;
}

} // namespace rb
