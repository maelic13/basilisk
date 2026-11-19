#pragma once

// Shared by every family module: the Texel trace's storage and macros, the
// game phase, and the middlegame/endgame score pair each family returns.
// Part of the evaluator's single translation unit: included by src/eval.cpp
// alone, which the engine library, the Texel tuner and the oracle build all
// compile as one file. Include it nowhere else.

#include "eval.h"
#include "attacks.h"
#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <utility>

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

// The untraced remainder by source (EvalRestTrace): an untraced endgame
// addition, recorded where it enters the sum.
thread_local EvalRestTrace g_rest{};
#define TR_REST_EG(source, val) (g_rest.eg[REST_##source] += (val))

#else // !TEXEL_TRACE
#define TR_MG(group, idx, val)   (void)0
#define TR_EG(group, idx, val)   (void)0
#define TR_BOTH(group, idx, val) (void)0
#define TR_PST_MG(pt, sq, val)   (void)0
#define TR_PST_EG(pt, sq, val)   (void)0
#define TR_REST_EG(source, val)  (void)0
#endif // TEXEL_TRACE

// Phase weights (frozen; not in EvalParams)
static constexpr int PHASE_W[PIECE_TYPE_NB] = {0, 0, 1, 1, 2, 4, 0};
static constexpr int TOTAL_PHASE = 24;

static Square forward_square(Color c, Square sq) {
    const int to = int(sq) + (c == WHITE ? 8 : -8);
    return (to >= 0 && to < 64) ? Square(to) : SQ_NONE;
}

// A family's contribution, in White's view: what it adds to the middlegame
// and the endgame sums before the taper.
struct EvalScore {
    int mg = 0;
    int eg = 0;
    EvalScore& operator+=(const EvalScore& o) {
        mg += o.mg;
        eg += o.eg;
        return *this;
    }
};

// The taper every read of the running sum uses.
static inline int taper(const EvalScore& s, int phase) {
    return (s.mg * phase + s.eg * (TOTAL_PHASE - phase)) / TOTAL_PHASE;
}
