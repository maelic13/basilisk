// The legacy node kernel: quiescence, negamax and the helpers only they call.
// The driver in search_worker.cpp reaches the kernel through root_search(),
// kernel_begin_search() and kernel_new_game() alone; the history and
// correction policy it trains is in search_history.cpp. The kernel owns the
// per-ply stack, the LMR table and the history tables, and writes the
// driver-visible results into SearchState: the PV table, selective depth,
// root effort, node counts and diagnostics.

#include "search_worker.h"
#include "constants.h"
#include "syzygy.h"
#include <algorithm>
#include <bit>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <utility>

// ---- LMR table -------------------------------------------------------------

#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
#define TRACE_DECISION(...) state_.trace.record(__VA_ARGS__)
#else
#define TRACE_DECISION(...) ((void)0)
#endif

#ifdef BASILISK_ABLATION
#define ABLATED(bit) (((config_.limits.ablation_mask >> (bit)) & 1) != 0)
#else
#define ABLATED(bit) false
#endif

// A probed WDL as a search value and the bound it proves (Stockfish's form):
// a win is at least `tablebaseValue - ply`, a loss at most its negation, and a
// result the rule-50 counter spoils is an exact +/-2. With the rule off a
// spoiled result counts in full.
struct TablebaseProbe { int value; TTFlag bound; };

static TablebaseProbe tablebase_probe(Syzygy::Wdl wdl, int ply, bool rule50) {
    const int win = tablebaseValue - ply;
    switch (wdl) {
        case Syzygy::Wdl::Win:
            return {win, TT_BETA};
        case Syzygy::Wdl::CursedWin:
            return rule50 ? TablebaseProbe{2, TT_EXACT} : TablebaseProbe{win, TT_BETA};
        case Syzygy::Wdl::Draw:
            return {0, TT_EXACT};
        case Syzygy::Wdl::BlessedLoss:
            return rule50 ? TablebaseProbe{-2, TT_EXACT} : TablebaseProbe{-win, TT_ALPHA};
        case Syzygy::Wdl::Loss:
            return {-win, TT_ALPHA};
    }
    std::unreachable();
}

void Searcher::init_lmr(float base, float divisor) {
    // Phase 6.7: table stored in 1024ths of a ply (fractional LMR). The floor
    // identity int(1024*x) >> 10 == int(x) keeps the base reduction identical to
    // the old integer table at default knobs; the finer resolution only matters
    // once 6.9 SPSA sets sub-ply adjustments. Consumers shift back with `>> 10`.
    for (int d = 1; d < 64; d++)
        for (int m = 1; m < 64; m++)
            lmr_table_[d][m] = int(1024.0f * (base + std::log(d) * std::log(m) / divisor));
}

void Searcher::tt_store(Key key, int depth, int score, TTFlag flag, Move m,
                        int ply, int static_eval) {
    DIAG_COUNT(++state_.diag.tt_stores);
    if (shared_.tt.store(key, depth, score, flag, m, ply, static_eval))
        DIAG_COUNT(++state_.diag.tt_stores_same_key);
}

// ---- Move loop bookkeeping ----------------------------------------------

static constexpr int MAX_TRACKED_QUIETS = 64;
static constexpr int MAX_TRACKED_BAD_CAPS = 32;

// ---- Quiescence search -----------------------------------------------------

