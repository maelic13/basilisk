#pragma once

#include <cstdint>
#include <span>
#include <type_traits>

// Tunable evaluation weights for Basilisk's HCE eval.
//
// Defaults are byte-for-byte equivalent to the inline constants they replace
// in eval.cpp. init_eval_tables(p) must be called again whenever p changes
// (it bakes material + PST values into the runtime MG_TABLE / EG_TABLE).
//
// X-macro registry (for the Step 2.1 loader and Step 2.2 tuner):
//   EVAL_PARAM_LIST(X) expands X(display_name, member, length)
//   - For array members (length > 1): p.member is int[length]; iterate p.member[i].
//   - For scalar members (length == 1): p.member is int; access as (&p.member)[0].
//   - For PST sub-rows (e.g. pst_mg[0]): p.pst_mg[0] decays to int* of length 64.

#include "eval_rb_params.h"

// ---- Helpers for iterating EVAL_PARAM_LIST generically --------------------
// Returns int* to the first element of a parameter field, whether it is a
// plain scalar (int) or a fixed-size array (int[N] / int[R][C] sub-row).
// Usage in a macro expansion: eval_param_ptr(p.member) or eval_param_cptr(p.member).
template<typename T>
inline int* eval_param_ptr(T& v) {
    if constexpr (std::is_array_v<T>) return v;
    else return &v;
}
template<typename T>
inline const int* eval_param_cptr(const T& v) {
    if constexpr (std::is_array_v<T>) return v;
    else return &v;
}

// Span forms. The X-macro already knows each field's length, so carrying it
// alongside a bare pointer -- in a std::pair, or as a second loop bound -- is
// an invitation to pass the wrong one. A span makes the length intrinsic and
// bounds-checkable, and compiles to the same code.
template<typename T>
inline std::span<int> eval_param_span(T& v, size_t len) {
    return std::span<int>(eval_param_ptr(v), len);
}
template<typename T>
inline std::span<const int> eval_param_cspan(const T& v, size_t len) {
    return std::span<const int>(eval_param_cptr(v), len);
}

// ---- X-macro parameter registry -------------------------------------------
// Usage: EVAL_PARAM_LIST(X) expands X(display_name, member, length).
//   Arrays (length > 1): p.member[i] for i in [0, length).
//   Scalars (length == 1): (&p.member)[0] gives the single int.
//   PST sub-rows (e.g. pst_mg[0]): int[64] decaying to int*, p.pst_mg[0][i].

// ---- Texel trace infrastructure (TEXEL_TRACE builds only) -----------------
// These definitions depend on EVAL_PARAM_LIST and therefore live after it.
#ifdef TEXEL_TRACE

// Group enum: one enumerator per EVAL_PARAM_LIST entry, in order.
enum EvalParamGroup : int {
#define X(name, member, len) EPG_##name,
    EVAL_PARAM_LIST(X)
#undef X
    EPG_COUNT
};

// Per-group lengths (same order as EVAL_PARAM_LIST).
constexpr int EVAL_PARAM_LENS[] = {
#define X(name, member, len) (len),
    EVAL_PARAM_LIST(X)
#undef X
};

// Flat offset: number of scalar elements before group g.
constexpr int eval_param_offset(int g) {
    int off = 0;
    for (int i = 0; i < g; i++) off += EVAL_PARAM_LENS[i];
    return off;
}

// Total flat element count across all groups.
constexpr int EVAL_PARAM_FLAT_SIZE = eval_param_offset(EPG_COUNT);

// Per-trace record populated by Evaluator::evaluate() when TEXEL_TRACE is
// defined. mg[i] / eg[i] hold the signed count contribution of flat param i
// to the middlegame / endgame accumulator (positive = white favoured).
// phase is the computed game phase [0..24]. What the score holds beyond the
// traced terms -- the frozen additions, the endgame step, the damping and the
// lazy checkpoint -- is recorded in g_rest (EvalRestTrace below).
struct EvalTrace {
    int16_t mg[EVAL_PARAM_FLAT_SIZE];
    int16_t eg[EVAL_PARAM_FLAT_SIZE];
    int phase;
};

// Thread-local trace populated by every evaluate() call in TEXEL_TRACE builds.
// Caller must zero this before calling evaluate().
extern thread_local EvalTrace g_trace;

// Reconstruct the linear part of the eval from trace counts and weights.
// Returns (mg_dot * phase + eg_dot * (24 - phase)) / 24.
int reconstruct(const EvalTrace& tr, const EvalParams& w);

// What `rest` is made of, recorded beside the trace so that it can be
// reported by source. The untraced additions to the running sum, by the term
// that made them; the sum the final taper read; that taper; and the score
// apply_endgame returned from it. All in White's view. evaluate() writes it;
// the caller zeroes it with g_trace.
// The rebuilt evaluation's untraced additions: the terms its knobs enter
// through truncating or squaring arithmetic.
enum EvalRestSource : int {
    REST_IMBALANCE,    // (White's polynomial - Black's) / 16, both halves
    REST_KING_DANGER,  // the danger index's square over 4096 and sixteenth
    REST_SPACE,        // the space count times the weight squared, over 256
    REST_SOURCE_NB
};
// What apply_endgame did to the tapered score it was given: left it, scaled it
// by num / den (C++ integer division), or replaced it outright with a known
// result. `verdict` is its draw verdict.
enum EvalEndgameKind : int { EG_UNCHANGED, EG_SCALED, EG_OVERRIDE };
struct EvalEndgameStep {
    int kind;
    int num, den;
    bool verdict;
};
// The rebuilt evaluation's record of how it served its value, in the
// classical form's units until `t`, then centipawns; White's view except
// `served`. The traced build evaluates past both lazy exits: `mg`/`eg` and
// g_trace hold every family, and `lazy_mg`/`lazy_eg` and g_lazy_trace what
// they held at the exit a release build takes (`lazy` 1 or 2), the families
// that build serves from.
struct EvalRestTrace {
    int mg[REST_SOURCE_NB];
    int eg[REST_SOURCE_NB];
    int lazy_mg[REST_SOURCE_NB];
    int lazy_eg[REST_SOURCE_NB];
    int lazy;
    int sum_mg, sum_eg;    // the sum served from, before winnable
    int complexity;        // winnable's complexity, which no slot moves
    int ph;                // the phase, 0 to 128
    int general_sf;        // the general heuristics' factor
    int t;                 // the converted taper at a factor of 64
    int drive;             // the mate drive's tapered addition to t
    int white;             // after the endgame layer
    EvalEndgameStep step;  // the layer on t + drive
    int served;            // side to move, before the rule-50 damping
};
extern thread_local EvalRestTrace g_rest;
extern thread_local EvalTrace g_lazy_trace;
// apply_endgame's last step, set by every call.
extern thread_local EvalEndgameStep g_endgame_step;

#endif // TEXEL_TRACE
