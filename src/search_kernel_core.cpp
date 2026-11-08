// The selectivity core's node kernel: quiescence, negamax
// and the helpers only they call. The driver in search_worker.cpp reaches the
// kernel through root_search(), kernel_begin_search() and kernel_new_game()
// alone. The kernel owns the per-ply stack, the reduction table and the
// history tables, and writes the driver-visible results into SearchState: the
// PV table, selective depth, root effort, node counts and diagnostics.

#include "search_worker.h"
#include "constants.h"
#include "search_b3.h"
#include "search_b4.h"
#include "syzygy.h"
#include <algorithm>
#include <bit>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <utility>

// ---- LMR table -------------------------------------------------------------

#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
#define TRACE_DECISION(...) state_.trace.record(__VA_ARGS__)
#else
#define TRACE_DECISION(...) ((void)0)
#endif

// Node checks: invariants of the per-ply record verified at every node entry.
// Live in the Diag flavour and in builds without NDEBUG (the sanitizer
// suite runs them); compiled out of release. A failure aborts with the
// condition, so a field a search forgets to reset cannot go unnoticed.
#if defined(BASILISK_DIAGNOSTIC) || !defined(NDEBUG)
#define CORE_NODE_CHECK(cond)                                                   \
    do {                                                                        \
        if (!(cond)) {                                                          \
            std::fprintf(stderr, "node check failed: %s (%s:%d)\n", #cond,      \
                         __FILE__, __LINE__);                                   \
            std::abort();                                                       \
        }                                                                       \
    } while (0)
#else
#define CORE_NODE_CHECK(cond) ((void)0)
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

int Searcher::lmr_reduction(bool improving, int depth, int move_count, int delta) const {
    const auto& p = config_.limits.params;
    const int scale = reductions_[std::min(depth, 255)] * reductions_[std::min(move_count, 255)];
    return scale - delta * 577 / root_delta_
         + (improving ? 0 : scale * p.lmr_not_improving / 512) + p.lmr_offset;
}

void Searcher::tt_store(Key key, int depth, int score, TTFlag flag, Move m,
                        int ply, int static_eval, bool pv) {
#ifdef BASILISK_PRICING
    if (state_.price.probing()) {
        state_.price.store_tt(key, depth, score, flag, m, ply, static_eval, pv);
        return;
    }
#endif
    DIAG_COUNT(++state_.diag.tt_stores);
    if (shared_.tt.store(key, depth, score, flag, m, ply, static_eval, pv))
        DIAG_COUNT(++state_.diag.tt_stores_same_key);
}

bool Searcher::tt_move_playable(Move m) const {
    if (m == MOVE_NONE)
        return false;
    const Piece p = state_.board->piece_on(from_sq(m));
    return p != NO_PIECE && color_of(p) == state_.board->turn() && state_.board->is_legal(m);
}

// The bound a TT entry proves on its score.
static bool proves_lower(TTFlag f) { return f == TT_BETA || f == TT_EXACT; }
static bool proves_upper(TTFlag f) { return f == TT_ALPHA || f == TT_EXACT; }

// ---- Pricing probes ----------------------------------------------------------

// A statement that only pricing builds compile.
#ifdef BASILISK_PRICING
#define PRICE(...) __VA_ARGS__
#else
#define PRICE(...) ((void)0)
#endif

#ifdef BASILISK_PRICING
// The probe may write the node's record and its children's, the PV length at
// its ply, the node counts, the selective depth, the null-move region and the
// counters; all are put back. A node limit counts the engine's own nodes.
template<class Probe>
int Searcher::isolated_probe(SearchStack* ss, int ply, int64_t& nodes, Probe&& probe) {
    const SearchStack records[3] = {ss[0], ss[1], ss[2]};
    const int64_t own_nodes = state_.nodes;
    const int64_t nodes_limit = state_.nodes_limit;
    const int64_t tb_hits = state_.tb_hits;
    const int sel_depth = state_.sel_depth;
    const int pv_len = state_.pv_len[ply];
    const int nmp_min_ply = nmp_min_ply_;
    const DiagCounters diag = state_.diag;
    state_.nodes_limit = 0;
    state_.price.enter();
    const int value = probe();
    state_.price.leave();
    nodes = state_.nodes - own_nodes;
    ss[0] = records[0];
    ss[1] = records[1];
    ss[2] = records[2];
    state_.nodes = own_nodes;
    state_.nodes_limit = nodes_limit;
    state_.tb_hits = tb_hits;
    state_.sel_depth = sel_depth;
    state_.pv_len[ply] = pv_len;
    nmp_min_ply_ = nmp_min_ply;
    state_.diag = diag;
    return value;
}
#endif

// ---- Move loop bookkeeping ----------------------------------------------

// The quiets and captures searched before a node's best move, which its
// update punishes: at most this many of each, from its first this-many moves.
static constexpr int SEARCHED_LIST_CAPACITY = 32;

// Per depth (1..16, deeper reads the last), the history that adds one ply to
// a quiet move's reduced depth for move-loop pruning.
static constexpr int LMR_HISTORY_DIVISOR[16] = {
    3637, 2787, 2761, 2939, 3171, 3347, 3147, 2762,
    2772, 3106, 3107, 3060, 3112, 2991, 3090, 3542};

// ---- Quiescence search -----------------------------------------------------

// A capture, an en passant or a promotion: what the picker's capture stage
// holds.
[[maybe_unused]] static bool is_capture_stage(const Board& board, Move m) {
    return board.piece_on(to_sq(m)) != NO_PIECE || move_type(m) == EN_PASSANT
        || move_type(m) == PROMOTION;
}

[[maybe_unused]] static PieceType capture_victim(const Board& board, Move m) {
    return move_type(m) == EN_PASSANT ? PAWN : type_of(board.piece_on(to_sq(m)));
}

[[maybe_unused]] static bool has_no_legal_move(const Board& board) {
    MoveList legal;
    board.gen_legal(legal);
    return legal.empty();
}

// Fail-soft throughout, in Stockfish's form: every return is the node's best
// value, which may lie outside the window, and the table stores it as the
// bound it proves -- a lower bound at a fail high, otherwise an upper bound,
// never an exact score. A PV node takes no table cutoff, so a depth-0 bound
// never stands in for its value on the principal variation.
template<NodeType NT>
int Searcher::quiescence(int alpha, int beta, int ply, int qply, SearchStack* ss,
                         bool counted) {
    constexpr bool is_pv = NT != NodeType::NonPV;
#ifdef BASILISK_PRICING
    // A pricing probe of this node's delta rule runs it without that rule.
    const bool bare = state_.price.take_bare();
    const int entry_alpha = alpha;
    const PriceNode node_kind = is_pv ? PriceNode::Pv : PriceNode::Qs;
#else
    constexpr bool bare = false;
#endif
    if (!counted) {
        record_node();
        // The clock is read every 2048 nodes, the stop flag at every node:
        // the main thread waits for every helper before `bestmove`.
        if ((state_.nodes & 2047) == 0 || shared_.stop.load(std::memory_order_relaxed))
            check_stop();
        if (state_.stopped) return 0;
        if (ply >= MAX_PLY) return evaluator_.evaluate(*state_.board);
        if (state_.board->is_draw(ply)) return 0;
        // The upcoming repetition, as at an interior node (which made the
        // test for a node it hands here).
        if (alpha < 0 && config_.limits.params.b3_cuckoo
            && state_.board->upcoming_repetition(ply)) {
            DIAG_COUNT(++state_.diag.cuckoo_draws);
            TRACE_DECISION(TraceEvent::CuckooDraw, ply, 0, MOVE_NONE,
                           alpha, beta, VALUE_NONE, -1, VALUE_NONE, VALUE_NONE,
                           0, 0, 0, 0);
            alpha = 0;
            if (alpha >= beta) return alpha;
        }
    }

    bool in_check = state_.board->is_in_check();
    ss->in_check = in_check;
    DIAG_COUNT(state_.diag.qs_nodes++);
    DIAG_COUNT(state_.diag.qs_pv_nodes += is_pv);

    // TT probe
    Key hash = state_.board->position_key();
    TTEntry tte{};
    bool tt_found = tt_probe(hash, tte);
    DIAG_COUNT(state_.diag.tt_probes++);
    if (tt_found) DIAG_COUNT(state_.diag.tt_hits++);
    // A store here keeps the pv bit an entry for this position already has.
    const bool pv_hit = tt_found && tte.is_pv();
    Move tt_move = MOVE_NONE;
    int  tt_score = VALUE_NONE;       // hoisted for the stand-pat tighten
    TTFlag tt_flag = TT_NONE;
    if (tt_found) {
        tt_move = move_from_tt(tte.move16);
        tt_score = TranspositionTable::score_from_tt(tte.score, ply, state_.board->rule50_count());
        tt_flag = tte.bound();
        if (!is_pv && tt_score != VALUE_NONE
            && (tt_score >= beta ? proves_lower(tt_flag) : proves_upper(tt_flag))) {
            TRACE_DECISION(TraceEvent::QsTtCutoff, ply, 0, tt_move,
                           alpha, beta, tt_score, -1, VALUE_NONE, VALUE_NONE,
                           0, 0, VALUE_NONE, tt_score);
            DIAG_COUNT(++state_.diag.qs_tt_cutoffs);
            return tt_score;
        }
    }

#ifdef BASILISK_PRICING
    // A pruned capture searched in quiescence with the node's window; those
    // that beat alpha are judged against the final best when the node ends.
    PricePending pending[8];
    int pending_count = 0;
    auto price_move = [&](PriceRule rule, Move m, int move_number, int move_class = 0) {
        const int cell = state_.price.decide(rule, move_class, node_kind, move_number);
        if (cell < 0)
            return;
        int64_t probe_nodes = 0;
        const int v = isolated_probe(ss, ply, probe_nodes, [&] {
            do_move(ss, m);
            const int s = -quiescence<NT>(-beta, -alpha, ply + 1, qply + 1, ss + 1);
            undo_move(ss, m);
            return s;
        });
        if (state_.stopped)
            return;
        state_.price.probed(cell, v > alpha, probe_nodes);
        if (v > alpha && pending_count < 8)
            pending[pending_count++] = {cell, v};
    };
    auto settle = [&](int final_best) {
        for (int k = 0; k < pending_count; ++k)
            if (pending[k].value > final_best)
                state_.price.changed(pending[k].cell);
    };
#endif

    const auto& kp = config_.limits.params;

    // A check is answered through the node kernel's own picker: the
    // table move first when it is a legal evasion, then the picker's stages,
    // so the history a node has learned orders its evasions. Once one evasion
    // holds a value that is not a loss, the quiets stay undelivered and a
    // capture that loses material is skipped; while the best is a loss every
    // evasion is searched, so a mate score here is a proven mate.
    if (in_check) {
        int best = -INF_SCORE;
        Move best_move = MOVE_NONE;
        Move best_scored = MOVE_NONE;
        DIAG_COUNT(state_.diag.qs_evasion_nodes++);
        const PieceToHistory* const conts[6] = {
            (ss - 1)->cont_hist, (ss - 2)->cont_hist, (ss - 3)->cont_hist,
            (ss - 4)->cont_hist, (ss - 5)->cont_hist, (ss - 6)->cont_hist};
        CoreMovePicker picker(*state_.board, hist_, pawn_bucket(), conts, tt_move, MOVE_NONE,
                              0, ply, nullptr, state_.move_buffers[ply][0]);
        int evasions_searched = 0;
        [[maybe_unused]] int quiets_searched = 0;
        PRICE(MoveList quiets_tried);
        for (Move m = picker.next(); m != MOVE_NONE; m = picker.next()) {
            DIAG_COUNT(state_.diag.qs_evasion_tt_move
                       += picker.last_source() == CoreMovePicker::Src::TT);
            const bool holds = kp.b4_evasion_prune && best > -tablebaseWinInMaxPly;
            if (holds && !state_.board->see_ge(m, kp.b4_qs_see_margin)) {
                TRACE_DECISION(TraceEvent::QsEvasionPrune, ply, 0, m,
                               alpha, beta, best, -1, VALUE_NONE, VALUE_NONE,
                               evasions_searched, 0, kp.b4_qs_see_margin, alpha);
                PRICE(price_move(PriceRule::QsEvasion, m, evasions_searched + 1,
                                 price_class(capture_victim(*state_.board, m),
                                             b4::is_recapture(m, (ss - 1)->move))));
                DIAG_COUNT(++state_.diag.qs_evasion_see_pruned);
                continue;
            }
            [[maybe_unused]] const bool quiet = !is_capture_stage(*state_.board, m);
            do_move(ss, m);
            const int s = -quiescence<NT>(-beta, -alpha, ply + 1, qply + 1, ss + 1);
            undo_move(ss, m);
            if (state_.stopped) return 0;
            DIAG_COUNT(++state_.diag.qs_evasion_moves);
            DIAG_COUNT(quiets_searched += quiet);
            PRICE(if (quiet) quiets_tried.push(m));
            ++evasions_searched;
            if (s > best) {
                best = s;
                best_scored = m;
            }
            if (s > alpha) {
                alpha = s;
                best_move = m;
            }
            if (alpha >= beta) {
                TRACE_DECISION(TraceEvent::QsBetaCutoff, ply, 0, m,
                               alpha, beta, VALUE_NONE, -1, VALUE_NONE, VALUE_NONE,
                               0, 0, VALUE_NONE, s);
                DIAG_COUNT(++state_.diag.qs_evasion_fail_highs);
                DIAG_COUNT(state_.diag.qs_evasion_fail_high_first += evasions_searched == 1);
                DIAG_COUNT(state_.diag.qs_evasion_fail_high_index_sum += evasions_searched - 1);
                DIAG_COUNT(state_.diag.qs_evasion_tt_cuts += m == tt_move);
                PRICE(settle(s));
                const int value = b4::fail_high_value(s, beta, kp.b4_qs_fail_high_blend);
                tt_store(hash, 0, value, TT_BETA, m, ply, INF_SCORE, pv_hit);
                return value;
            }
            if (kp.b4_evasion_prune && best > -tablebaseWinInMaxPly && !picker.quiets_skipped())
                picker.skip_quiets();
        }
        if (evasions_searched == 0) {
            CORE_NODE_CHECK(has_no_legal_move(*state_.board));
            return -(MATE_SCORE - ply);
        }
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC) \
    || defined(BASILISK_RELEASE_DIAG_COUNTERS)
        // What the rule left unsearched at a node that found no cutoff. It
        // costs a generation of the quiets, so it runs only for its readers:
        // the counters and the trace under Diag, the prices under Pricing.
        // Unread, it would make a tune build's rule dearer than its switch.
        bool accounted = config_.limits.diag;
#ifdef BASILISK_PRICING
        accounted = accounted || state_.price.enabled();
#endif
        if (accounted && picker.quiets_skipped()) {
            MoveList quiets;
            state_.board->gen_legal_quiets(quiets);
            DIAG_COUNT(state_.diag.qs_evasion_quiets_pruned
                       += static_cast<int>(quiets.size()) - quiets_searched);
            TRACE_DECISION(TraceEvent::QsEvasionPrune, ply, 0, MOVE_NONE,
                           alpha, beta, best, -1, VALUE_NONE, VALUE_NONE,
                           evasions_searched, 0, VALUE_NONE, alpha);
#ifdef BASILISK_PRICING
            int move_number = evasions_searched;
            for (Move m : quiets)
                if (std::find(quiets_tried.begin(), quiets_tried.end(), m) == quiets_tried.end())
                    price_move(PriceRule::QsEvasion, m, ++move_number,
                               price_class(NO_PIECE_TYPE, false));
#endif
        }
#endif
        PRICE(settle(best));
        if (best_move == MOVE_NONE && !kp.b3_fail_low_store)
            best_move = best_scored;
        tt_store(hash, 0, best, TT_ALPHA, best_move, ply, INF_SCORE, pv_hit);
        return best;
    }

    // Stand-pat evaluation. The TT holds the evaluation without the halfmove
    // clock, which its key lacks; the damping is applied at this node's clock.
    int tt_eval;
    if (tt_found && tte.static_eval != INF_SCORE)
        tt_eval = tte.static_eval;
    else
        tt_eval = evaluator_.evaluate_undamped(*state_.board);
    const int raw_eval = damp_rule50(tt_eval, state_.board->rule50_count());

    int stand_pat = corrected_eval(raw_eval, correction_value(ss));
    const int static_eval = stand_pat;
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
    const int correction = stand_pat - raw_eval;
