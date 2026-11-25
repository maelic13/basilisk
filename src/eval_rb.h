#pragma once

// The rebuilt evaluation: the classical form's families in its own order,
// then Basilisk's endgame layer, the conversion to centipawns, the tempo and
// the bounds (analysis/c31_rebuild_2026-11-25.md §6 and its Amendment 1).
// Part of the evaluator's single translation unit: included by src/eval.cpp
// alone when BASILISK_EVAL_REBUILD is defined. Include it nowhere else.

#include "eval_rb_material.h"
#include "eval_rb_threats.h"

namespace rb {

// The one conversion from the form's units to centipawns: cp = v * S / 1024,
// truncated toward zero. A gauge, fixed once (§7); not a fitted value.
inline constexpr int CONVERSION_S = 628;

inline int to_centipawns(int v) { return v * CONVERSION_S / 1024; }

// The lazy exits' structure constants: the families after an exit are not
// evaluated once half the halves' sum exceeds its threshold plus a 64th of
// the non-pawn material.
inline constexpr int LAZY_THRESHOLD_1 = 1400;
inline constexpr int LAZY_THRESHOLD_2 = 1300;

inline bool lazy_skip(Score s, int threshold, int npm) {
    return std::abs(s.mg + s.eg) / 2 > threshold + npm / 64;
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

// One evaluation's working state. Rec builds also fill an EvalRebuildRecord
// with every intermediate the parity test compares; the served value is the
// same in both.
template<bool Rec>
class Evaluation {
public:
    Evaluation(const Board& board, PawnEntry (&pawn_table)[PAWN_TABLE_SIZE],
               ImbalanceEntry (&imbalance_table)[1 << IMBALANCE_TABLE_BITS],
               int& imbalance_generation, int tables_generation, EvalRebuildRecord* record)
        : b(board), pawn_table_(pawn_table), imbalance_table_(imbalance_table),
          imbalance_generation_(imbalance_generation), tables_generation_(tables_generation),
          rec(record) {}

    // The value from the side to move's view, in centipawns.
    int value(bool& verdict);

private:
    const Board& b;
    PawnEntry (&pawn_table_)[PAWN_TABLE_SIZE];
    ImbalanceEntry (&imbalance_table_)[1 << IMBALANCE_TABLE_BITS];
    int& imbalance_generation_;
    int tables_generation_;
    EvalRebuildRecord* rec;
};

template<bool Rec>
int Evaluation<Rec>::value(bool& verdict) {
    verdict = false;
    const EvalParams& p = g_eval_params;

    // ---- The start: material, tables and imbalance -----------------------------
    const Score psq = material(b);
    const int imb = imbalance(b, imbalance_table_, imbalance_generation_, tables_generation_);
    Score score = psq + Score{imb, imb};
    const int ph = game_phase(b);

    // ---- Pawns -----------------------------------------------------------------
    PawnEntry& pe = pawn_probe(b, pawn_table_);
    const Score pawns{pe.score[WHITE][0] - pe.score[BLACK][0], pe.score[WHITE][1] - pe.score[BLACK][1]};
    score += pawns;

    if constexpr (Rec) {
        record_pair(rec->psq, psq);
        rec->imbalance[0] = rec->imbalance[1] = imb;
        rec->ph = ph;
        record_pair(rec->family[EVAL_ABLATE_PAWNS], pawns);
    }

    // ---- First lazy exit ---------------------------------------------------------
    // A Rec build evaluates past both exits, so the record holds every family,
    // and serves what a release build serves.
    const int npm = non_pawn_material(b);
    int lazy = lazy_skip(score, LAZY_THRESHOLD_1, npm) ? 1 : 0;
    if (Rec || !lazy) {
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
        if (!lazy)
            score += piece_sum + mobility;

        // ---- King safety ------------------------------------------------------
        const Score king = king_terms<WHITE>(b, pe, m) - king_terms<BLACK>(b, pe, m);
        if (!lazy)
            score += king;

        // ---- Passed pawns, then the second lazy exit ---------------------------
        const Score passed = passer_terms<WHITE>(b, pe, m) - passer_terms<BLACK>(b, pe, m);
        if (!lazy) {
            score += passed;
            if (lazy_skip(score, LAZY_THRESHOLD_2, npm))
                lazy = 2;
        }

        // ---- Threats and space ---------------------------------------------------
        if (Rec || !lazy) {
            const Score threats = threat_terms<WHITE>(b, m) - threat_terms<BLACK>(b, m);
            const Score space = space_terms<WHITE>(b, pe, m) - space_terms<BLACK>(b, pe, m);
            if (!lazy)
                score += threats + space;
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
    if constexpr (Rec)
        rec->lazy = lazy;

    const int v = taper(score, ph, 64);
    const int cp = to_centipawns(v);
    return (b.turn() == WHITE ? cp : -cp) + p.tempo * CONVERSION_S / 1024;
}

} // namespace rb