int Searcher::quiescence(int alpha, int beta, int ply, int qply, SearchStack* ss) {
    record_node();
    if ((state_.nodes & 2047) == 0) check_stop();
    if (state_.stopped) return 0;
    if (ply >= MAX_PLY) return evaluator_.evaluate(*state_.board);
    if (state_.board->is_draw(ply)) return 0;

    bool in_check = state_.board->is_in_check();
    DIAG_COUNT(state_.diag.qs_nodes++);

    // TT probe
    Key hash = state_.board->position_key();
    TTEntry tte{};
    bool tt_found = shared_.tt.probe_copy(hash, tte);
    DIAG_COUNT(state_.diag.tt_probes++);
    if (tt_found) DIAG_COUNT(state_.diag.tt_hits++);
    Move tt_move = MOVE_NONE;
    int  tt_score = VALUE_NONE;       // hoisted (Step 6.1) for the stand-pat tighten
    TTFlag tt_flag = TT_NONE;
    if (tt_found) {
        tt_move = move_from_tt(tte.move16);
        tt_score = TranspositionTable::score_from_tt(tte.score, ply, state_.board->rule50_count());
        tt_flag = TTFlag(tte.flag_age & 3);
        if (tt_flag == TT_EXACT
            || (tt_flag == TT_ALPHA && tt_score <= alpha)
            || (tt_flag == TT_BETA && tt_score >= beta)) {
            TRACE_DECISION(TraceEvent::QsTtCutoff, ply, 0, tt_move,
                           alpha, beta, tt_score, -1, VALUE_NONE, VALUE_NONE,
                           0, 0, VALUE_NONE, tt_score);
            return tt_score;
        }
    }

    if (in_check) {
        // No qsearch-depth cap for evasion nodes: a static eval of an
        // in-check position is not a valid bound and can mask mates in long
        // check chains, poisoning parent TT stores (search audit 7 / 8.1e).
        // The ply >= MAX_PLY guard at the top of quiescence() remains the
        // termination backstop; check chains cannot exceed it.
        MoveList legal;
        state_.board->gen_legal(legal);
        int best = -INF_SCORE;
        DIAG_COUNT(state_.diag.qs_evasion_nodes++);
        bool has_legal = false;
        for (Move m : legal) {
            has_legal = true;
            do_move(ss, m);
            int s = -quiescence(-beta, -alpha, ply + 1, qply + 1, ss + 1);
            undo_move(ss, m);
            if (state_.stopped) return 0;
            if (s > best) best = s;
            if (s > alpha) alpha = s;
            if (alpha >= beta) {
                TRACE_DECISION(TraceEvent::QsBetaCutoff, ply, 0, m,
                               alpha, beta, VALUE_NONE, -1, VALUE_NONE, VALUE_NONE,
                               0, 0, VALUE_NONE, s);
                best = s;
                break;
            }
        }
        return has_legal ? best : -(MATE_SCORE - ply);
    }

    // Stand-pat evaluation
    int raw_eval;
    if (tt_found && tte.static_eval != TranspositionTable::INF_EVAL)
        raw_eval = tte.static_eval;
    else
        raw_eval = evaluator_.evaluate(*state_.board);

    int stand_pat = raw_eval;
    stand_pat += correction_value(state_.board->turn(), *state_.board, ss);
    stand_pat = std::clamp(stand_pat, -(tablebaseWinInMaxPly - 1), tablebaseWinInMaxPly - 1);
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
    const int correction = stand_pat - raw_eval;
#endif

    // Step 6.1 mirror: tighten the stand-pat with the TT bound when it proves a
    // better estimate (a fail-high above it / fail-low below it). The raw eval
    // stored as the TT static_eval (raw_eval) is unchanged.
    if (tt_found && tt_score != VALUE_NONE
        && ((tt_flag == TT_BETA  && tt_score > stand_pat)
            || (tt_flag == TT_ALPHA && tt_score < stand_pat)))
        stand_pat = tt_score;

    if (stand_pat >= beta) {
        TRACE_DECISION(TraceEvent::QsStandPatCutoff, ply, 0, MOVE_NONE,
                       alpha, beta, stand_pat, -1, correction, VALUE_NONE,
                       0, 0, 0, stand_pat);
        tt_store(hash, 0, stand_pat, TT_BETA, MOVE_NONE, ply, raw_eval);
        return stand_pat;
    }

    // Delta pruning: even capturing the best possible piece can't raise
    // alpha. NOTE (8.1f): the audit's fail-soft return (stand_pat + margin)
    // was implemented and MEASURED to break the KBNK fixed-depth conversion
    // canary -- the fail-hard alpha echo is load-bearing for mate-range
    // bounds under the current fail-hard qsearch + 6.1 stand-pat tightening.
    // (Likewise, seeding the final store from the tightened stand-pat
    // scrambled KQK mate distances: a TT_BETA-tightened value is not a
    // provable upper bound.) Consistent fail-soft is the Phase 10.4
    // bound-shaping job; do not change this return in isolation.
    if (stand_pat < alpha - PIECE_VALUE[QUEEN] - 200) {
        TRACE_DECISION(TraceEvent::QsDeltaPrune, ply, 0, MOVE_NONE,
                       alpha, beta, stand_pat, -1, correction, VALUE_NONE,
                       0, 0, PIECE_VALUE[QUEEN] + 200, alpha);
        return alpha;
    }

    if (stand_pat > alpha) alpha = stand_pat;

    if (qply >= MAX_QSEARCH_PLY) return stand_pat;  // fail-soft (8.1f)

    MoveList captures;
    state_.board->gen_legal_captures(captures);

    // Score captures: MVV + cap_hist; prefer TT move
    ScoredMove* sm = state_.move_buffers[ply][0];
    int nm = 0;
    for (Move m : captures) {
        PieceType atk = type_of(state_.board->piece_on(from_sq(m)));
        PieceType cap = (move_type(m) == EN_PASSANT) ? PAWN : type_of(state_.board->piece_on(to_sq(m)));
        int score = (m == tt_move) ? 10'000'000
                  : PIECE_VALUE[cap] * 16 - PIECE_VALUE[atk] + hist_.capture[atk][to_sq(m)][cap];
        sm[nm++] = {m, score};
    }

    Move best_move = MOVE_NONE;
    int  orig_alpha = alpha;

    for (int i = 0; i < nm; i++) {
        Move m = pick_next(sm, i, nm);

        const MoveType mt = move_type(m);
        const bool is_promo = mt == PROMOTION;
        const Piece target = state_.board->piece_on(to_sq(m));
        const int captured_value = (mt == EN_PASSANT) ? PIECE_VALUE[PAWN]
                                 : (target != NO_PIECE) ? PIECE_VALUE[type_of(target)]
                                 : 0;
        const int promotion_gain = is_promo ? PIECE_VALUE[promo_type(m)] - PIECE_VALUE[PAWN] : 0;
        const int tactical_gain = captured_value + promotion_gain;

        bool gives_check_known = false;
        bool gives_check = false;
        auto move_gives_check = [&]() {
            if (!gives_check_known) {
                gives_check = state_.board->gives_check(m);
                gives_check_known = true;
            }
            return gives_check;
        };
        if (!is_promo
            && stand_pat + tactical_gain + 150 <= alpha
            && !move_gives_check()) {
            TRACE_DECISION(TraceEvent::QsFutilityPrune, ply, 0, m,
                           alpha, beta, stand_pat, -1, correction, VALUE_NONE,
                           i, 0, tactical_gain + 150, alpha);
            continue;
        }

        const int see_threshold = std::clamp(alpha - stand_pat - 200, -800, 200);
        if (!state_.board->see_ge(m, see_threshold)) {
            TRACE_DECISION(TraceEvent::QsSeePrune, ply, 0, m,
                           alpha, beta, stand_pat, -1, correction, VALUE_NONE,
                           i, 0, see_threshold, alpha);
            continue;
        }

        if (!is_promo && i >= 6 && !state_.board->see_ge(m, -50) && !move_gives_check()) {
            TRACE_DECISION(TraceEvent::QsLatePrune, ply, 0, m,
                           alpha, beta, stand_pat, -1, correction, VALUE_NONE,
                           i, 0, -50, alpha);
            continue;
        }

        do_move(ss, m);
        int s = -quiescence(-beta, -alpha, ply + 1, qply + 1, ss + 1);
        undo_move(ss, m);

        if (state_.stopped) return 0;
        if (s > alpha) {
            alpha = s;
            best_move = m;
        }
        if (s >= beta) {
            TRACE_DECISION(TraceEvent::QsBetaCutoff, ply, 0, m,
                           alpha, beta, stand_pat, -1, correction, VALUE_NONE,
                           i, 0, VALUE_NONE, s);
            tt_store(hash, 0, s, TT_BETA, m, ply, raw_eval);
            return s;
        }
    }

    // Deliberately fail-hard here (store/return alpha, NOT a seeded best):
    // stand_pat may have been tightened UPWARD by a TT_BETA (lower) bound via
    // the 6.1 mirror above, so a best seeded from it is not a provable UPPER
    // bound -- storing it as TT_ALPHA poisons mate-distance resolution
    // (measured: KQK mate-in-5 degraded to "mate 63"). The audited 8.1f
    // fail-soft fixes live in the delta-pruning and qsearch-cap returns
    // above; bound shaping proper is Phase 10.4.
    TTFlag flag = (alpha > orig_alpha) ? TT_EXACT : TT_ALPHA;
    tt_store(hash, 0, alpha, flag, best_move, ply, raw_eval);
    return alpha;
}

// ---- Negamax search --------------------------------------------------------

