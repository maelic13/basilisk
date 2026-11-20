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
#ifdef BASILISK_TUNE
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#endif
#include "board_trace.h"

// The evaluator is this one translation unit; its families live in the
// headers below, which nothing else includes. The order of the sum and
// its three reads are evaluate_full()'s, at the end of this file.
#include "eval_common.h"
#include "eval_endgame.h"
#include "eval_material.h"
#include "eval_mobility.h"
#include "eval_attacks.h"
#include "eval_pawns.h"
#include "eval_pieces.h"
#include "eval_threats.h"
#include "eval_king.h"

#ifdef TEXEL_TRACE
// Reconstruction: linear tapered score from trace counts and weights.
int reconstruct(const EvalTrace& tr, const EvalParams& w) {
    int mg_dot = 0, eg_dot = 0;
#define X(name, member, len) \
    { const auto weights = eval_param_cspan(w.member, (len)); \
      int base = eval_param_offset(EPG_##name); \
      for (size_t i = 0; i < weights.size(); i++) { \
          mg_dot += tr.mg[base + int(i)] * weights[i]; \
          eg_dot += tr.eg[base + int(i)] * weights[i]; \
      } }
    EVAL_PARAM_LIST(X)
#undef X
    return (mg_dot * tr.phase + eg_dot * (24 - tr.phase)) / 24;
}
#endif // TEXEL_TRACE

// Global evaluation parameters (defaults in EvalParams.h).
// Call init_eval_tables(g_eval_params) again after changing these.
EvalParams g_eval_params;

// Lazy-eval margin (Step 3.11): if the cheap (material/PST/imbalance/pawns/minor)
// tapered score exceeds this, the expensive positional block is skipped. Tuned
// under a lazy-on-vs-off SPRT; conservative start.
[[maybe_unused]] static constexpr int LAZY_MARGIN  = 700;  // unused under TEXEL_TRACE (lazy eval off there)
// Lazy-audit "did not fire" marker (any value the eval can't produce).
[[maybe_unused]] static constexpr int VALUE_NONE_SENTINEL = INT_MIN;

// Counts init_eval_tables() calls: the parameters a cached value was computed on.
static int g_eval_tables_generation = 0;

void init_eval_tables(const EvalParams& p) {
    ++g_eval_tables_generation;
    // Build the KPK bitbase at start-up, not on its first probe: that probe
    // runs inside whichever search first reaches KPK, late in a game, and the
    // ~10 ms build was charged to that move's clock. call_once keeps the probe
    // path safe for callers that never ran this.
    std::call_once(g_kpk_once, kpk_init);

    for (int pt = PAWN; pt <= KING; pt++) {
        for (int sq = 0; sq < 64; sq++) {
            PIECE_SQUARE[make_piece(WHITE, PieceType(pt))][sq] = {
                p.mg_val[pt] + p.pst_mg[pt - 1][sq], p.eg_val[pt] + p.pst_eg[pt - 1][sq]};
            // Black: mirror rank
            int msq = sq ^ 56;
            PIECE_SQUARE[make_piece(BLACK, PieceType(pt))][sq] = {
                -(p.mg_val[pt] + p.pst_mg[pt - 1][msq]), -(p.eg_val[pt] + p.pst_eg[pt - 1][msq])};
        }
    }
}

Evaluator::Evaluator() {
    clear_pawn_table();
}

void Evaluator::clear_pawn_table() {
    std::memset(pawn_table_, 0, sizeof(pawn_table_));
    std::memset(imbalance_table_, 0, sizeof(imbalance_table_));
    imbalance_generation_ = g_eval_tables_generation;
}

// ---- Main evaluation -------------------------------------------------------

int Evaluator::evaluate_undamped(const Board& b) {
    bool verdict = false;
    return evaluate_full(b, verdict, false);
}

bool Evaluator::draw_verdict(const Board& b) {
    bool verdict = false;
    evaluate_full(b, verdict, true);
    return verdict;
}

// A verdict query evaluates a node the search has already counted, so it
// adds nothing to the endgame census or the lazy audit; eval_calls counts it,
// since it pays for an evaluation.
int Evaluator::evaluate_full(const Board& b, bool& verdict, bool query) {
#ifdef BASILISK_BOARD_TRACE
    board_trace::EvalScope trace_eval_scope;
#endif
    DIAG_COUNT(++eval_calls);
#ifdef BASILISK_TUNE
    if (diag_endgames && !query)
        record_endgame_occurrence(b, endgame_occurrence);
#endif
    const EvalParams& p = g_eval_params;
    int phase = 0;
    EvalScore sum;

    // ---- The order of the sum, and its three reads -------------------------
    // Every family's contribution enters `sum` through one call below. The
    // running sum is read three times, and each read must see exactly the
    // terms before it:
    //   1. the lazy checkpoint reads the cheap block alone (material and
    //      tables, imbalance, the pawn cache and the cheap passer terms, the
    //      bishop pair, the rook and knight bonuses);
    //   2. the mate drive reads every family, without the tempo or itself;
    //   3. the final taper reads everything.
    // A term moved across the lazy checkpoint changes every lazy-served score
    // it touches; test_eval's lazy-checkpoint test fails when one is.

    // ---- The cheap block ----------------------------------------------------
    sum += material(b, imbalance_table_, imbalance_generation_, g_eval_tables_generation, phase);
    PawnEntry pawns;
    pawn_probe(b, pawn_table_, pawn_probes, pawn_hits, pawns);
    sum += pawns_cheap(b, pawns);
    sum += pieces_cheap(b);

    // ---- Lazy eval checkpoint: read 1 ----------------------------------------
    // If the cheap block's tapered score is decisive, skip the attack-map
    // families (mobility, king safety, threats, the full pawn and piece terms,
    // space) and finish with endgame scaling -- which still runs, so
    // KBNK/KPK/draw-scaling survive the skip and the draw verdict is set on
    // this path too. Disabled under TEXEL_TRACE so the tuner fits the full eval
    // (--verify exact).
#ifndef TEXEL_TRACE
    // Under the lazy audit (Tune and Diag builds) the skip does NOT return —
    // the full tail runs for measurement and the final return serves the
    // remembered lazy value, so the audit is behaviour-identical
    // (bench-identical) when on.
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
    int lazy_served = VALUE_NONE_SENTINEL;
#endif
    {
        int lazy = taper(sum, phase);
        int lazy_abs = lazy < 0 ? -lazy : lazy;
        if (lazy_abs > LAZY_MARGIN) {
            const int score = apply_endgame(b, lazy, verdict);
            const int served = (b.turn() == WHITE) ? score : -score;
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
            if (!diag_lazy || query)
                return served;
            lazy_served = served;
#else
            return served;
#endif
        }
    }
#endif

    // ---- The full block -------------------------------------------------------
    AttackInfo ai;
    fill_attack_info(b, pawns, ai);
    sum += threats(b, ai);
    sum += king(b, ai);
    sum += pawns_full(b, ai);
    sum += pieces_full(b, ai, phase);
    sum += mobility_space(b, ai.mobility, ai.pawn_atk);

    // ---- Endgame mate drive: read 2 -------------------------------------------
    sum += mate_drive(b, sum, phase);

    // ---- Tempo bonus -------------------------------------------------------
    {
        int tempo_sign = (b.turn() == WHITE) ? 1 : -1;
        sum.mg += tempo_sign * p.tempo;
        TR_MG(Tempo, 0, tempo_sign);
    }

    // ---- Taper and return: read 3 -------------------------------------------
    int score = taper(sum, phase);
#ifdef TEXEL_TRACE
    g_rest.sum_mg  = sum.mg;
    g_rest.sum_eg  = sum.eg;
    g_rest.tapered = score;
#endif

    // ---- Endgame scaling + knowledge ----------------------------------------
    score = apply_endgame(b, score, verdict);
#ifdef TEXEL_TRACE
    g_rest.endgame = score;
#endif

    const int full_served = (b.turn() == WHITE) ? score : -score;
#if !defined(TEXEL_TRACE) && (defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC))
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
    { const auto values = eval_param_cspan(p.member, (len)); \
      for (size_t i = 0; i < values.size(); i++) \
          std::cout << #name << " " << i << " " << values[i] << "\n"; }
    EVAL_PARAM_LIST(X)
#undef X
    std::cout.flush();
}

// Load "name index value" lines from filename into p, then rebuild eval tables.
// Unknown names are a hard error; indices out of range are a hard error.
static void load_eval_params(const char* filename, EvalParams& p) {
    // A span rather than a {pointer, length} pair: the length travels with the
    // data, so the range check below cannot consult the wrong one.
    std::unordered_map<std::string, std::span<int>> table;
#define X(name, member, len) \
    table[#name] = eval_param_span(p.member, (len));
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
        const std::span<int> slot = it->second;
        if (idx < 0 || size_t(idx) >= slot.size())
            throw std::runtime_error("Index " + std::to_string(idx)
                                     + " out of range for '" + name
                                     + "' (length " + std::to_string(slot.size())
                                     + ") at line " + std::to_string(lineno));
        slot[size_t(idx)] = val;
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