#endif

    // A TT score whose bound proves it a better estimate than the evaluation
    // replaces it as the stand-pat; the stored evaluation (tt_eval) is
    // unchanged. Never a mate or tablebase score: one proven at another
    // window would leave the evaluation band as a value no capture searched.
    if (tt_found && tt_score != VALUE_NONE
        && !is_decisive(tt_score)
        && (tt_score > stand_pat ? proves_lower(tt_flag) : proves_upper(tt_flag))) {
        DIAG_COUNT(++state_.diag.qs_standpat_refined);
        DIAG_COUNT(state_.diag.qs_standpat_refine_mass += std::abs(tt_score - stand_pat));
        stand_pat = tt_score;
    }

    if (stand_pat >= beta) {
        DIAG_COUNT(++state_.diag.qs_standpat_cuts);
        DIAG_COUNT(state_.diag.qs_standpat_excess += stand_pat - beta);
        // The value returned lies near the bound it proved, so the main
        // search's margins read a leaf's value and not its excess.
        const int value = b4::fail_high_value(stand_pat, beta, kp.b4_qs_stand_pat_blend);
        TRACE_DECISION(TraceEvent::QsStandPatCutoff, ply, 0, MOVE_NONE,
                       alpha, beta, stand_pat, -1, correction, VALUE_NONE,
                       0, 0, 0, value);
        tt_store(hash, 0, value, TT_BETA, MOVE_NONE, ply, tt_eval, pv_hit);
        return value;
    }

    // The pruning below reads a capture's gain as the piece it takes. Not
    // when the opponent holds a known win: a capture there can end the
    // ending, as Kxh8 ends a KBB-K in a dead draw, and the stand-pat says
    // nothing about what it leaves (the oracle's futilityBase > -KNOWN_WIN).
    // The gain is read against the static eval, not the table's tighter
    // stand-pat: a bound proved for this node says nothing about what a
    // capture leaves.
    const int futility_base = static_eval + kp.b4_qs_fut_base;
    const bool prune_by_gain = futility_base > -KNOWN_WIN;

    // Delta pruning (B4QsDelta, off by default): even capturing a queen with a
    // margin to spare cannot raise alpha, so that optimistic value is the
    // node's upper bound.
    const int delta_bound = stand_pat + PIECE_VALUE[QUEEN] + 200;
    if (kp.b4_qs_delta && !bare && prune_by_gain && delta_bound < alpha) {
        DIAG_COUNT(++state_.diag.qs_delta_pruned);
        TRACE_DECISION(TraceEvent::QsDeltaPrune, ply, 0, MOVE_NONE,
                       alpha, beta, stand_pat, -1, correction, VALUE_NONE,
                       0, 0, PIECE_VALUE[QUEEN] + 200, delta_bound);
#ifdef BASILISK_PRICING
        if (const int cell = state_.price.decide(PriceRule::QsDelta, 0, node_kind, 0); cell >= 0) {
            int64_t probe_nodes = 0;
            const int v = isolated_probe(ss, ply, probe_nodes, [&] {
                state_.price.set_bare();
                return quiescence<NT>(entry_alpha, beta, ply, qply, ss, counted);
            });
            if (!state_.stopped)
                state_.price.probed(cell, v > alpha, probe_nodes);
        }
#endif
        tt_store(hash, 0, delta_bound, TT_ALPHA, MOVE_NONE, ply, tt_eval, pv_hit);
        return delta_bound;
    }

    int best = stand_pat;
    if (stand_pat > alpha) alpha = stand_pat;

    MoveList captures;
    state_.board->gen_legal_captures(captures);

    // Score captures as the picker's capture stage does, the TT move first.
    ScoredMove* sm = state_.move_buffers[ply][0];
    int nm = 0;
    for (Move m : captures)
        sm[nm++] = {m, m == tt_move ? 10'000'000
                                    : capture_order_score(*state_.board, hist_, m)};

    Move best_move = MOVE_NONE;
    DIAG_COUNT(state_.diag.qs_moves += nm);
    [[maybe_unused]] int captures_searched = 0;
    int move_count = 0;
    const Move previous_move = (ss - 1)->move;
    Move best_scored = MOVE_NONE;
    int best_scored_value = -INF_SCORE;

    for (int i = 0; i < nm; i++) {
        Move m = pick_next(sm, i, nm);

        const bool is_promo = move_type(m) == PROMOTION;

        bool gives_check_known = false;
        bool gives_check = false;
        auto move_gives_check = [&]() {
            if (!gives_check_known) {
                gives_check = state_.board->gives_check(m);
                gives_check_known = true;
            }
            return gives_check;
        };
#ifdef BASILISK_PRICING
        const int move_class = price_class(capture_victim(*state_.board, m),
                                           b4::is_recapture(m, (ss - 1)->move));
#endif
        // A static margin is nearly exact for a capture that starts an
        // exchange and often wrong for one that finishes it, so a
        // recapture, like a check or a promotion, is not judged by what
        // it takes. Every capture must still clear the plain SEE.
        ++move_count;
        const bool recapture = kp.b4_qs_recapture && b4::is_recapture(m, previous_move);
        DIAG_COUNT(state_.diag.qs_recapture_exempt
                   += prune_by_gain && !is_promo && recapture && !move_gives_check());
        if (prune_by_gain && !is_promo && !recapture && !move_gives_check()) {
            DIAG_COUNT(++state_.diag.qs_gain_tested);
            if (move_count > kp.b4_qs_move_count) {
                TRACE_DECISION(TraceEvent::QsCountPrune, ply, 0, m,
                               alpha, beta, stand_pat, -1, correction, VALUE_NONE,
                               i, 0, kp.b4_qs_move_count, alpha);
                PRICE(price_move(PriceRule::QsCount, m, i + 1, move_class));
                DIAG_COUNT(++state_.diag.qs_count_pruned);
                continue;
            }
            const int gain = b4::gain_value(capture_victim(*state_.board, m),
                                            kp.b4_gain_values);
            const int futility_value = futility_base + gain;
            if (futility_value <= alpha) {
                TRACE_DECISION(TraceEvent::QsFutilityPrune, ply, 0, m,
                               alpha, beta, stand_pat, -1, correction, VALUE_NONE,
                               i, 0, kp.b4_qs_fut_base + gain, alpha);
                PRICE(price_move(PriceRule::QsFutility, m, i + 1, move_class));
                DIAG_COUNT(++state_.diag.qs_futility_pruned);
                best = std::max(best, futility_value);
                continue;
            }
            if (!state_.board->see_ge(m, alpha - futility_base)) {
                TRACE_DECISION(TraceEvent::QsSeePrune, ply, 0, m,
                               alpha, beta, stand_pat, -1, correction, VALUE_NONE,
                               i, 0, alpha - futility_base, alpha);
                PRICE(price_move(PriceRule::QsSee, m, i + 1, move_class));
                DIAG_COUNT(++state_.diag.qs_see_pruned);
                best = std::max(best, std::min(alpha, futility_base));
                continue;
            }
        }
        if (!state_.board->see_ge(m, kp.b4_qs_see_margin)) {
            TRACE_DECISION(TraceEvent::QsSeePrune, ply, 0, m,
                           alpha, beta, stand_pat, -1, correction, VALUE_NONE,
                           i, 0, kp.b4_qs_see_margin, alpha);
            PRICE(price_move(PriceRule::QsSee, m, i + 1, move_class));
            DIAG_COUNT(++state_.diag.qs_see_pruned);
            continue;
        }

        do_move(ss, m);
        int s = -quiescence<NT>(-beta, -alpha, ply + 1, qply + 1, ss + 1);
        undo_move(ss, m);

        if (state_.stopped) return 0;
        DIAG_COUNT(++state_.diag.qs_searched);
        DIAG_COUNT(++captures_searched);
        if (s > best_scored_value) {
            best_scored_value = s;
            best_scored = m;
        }
        if (s > best) best = s;
        if (s > alpha) {
            alpha = s;
            best_move = m;
        }
        if (s >= beta) {
            DIAG_COUNT(++state_.diag.qs_fail_highs);
            DIAG_COUNT(state_.diag.qs_fail_high_first += captures_searched == 1);
            DIAG_COUNT(state_.diag.qs_fail_high_index_sum += captures_searched - 1);
            DIAG_COUNT(state_.diag.qs_fail_high_excess += is_decisive(s) ? 0 : s - beta);
            TRACE_DECISION(TraceEvent::QsBetaCutoff, ply, 0, m,
                           alpha, beta, stand_pat, -1, correction, VALUE_NONE,
                           i, 0, VALUE_NONE, s);
            PRICE(settle(s));
            const int value = b4::fail_high_value(s, beta, kp.b4_qs_fail_high_blend);
            tt_store(hash, 0, value, TT_BETA, m, ply, tt_eval, pv_hit);
            return value;
        }
    }

    // A quiet check is the one threat the captures cannot show. At the first
    // ply, after them, up to the limit are searched as captures are; a check
    // that hangs the piece shows nothing a stand-pat does not.
    if (kp.b4_quiet_checks > 0 && qply == 0) {
        MoveList checks;
        state_.board->gen_quiet_checks(checks);
        int tried = 0;
        for (Move m : checks) {
            if (tried == kp.b4_quiet_checks)
                break;
            if (!state_.board->see_ge(m, 0))
                continue;
            ++tried;
            do_move(ss, m);
            const int s = -quiescence<NT>(-beta, -alpha, ply + 1, qply + 1, ss + 1);
            undo_move(ss, m);
            if (state_.stopped) return 0;
            DIAG_COUNT(++state_.diag.qs_searched);
            DIAG_COUNT(++state_.diag.qs_quiet_checks);
            DIAG_COUNT(++captures_searched);
            if (s > best_scored_value) {
                best_scored_value = s;
                best_scored = m;
            }
            if (s > best) best = s;
            if (s > alpha) {
                alpha = s;
                best_move = m;
            }
            if (s >= beta) {
                DIAG_COUNT(++state_.diag.qs_fail_highs);
                DIAG_COUNT(state_.diag.qs_fail_high_first += captures_searched == 1);
                DIAG_COUNT(state_.diag.qs_fail_high_index_sum += captures_searched - 1);
                DIAG_COUNT(state_.diag.qs_fail_high_excess += is_decisive(s) ? 0 : s - beta);
                TRACE_DECISION(TraceEvent::QsBetaCutoff, ply, 0, m,
                               alpha, beta, stand_pat, -1, correction, VALUE_NONE,
                               nm + tried - 1, 0, VALUE_NONE, s);
                PRICE(settle(s));
                const int value = b4::fail_high_value(s, beta, kp.b4_qs_fail_high_blend);
                tt_store(hash, 0, value, TT_BETA, m, ply, tt_eval, pv_hit);
                return value;
            }
        }
    }

    // A side with only king and pawns that has just lost a piece may have no
    // move at all, which the stand-pat cannot see. The value is the draw, exact,
    // so it takes no blend and may lie on either side of the window.
    if (kp.b4_stalemate && nm == 0 && type_of(state_.board->captured_piece()) >= KNIGHT
        && !state_.board->has_non_pawn_material(state_.board->turn())) {
        MoveList legal;
        state_.board->gen_legal(legal);
        if (legal.empty()) {
            DIAG_COUNT(++state_.diag.qs_stalemates);
            TRACE_DECISION(TraceEvent::QsStalemate, ply, 0, MOVE_NONE,
                           alpha, beta, stand_pat, -1, correction, VALUE_NONE,
                           0, 0, VALUE_NONE, 0);
            best = 0;
        }
    }

    PRICE(settle(best));
    DIAG_COUNT(++state_.diag.qs_fail_lows);
    DIAG_COUNT(state_.diag.qs_fail_low_searched += captures_searched > 0);
    if (best_move == MOVE_NONE && !kp.b3_fail_low_store)
        best_move = best_scored;
    tt_store(hash, 0, best, best >= beta ? TT_BETA : TT_ALPHA, best_move, ply, tt_eval, pv_hit);
    return best;
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
    // The node's own type in quiescence, where the root is a PV node.
    constexpr NodeType QsType = is_pv ? NodeType::PV : NodeType::NonPV;
    assert(is_root == (ply == 0));