template<NodeType NT>
int Searcher::negamax(int depth, int alpha, int beta, int ply,
                      SearchStack* ss, bool allow_null, bool cut_node) {
    constexpr bool is_root = NT == NodeType::Root;
    constexpr bool is_pv   = NT != NodeType::NonPV;
    // A child of a PV node searched with the full window is PV; the root is
    // never a child.
    constexpr NodeType PvChild = is_pv ? NodeType::PV : NodeType::NonPV;
    assert(is_root == (ply == 0));
    record_node();
    if ((state_.nodes & 2047) == 0) {
        check_stop();
    }
    if (state_.stopped) return 0;

    if (ply >= MAX_PLY) return evaluator_.evaluate(*state_.board);
    state_.pv_len[ply] = ply;

    if (!is_root && state_.board->is_draw(ply)) return 0;

    // In-search tablebase probe. A result that decides the node returns and
    // is stored as the bound it proves; otherwise a PV node keeps searching
    // inside it: a win raises the floor, a loss caps the result.
    int tb_floor = -INF_SCORE;
    int tb_cap   = INF_SCORE;
    if (!is_root && depth > 0 && ss->excluded == MOVE_NONE && state_.tb_probe_in_search
        && config_.limits.syzygy_probe_depth > 0) {
        const int limit = std::min(config_.limits.syzygy_probe_limit, Syzygy::largest());
        const int pieces = std::popcount(state_.board->all_pieces());
        if (pieces < limit || depth >= config_.limits.syzygy_probe_depth) {
            if (auto wdl = Syzygy::probe_wdl(*state_.board,
                                             config_.limits.syzygy_probe_limit,
                                             config_.limits.syzygy_50_move_rule)) {
                record_tbhit();
                const TablebaseProbe tb = tablebase_probe(*wdl, ply,
                                                          config_.limits.syzygy_50_move_rule);
                if (tb.bound == TT_EXACT
                    || (tb.bound == TT_BETA ? tb.value >= beta : tb.value <= alpha)) {
                    tt_store(state_.board->position_key(), std::min(MAX_PLY - 1, depth + 6),
                             tb.value, tb.bound, MOVE_NONE, ply, TranspositionTable::INF_EVAL);
                    return tb.value;
                }
                if (is_pv) {
                    if (tb.bound == TT_BETA) {
                        tb_floor = tb.value;
                        alpha = std::max(alpha, tb.value);
                    } else {
                        tb_cap = tb.value;
                    }
                }
            }
        }
    }

    bool in_check = state_.board->is_in_check();

    // Check extension: when the side to move is in check, extend by 1 ply.
    // Guard with ss->excluded to prevent stacking with singular extensions.
    DIAG_COUNT(state_.diag.interior_nodes++);
    if (in_check) DIAG_COUNT(state_.diag.in_check_nodes++);
    // The extension is unconditional: every in-check node gets a ply.
    //
    // 5.7.6 removed check_ext_path_cap, which bounded the accumulation per path
    // and defaulted to 0 (disabled). It was added inert for 5.4.4, and that
    // cluster closed with BAS-S16 REJECTED at -3.48 +/- 3.32 -- so it was the
    // residue of a failed trial, not an avenue still open. 5.7.3 separately
    // measured that reducing extension at checking nodes fails our WAC floor.
    bool did_check_ext = false;
    if (!ABLATED(6) && in_check && ss->excluded == MOVE_NONE && ply < MAX_PLY - 2) {
        depth++;
        did_check_ext = true;
        DIAG_COUNT(state_.diag.check_exts++);
        TRACE_DECISION(TraceEvent::CheckExtension, ply, depth, MOVE_NONE,
                       alpha, beta, VALUE_NONE, -1, VALUE_NONE, VALUE_NONE,
                       0, 0, 1, depth);
    }

    if (depth <= 0)
        return quiescence(alpha, beta, ply, 0, ss);

    // Mate distance pruning
    if (!is_root) {
        alpha = std::max(alpha, -(MATE_SCORE - ply));
        beta  = std::min(beta,   (MATE_SCORE - ply - 1));
        if (alpha >= beta) return alpha;
    }

    // ---- Transposition table lookup ----------------------------------------
    Key hash     = state_.board->position_key();
    TTEntry tte{};
    bool tt_found = shared_.tt.probe_copy(hash, tte);
    DIAG_COUNT(state_.diag.tt_probes++);
    if (tt_found) DIAG_COUNT(state_.diag.tt_hits++);

    Move  tt_move  = MOVE_NONE;
    int   tt_score = VALUE_NONE;
    int   tt_depth = 0;
    TTFlag tt_flag  = TT_NONE;

    if (tt_found) {
        tt_move  = move_from_tt(tte.move16);
        tt_score = TranspositionTable::score_from_tt(tte.score, ply, state_.board->rule50_count());
        // depth is int8_t with a deliberate -1 sentinel; tidy's suggested
        // unsigned cast would corrupt it.
        // NOLINTNEXTLINE(bugprone-signed-char-misuse)
        tt_depth = tte.depth;
        tt_flag  = TTFlag(tte.flag_age & 3);

        if (!is_pv && ss->excluded == MOVE_NONE && tt_depth >= depth) {
            if (tt_flag == TT_EXACT
                || (tt_flag == TT_ALPHA && tt_score <= alpha)
                || (tt_flag == TT_BETA  && tt_score >= beta)) {
                DIAG_COUNT(state_.diag.tt_cutoffs++);
                TRACE_DECISION(TraceEvent::TtCutoff, ply, depth, tt_move,
                               alpha, beta, tt_score, -1, VALUE_NONE, VALUE_NONE,
                               0, 0, tt_depth, tt_score);
                return tt_score;
            }
        }
    }

    ss->tt_pv = is_pv || (tt_found && tt_flag == TT_EXACT && tt_depth >= depth - 1);
    if (ss->tt_pv) DIAG_COUNT(state_.diag.tt_pv_nodes++);

    // Phase 6.7: is the TT move a capture? (LMR input, lmr_tt_capture)
    const bool tt_capture = tt_move != MOVE_NONE
        && (state_.board->piece_on(to_sq(tt_move)) != NO_PIECE
            || move_type(tt_move) == EN_PASSANT);

    // ---- Static evaluation -------------------------------------------------
    int static_eval;
    int raw_static_eval = VALUE_NONE;
    if (in_check) {
        ss->eval = static_eval = VALUE_NONE;
    } else if (ss->excluded != MOVE_NONE) {
        // Inherit eval from parent to avoid calling evaluate twice
        static_eval = ss->eval;
    } else {
        if (tt_found && tte.static_eval != TranspositionTable::INF_EVAL)
            raw_static_eval = tte.static_eval;
        else
            raw_static_eval = evaluator_.evaluate(*state_.board);

        // TT stores the raw static eval; correction is applied at probe time.
        static_eval = raw_static_eval;
        static_eval += correction_value(state_.board->turn(), *state_.board, ss);
        static_eval  = std::clamp(static_eval, -(tablebaseWinInMaxPly - 1), tablebaseWinInMaxPly - 1);
        ss->eval = static_eval;
    }

    // Step 6.1: value used for PRUNING decisions only. When a TT entry's bound
    // proves its score a tighter estimate than the (corrected) static eval —
    // exact, or a fail-high above it, or a fail-low below it — prune on that
    // instead. ss->eval / static_eval stay the raw corrected value, so
    // `improving` and correction-history are unaffected.
    // A TT mate/TB-range score must NOT drive this: RFP returns `eval` directly
    // (unlike SF, which dampens + guards it), so a shallow mate bound would leak
    // out as an unverified mate cutoff — clamp the refinement to normal scores.
    int eval = static_eval;
    if (tt_found && static_eval != VALUE_NONE && tt_score != VALUE_NONE
        && !is_decisive(tt_score)
        && (tt_flag == TT_EXACT
            || (tt_flag == TT_BETA  && tt_score > static_eval)
            || (tt_flag == TT_ALPHA && tt_score < static_eval)))
        eval = tt_score;

#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
    const int correction = raw_static_eval == VALUE_NONE
                         ? VALUE_NONE : static_eval - raw_static_eval;
#endif

    // Improving: eval is better than 2 plies ago
    bool improving = !in_check && ply >= 2
                   && (ss-2)->eval != VALUE_NONE
                   && static_eval > (ss-2)->eval;

    // ---- Non-PV pruning (skip if in check, in PV, or singular search) ------
    if (!is_pv && !in_check && ss->excluded == MOVE_NONE
        && static_eval != VALUE_NONE) {

        // Reverse futility pruning
        if (!ABLATED(1) && depth <= 9) {
            const auto& p = config_.limits.params;
            int margin = p.rfp_coeff * depth - (improving ? p.rfp_improving : 0);
            if (eval - margin >= beta) {
                DIAG_COUNT(state_.diag.rfp_cuts++);
                TRACE_DECISION(TraceEvent::RfpPrune, ply, depth, MOVE_NONE,
                               alpha, beta, eval, improving, correction, VALUE_NONE,
                               0, 0, margin, eval);
                return eval;
            }
        }

        // Razoring
        if (!ABLATED(0) && depth <= 3
            && eval + config_.limits.params.razor_coeff * depth <= alpha) {
            int q = quiescence(alpha, beta, ply, 0, ss);
            if (q <= alpha) {
                DIAG_COUNT(state_.diag.razor_cuts++);
                TRACE_DECISION(TraceEvent::RazorPrune, ply, depth, MOVE_NONE,
                               alpha, beta, eval, improving, correction, VALUE_NONE,
                               0, 0, config_.limits.params.razor_coeff * depth, q);
                return q;
            }
        }

        // Null-move pruning
        if (!ABLATED(2) && allow_null && depth >= 3
            && eval >= beta
            && state_.board->has_non_pawn_material(state_.board->turn())
            && (ss-1)->move != MOVE_NULL) {

            int r = config_.limits.params.null_base + depth / 4
                  + std::min((eval - beta) / config_.limits.params.null_eval_div, 3);
            DIAG_COUNT(state_.diag.null_tries++);
            do_null_move(ss);
            shared_.tt.prefetch(state_.board->position_key());   // 8.7.6(c)
            int null_score = -negamax<NodeType::NonPV>(std::max(0, depth - r), -beta, -(beta - 1),
                                      ply + 1, ss + 1, false, true);
            undo_null_move(ss);
            if (state_.stopped) return 0;
            if (null_score >= beta) {
                if (is_decisive(null_score)) null_score = beta;
                bool verified = true;
                if (depth >= 10) {
                    const int verify_depth = std::max(1, depth - r);
                    const int verify_score = negamax<NodeType::NonPV>(verify_depth, beta - 1, beta,
                                                     ply, ss, false, false);
                    if (state_.stopped) return 0;
                    verified = verify_score >= beta;
                }
                if (verified) {
                    DIAG_COUNT(state_.diag.null_cuts++);
                    TRACE_DECISION(TraceEvent::NullCutoff, ply, depth, MOVE_NULL,
                                   alpha, beta, eval, improving, correction, VALUE_NONE,
                                   0, r, VALUE_NONE, null_score);
                    return null_score;
                }
            }
        }

        // ProbCut: if a capture is likely to fail high at reduced depth
        if (!ABLATED(3) && depth >= 5 && !is_decisive(beta)) {
            int pc_beta = std::min(beta + config_.limits.params.probcut_margin,
                                   MATE_SCORE - MAX_PLY - 1);
            MoveList pcaps;
            state_.board->gen_legal_captures(pcaps);
            for (Move m : pcaps) {
                if (m == ss->excluded) continue;
                if (!state_.board->see_ge(m, pc_beta - static_eval)) continue;

                DIAG_COUNT(state_.diag.probcut_tries++);
                do_move(ss, m);
                shared_.tt.prefetch(state_.board->position_key());   // 8.7.6(c)
                // Quick check via QSearch first
                int val = -quiescence(-pc_beta, -pc_beta + 1, ply + 1, 0, ss + 1);
                if (val >= pc_beta)
                    val = -negamax<NodeType::NonPV>(depth - 4, -pc_beta, -pc_beta + 1,
                                   ply + 1, ss + 1, true, true);
                undo_move(ss, m);
                if (state_.stopped) return 0;
                if (val >= pc_beta) {
                    tt_store(hash, depth - 3, pc_beta, TT_BETA, m, ply,
                              raw_static_eval == VALUE_NONE
                                  ? TranspositionTable::INF_EVAL : raw_static_eval);
                    DIAG_COUNT(state_.diag.probcut_cuts++);
                    TRACE_DECISION(TraceEvent::ProbcutCutoff, ply, depth, m,
                                   alpha, beta, eval, improving, correction, VALUE_NONE,
                                   0, depth - 4, pc_beta - static_eval, pc_beta);
                    return pc_beta;
                }
            }
        }
    }

    // IIR: reduce non-PV nodes when no TT move (or a stale TT entry) guides the search.
    if (!ABLATED(4) && !is_pv && depth >= 4
        && (tt_move == MOVE_NONE || (tt_found && tt_depth < depth - 3))) {
        TRACE_DECISION(TraceEvent::IirReduction, ply, depth, tt_move,
                       alpha, beta, eval, improving, correction, VALUE_NONE,
                       0, 1, depth - 3, depth - 1);
        depth--;
    }

    int  orig_alpha  = alpha;
    Move best_move   = MOVE_NONE;
    int  best_score  = tb_floor;
    int  searched    = 0;

    Move quiets_searched[MAX_TRACKED_QUIETS];
    Move bad_caps_searched[MAX_TRACKED_BAD_CAPS];
    int quiets_count = 0;
    int bad_caps_count = 0;

    int lmp_thresh = improving ? (3 + depth * depth) : (2 + depth * depth / 2);
    int root_ordinal = 0;
    bool immediate_return = false;
    int immediate_score = 0;

    // 9.6: score_moves() has already hoisted these bases for ordering. The
    // history-pruning and LMR-stat paths below revisit the same quiet move, so
    // hoist their node-invariant dimensions here as well.
    const auto& main_hist = hist_.main[state_.board->turn()];
    const auto& pawn_hist = hist_.pawn->data[
        state_.board->pawn_key_value() & (HistoryTables::PAWN_HIST_SIZE - 1)];
    const auto* low_ply_hist = ply < HistoryTables::LOW_PLY_HISTORY_SIZE
                             ? &hist_.low_ply[ply] : nullptr;

    // 5.7.2: set when the TT move proves singular at this node, and then read
    // by LMR for every LATER move here.
    //
    // The scope is the subtle part and is the whole mechanism: the reference
    // resets this once per NODE, not per move, so a singular TT move relaxes
    // the reduction on all its siblings. A singular TT move means one move is
    // materially better than every alternative, which is exactly a position
    // where the alternatives deserve a closer look before being reduced away.
    //
    // It deliberately does NOT reduce the singular move itself: that move is
    // the TT move, ordered first, so `searched < 2` blocks LMR from ever
    // reaching it. Reading the flag as "reduce the extended move less" produces
    // dead code.
    bool singular_quiet_lmr = false;

    auto search_one = [&](Move m, int picker_see, MovePicker::Src picker_src) {
        if (is_root && !move_in_root_moves(m, config_.limits.root_moves))
            return false;
        if (is_root && config_.root_filter_index >= 0) {
            const int ordinal = root_ordinal++;
            if ((ordinal % config_.root_filter_count) != config_.root_filter_index)
                return false;
        }
        if (is_root && !root_tablebase_allows(m))
            return false;
        if (is_root && !state_.root_excluded.empty()
            && std::find(state_.root_excluded.begin(), state_.root_excluded.end(), m) != state_.root_excluded.end())
            return false;

        bool is_cap   = (state_.board->piece_on(to_sq(m)) != NO_PIECE)
                     || (move_type(m) == EN_PASSANT);
        bool is_promo = (move_type(m) == PROMOTION);
        bool is_quiet = !is_cap && !is_promo;
        // 8.7.5(a): seed see_score with the picker's already-computed verdict
        // (0 good / -1 bad capture; VALUE_NONE otherwise) so the two lazy
        // see_ge(m,0) recompute sites below are skipped for classified
        // captures. Identical value => bench-identical.
        int see_score = picker_see;
#ifndef NDEBUG
        if (is_cap && !is_promo && see_score != VALUE_NONE)
            assert(see_score == (state_.board->see_ge(m, 0) ? 0 : -1)
                   && "8.7.5(a) memoized see_score disagrees with a fresh see_ge");
#endif
        bool gives_check_known = false;
        bool gives_check = false;
        auto move_gives_check = [&]() {
            if (!gives_check_known) {
                gives_check = state_.board->gives_check(m);
                gives_check_known = true;
            }
            return gives_check;
        };
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
        int trace_history = VALUE_NONE;
        if (state_.trace.enabled() && is_quiet) {
            const PieceType pt = type_of(state_.board->piece_on(from_sq(m)));
            trace_history = main_hist[from_sq(m)][to_sq(m)]
                          + cont_hist_score(ss, pt, Square(to_sq(m)))
                          + pawn_hist[pt][to_sq(m)]
                          + (low_ply_hist ? (*low_ply_hist)[from_sq(m)][to_sq(m)] : 0);
        }
#endif

        // ---- Late-move pruning / futility ----------------------------------
        if (!ABLATED(5) && !is_root && searched > 0
            && best_score > -tablebaseWinInMaxPly) {

            if (is_quiet) {
                // Futility pruning
                if (!is_pv && !in_check && depth <= 6
                    && eval != VALUE_NONE
                    && eval + config_.limits.params.futility_base
                            + config_.limits.params.futility_coeff * depth <= alpha
                    && !move_gives_check()) {
                    DIAG_COUNT(state_.diag.fut_prunes++);
                    TRACE_DECISION(TraceEvent::FutilityPrune, ply, depth, m,
                                   alpha, beta, eval, improving, correction, trace_history,
                                   searched, 0,
                                   config_.limits.params.futility_base
                                       + config_.limits.params.futility_coeff * depth,
                                   alpha);
                    return false;
                }

                // Late move pruning (LMP) — never in PV
                if (!is_pv && !in_check && depth <= 6 && searched >= lmp_thresh
                    && !move_gives_check()) {
                    DIAG_COUNT(state_.diag.lmp_prunes++);
                    TRACE_DECISION(TraceEvent::LmpPrune, ply, depth, m,
                                   alpha, beta, eval, improving, correction, trace_history,
                                   searched, 0, lmp_thresh, alpha);
                    return false;
                }

                // History pruning: skip moves with very bad combined history
                if (!is_pv && depth <= 6) {
                    PieceType pt = type_of(state_.board->piece_on(from_sq(m)));
                    int hist = main_hist[from_sq(m)][to_sq(m)]
                             + cont_hist_score(ss, pt, Square(to_sq(m)))
                             + pawn_hist[pt][to_sq(m)]
                             + (low_ply_hist ? (*low_ply_hist)[from_sq(m)][to_sq(m)] : 0);
                    // 5.6 reachability probe. The live condition below is
                    // byte-identical; these only observe how far the actual
                    // history distribution sits from the threshold, and add no
                    // move_gives_check() calls.
                    {
                        const int64_t thr =
                            int64_t(config_.limits.params.hist_prune_coeff) * depth;
                        DIAG_COUNT(++state_.diag.hist_prune_tested);
                        if (hist < -(thr / 2)) DIAG_COUNT(++state_.diag.hist_below_half);
                        if (hist < -(thr / 4)) DIAG_COUNT(++state_.diag.hist_below_quarter);
                        if (hist < -(thr / 8)) DIAG_COUNT(++state_.diag.hist_below_eighth);
                    }
                    if (hist < -config_.limits.params.hist_prune_coeff * depth && !move_gives_check()) {
                        DIAG_COUNT(state_.diag.hist_prunes++);
                        TRACE_DECISION(TraceEvent::HistoryPrune, ply, depth, m,
                                       alpha, beta, eval, improving, correction, hist,
                                       searched, 0,
                                       config_.limits.params.hist_prune_coeff * depth,
                                       alpha);
                        return false;
                    }
                }
            } else if (is_cap) {
                // SEE pruning for bad captures
                if (!is_pv && depth <= 8 && !is_promo) {
                    if (!state_.board->see_ge(m, -depth * config_.limits.params.see_prune_coeff) && !move_gives_check()) {
                        DIAG_COUNT(state_.diag.see_prunes++);
                        TRACE_DECISION(TraceEvent::CaptureSeePrune, ply, depth, m,
                                       alpha, beta, eval, improving, correction, VALUE_NONE,
                                       searched, 0,
                                       -depth * config_.limits.params.see_prune_coeff,
                                       alpha);
                        return false;
                    }
                }
            }
        }

        if (is_cap && !is_promo && depth >= 2 && searched >= 2 && see_score == VALUE_NONE)
            see_score = state_.board->see_ge(m, 0) ? 0 : -1;

        // ---- Extensions -------------------------------------------------------
        int extension = 0;

        // ---- Singular extension (only for TT move) -------------------------
        if (!ABLATED(6) && !is_root && m == tt_move && ss->excluded == MOVE_NONE
            && depth >= config_.limits.params.singular_min_depth
            && tt_found && tt_depth >= depth - 3
            && (tt_flag == TT_BETA || tt_flag == TT_EXACT)
            && !is_decisive(tt_score)) {

            int s_beta  = tt_score - config_.limits.params.singular_beta_mult * depth;
            int s_depth = (depth - 1) / 2;

            ss->excluded = m;
            int s_val = negamax<NodeType::NonPV>(s_depth, s_beta - 1, s_beta, ply, ss, false, true);
            ss->excluded = MOVE_NONE;

            if (state_.stopped) {
                immediate_return = true;
                immediate_score = 0;
                return true;
            }

            if (s_val < s_beta) {
                // TT move is singular — extend it. Phase 6.4 rider: cap stacked
                // 2-ply extensions along this path so a pathological line can't
                // chain unbounded double-extensions.
                bool allow_double = !is_pv
                    && s_val < s_beta - config_.limits.params.singular_double_margin
                    && ss->double_exts < config_.limits.params.double_ext_max;
                // 5.7.3 REFUTED: our per-node check extension composes with
                // this per-move one, so a checking node with a singular TT move
                // can take 3 plies where the reference allows 1. Making them
                // exclusive was measured and is WORSE -- WAC 137 -> 124 against
                // a floor of 130, failing outright; the intermediate "no double
                // when in check" still cost 5 solved for +0.009 ply. Our check
                // extension is unconditional where the reference gates on
                // discovery-or-SEE, so removing the composition removes strictly
                // more than it would there. Composition stays. (BAS-D11)
                extension += allow_double ? 2 : 1;
                TRACE_DECISION(TraceEvent::SingularExtension, ply, depth, m,
                               alpha, beta, eval, improving, correction, trace_history,
                               searched, extension, s_beta, s_val);

                // 5.7.3 probe: count the stack, do not change it yet.
                DIAG_COUNT(++state_.diag.sing_fired);
                if (allow_double)   DIAG_COUNT(++state_.diag.sing_double);
                if (did_check_ext)  DIAG_COUNT(++state_.diag.sing_in_check);
                if (allow_double && did_check_ext) DIAG_COUNT(++state_.diag.sing_triple);

                // 5.7.2: relax LMR for this node's remaining moves. Suppressed
                // when the TT move is a capture -- a singular capture says the
                // tactics are forced, not that the quiet alternatives are
                // delicate, and `lmr_tt_capture` already raises r for exactly
                // that case. Letting both fire would have them cancel.
                singular_quiet_lmr = !tt_capture;
            } else if (s_beta >= beta) {
                // Multicut: likely to fail high without this move too
                TRACE_DECISION(TraceEvent::SingularMulticut, ply, depth, m,
                               alpha, beta, eval, improving, correction, trace_history,
                               searched, 0, s_beta, s_val);
                immediate_return = true;
                immediate_score = s_beta;
                return true;
            } else if (tt_score >= beta) {
                DIAG_COUNT(++state_.diag.sing_ttbeta);
                TRACE_DECISION(TraceEvent::SingularNegative, ply, depth, m,
                               alpha, beta, eval, improving, correction, trace_history,
                               searched, -1, s_beta, s_val);

                // 5.7.4 REFUTED: the reference replaces this negative
                // extension with a SECOND verification search that can cut the
                // whole subtree. Implemented behind a knob and measured: depth
                // at equal nodes was mean +0.383 ply but **median +0.0**, with
                // 27 better against 25 worse -- the mean carried entirely by
                // three trivial pawn/king endgames (+11, +11, +9) where depth is
                // cheap. WAC 138 vs 137, noise. No broad gain by either
                // instrument, and it costs an extra search. Ours stays. (BAS-D13)
                extension--; // Negative extension: not clearly best
            }
        }

        PieceType moved_pt = type_of(state_.board->piece_on(from_sq(m)));
        int move_stat_score = 0;
        if (is_quiet) {
            move_stat_score = main_hist[from_sq(m)][to_sq(m)];
            move_stat_score += cont_hist_score(ss, moved_pt, Square(to_sq(m)));
            move_stat_score += pawn_hist[moved_pt][to_sq(m)];
            if (low_ply_hist)
                move_stat_score += (*low_ply_hist)[from_sq(m)][to_sq(m)];
        }

        ss->stat_score  = move_stat_score;
        ss->reduction   = 0;
        const int64_t nodes_before_move = state_.nodes;
        do_move(ss, m);
        shared_.tt.prefetch(state_.board->position_key());
        state_.sel_depth = std::max(state_.sel_depth, ply + 1);

        int new_depth = depth - 1 + extension;
        // Phase 6.4 rider: propagate the stacked double-extension count to the
        // child so a chain of singular double-extensions is eventually capped.
        (ss + 1)->double_exts = ss->double_exts + (extension >= 2 ? 1 : 0);

        int score;
        if (searched == 0) {
            score = -negamax<PvChild>(new_depth, -beta, -alpha, ply + 1, ss + 1, true, false);
        } else {
            // Late Move Reductions
            int reduction = 0;
            // 5.2 (BAS-O03): the gate below is unchanged, but it is now
            // evaluated as an if/else-if chain so each rejection is
            // attributable. The predicate order and short-circuiting are
            // identical to the original single condition — in particular
            // move_gives_check() is still reached only when the first four
            // pass, so its call count and cost do not move.
            //
            // This matters because lmr_applied alone cannot tell "rarely
            // eligible" from "eligible but never reduced", and those have
            // opposite repairs. Our EBF is 2.20 against the reference's 1.61.
            if (!ABLATED(7)) DIAG_COUNT(++state_.diag.lmr_eligible);
            const bool lmr_type_ok = is_quiet || (is_cap && !is_promo && see_score < 0);
            if (!ABLATED(7) && depth < 2)          DIAG_COUNT(++state_.diag.lmr_blocked_depth);
            else if (!ABLATED(7) && searched < 2) DIAG_COUNT(++state_.diag.lmr_blocked_searched);
            else if (!ABLATED(7) && in_check)     DIAG_COUNT(++state_.diag.lmr_blocked_in_check);
            else if (!ABLATED(7) && !lmr_type_ok) DIAG_COUNT(++state_.diag.lmr_blocked_movetype);
            // Checking moves are never reduced. 5.7.6 removed the
            // lmr_allow_check switch that could have relaxed this: it was added
            // inert for 5.4.4, which closed rejected (BAS-S16).
            else if (!ABLATED(7) && move_gives_check())
                DIAG_COUNT(++state_.diag.lmr_blocked_gives_check);
            // LMR applies to: quiets, and bad captures — but NOT promotions
            else if (!ABLATED(7)) {
                // Phase 6.7: accumulate the reduction in 1024ths of a ply, then
                // shift back at the end. Behaviour-identical at default knobs
                // (adjustments are the old integer values ×1024; history stays
                // integer-quantised via the ×1024-after-divide form).
                int r = lmr_table_[std::min(depth, 63)][std::min(searched, 63)];

                if (is_quiet) {
                    const auto& p = config_.limits.params;
                    if (!is_pv)     r += p.lmr_non_pv_adj;
                    if (cut_node)   r += p.lmr_cut_node_adj;
                    if (ss->tt_pv)  r -= p.lmr_tt_pv_adj;
                    if (!improving) r += p.lmr_not_improving_adj;
                    if (tt_capture) r += p.lmr_tt_capture;
                    // 5.7.2: see the declaration of singular_quiet_lmr.
                    if (singular_quiet_lmr) r -= p.lmr_singular_quiet;
                    // History-based adjustment: good moves get reduced less, bad
                    // more. Kept integer-quantised (÷div then ×1024) so 6.7 is
                    // behaviour-identical; the fractional form (×1024 ÷ div) is a
                    // 6.9 experiment.
                    //
                    // 5.4.3 tested that fractional form and MEASURED IT WORSE
                    // (BAS-S13): applied 36.1%→32.5%, clamp-to-zero 16.2%→19.8%,
                    // depth at equal nodes 20.80→20.70. The quantisation is not
                    // only a resolution defect — it also acts as a threshold.
                    // Most moves carry positive history and history SUBTRACTS
                    // from r, so a continuous response shaves a little off nearly
                    // every reduction, while the integer form shaved a whole ply
                    // off only the |stat| ≥ div minority. Retry trigger: base and
                    // context reductions are materially larger, so there is
                    // enough r for a continuous response to modulate rather than
                    // erase.
                    r -= (move_stat_score / p.lmr_hist_div) * 1024;
                } else {
                    // Bad captures get less reduction than quiets. Computed in
                    // integer plies then rescaled, so no rounding drift.
                    r = (((r >> 10) - 1) / 2) << 10;
                }

                // 5.4.3: record whether the ceiling bound before clamping, so
                // "modulation too small" and "modulation cannot matter here" are
                // separable. Most LMR-eligible nodes sit near the leaves, where
                // new_depth-1 is 1 or 2 and no policy change can move the
                // reduction actually taken.
                if ((r >> 10) > new_depth - 1) DIAG_COUNT(++state_.diag.lmr_clamped_high);
                reduction = std::clamp(r >> 10, 0, new_depth - 1);
                // 5.2: the gate passed but the computed reduction was zero —
                // distinct from being blocked, and a different repair. Counted
                // here so that
                //   eligible = applied + clamped_zero + sum(blocked_*)
                // holds exactly, which is what makes the breakdown auditable.
                if (reduction == 0) DIAG_COUNT(++state_.diag.lmr_clamped_zero);
            }
            ss->reduction = reduction;
            TRACE_DECISION(TraceEvent::LmrReduction, ply, depth, m,
                           alpha, beta, eval, improving, correction, move_stat_score,
                           searched, reduction, new_depth - 1, VALUE_NONE);
            if (reduction > 0) {
                DIAG_COUNT(state_.diag.lmr_applied++);
                // Mean reduction over applied = reduction_plies / applied. A
                // timid-LMR hypothesis is decided by this number, not by how
                // often LMR fired.
                DIAG_COUNT(state_.diag.lmr_reduction_plies += reduction);
            }

            score = -negamax<NodeType::NonPV>(new_depth - reduction, -alpha - 1, -alpha,
                             ply + 1, ss + 1, true, true);
            // Re-search at full depth if LMR didn't fail low
            if (reduction > 0 && score > alpha && !state_.stopped) {
                DIAG_COUNT(state_.diag.lmr_researched++);
                TRACE_DECISION(TraceEvent::LmrResearch, ply, depth, m,
                               alpha, beta, eval, improving, correction, move_stat_score,
                               searched, reduction, VALUE_NONE, score);
                score = -negamax<NodeType::NonPV>(new_depth, -alpha - 1, -alpha,
                                 ply + 1, ss + 1, true, !cut_node);
            }
            // Re-search as PV if score is within window
            if (is_pv && score > alpha && score < beta && !state_.stopped)
                score = -negamax<NodeType::PV>(new_depth, -beta, -alpha,
                                 ply + 1, ss + 1, true, false);
        }

        undo_move(ss, m);

        if (state_.stopped)
            return true;

        searched++;
        const int64_t move_nodes = state_.nodes - nodes_before_move;
        if (is_root) {
            state_.root_depth_nodes += std::max<int64_t>(0, move_nodes);
        }

        // Track for history updates
        if (is_cap && !is_promo) {
            if (see_score == VALUE_NONE)
                see_score = state_.board->see_ge(m, 0) ? 0 : -1;
            if (see_score < 0 && bad_caps_count < MAX_TRACKED_BAD_CAPS)
                bad_caps_searched[bad_caps_count++] = m;
        } else if (is_quiet && quiets_count < MAX_TRACKED_QUIETS) {
            quiets_searched[quiets_count++] = m;
        }

        if (score > best_score) {
            if (is_root)
                state_.root_best_nodes = std::max<int64_t>(0, move_nodes);
            best_score = score;
            best_move  = m;
            if (score > alpha) {
                alpha = score;
                // Update PV
                state_.pv_table[ply][ply] = m;
                int child_pv_len = std::clamp(state_.pv_len[ply + 1], ply + 1, MAX_PLY);
                for (int k = ply + 1; k < child_pv_len; k++)
                    state_.pv_table[ply][k] = state_.pv_table[ply + 1][k];
                state_.pv_len[ply] = child_pv_len;
            }
        }

        if (alpha >= beta) {
            TRACE_DECISION(TraceEvent::BetaCutoff, ply, depth, m,
                           orig_alpha, beta, eval, improving, correction, move_stat_score,
                           searched, ss->reduction, VALUE_NONE, score);
            // 5.2 (BAS-O03): ordering quality at the point it costs something.
            // `searched` was incremented above, so the cutting move's index is
            // searched - 1. A cutoff on index 0 costs one move's search; on
            // index n it costs n+1, so the mean index is a direct multiplier on
            // tree width — the quantity separating our 2.20 EBF from ~1.61.
            // cutoff_src says which picker stage to fix rather than merely that
            // ordering is imperfect.
            DIAG_COUNT(++state_.diag.fail_highs);
            DIAG_COUNT(state_.diag.fail_high_index_sum += searched - 1);
            if (searched == 1) DIAG_COUNT(++state_.diag.fail_high_first);
            switch (picker_src) {
                case MovePicker::Src::TT:           DIAG_COUNT(++state_.diag.cutoff_src_tt); break;
                case MovePicker::Src::GoodTactical: DIAG_COUNT(++state_.diag.cutoff_src_good_tactical); break;
                case MovePicker::Src::Quiet:        DIAG_COUNT(++state_.diag.cutoff_src_quiet); break;
                case MovePicker::Src::BadTactical:  DIAG_COUNT(++state_.diag.cutoff_src_bad_tactical); break;
                case MovePicker::Src::None:         break;
            }
            // 8.5.10(e): boost the bonus when the cutoff was "surprising" -- the
            // node's static eval was below beta, so the search found a good move
            // the eval did not credit.
            const int es = (static_eval != VALUE_NONE && static_eval < beta) ? 125 : 100;
            update_all_histories(m, m == tt_move, quiets_searched, quiets_count,
                                 bad_caps_searched, bad_caps_count,
                                 state_.board->turn(), depth, ss,
                                 /*reward_only=*/false, /*bonus_scale=*/es);
            return true;
        }

        return false;
    };

    // ---- Staged move picking -----------------------------------------------
    // TT move first, then tactical moves, then quiet moves. Quiet generation and
    // scoring are delayed until captures/promotions fail to produce a cutoff.
    MovePicker picker(*state_.board, hist_, tt_move, ss->excluded, ss, ply,
                      is_root ? &state_.root_ordering : nullptr,
                      state_.move_buffers[ply][0], state_.move_buffers[ply][1]);
    while (true) {
        Move move = picker.next();
        if (move == MOVE_NONE)
            break;
        if (search_one(move, picker.last_see_score(), picker.last_source()))
            break;
    }

    if (immediate_return)
        return immediate_score;

    if (state_.stopped)
        return (is_root && best_move != MOVE_NONE) ? best_score : 0;

    // No legal moves
    if (searched == 0)
        return in_check ? -(MATE_SCORE - ply) : 0;

    best_score = std::min(best_score, tb_cap);

    // 8.5.10(b') exact/PV best-move history training, REWARD-ONLY.
    // A beta cutoff trains history inside search_one. An EXACT node -- best_move
    // improved alpha but did not cut off -- was left untrained. The full updater
    // also maluses every non-best sibling, which at an exact node (all moves
    // searched, best-vs-second often a few cp) poisons ordering: the reward+malus
    // variant lost -84 Elo. Here we reward the PV move's graded history ONLY (no
    // sibling malus, no killer/countermove) to isolate whether the reward helps.
    // best_score < beta excludes the already-trained cutoff case (best_score >=
    // beta there), so there is no double update.
    if (best_move != MOVE_NONE && best_score > orig_alpha && best_score < beta) {
        update_all_histories(best_move, best_move == tt_move,
                             quiets_searched, quiets_count,
                             bad_caps_searched, bad_caps_count,
                             state_.board->turn(), depth, ss,
                             /*reward_only=*/true);
    }

    // Update correction history with search result
    if (!in_check && ss->excluded == MOVE_NONE && static_eval != VALUE_NONE
        && !is_decisive(best_score)
        && (best_score >= beta || best_score > orig_alpha)) {
        update_correction(state_.board->turn(), *state_.board, ss,
                          best_score - static_eval, depth);
    }

    // Store to TT
    TTFlag flag = (best_score >= beta)    ? TT_BETA
                : (best_score > orig_alpha) ? TT_EXACT
                :                             TT_ALPHA;
    // A later MultiPV line searched the root without its best moves; its
    // result is not the root's, so it is not stored (Stockfish skips it too).
    if (ss->excluded == MOVE_NONE && !(is_root && !state_.root_excluded.empty()))
        tt_store(hash, depth, best_score, flag, best_move, ply,
                  raw_static_eval == VALUE_NONE ? TranspositionTable::INF_EVAL : raw_static_eval);

    return best_score;
}

// ---- Kernel entry points ---------------------------------------------------

void Searcher::kernel_new_game() {
    hist_.clear();
    history_age_counter_ = 0;
}

void Searcher::kernel_begin_search() {
    init_lmr(static_cast<float>(config_.limits.params.lmr_base)    / 100.0f,
             static_cast<float>(config_.limits.params.lmr_divisor) / 100.0f);
    if (++history_age_counter_ >= 2) {
        age_history();
        history_age_counter_ = 0;
    }
    // A default record is the sentinel: no move, no piece, no evaluation. The
    // plies below the root read as such, and every ply starts clean.
    for (auto& s : state_.stack) s = SearchStack{};
}

int Searcher::root_search(int depth, int alpha, int beta) {
    return negamax<NodeType::Root>(depth, alpha, beta, 0, state_.stack + STACK_SENTINELS,
                                   true, false);
}
