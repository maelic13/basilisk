#pragma once

// The rebuilt evaluation: the classical form's families in its own order,
// then Basilisk's endgame layer, the conversion to centipawns, the tempo and
// the bounds (analysis/c31_rebuild_2026-11-25.md §6 and its Amendment 1).
// Part of the evaluator's single translation unit: included by src/eval.cpp
// alone when BASILISK_EVAL_REBUILD is defined. Include it nowhere else.

#include "eval_rb_material.h"
#include "eval_rb_pawns.h"

namespace rb {

// The one conversion from the form's units to centipawns: cp = v * S / 1024,
// truncated toward zero. A gauge, fixed once (§7); not a fitted value.
inline constexpr int CONVERSION_S = 628;

inline int to_centipawns(int v) { return v * CONVERSION_S / 1024; }

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
        rec->psq[0] = psq.mg;
        rec->psq[1] = psq.eg;
        rec->imbalance[0] = rec->imbalance[1] = imb;
        rec->ph = ph;
        rec->family[EVAL_ABLATE_PAWNS][0] = pawns.mg;
        rec->family[EVAL_ABLATE_PAWNS][1] = pawns.eg;
    }

    const int v = taper(score, ph, 64);
    const int cp = to_centipawns(v);
    return (b.turn() == WHITE ? cp : -cp) + p.tempo * CONVERSION_S / 1024;
}

} // namespace rb