#ifdef BASILISK_PRICING
    // A pricing probe of a node rule runs this node again from its entry
    // without its node-level shortcuts (razoring, reverse futility, the null
    // move, ProbCut and the singular multi-cut).
    const bool bare = state_.price.take_bare();
    const int entry_depth = depth;
    const int entry_alpha = alpha;
    const int entry_beta = beta;
    const PriceNode node_kind = is_pv ? PriceNode::Pv : cut_node ? PriceNode::Cut : PriceNode::All;
#else
    constexpr bool bare = false;
#endif
    record_node();
    // The clock is read every 2048 nodes, the stop flag at every node: the
    // main thread waits for every helper before `bestmove`, so a helper
    // that saw the stop only at its next clock read held the move back by
    // up to 2048 nodes.
    if ((state_.nodes & 2047) == 0 || shared_.stop.load(std::memory_order_relaxed))
        check_stop();
    if (state_.stopped) return 0;

    if (ply >= MAX_PLY) return evaluator_.evaluate(*state_.board);
    state_.pv_len[ply] = ply;

    if (!is_root && state_.board->is_draw(ply)) return 0;
    // A reversible move to a repetition is available, so the
    // node is worth at least the draw (B3Cuckoo; never when alpha is
    // already at it).
    if (!is_root && alpha < 0 && config_.limits.params.b3_cuckoo
        && state_.board->upcoming_repetition(ply)) {
        DIAG_COUNT(++state_.diag.cuckoo_draws);
        TRACE_DECISION(TraceEvent::CuckooDraw, ply, depth, MOVE_NONE,
                       alpha, beta, VALUE_NONE, -1, VALUE_NONE, VALUE_NONE,
                       0, 0, 0, 0);
        alpha = 0;
        if (alpha >= beta) return alpha;
    }

    bool in_check = state_.board->is_in_check();
    ss->in_check   = in_check;
    ss->move_count = 0;
    ss->stat_score = 0;
    // The grandchildren's record starts this node's children clean: the
    // children share (ss + 1), and each that fails high counts in it.
    (ss + 2)->cutoff_cnt = 0;
    ss->follow_pv = b3::follow_pv(is_root, (ss - 1)->follow_pv, ply, (ss - 1)->move,
                                  state_.prev_pv, state_.prev_pv_len);
    ss->ext_taken = 0;

    // Check extension: the donor's policy extends no in-check node; the
    // B3CheckExt categorical restores a ply for every one. ss->excluded keeps
    // it from stacking with a singular extension.
    if (in_check) DIAG_COUNT(state_.diag.in_check_nodes++);
    bool did_check_ext = false;
    if (!ABLATED(6) && in_check && ss->excluded == MOVE_NONE && ply < MAX_PLY - 2
        && config_.limits.params.b3_check_ext) {
        depth++;
        did_check_ext = true;
        DIAG_COUNT(state_.diag.check_exts++);
        TRACE_DECISION(TraceEvent::CheckExtension, ply, depth, MOVE_NONE,
                       alpha, beta, VALUE_NONE, -1, VALUE_NONE, VALUE_NONE,
                       0, 0, 1, depth);
    }

    if (depth <= 0)
        return quiescence<QsType>(alpha, beta, ply, 0, ss, true);
    DIAG_COUNT(state_.diag.interior_nodes++);
    if (ss->follow_pv) DIAG_COUNT(++state_.diag.follow_pv_nodes);

    // Mate distance pruning
    if (!is_root) {
        alpha = std::max(alpha, -(MATE_SCORE - ply));
        beta  = std::min(beta,   (MATE_SCORE - ply - 1));
        if (alpha >= beta) return alpha;
    }

    // The reduction the parent applied to reach this node, read once: a
    // re-search of the same move must not see it again.
    const auto& kp = config_.limits.params;
    // The check policy: in-check nodes are reduced and move-loop pruned as
    // the donor's are (quiet futility excepted), unless B3CheckPrune is 0.
    const bool check_prune = kp.b3_check_prune;
    const int prior_reduction = (ss - 1)->reduction;
    (ss - 1)->reduction = 0;

#ifdef BASILISK_PRICING
    // A node rule's decision: the node searched again from its entry, with
    // the reduction its parent gave it, and judged against the bound the
    // rule's value claims (a fail high at or above beta, a fail low at or
    // below alpha). Returns VALUE_NONE when the search stopped.
    auto node_probe = [&]() {
        int64_t probe_nodes = 0;
        const int v = isolated_probe(ss, ply, probe_nodes, [&] {
            (ss - 1)->reduction = prior_reduction;
            state_.price.set_bare();
            const int s = negamax<NT>(entry_depth, entry_alpha, entry_beta, ply, ss,
                                      allow_null, cut_node);
            (ss - 1)->reduction = 0;
            return s;
        });
        return std::pair<int, int64_t>{state_.stopped ? VALUE_NONE : v, probe_nodes};
    };
    auto node_record = [&](int cell, int value, std::pair<int, int64_t> probe) {
        if (probe.first == VALUE_NONE)
            return;
        const bool beats = value >= beta ? probe.first < beta
                                         : value <= alpha && probe.first > alpha;
        state_.price.probed(cell, beats, probe.second);
    };
    auto price_node = [&](PriceRule rule, int value) {
        if (const int cell = state_.price.decide(rule, depth, node_kind, 0); cell >= 0)
            node_record(cell, value, node_probe());
    };
#endif

    // ---- Transposition table lookup ----------------------------------------
    Key hash     = state_.board->position_key();
    TTEntry tte{};
    bool tt_found = tt_probe(hash, tte);
    DIAG_COUNT(state_.diag.tt_probes++);
    if (tt_found) DIAG_COUNT(state_.diag.tt_hits++);
    ss->tt_hit = tt_found;

    Move  tt_move  = MOVE_NONE;
    int   tt_score = VALUE_NONE;
    int   tt_depth = 0;
    TTFlag tt_flag  = TT_NONE;

    if (tt_found) {
        tt_move  = move_from_tt(tte.move16);
        // An evaluation-only entry has no bound and reads back as VALUE_NONE.
        tt_score = TranspositionTable::score_from_tt(tte.score, ply, state_.board->rule50_count());
        // depth is int8_t with a deliberate -1 sentinel; tidy's suggested
        // unsigned cast would corrupt it.
        // NOLINTNEXTLINE(bugprone-signed-char-misuse)
        tt_depth = tte.depth;
        tt_flag  = tte.bound();
    }
    if (tt_move == MOVE_NONE) DIAG_COUNT(++state_.diag.tt_move_absent);
    else if (tt_depth <= 0) DIAG_COUNT(++state_.diag.tt_move_d0);

    // The node lies on or near a principal variation: a PV node, or a
    // position the table marked so. An excluded search keeps its node's flag.
    if (ss->excluded == MOVE_NONE)
        ss->tt_pv = is_pv || (tt_found && tte.is_pv());
    if (ss->tt_pv) DIAG_COUNT(state_.diag.tt_pv_nodes++);

    // The record starts this node at its defaults: the counts this node
    // reset, the reduction and the children's exclusion the parent cleared
    // after using them, and the pv flag the node type and the table set.
    CORE_NODE_CHECK(ss->in_check == in_check);
    CORE_NODE_CHECK(ss->move_count == 0);
    CORE_NODE_CHECK(ss->stat_score == 0);
    CORE_NODE_CHECK(ss->reduction == 0);
    CORE_NODE_CHECK((ss + 2)->cutoff_cnt == 0);
    CORE_NODE_CHECK((ss + 1)->excluded == MOVE_NONE);
    CORE_NODE_CHECK(ss->excluded != MOVE_NONE
                    || ss->tt_pv == (is_pv || (tt_found && tte.is_pv())));
    // A child's extension plies are its parent's plus the
    // parent's positive extension, and only a node whose parent follows
    // the previous PV can follow it.
    CORE_NODE_CHECK(is_root || ss->ext_plies == (ss - 1)->ext_plies + (ss - 1)->ext_taken);
    CORE_NODE_CHECK(!kp.b3_ext_budget || ss->ext_plies <= root_depth_);
    CORE_NODE_CHECK(is_root || !ss->follow_pv || (ss - 1)->follow_pv);

    // Is the TT move from the capture stage (a capture or a promotion)?
    const bool tt_capture = tt_move != MOVE_NONE
        && (state_.board->piece_on(to_sq(tt_move)) != NO_PIECE
            || move_type(tt_move) == EN_PASSANT || move_type(tt_move) == PROMOTION);

    // ---- Static evaluation -------------------------------------------------
    // The corrected static eval: the raw eval plus the correction histories'
    // weighted value over 131072, inside the tablebase band.
    const int correction_raw = correction_value(ss);
    // The TT holds the evaluation without the halfmove clock, which its key
    // lacks (tt_static_eval); the damping is applied once, at this node's
    // clock, to give the raw static eval.
    int static_eval;
    int raw_static_eval = VALUE_NONE;
    int tt_static_eval = INF_SCORE;
    if (in_check) {
        // The record carries the eval from two plies up, so `improving` reads
        // across the check; only capture futility, the donor's form, prunes
        // on it here.
        ss->eval = (ss - 2)->eval;
        static_eval = VALUE_NONE;
    } else if (ss->excluded != MOVE_NONE) {
        // Inherit eval from parent to avoid calling evaluate twice
        static_eval = ss->eval;
    } else {
        if (tt_found && tte.static_eval != INF_SCORE)
            tt_static_eval = tte.static_eval;
        else {
            tt_static_eval = evaluator_.evaluate_undamped(*state_.board);
            // A miss stores the evaluation alone, so a transposition reuses it.
            if (!tt_found)
                tt_store(hash, TT_DEPTH_UNSEARCHED, VALUE_NONE, TT_NONE, MOVE_NONE, ply,
                         tt_static_eval, ss->tt_pv);
        }
        raw_static_eval = damp_rule50(tt_static_eval, state_.board->rule50_count());

        // TT stores the raw static eval; correction is applied at probe time.
        static_eval = corrected_eval(raw_static_eval, correction_raw);
        ss->eval = static_eval;
        ss->raw_eval = raw_static_eval;
    }

    // Value used for PRUNING decisions only. When a TT entry's bound
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
            || (tt_flag == TT_ALPHA && tt_score < static_eval))) {
        DIAG_COUNT(++state_.diag.eval_refined);
        DIAG_COUNT(state_.diag.eval_refine_mass += std::abs(tt_score - static_eval));
        if (tt_depth <= 0) {
            DIAG_COUNT(++state_.diag.eval_refined_d0);
            DIAG_COUNT(state_.diag.eval_refine_mass_d0 += std::abs(tt_score - static_eval));
        }
        eval = tt_score;
    }

#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
    const int correction = raw_static_eval == VALUE_NONE
                         ? VALUE_NONE : static_eval - raw_static_eval;
#endif

    // Improving: the static eval rose since this side's previous move (a
    // record without an eval reads VALUE_NONE, which nothing exceeds).
    // Opponent worsening: it rose since the opponent's, seen from this side.
    bool improving = ss->eval > (ss - 2)->eval;
    const bool opponent_worsening = ss->eval > -(ss - 1)->eval;

    // ---- Hindsight depth adjustment ------------------------------------------
    // A child searched at a reduced depth gets a ply back when the reduction
    // was large and the opponent's position did not worsen, and loses one
    // when the reduction was moderate and the two static evals sum high (a
    // quiet node both sides rate well for themselves).
    if (prior_reduction >= 3 && !opponent_worsening) {
        ++depth;
        DIAG_COUNT(++state_.diag.hindsight_up);
    }
    [[maybe_unused]] bool hindsight_reduced = false;
    if (prior_reduction >= 2 && depth >= 2 && ss->eval + (ss - 1)->eval > 47) {
        --depth;
        DIAG_COUNT(++state_.diag.hindsight_down);
        DIAG_COUNT(hindsight_reduced = true);
    }


    // ---- Early TT cutoff -----------------------------------------------------
    // A non-PV node takes the TT score when the entry is deep enough and its
    // bound proves the side of the window the score lies on; a fail-low entry
    // may be one ply shallower. Unless the node-typed rule is off, a shallow
    // node only cuts where the score agrees with its expected type.
    if (!is_pv && ss->excluded == MOVE_NONE && tt_score != VALUE_NONE
        && tt_depth > depth - (tt_score <= beta)
        && (tt_score >= beta ? proves_lower(tt_flag) : proves_upper(tt_flag))
        && (!kp.tt_cutoff_node_typed || cut_node == (tt_score >= beta) || depth > 4)) {

        // A quiet TT move that fails high trains the quiet histories, and the
        // parent's early quiet move it refutes is penalised.
        if (tt_score >= beta && tt_move_playable(tt_move)) {
            if (!tt_capture) {
                update_quiet_histories(ss, tt_move, kp.tt_cutoff_bonus_slope * depth);
                DIAG_COUNT(++state_.diag.tt_cutoff_quiet_bonus);
            }
            const Move prev = (ss - 1)->move;
            if (prev != MOVE_NONE && prev != MOVE_NULL && (ss - 1)->move_count < 5
                && state_.board->captured_piece() == NO_PIECE)
                update_continuation_histories(
                    ss - 1, state_.board->piece_on(to_sq(prev)), Square(to_sq(prev)), -2210);
        }

        // Near the rule-50 limit the stored score may not hold, so no cutoff.
        // Deep enough, a cutoff is checked one ply down: the TT move's child
        // entry must agree that the move fails high (or low), against
        // graph-history errors.
        bool cut = state_.board->rule50_count() < 96;
        if (cut && depth >= 7 && !is_decisive(tt_score) && tt_move_playable(tt_move)) {
            do_move(ss, tt_move);
            TTEntry next{};
            const bool next_found = tt_probe(state_.board->position_key(), next);
            undo_move(ss, tt_move);
            if (next_found && next.score != VALUE_NONE
                && (tt_score >= beta) != (-int(next.score) >= beta)) {
                cut = false;
                DIAG_COUNT(++state_.diag.tt_cutoff_graph_refused);
            }
        }
        if (cut) {
            DIAG_COUNT(state_.diag.tt_cutoffs++);
            TRACE_DECISION(TraceEvent::TtCutoff, ply, depth, tt_move,
                           alpha, beta, tt_score, -1, VALUE_NONE, VALUE_NONE,
                           0, 0, tt_depth, tt_score);
            return tt_score;
        }
    } else if (kp.b3_tt_penalty && !is_pv && ss->excluded == MOVE_NONE
               && tt_score != VALUE_NONE && tt_depth > depth - (tt_score <= beta)
               && tt_flag == (tt_score >= beta ? TT_ALPHA : TT_BETA) && depth > 5) {
        // Deep enough to cut, but the bound lies on the wrong side of beta:
        // the entry no longer answers this window, so it gives up a ply of
        // depth to the next store (the donor's penalty).
        if (!PRICE_PROBING())
            shared_.tt.penalize(hash);
        DIAG_COUNT(++state_.diag.tt_penalized);
        TRACE_DECISION(TraceEvent::TtPenalty, ply, depth, tt_move,
                       alpha, beta, tt_score, -1, VALUE_NONE, VALUE_NONE,
                       0, 0, tt_depth, tt_score);
    }

    // ---- In-search tablebase probe ------------------------------------------
    // After the TT cutoff, as in Stockfish, so a TT hit pays no tablebase
    // read and the store takes this node's PV flag. A result that decides the
    // node returns and is stored as the bound it proves; otherwise a PV node
    // keeps searching inside it: a win raises the floor, a loss caps the
    // result.
    int tb_floor = -INF_SCORE;
    int tb_cap   = INF_SCORE;
    if (!is_root && ss->excluded == MOVE_NONE && state_.tb_probe_in_search
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
                    DIAG_COUNT(state_.diag.tb_store_stale_pv
                               += ss->tt_pv != (is_pv || (tt_found && tte.is_pv())));
                    tt_store(state_.board->position_key(), std::min(MAX_PLY - 1, depth + 6),
                             tb.value, tb.bound, MOVE_NONE, ply, INF_SCORE,
                             ss->tt_pv);
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

    // ---- Eval-difference training ------------------------------------------
    // The parent's quiet move is trained by how much it lowered the static
    // eval of the side that answers it (both evals from their mover's view).
    if (!in_check && (ss - 1)->move != MOVE_NONE && (ss - 1)->move != MOVE_NULL
        && !(ss - 1)->in_check && state_.board->captured_piece() == NO_PIECE) {
        // The sum in the donor's evaluation units, under its clamp and offset.
        const int eval_diff = std::clamp(-((ss - 1)->eval + ss->eval) * 1000 / 282, -189, 194) + 60;
        update_parent_by_eval_difference(ss, eval_diff, tt_found);
    }

    // ---- Non-PV pruning (skip if in check, in PV, or singular search) ------
    if (!is_pv && !in_check && ss->excluded == MOVE_NONE
        && static_eval != VALUE_NONE) {

        // Razoring: an estimate this far below alpha at a shallow node is
        // left to quiescence, whose value is returned. Not when the table
        // suggests a quiet move, or proves the node fails high.
        if (!bare && !ABLATED(0) && depth <= kp.razor_depth_cap
            && eval < alpha - kp.razor_margin * depth
            && (tt_move == MOVE_NONE || tt_capture) && tt_flag != TT_BETA) {
            // Priced before the quiescence search, whose stores at this
            // position the probe would otherwise read.
#ifdef BASILISK_PRICING
            const int razor_cell = state_.price.decide(PriceRule::Razor, depth, node_kind, 0);
            const auto razor_probe = razor_cell >= 0 ? node_probe()
                                                     : std::pair<int, int64_t>{VALUE_NONE, 0};
#endif
            const int q = quiescence<NodeType::NonPV>(alpha, beta, ply, 0, ss, true);
            PRICE(if (razor_cell >= 0) node_record(razor_cell, q, razor_probe));
            DIAG_COUNT(state_.diag.razor_cuts++);
            DIAG_COUNT(++(depth <= 3 ? state_.diag.razor_cuts_d1_3
                          : depth <= 7 ? state_.diag.razor_cuts_d4_7
                                       : state_.diag.razor_cuts_d8p));
            TRACE_DECISION(TraceEvent::RazorPrune, ply, depth, MOVE_NONE,
                           alpha, beta, eval, improving, correction, VALUE_NONE,
                           0, 0, kp.razor_margin * depth, q,
                           -1, VALUE_NONE, -1, TraceFamily::Razor);
            return q;
        }

        // Reverse futility pruning: an estimate this far above beta returns a
        // value between the two. The margin shrinks when this side improves
        // or the opponent's position worsens, and grows with the size of the
        // correction (off at a multiplier of 0). Not near a PV, not when the
        // table suggests a quiet move, and never on a decisive bound.
        if (!bare && !ABLATED(1) && !ss->tt_pv && depth <= kp.rfp_depth_cap && eval >= beta
            && (tt_move == MOVE_NONE || tt_capture)
            && beta > -tablebaseWinInMaxPly && eval < tablebaseWinInMaxPly) {
            int margin = kp.rfp_per_ply * depth
                       - kp.rfp_improving * improving
                       - kp.rfp_worsening * opponent_worsening;
            const int correction_term
                = std::abs(correction_raw) / 131072 * kp.rfp_corr_mult / 1024;
            margin += correction_term;
            // The node would have cut without the correction term.
            if (eval - margin < beta && eval - margin + correction_term >= beta) {
                DIAG_COUNT(++state_.diag.rfp_corr_blocked);
                DIAG_COUNT(state_.diag.rfp_corr_blocked_tt_eval += eval != static_eval);
            }
            const bool seek_mate_capped = kp.b4_rfp_seek_mate
                && b4::rfp_past_seek_mate_cap(
                       b3::seek_mate(root_depth_, root_score_, kp.b4_rfp_seek_mate_depth,
                                     kp.b3_seek_mate_score),
                       depth, kp.b4_rfp_seek_mate_cap);
            DIAG_COUNT(state_.diag.rfp_seekmate_capped
                       += seek_mate_capped && eval - margin >= beta);
            if (!seek_mate_capped && eval - margin >= beta) {
                DIAG_COUNT(state_.diag.rfp_cuts_tt_eval += eval != static_eval);
                const int value = (kp.rfp_beta_weight * beta
                                   + (1024 - kp.rfp_beta_weight) * eval) / 1024;
                DIAG_COUNT(state_.diag.rfp_cuts++);
                DIAG_COUNT(++(depth <= 3 ? state_.diag.rfp_cuts_d1_3
                              : depth <= 7 ? state_.diag.rfp_cuts_d4_7
                                           : state_.diag.rfp_cuts_d8p));
                TRACE_DECISION(TraceEvent::RfpPrune, ply, depth, MOVE_NONE,
                               alpha, beta, eval, improving, correction, VALUE_NONE,
                               0, 0, margin, value,
                               -1, VALUE_NONE, -1, TraceFamily::Rfp);
                PRICE(price_node(PriceRule::Rfp, value));
                return value;
            }
        }

        // Null-move pruning, the donor's form: at an expected cut
        // node whose static eval clears a depth-scaled margin. The null child
        // is an all node, and a win is never returned unproven. A deep cut
        // outside a verification is verified at this ply with null moves
        // barred above a region of plies below it.
        if (!bare && !ABLATED(2) && (cut_node || !kp.b3_nmp_cut_only) && allow_null
            && (ss - 1)->move != MOVE_NULL
            && static_eval >= beta - kp.b3_nmp_per_ply * depth
                              - kp.b3_nmp_improving * improving + kp.b3_nmp_base
            && beta >= kp.b3_nmp_beta_floor && (!kp.b3_nmp_eval_gate || eval >= beta)
            && state_.board->has_non_pawn_material(state_.board->turn())) {
            const int r = kp.b3_nmp_red_base + depth / kp.b3_nmp_red_depth_div
                        + std::max((static_eval - beta) / kp.b3_nmp_red_eval_div, 0);
            if (ply < nmp_min_ply_) {
                DIAG_COUNT(++state_.diag.null_region_refusals);
                TRACE_DECISION(TraceEvent::NullRegionRefused, ply, depth, MOVE_NULL,
                               alpha, beta, static_eval, improving, correction, VALUE_NONE,
                               0, r, nmp_min_ply_, VALUE_NONE);
            } else {
                DIAG_COUNT(state_.diag.null_tries++);
                DIAG_COUNT(++(depth <= 3 ? state_.diag.null_tries_d1_3
                              : depth <= 7 ? state_.diag.null_tries_d4_7
                                           : state_.diag.null_tries_d8p));
                do_null_move(ss);
                shared_.tt.prefetch(state_.board->position_key());
                (ss + 1)->ext_plies = ss->ext_plies;
                int null_score = -negamax<NodeType::NonPV>(std::max(0, depth - r), -beta,
                                                           -(beta - 1), ply + 1, ss + 1,
                                                           false, false);
                undo_null_move(ss);
                if (state_.stopped) return 0;
                if (null_score >= beta && !is_decisive(null_score)) {
                    const int mode = kp.b3_nmp_verify;
                    const bool verify = mode == 1 ? nmp_min_ply_ == 0
                                                    && depth >= kp.b3_nmp_verify_depth
                                      : mode == 2 && depth >= 10;
                    bool verified = true;
                    if (verify) {
                        // The verification re-enters this record; the
                        // fields its move loop leaves behind are put back,
                        // so ProbCut's children read this node's own.
                        const int saved_move_count = ss->move_count;
                        const int saved_stat_score = ss->stat_score;
                        int v;
                        if (mode == 1) {
                            nmp_min_ply_ = ply + 3 * (depth - r) / 4;
                            v = negamax<NodeType::NonPV>(depth - r, beta - 1, beta,
                                                         ply, ss, true, false);
                            nmp_min_ply_ = 0;
                        } else {
                            v = negamax<NodeType::NonPV>(std::max(1, depth - r), beta - 1,
                                                         beta, ply, ss, false, false);
                        }
                        ss->move_count = saved_move_count;
                        ss->stat_score = saved_stat_score;
                        if (state_.stopped) return 0;
                        verified = v >= beta;
                        DIAG_COUNT(++state_.diag.null_verifications);
                        if (!verified) DIAG_COUNT(++state_.diag.null_verify_failed);
                        TRACE_DECISION(TraceEvent::NullVerify, ply, depth, MOVE_NULL,
                                       alpha, beta, static_eval, improving, correction,
                                       VALUE_NONE, 0, r, null_score, v);
                    }
                    if (verified) {
                        DIAG_COUNT(state_.diag.null_cuts++);
                        DIAG_COUNT(++(depth <= 3 ? state_.diag.null_cuts_d1_3
                                      : depth <= 7 ? state_.diag.null_cuts_d4_7
                                                   : state_.diag.null_cuts_d8p));
                        TRACE_DECISION(TraceEvent::NullCutoff, ply, depth, MOVE_NULL,
                                       alpha, beta, static_eval, improving, correction,
                                       VALUE_NONE, 0, r, VALUE_NONE, null_score);
                        PRICE(price_node(PriceRule::NullMove, null_score));
                        return null_score;
                    }
                }
            }
        }

    }

    if (!in_check)
        improving = improving || ss->eval >= beta;

    // IIR, the donor's form: a PV or cut node off the previous
    // iteration's PV, not in check, without a TT move loses a ply.
    if (!ABLATED(4) && kp.b3_iir && !in_check && !ss->follow_pv && (is_pv || cut_node)
        && depth >= kp.b3_iir_min_depth
        && (tt_move == MOVE_NONE
            || (kp.b3_iir_stale && tt_found && tt_depth < depth - 3))) {
        TRACE_DECISION(TraceEvent::IirReduction, ply, depth, tt_move,
                       alpha, beta, eval, improving, correction, VALUE_NONE,
                       0, 1, depth - 3, depth - 1);
        DIAG_COUNT(++state_.diag.iir_reductions);
        if (tt_move != MOVE_NONE) DIAG_COUNT(++state_.diag.iir_stale);
        if (hindsight_reduced)    DIAG_COUNT(++state_.diag.iir_after_hindsight);
        if (is_pv)                DIAG_COUNT(++state_.diag.iir_pv_nodes);
        depth--;
    }

    // ProbCut, the donor's form, after IIR and at PV nodes too:
    // a capture or queen promotion that holds a margin above beta in
    // quiescence and at a reduced depth proves the node, which returns the
    // value less the margin; a valid TT score below the margin says none will.
    const int pc_beta = beta + kp.b3_probcut_base - kp.b3_probcut_improving * improving;
    if (!bare && !ABLATED(3) && !is_root && !in_check && (!is_pv || kp.b3_probcut_pv)
        && depth >= kp.b3_probcut_min_depth && !is_decisive(beta)
        && !(tt_score != VALUE_NONE && tt_score < pc_beta)) {
        const int pc_depth = depth - (improving ? kp.b3_probcut_depth_improving
                                                : kp.b3_probcut_depth);
        // The store keeps the raw evaluation; an exclusion search, which
        // computed none, keeps the entry's own.
        const int store_eval = tt_static_eval != INF_SCORE ? tt_static_eval
                             : tt_found ? int(tte.static_eval) : INF_SCORE;
        ProbCutPicker pc(*state_.board, hist_, tt_move, ss->excluded, pc_beta - static_eval,
                         state_.move_buffers[ply][1]);
        for (Move m = pc.next(); m != MOVE_NONE; m = pc.next()) {
            DIAG_COUNT(state_.diag.probcut_tries++);
            if (is_pv) DIAG_COUNT(++state_.diag.probcut_pv_tries);
            do_move(ss, m);
            shared_.tt.prefetch(state_.board->position_key());
            (ss + 1)->ext_plies = ss->ext_plies;
            int val = -quiescence<NodeType::NonPV>(-pc_beta, -pc_beta + 1, ply + 1, 0, ss + 1);
            if (val >= pc_beta && pc_depth > 0)
                val = -negamax<NodeType::NonPV>(pc_depth, -pc_beta, -pc_beta + 1,
                                                ply + 1, ss + 1, true, !cut_node);
            undo_move(ss, m);
            if (state_.stopped) return 0;
            if (val >= pc_beta) {
                // Priced before the store at this position, which the
                // probe would otherwise read.
                PRICE(if (!is_decisive(val))
                          price_node(PriceRule::ProbCut, val - (pc_beta - beta)));
                tt_store(hash, pc_depth + 1, val, TT_BETA, m, ply, store_eval, ss->tt_pv);
                if (!is_decisive(val)) {
                    DIAG_COUNT(state_.diag.probcut_cuts++);
                    TRACE_DECISION(TraceEvent::ProbcutCutoff, ply, depth, m,
                                   alpha, beta, eval, improving, correction, VALUE_NONE,
                                   0, depth - pc_depth, pc_beta - static_eval,
                                   val - (pc_beta - beta));
                    return val - (pc_beta - beta);
                }
            }
        }
    }

    // The small ProbCut, when switched on, in check too: a lower bound
    // nearly deep enough that clears beta by a wide margin returns that
    // margin.
    const int tt_pc_beta = beta + kp.b3_probcut_tt_margin;
    if (!bare && !ABLATED(3) && kp.b3_probcut_tt && !is_root && proves_lower(tt_flag)
        && tt_depth >= depth - kp.b3_probcut_tt_depth
        && tt_score != VALUE_NONE && tt_score >= tt_pc_beta
        && !is_decisive(beta) && !is_decisive(tt_score)) {
        DIAG_COUNT(++state_.diag.probcut_tt_cuts);
        TRACE_DECISION(TraceEvent::ProbcutTtCutoff, ply, depth, tt_move,
                       alpha, beta, eval, improving, correction, VALUE_NONE,
                       0, 0, kp.b3_probcut_tt_margin, tt_pc_beta);
        PRICE(price_node(PriceRule::ProbCutTt, tt_pc_beta));
        return tt_pc_beta;
    }

    int  orig_alpha  = alpha;
    Move best_move   = MOVE_NONE;
    int  best_score  = tb_floor;
    int  searched    = 0;
    // Moves picked here; the record publishes it, so a search re-entering
    // this record (singular, null-move verification) leaves it intact.
    int  move_count  = 0;
    // The alpha the move that failed high had to beat (the fail-high blend
    // reads it).
    int  cut_alpha   = alpha;
    const bool all_node = !is_pv && !cut_node;
    // The estimate the reduction's alpha term reads; in check, the record's.
    const int lmr_eval = in_check ? ss->eval : eval;

    // The move that last raised alpha (none after a fail low), and the moves
    // searched before it that did not.
    Move alpha_move = MOVE_NONE;
    Move quiets_searched[SEARCHED_LIST_CAPACITY];
    Move captures_searched[SEARCHED_LIST_CAPACITY];
    int quiets_count = 0;
    int captures_count = 0;

    int root_ordinal = 0;
    bool immediate_return = false;
    int immediate_score = 0;

    // The history-pruning and LMR-stat paths below read these node-invariant
    // rows for every quiet move.
    const auto& main_hist = hist_.main.entry[state_.board->turn()];
    const auto& pawn_hist = pawn_bucket().entry;
    // The decision trace also reads the low-ply history.
    [[maybe_unused]] const auto* low_ply_hist =
        ply < LowPlyHistory::PLIES ? &hist_.low_ply.entry[ply] : nullptr;

    // Set when the TT move proves singular at this node, and then read
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

    // TT move, good captures, good quiets, bad captures, bad quiets; quiet
    // generation and scoring wait until the good captures are spent.
    const PieceToHistory* const conts[6] = {
        (ss - 1)->cont_hist, (ss - 2)->cont_hist, (ss - 3)->cont_hist,
        (ss - 4)->cont_hist, (ss - 5)->cont_hist, (ss - 6)->cont_hist};
    CoreMovePicker picker(*state_.board, hist_, pawn_bucket(), conts, tt_move, ss->excluded,
                          depth, ply, is_root ? &state_.root_ordering : nullptr,
                          state_.move_buffers[ply][0]);

#ifdef BASILISK_PRICING
    // A move searched as a rule denied it, at `child_depth` with the node's
    // window, as a later move's child (`made`: the move is already on the
    // board). Those that beat alpha are judged against the node's final best
    // when its move loop ends.
    PricePending pending[8];
    int pending_count = 0;
    auto probe_move = [&](int cell, Move m, int child_depth, bool made) {
        int64_t probe_nodes = 0;
        const int v = isolated_probe(ss, ply, probe_nodes, [&] {
            if (!made) {
                ss->ext_taken = 0;
                (ss + 1)->ext_plies = ss->ext_plies;
                do_move(ss, m);
            }
            const int s = -negamax<PvChild>(child_depth, -beta, -alpha, ply + 1, ss + 1, true,
                                             !is_pv && !cut_node);
            if (!made)
                undo_move(ss, m);
            return s;
        });
        if (state_.stopped)
            return;
        state_.price.probed(cell, v > alpha, probe_nodes);
        if (v > alpha && pending_count < 8)
            pending[pending_count++] = {cell, v};
    };
    // A pruned move at the depth it would have had before any reduction.
    auto price_move = [&](PriceRule rule, Move m) {
        if (const int cell = state_.price.decide(rule, depth, node_kind, move_count); cell >= 0)
            probe_move(cell, m, depth - 1, false);
    };
    // The move-count rule: the first quiet it keeps from the picker.
    auto price_skip = [&]() {
        const int cell = state_.price.decide(PriceRule::SkipQuiets, depth, node_kind, move_count);
        if (cell < 0)
            return;
        const Move denied = picker.first_denied_quiet(state_.move_buffers[ply][1]);
        if (denied == MOVE_NONE)
            state_.price.probed(cell, false, 0);
        else
            probe_move(cell, denied, depth - 1, false);
    };
#endif

    // The root's queen promotions searched so far, for the rule below.
    static constexpr int ROOT_UNDERPROMOTION_MARGIN = 200;
    Move root_queen[8];
    int  root_queen_score[8];
    int  root_queen_count = 0;

    auto search_one = [&](Move m, CoreMovePicker::Src picker_src) {
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
        ss->move_count = ++move_count;
        int r = lmr_reduction(improving, depth, ss->move_count, beta - alpha);
        if (ss->tt_pv)
            r += kp.lmr_tt_pv;

        bool is_cap   = (state_.board->piece_on(to_sq(m)) != NO_PIECE)
                     || (move_type(m) == EN_PASSANT);
        bool is_promo = (move_type(m) == PROMOTION);
        [[maybe_unused]] const bool is_quiet = !is_cap && !is_promo;   // read by the trace
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
                          + cont_hist_score(ss, hist_piece(state_.board->turn(), pt), Square(to_sq(m)))
                          + pawn_hist[pt][to_sq(m)]
                          + (low_ply_hist ? (*low_ply_hist)[from_sq(m)][to_sq(m)] : 0);
        }
#endif

        // ---- Move-loop pruning ---------------------------------------------
        // Not at the root, in check (unless B3CheckPrune) or before a move
        // was searched, not while
        // the best score is a proven loss, and not for a side left with king
        // and pawns. Each test reads the depth the move's reduced search
        // would have, adjusted by the quiet move's history (`lmr_depth`).
        if (!ABLATED(5) && !is_root && (!in_check || check_prune) && searched > 0
            && best_score > -tablebaseWinInMaxPly
            && state_.board->has_non_pawn_material(state_.board->turn())) {
            DIAG_COUNT(++state_.diag.moveloop_tested);
            const Square from = Square(from_sq(m));
            const Square to   = Square(to_sq(m));
            const PieceType moved = type_of(state_.board->piece_on(from));
            const int moved_hp = hist_piece(state_.board->piece_on(from));

            // Count pruning: past this many moves the picker delivers no
            // more quiets (direct checks excepted while the switch is on).
            if (!picker.quiets_skipped()
                && move_count >= (kp.lmp_base + depth * depth * kp.lmp_square_scale / 16)
                                 / (2 - improving)) {
                picker.skip_quiets(kp.lmp_exempt_checks != 0);
                DIAG_COUNT(++state_.diag.skip_quiets_nodes);
                if (in_check) DIAG_COUNT(++state_.diag.in_check_skip_quiets);
                TRACE_DECISION(TraceEvent::SkipQuiets, ply, depth, m,
                               alpha, beta, eval, improving, correction, VALUE_NONE,
                               move_count, 0, VALUE_NONE, alpha,
                               -1, VALUE_NONE, 1, TraceFamily::SkipQuiets);
                PRICE(price_skip());
            }

            int lmr_depth = depth - 1 - r / 1024;

            if (is_cap || is_promo || move_gives_check()) {
                const PieceType captured = move_type(m) == EN_PASSANT ? PAWN
                                         : type_of(state_.board->piece_on(to));
                const int capt_hist = hist_.capture.entry[moved_hp][to][captured];
                HIST_SHADOW(hist_.shadow->read(ShadowTable::Capture,
                                               CoreHistoryTables::capture_cell(moved_hp, to, captured),
                                               state_.board->turn()));

                // Capture futility: even the captured piece and a margin
                // that grows with the reduced depth leave the static eval
                // below alpha.
                if (!move_gives_check() && lmr_depth < kp.capt_fut_depth_cap
                    && ss->eval + kp.capt_fut_base + kp.capt_fut_slope * lmr_depth
                           + PIECE_VALUE[captured] + capt_hist / kp.capt_fut_hist_div <= alpha) {
                    DIAG_COUNT(++state_.diag.capture_futility_pruned);
                    if (in_check) DIAG_COUNT(++state_.diag.in_check_capt_fut);
                    TRACE_DECISION(TraceEvent::CaptureFutilityPrune, ply, depth, m,
                                   alpha, beta, eval, improving, correction, capt_hist,
                                   move_count, 0,
                                   kp.capt_fut_base + kp.capt_fut_slope * lmr_depth, alpha,
                                   -1, lmr_depth, picker.quiets_skipped(),
                                   TraceFamily::CaptureFutility);
                    PRICE(price_move(PriceRule::CaptureFutility, m));
                    return false;
                }

                // SEE pruning of captures and checks: the exchange loses more
                // than a depth-scaled margin. A side giving up its last piece
                // may be playing for stalemate, so that is spared below 0.
                const int margin = kp.capt_see_coeff * depth + capt_hist / kp.capt_see_hist_div;
                const Bitboard pieces = state_.board->piece_bb(state_.board->turn(), KNIGHT)
                                      | state_.board->piece_bb(state_.board->turn(), BISHOP)
                                      | state_.board->piece_bb(state_.board->turn(), ROOK)
                                      | state_.board->piece_bb(state_.board->turn(), QUEEN);
                const bool last_piece = moved >= KNIGHT && moved <= QUEEN && popcount(pieces) == 1;
                if ((alpha >= 0 || !last_piece) && !state_.board->see_ge(m, -margin)) {
                    DIAG_COUNT(++state_.diag.capture_see_pruned);
                    if (in_check) DIAG_COUNT(++state_.diag.in_check_capt_see);
                    TRACE_DECISION(TraceEvent::CaptureSeePrune, ply, depth, m,
                                   alpha, beta, eval, improving, correction, capt_hist,
                                   move_count, 0, -margin, alpha,
                                   -1, lmr_depth, picker.quiets_skipped(),
                                   TraceFamily::CaptureSee);
                    PRICE(price_move(PriceRule::CaptureSee, m));
                    return false;
                }
            } else if (!is_pv || (kp.b3_pv_quiet_prune && !ss->follow_pv)) {
                // Quiets are also pruned at a PV node off the previous
                // iteration's PV, as the donor does (B3PvQuietPrune).
                // Continuation-history pruning: the two nearest continuations
                // and the pawn history rate the move this far below zero.
#ifdef BASILISK_DIAGNOSTIC
                for (int back : {1, 2})
                    hist_.shadow->read(ShadowTable::Continuation,
                                       hist_.continuation_cell((ss - back)->cont_hist, moved_hp, to),
                                       state_.board->turn());
#endif
                int history = (ss - 1)->cont_hist->entry[moved_hp][to]
                            + (ss - 2)->cont_hist->entry[moved_hp][to]
                            + pawn_hist[moved][to];
                if (history < -kp.cont_hist_prune_slope * depth) {
                    DIAG_COUNT(++state_.diag.cont_hist_pruned);
                    if (in_check) DIAG_COUNT(++state_.diag.in_check_cont_hist);
                    if (is_pv)    DIAG_COUNT(++state_.diag.pv_quiet_pruned);
                    TRACE_DECISION(TraceEvent::HistoryPrune, ply, depth, m,
                                   alpha, beta, eval, improving, correction, history,
                                   move_count, 0, -kp.cont_hist_prune_slope * depth, alpha,
                                   -1, lmr_depth, picker.quiets_skipped(),
                                   TraceFamily::ContHist);
                    PRICE(price_move(PriceRule::ContHist, m));
                    return false;
                }

                history += 69 * main_hist[from][to] / 32;
                lmr_depth += history / LMR_HISTORY_DIVISOR[std::min(depth, 16) - 1];

                // Quiet futility: the static eval and a margin that grows
                // with the reduced depth stay below alpha. The best score
                // rises to that bound, so a node whose quiets are all pruned
                // fails low softly.
                const int futility = ss->eval + kp.quiet_fut_base + kp.quiet_fut_slope * lmr_depth
                                   + kp.quiet_fut_alpha_term * (ss->eval > alpha);
                if (!in_check && lmr_depth < kp.quiet_fut_depth_cap && futility <= alpha) {
                    if (best_score <= futility && !is_decisive(best_score)
                        && futility < tablebaseWinInMaxPly)
                        best_score = futility;
                    DIAG_COUNT(state_.diag.fut_prunes++);
                    if (is_pv) DIAG_COUNT(++state_.diag.pv_quiet_pruned);
                    TRACE_DECISION(TraceEvent::FutilityPrune, ply, depth, m,
                                   alpha, beta, eval, improving, correction, history,
                                   move_count, 0, futility - ss->eval, alpha,
                                   -1, lmr_depth, picker.quiets_skipped(),
                                   TraceFamily::QuietFutility);
                    PRICE(price_move(PriceRule::QuietFutility, m));
                    return false;
                }

                // SEE pruning of quiets: the moved piece is lost for more
                // than the square of the reduced depth.
                lmr_depth = std::max(lmr_depth, 0);
                if (!state_.board->see_ge(m, -kp.quiet_see_coeff * lmr_depth * lmr_depth)) {
                    DIAG_COUNT(++state_.diag.quiet_see_pruned);
                    if (in_check) DIAG_COUNT(++state_.diag.in_check_quiet_see);
                    if (is_pv)    DIAG_COUNT(++state_.diag.pv_quiet_pruned);
                    TRACE_DECISION(TraceEvent::QuietSeePrune, ply, depth, m,
                                   alpha, beta, eval, improving, correction, history,
                                   move_count, 0, -kp.quiet_see_coeff * lmr_depth * lmr_depth,
                                   alpha, -1, lmr_depth, picker.quiets_skipped(),
                                   TraceFamily::QuietSee);
                    PRICE(price_move(PriceRule::QuietSee, m));
                    return false;
                }
            }
        }

        // ---- Extensions -------------------------------------------------------
        int extension = 0;
        // The singular extension also deepens the node by a ply for
        // the moves after this one (the donor's form); this move's own depth
        // is not raised by it.
        int depth_bump = 0;

        // ---- Singular extension, the donor's form -------------------------
        // The TT move is searched alone against a bound just below its score:
        // when every other move falls below it, the move is extended one to
        // three plies by how far they fell, within the line's budget, and the
        // node deepens; when they clear the node's beta it is a multi-cut;
        // otherwise a move the table says fails high, or one at an expected
        // cut node, is searched shallower.
        if (!ABLATED(6) && !is_root && m == tt_move && ss->excluded == MOVE_NONE
            && ply < MAX_PLY - 2 && depth >= kp.b3_sing_min_depth + int(ss->tt_pv)
            && tt_score != VALUE_NONE && !is_decisive(tt_score) && proves_lower(tt_flag)
            && tt_depth >= depth - (kp.b3_sing_tt_depth ? 2 : 3)) {
            if (b3::is_shuffling(tt_capture, state_.board->rule50_count(),
                                 state_.board->plies_since_null(), ply, m,
                                 (ss - 2)->move, (ss - 4)->move)) {
                DIAG_COUNT(++state_.diag.sing_shuffle_refused);
            } else if (b3::seek_mate(root_depth_, root_score_, kp.b3_seek_mate_depth,
                                     kp.b3_seek_mate_score)) {
                DIAG_COUNT(++state_.diag.sing_seekmate_refused);
            } else {
                const int s_beta = b3::singular_beta(tt_score, depth, ss->tt_pv && !is_pv,
                                                     kp.b3_sing_margin, kp.b3_sing_margin_ttpv);
                const int s_depth = (depth - 1) / 2;

                DIAG_COUNT(++state_.diag.sing_tries);
                ss->excluded = m;
                const int s_val = negamax<NodeType::NonPV>(s_depth, s_beta - 1, s_beta, ply, ss,
                                                           false, cut_node);
                ss->excluded = MOVE_NONE;

                if (state_.stopped) {
                    immediate_return = true;
                    immediate_score = 0;
                    return true;
                }

                if (s_val < s_beta) {
                    const int corr_adj = std::min(std::abs(correction_raw) / kp.b3_sing_corr_div,
                                                  kp.b3_sing_corr_cap);
                    const bool deep = ply > root_depth_;
                    const int double_margin = kp.b3_sing_double_base + kp.b3_sing_double_pv * is_pv
                        + kp.b3_sing_double_quiet * !tt_capture - corr_adj
                        - kp.b3_sing_tmh_mult * hist_.tt_move.value / 114178
                        + kp.b3_sing_double_deep * deep;
                    const int triple_margin = kp.b3_sing_triple_base + kp.b3_sing_triple_pv * is_pv
                        + kp.b3_sing_triple_quiet * !tt_capture
                        + kp.b3_sing_triple_ttpv * ss->tt_pv - corr_adj
                        + kp.b3_sing_triple_deep * deep;
                    const int graded = b3::graded_extension(s_val, s_beta, double_margin,
                                                            triple_margin);
                    extension = graded;
                    if (kp.b3_ext_budget) {
                        bool truncated = false;
                        extension = b3::budget_extension(graded, ss->ext_plies, ext_budget(),
                                                         truncated);
                        if (truncated) {
                            DIAG_COUNT(++state_.diag.ext_budget_bound);
                            TRACE_DECISION(TraceEvent::ExtBudgetTruncate, ply, depth, m,
                                           alpha, beta, eval, improving, correction, VALUE_NONE,
                                           searched, extension, ss->ext_plies, ext_budget());
                        }
                    }
                    ++depth;
                    depth_bump = 1;
                    TRACE_DECISION(TraceEvent::SingularExtension, ply, depth, m,
                                   alpha, beta, eval, improving, correction, trace_history,
                                   searched, extension, s_beta, s_val);
                    DIAG_COUNT(++state_.diag.sing_fired);
                    if (graded >= 2) DIAG_COUNT(++state_.diag.sing_double);
                    if (graded == 3) DIAG_COUNT(++state_.diag.sing_ext3);
                    if (did_check_ext) DIAG_COUNT(++state_.diag.sing_in_check);
                    if (graded >= 2 && did_check_ext) DIAG_COUNT(++state_.diag.sing_triple);
                    singular_quiet_lmr = kp.b3_singular_quiet_lmr && !tt_capture;
                } else if (!bare && b3::multicut_admits(kp.b3_multi_cut, s_val, s_beta, beta,
                                                        is_decisive(s_val))) {
                    // Priced before the multi-cut's training.
                    PRICE(price_node(PriceRule::MultiCut, s_val));
                    // The node fails high without its TT move too: the
                    // TT move's history learns it was not needed, and the
                    // correction learns the static eval was low.
                    if (!PRICE_PROBING())
                        TTMoveHistory::update(hist_.tt_move.value,
                                              -kp.b3_sing_mc_tmh_base
                                              - kp.b3_sing_mc_tmh_slope * depth);
                    if (!in_check && s_val > static_eval) {
                        const int bonus = std::clamp(
                            (s_val - static_eval) * 1000 / 282 * s_depth * kp.b3_sing_mc_corr / 1024,
                            -256, 256);
                        update_correction_history(ss, bonus);
                    }
                    DIAG_COUNT(++state_.diag.sing_multicut);
                    TRACE_DECISION(TraceEvent::SingularMulticut, ply, depth, m,
                                   alpha, beta, eval, improving, correction, trace_history,
                                   searched, 0, s_beta, s_val);
                    immediate_return = true;
                    immediate_score = s_val;
                    return true;
                } else if (b3::negative_extension(tt_score, beta, cut_node)) {
                    extension = -kp.b3_sing_negative;
                    DIAG_COUNT(++state_.diag.sing_ttbeta);
                    if (tt_score < beta) DIAG_COUNT(++state_.diag.sing_negative_cut);
                    TRACE_DECISION(TraceEvent::SingularNegative, ply, depth, m,
                                   alpha, beta, eval, improving, correction, trace_history,
                                   searched, extension, s_beta, s_val);
                }
            }
        }

        // The move's history score: a capture's captured value and capture
        // history, a quiet's main and nearest continuation histories.
        const PieceType moved_pt = type_of(state_.board->piece_on(from_sq(m)));
        const Square move_to = Square(to_sq(m));
        const bool capture_stage = is_cap || is_promo;
        int stat_score;
        if (capture_stage) {
            const PieceType captured = move_type(m) == EN_PASSANT ? PAWN
                                     : type_of(state_.board->piece_on(move_to));
            stat_score = 873 * HIST_PIECE_VALUE[captured] / 128
                       + hist_.capture.entry[hist_piece(state_.board->turn(), moved_pt)][move_to][captured];
            HIST_SHADOW(hist_.shadow->read(
                ShadowTable::Capture,
                CoreHistoryTables::capture_cell(hist_piece(state_.board->turn(), moved_pt), move_to, captured),
                state_.board->turn()));
        } else {
            const int moved_piece = hist_piece(state_.board->turn(), moved_pt);
            stat_score = (2252 * main_hist[from_sq(m)][move_to]
                          + 1126 * (ss - 1)->cont_hist->entry[moved_piece][move_to]
                          + 1093 * (ss - 2)->cont_hist->entry[moved_piece][move_to]) / 1024;
#ifdef BASILISK_DIAGNOSTIC
            for (int back : {1, 2})
                hist_.shadow->read(ShadowTable::Continuation,
                                   hist_.continuation_cell((ss - back)->cont_hist, moved_piece, move_to),
                                   state_.board->turn());
#endif
        }
        ss->stat_score = stat_score;

        const int64_t nodes_before_move = state_.nodes;
        do_move(ss, m);
        shared_.tt.prefetch(state_.board->position_key());
        state_.sel_depth = std::max(state_.sel_depth, ply + 1);
        ss->ext_taken = std::max(extension, 0);
        (ss + 1)->ext_plies = ss->ext_plies + ss->ext_taken;

        int new_depth = depth - 1 + extension - depth_bump;

        // ---- The move's reduction (1024ths of a ply) ------------------------
        // Less near a PV the table knows, more at expected cut and all nodes,
        // after a capture TT move and when the children keep failing high;
        // less for the TT move, with a good history and the later the move
        // comes, and when the estimate is below alpha only by a little.
        if (ss->tt_pv)
            r -= kp.lmr_tt_pv_base + kp.lmr_tt_pv_pv_node * is_pv
               + kp.lmr_tt_pv_above_alpha * (tt_score > alpha)
               + (tt_depth >= depth) * (kp.lmr_tt_pv_depth_ok + 940 * cut_node);
        r += 697;
        r -= move_count * 65;
        // The donor's slope in this engine's evaluation units (26310 x 0.282),
        // capped at the donor's largest value of the term: its correction
        // tables saturate at (15341 + 10569 + 2*12906 + 2*8761) * 1024, and
        // ours, keeping that limit, would otherwise reach 3.5 times further.
        r -= std::min(std::abs(correction_raw) / 7419, 2695);
        if (cut_node)
            r += kp.lmr_cut_node + kp.lmr_cut_node_no_tt * (tt_move == MOVE_NONE);
        if (tt_capture)
            r += kp.lmr_tt_capture;
        if ((ss + 1)->cutoff_cnt > 1)
            r += kp.lmr_cutoff_cnt + kp.lmr_cutoff_cnt2 * ((ss + 1)->cutoff_cnt > 2)
               + kp.lmr_cutoff_cnt_all * all_node;
        else if (m == tt_move)
            r -= 2179;
        r -= stat_score * kp.lmr_stat_scale / 4096;
        if (!capture_stage && !is_decisive(alpha))
            // The difference in the donor's evaluation units, under its clamp.
            r += kp.lmr_alpha_eval_scale * std::clamp((alpha - lmr_eval) * 1000 / 282, -64, 96);
        // A singular TT move relaxes its quiet siblings' reductions when
        // B3SingularQuietLmr keeps the old relaxation.
        if (!capture_stage && singular_quiet_lmr)
            r -= kp.lmr_singular_quiet;
        if (all_node)
            r += r * kp.lmr_all_node_scale / (256 * depth + 268);

        int score;
        if (searched == 0) {
            // At a non-PV node a first move whose reduction would be large is
            // searched one or two plies shallower (never at the root, nor in
            // check unless B3CheckPrune), as later moves without LMR are.
            int step = 0;
            if (!is_pv && !ABLATED(7) && !is_root && (!in_check || check_prune)) {
                const int rr = r + (tt_move == MOVE_NONE ? 1127 : 0);
                step = (rr > 5234) + (rr > 5487 && new_depth > 2);
            }
            // At a PV node the TT move is searched at least one ply deep when
            // its entry is deep or decisive, as on the re-search below.
            if (is_pv && m == tt_move
                && ((tt_score != VALUE_NONE && is_decisive(tt_score) && tt_depth > 0)
                    || tt_depth > 1))
                new_depth = std::max(new_depth, 1);
            // At a non-PV node the first move's child is the opposite type:
            // the expected refutation of an all node's move is a cut node.
            score = -negamax<PvChild>(new_depth - step, -beta, -alpha, ply + 1, ss + 1, true,
                                      !is_pv && !cut_node);
        } else {
            // Nothing is reduced at the root, nor in check unless B3CheckPrune.
            const bool reducible = !ABLATED(7) && !is_root && (!in_check || check_prune);
            if (!ABLATED(7) && !is_root) {
                DIAG_COUNT(++state_.diag.lmr_eligible);
                if (depth < 2)     DIAG_COUNT(++state_.diag.lmr_blocked_depth);
                else if (in_check && !check_prune) DIAG_COUNT(++state_.diag.lmr_blocked_in_check);
            }
            if (reducible && depth >= 2) {
                // The reduced depth: at least one ply, at most two beyond the
                // full depth (a negative reduction) unless that is switched
                // off, plus one at PV nodes.
                const int reduced = new_depth - r / 1024;
                const int ceiling = kp.lmr_negative_allowed ? new_depth + 2 : new_depth;
                const int d = std::max(1, std::min(reduced, ceiling)) + is_pv;
                if (reduced < 1) {
                    DIAG_COUNT(++state_.diag.lmr_floor_hits);
                    DIAG_COUNT(++state_.diag.lmr_clamped_high);
                }
                if (d < new_depth) {
                    DIAG_COUNT(state_.diag.lmr_applied++);
                    DIAG_COUNT(state_.diag.lmr_reduction_plies += new_depth - d);
                    if (in_check) DIAG_COUNT(++state_.diag.in_check_lmr_applied);
                } else {
                    DIAG_COUNT(++state_.diag.lmr_clamped_zero);
                    if (d > new_depth) DIAG_COUNT(++state_.diag.lmr_extended);
                }
                ss->reduction = new_depth - d;
                TRACE_DECISION(TraceEvent::LmrReduction, ply, depth, m,
                               alpha, beta, eval, improving, correction, stat_score,
                               ss->move_count, new_depth - d, r, VALUE_NONE,
                               (ss + 1)->cutoff_cnt);
                score = -negamax<NodeType::NonPV>(d, -alpha - 1, -alpha,
                                                  ply + 1, ss + 1, true, true);
                ss->reduction = 0;
#ifdef BASILISK_PRICING
                // A reduced search that failed low, searched again at the
                // move's full depth.
                if (score <= alpha && d < new_depth && !state_.stopped)
                    if (const int cell = state_.price.decide(PriceRule::LmrFailLow, depth,
                                                             node_kind, move_count);
                        cell >= 0)
                        probe_move(cell, m, new_depth, true);
#endif

                // A reduced search that fails high is searched again at full
                // depth, a ply deeper when it beat the best by a margin and a
                // ply shallower when it barely reached it.
                if (score > alpha && !state_.stopped) {
                    const bool deeper    = d < new_depth && score > best_score + kp.research_deeper;
                    const bool shallower = score < best_score + kp.research_shallower;
                    if (deeper)    DIAG_COUNT(++state_.diag.lmr_research_deeper);
                    if (shallower) DIAG_COUNT(++state_.diag.lmr_research_shallower);
                    new_depth += int(deeper) - int(shallower);
                    if (new_depth > d) {
                        DIAG_COUNT(state_.diag.lmr_researched++);
                        TRACE_DECISION(TraceEvent::LmrResearch, ply, depth, m,
                                       alpha, beta, eval, improving, correction, stat_score,
                                       ss->move_count, new_depth - d, VALUE_NONE, score,
                                       (ss + 1)->cutoff_cnt);
                        score = -negamax<NodeType::NonPV>(new_depth, -alpha - 1, -alpha,
                                                          ply + 1, ss + 1, true, !cut_node);
                    }
                    // A reduced move that failed high trains its continuations.
                    // The move is still made, so its side is the one not to move.
                    update_continuation_histories(ss, make_piece(~state_.board->turn(), moved_pt),
                                                  move_to, 1334);
                }
            } else {
                // Without LMR the move is searched at full depth, one or two
                // plies less when its reduction would have been large.
                int step = 0;
                if (reducible) {
                    const int rr = r + (tt_move == MOVE_NONE ? 1127 : 0);
                    step = (rr > 5234) + (rr > 5487 && new_depth > 2);
                }
                score = -negamax<NodeType::NonPV>(new_depth - step, -alpha - 1, -alpha,
                                                  ply + 1, ss + 1, true, !cut_node);
            }
            // At PV nodes a move that raised alpha is searched again with the
            // full window; the TT move at least one ply deep when its entry
            // is deep or decisive.
            if (is_pv && score > alpha && !state_.stopped) {
                if (m == tt_move
                    && ((tt_score != VALUE_NONE && is_decisive(tt_score) && tt_depth > 0)
                        || tt_depth > 1))
                    new_depth = std::max(new_depth, 1);
                score = -negamax<NodeType::PV>(new_depth, -beta, -alpha,
                                               ply + 1, ss + 1, true, false);
            }
        }

        undo_move(ss, m);
        ss->ext_taken = 0;

        if (state_.stopped)
            return true;

        searched++;
        const int64_t move_nodes = state_.nodes - nodes_before_move;
        if (is_root) {
            state_.root_depth_nodes += std::max<int64_t>(0, move_nodes);
        }

        // A rook or bishop promotion at the root counts as better than the
        // queen promotion on the same squares only by a clear margin. A queen
        // makes every move either piece can, so one wins only by avoiding a
        // stalemate, a difference of a draw against a win; a few points more
        // is a shallower subtree that has not yet met the reply that works
        // only against the weaker piece. Only an exact score is judged: a
        // fail high is a lower bound, and capping it would keep the window
        // from widening to the win it hides.
        if (is_root && move_type(m) == PROMOTION && score < beta) {
            if (promo_type(m) == QUEEN) {
                if (root_queen_count < 8) {
                    root_queen[root_queen_count] = m;
                    root_queen_score[root_queen_count++] = score;
                }
            } else if (rook_or_bishop_promotion(m)) {
                for (int k = 0; k < root_queen_count; ++k)
                    if (from_sq(root_queen[k]) == from_sq(m) && to_sq(root_queen[k]) == to_sq(m)
                        && score <= root_queen_score[k] + ROOT_UNDERPROMOTION_MARGIN)
                        score = std::min(score, root_queen_score[k]);
            }
        }

        // A move equal to the best, deep in the tree, sometimes replaces it
        // as though it had just exceeded alpha. Never at the root: there a
        // later move is searched with a null window at alpha, so an equal
        // score is usually a fail-low bound, and the move played must have
        // proved itself, as Stockfish's root moves must exceed alpha.
        const int inc = !is_root && score == best_score && ply + 2 >= root_depth_
                     && (int(state_.nodes) & 14) == 0
                     && std::abs(score) + 1 < tablebaseWinInMaxPly;
        if (inc)
            DIAG_COUNT(++(score == alpha ? state_.diag.tie_promotions_at_alpha
                                         : state_.diag.tie_promotions_below_alpha));
        if (score + inc > best_score) {
            if (is_root)
                state_.root_best_nodes = std::max<int64_t>(0, move_nodes);
            best_score = score;
            best_move  = m;
            if (score + inc > alpha) {
                alpha_move = m;
                cut_alpha = alpha;
                alpha = score;
                // Once a move raised alpha the rest are searched shallower
                // (never at the root, nor in check unless B3CheckPrune).
                if (score < beta && depth > 3 && depth < 12 && !is_decisive(score)
                    && !is_root && (!in_check || check_prune))
                    depth -= 3;
                // Update PV. A child at MAX_PLY returns before it sets a
                // length, and pv_len has no entry for it, so it adds none.
                state_.pv_table[ply][ply] = m;
                const int child_pv_len = ply + 1 < MAX_PLY
                    ? std::clamp(state_.pv_len[ply + 1], ply + 1, MAX_PLY) : MAX_PLY;
                for (int k = ply + 1; k < child_pv_len; k++)
                    state_.pv_table[ply][k] = state_.pv_table[ply + 1][k];
                state_.pv_len[ply] = child_pv_len;
            }
        }

        // A move that did not raise alpha is remembered for the update.
        if (m != alpha_move && move_count <= SEARCHED_LIST_CAPACITY) {
            if (is_cap || is_promo) {
                if (captures_count < SEARCHED_LIST_CAPACITY)
                    captures_searched[captures_count++] = m;
            } else if (quiets_count < SEARCHED_LIST_CAPACITY) {
                quiets_searched[quiets_count++] = m;
            }
        }

        if (alpha >= beta) {
            ss->cutoff_cnt += (extension < 2) || is_pv;
            TRACE_DECISION(TraceEvent::BetaCutoff, ply, depth, m,
                           orig_alpha, beta, eval, improving, correction, stat_score,
                           searched, ss->reduction, VALUE_NONE, score,
                           ss->cutoff_cnt);
            // Ordering quality (BAS-O03) at the point it costs something.
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
                case CoreMovePicker::Src::TT:           DIAG_COUNT(++state_.diag.cutoff_src_tt); break;
                case CoreMovePicker::Src::GoodTactical: DIAG_COUNT(++state_.diag.cutoff_src_good_tactical); break;
                case CoreMovePicker::Src::Quiet:        DIAG_COUNT(++state_.diag.cutoff_src_quiet); break;
                case CoreMovePicker::Src::BadTactical:  DIAG_COUNT(++state_.diag.cutoff_src_bad_tactical); break;
                case CoreMovePicker::Src::None:         break;
            }
            return true;
        }

        return false;
    };

    // ---- Staged move picking -----------------------------------------------
    // At the root a queen promotion is searched before the rook or bishop
    // promotion on the same squares, so the rule in search_one has its score
    // whichever the picker would deliver first.
    Move root_queens_seen[8];
    int  root_queens_seen_count = 0;
    auto root_queen_seen = [&](Move q) {
        return std::find(root_queens_seen, root_queens_seen + root_queens_seen_count, q)
            != root_queens_seen + root_queens_seen_count;
    };
    while (true) {
        Move move = picker.next();
        if (move == MOVE_NONE)
            break;
        if (is_root && move_type(move) == PROMOTION) {
            const Move queen = make_promotion(Square(from_sq(move)), Square(to_sq(move)), QUEEN);
            if (root_queen_seen(queen)) {
                if (move == queen)
                    continue;
            } else if (root_queens_seen_count < 8) {
                root_queens_seen[root_queens_seen_count++] = queen;
                if (move != queen && search_one(queen, picker.last_source()))
                    break;
            }
        }
        if (search_one(move, picker.last_source()))
            break;
    }

#ifdef BASILISK_PRICING
    if (!immediate_return && !state_.stopped)
        for (int k = 0; k < pending_count; ++k)
            if (pending[k].value > best_score)
                state_.price.changed(pending[k].cell);
#endif

    if (immediate_return)
        return immediate_score;

    if (state_.stopped)
        return (is_root && best_move != MOVE_NONE) ? best_score : 0;

    // No legal moves, or only the excluded one
    if (searched == 0) {
        if (ss->excluded != MOVE_NONE)
            DIAG_COUNT(++state_.diag.singular_lone_move);
        return no_move_score(ss->excluded != MOVE_NONE, in_check, alpha, ply);
    }

    // A fail high's value is pulled toward beta, the more the shallower.
    if (best_score >= beta && !is_decisive(best_score) && !is_decisive(cut_alpha))
        best_score = (best_score * depth + beta) / (depth + 1);

    // The histories learn from every node with a move that raised alpha,
    // fail highs and exact nodes alike; a node that failed low rewards the
    // parent's move instead.
    if (alpha_move != MOVE_NONE) {
        DIAG_COUNT(++(best_score >= beta ? state_.diag.hist_cutoff_updates
                                         : state_.diag.hist_reward_updates));
        update_all_stats(ss, alpha_move, tt_move, quiets_searched, quiets_count,
                         captures_searched, captures_count, depth, is_pv);
        if (!is_pv && !PRICE_PROBING())
            TTMoveHistory::update(hist_.tt_move.value, alpha_move == tt_move ? 918 : -747);
    } else if ((ss - 1)->move != MOVE_NONE && (ss - 1)->move != MOVE_NULL) {
        update_fail_low_parent(ss, depth, best_score);
    }

    best_score = std::min(best_score, tb_cap);


    // Correction training: never in check, under exclusion, from a decisive
    // score or from a node whose best move is a capture. The default
    // admission takes the error only where its sign agrees with the bound:
    // above the static eval exactly when a move raised alpha. It also
    // trains on decisive scores (B3CorrDecisive) and under exclusion
    // (B3CorrExcluded), as the donor does, except at an exclusion node whose
    // raw static eval is 0: that is the evaluator's verdict (a recognised
    // draw), and the value without the only move is no error of it. The
    // bonus is clamped either way.
    const bool raised_alpha = best_score > orig_alpha;
    const bool train_decisive = kp.b3_corr_decisive;
    const bool train_excluded = kp.b3_corr_excluded;
    if (!in_check && (ss->excluded == MOVE_NONE || (train_excluded && ss->raw_eval != 0))
        && static_eval != VALUE_NONE
        && (train_decisive || !is_decisive(best_score))
        && !(raised_alpha && (state_.board->piece_on(to_sq(best_move)) != NO_PIECE
                              || move_type(best_move) == EN_PASSANT))
        && (kp.corr_admission ? (best_score > static_eval) == raised_alpha
                              : (best_score >= beta || raised_alpha))) {
        const int scale = raised_alpha ? kp.corr_update_scale : kp.corr_update_scale * 3 / 2;
        const int bonus = std::clamp((best_score - static_eval) * depth * scale / 128, -256, 256);
        update_correction_history(ss, 1061 * bonus / 1024);
    }

    // A node that did not fail high under a PV-near parent is kept near the
    // PV: the parent's move is probably good, and this position joins its
    // line.
    if (best_score < beta)
        ss->tt_pv = ss->tt_pv || (ss - 1)->tt_pv;

    // Store to TT
    TTFlag flag = (best_score >= beta)    ? TT_BETA
                : (best_score > orig_alpha) ? TT_EXACT
                :                             TT_ALPHA;
    // A later MultiPV line searched the root without its best moves; its
    // result is not the root's, so it is not stored (Stockfish skips it too).
    // No move is stored after a fail low (B3FailLowStore): no move proved
    // itself, and the entry keeps the move it had. Quiescence's store follows
    // the same switch.
    const Move stored_move = (kp.b3_fail_low_store && flag == TT_ALPHA)
                           ? MOVE_NONE : best_move;
    if (ss->excluded == MOVE_NONE && !(is_root && !state_.root_excluded.empty()))
        tt_store(hash, depth, best_score, flag, stored_move, ply, tt_static_eval,
                  ss->tt_pv);

    return best_score;
}

// ---- Kernel entry points ---------------------------------------------------

void Searcher::kernel_new_game() {
    hist_.clear();
    hist_fresh_ = true;
}

void Searcher::kernel_begin_search() {
    if (hist_fresh_ && config_.limits.params.core_history_fills)
        hist_.clear(true);
    hist_fresh_ = false;
    nmp_min_ply_ = 0;
    reductions_[0] = 0;
    for (int i = 1; i < 256; ++i)
        reductions_[i] = int(config_.limits.params.lmr_table_scale / 128.0 * std::log(i));
    hist_.age(config_.limits.params.main_hist_decay);
    pawn_hist_mask_ = (Key(1) << std::clamp(config_.limits.params.pawn_hist_buckets_log2,
                                            0, PawnHistory::MAX_BUCKETS_LOG2)) - 1;
    // A default record is the sentinel: no move, no piece, no evaluation, and
    // the sentinel continuation row. The plies below the root read as such,
    // and every ply starts clean.
    for (auto& s : state_.stack) {
        s = SearchStack{};
        s.cont_hist = hist_.sentinel_row();
        s.cont_corr = hist_.sentinel_corr_row();
    }
}

int Searcher::root_search(int depth, int alpha, int beta) {
    root_delta_ = std::max(1, beta - alpha);
    root_depth_ = depth;
    root_score_ = state_.prev_root_score;
    return negamax<NodeType::Root>(depth, alpha, beta, 0, state_.stack + STACK_SENTINELS,
                                   true, false);
}
