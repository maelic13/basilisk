// The selectivity core's node kernel: quiescence, negamax
// and the helpers only they call. The driver in search_worker.cpp reaches the
// kernel through root_search(), kernel_begin_search() and kernel_new_game()
// alone. The kernel owns the per-ply stack, the reduction table and the
// history tables, and writes the driver-visible results into SearchState: the
// PV table, selective depth, root effort, node counts and diagnostics.

#include "search_worker.h"
#include "constants.h"
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

// Fail-soft throughout, in Stockfish's form: every return is the node's best
// value, which may lie outside the window, and the table stores it as the
// bound it proves -- a lower bound at a fail high, otherwise an upper bound,
// never an exact score. A PV node takes no table cutoff, so a depth-0 bound
// never stands in for its value on the principal variation.
template<NodeType NT>
int Searcher::quiescence(int alpha, int beta, int ply, int qply, SearchStack* ss,
                         bool counted) {
    constexpr bool is_pv = NT != NodeType::NonPV;
    if (!counted) {
        record_node();
        if ((state_.nodes & 2047) == 0) check_stop();
        if (state_.stopped) return 0;
        if (ply >= MAX_PLY) return evaluator_.evaluate(*state_.board);
        if (state_.board->is_draw(ply)) return 0;
    }

    bool in_check = state_.board->is_in_check();
    ss->in_check = in_check;
    DIAG_COUNT(state_.diag.qs_nodes++);

    // TT probe
    Key hash = state_.board->position_key();
    TTEntry tte{};
    bool tt_found = shared_.tt.probe_copy(hash, tte);
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
            return tt_score;
        }
    }

    if (in_check) {
        // No qsearch-depth cap for evasion nodes: a static eval of an
        // in-check position is not a valid bound and can mask mates in long
        // check chains, poisoning parent TT stores (search audit 7).
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
            int s = -quiescence<NT>(-beta, -alpha, ply + 1, qply + 1, ss + 1);
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

    // Stand-pat evaluation. The TT holds the evaluation without the halfmove
    // clock, which its key lacks; the damping is applied at this node's clock.
    int tt_eval;
    if (tt_found && tte.static_eval != INF_SCORE)
        tt_eval = tte.static_eval;
    else
        tt_eval = evaluator_.evaluate_undamped(*state_.board);
    const int raw_eval = damp_rule50(tt_eval, state_.board->rule50_count());

    int stand_pat = raw_eval;
    stand_pat += correction_value(ss) / 131072;
    stand_pat = std::clamp(stand_pat, -(tablebaseWinInMaxPly - 1), tablebaseWinInMaxPly - 1);
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
    const int correction = stand_pat - raw_eval;
#endif

    // A TT score whose bound proves it a better estimate than the evaluation
    // replaces it as the stand-pat; the stored evaluation (tt_eval) is
    // unchanged. Never a mate or tablebase score: one proven at another
    // window would leave the evaluation band as a value no capture searched.
    if (tt_found && tt_score != VALUE_NONE
        && !is_decisive(tt_score)
        && (tt_score > stand_pat ? proves_lower(tt_flag) : proves_upper(tt_flag)))
        stand_pat = tt_score;

    if (stand_pat >= beta) {
        TRACE_DECISION(TraceEvent::QsStandPatCutoff, ply, 0, MOVE_NONE,
                       alpha, beta, stand_pat, -1, correction, VALUE_NONE,
                       0, 0, 0, stand_pat);
        tt_store(hash, 0, stand_pat, TT_BETA, MOVE_NONE, ply, tt_eval, pv_hit);
        return stand_pat;
    }

    // Delta pruning: even capturing a queen with a margin to spare cannot
    // raise alpha, so that optimistic value is the node's upper bound.
    const int delta_bound = stand_pat + PIECE_VALUE[QUEEN] + 200;
    if (delta_bound < alpha) {
        TRACE_DECISION(TraceEvent::QsDeltaPrune, ply, 0, MOVE_NONE,
                       alpha, beta, stand_pat, -1, correction, VALUE_NONE,
                       0, 0, PIECE_VALUE[QUEEN] + 200, delta_bound);
        tt_store(hash, 0, delta_bound, TT_ALPHA, MOVE_NONE, ply, tt_eval, pv_hit);
        return delta_bound;
    }

    int best = stand_pat;
    if (stand_pat > alpha) alpha = stand_pat;

    if (qply >= MAX_QSEARCH_PLY) return stand_pat;

    MoveList captures;
    state_.board->gen_legal_captures(captures);

    // Score captures: MVV + cap_hist; prefer TT move
    ScoredMove* sm = state_.move_buffers[ply][0];
    int nm = 0;
    for (Move m : captures) {
        PieceType atk = type_of(state_.board->piece_on(from_sq(m)));
        PieceType cap = (move_type(m) == EN_PASSANT) ? PAWN : type_of(state_.board->piece_on(to_sq(m)));
        int score = (m == tt_move) ? 10'000'000
                  : PIECE_VALUE[cap] * 16 - PIECE_VALUE[atk] + hist_.capture.entry[atk][to_sq(m)][cap];
        sm[nm++] = {m, score};
    }

    Move best_move = MOVE_NONE;

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
        // Futility: the capture's optimistic value cannot raise alpha, and
        // stands in for the move in the node's upper bound.
        const int futility_value = stand_pat + tactical_gain + 150;
        if (!is_promo
            && futility_value <= alpha
            && !move_gives_check()) {
            TRACE_DECISION(TraceEvent::QsFutilityPrune, ply, 0, m,
                           alpha, beta, stand_pat, -1, correction, VALUE_NONE,
                           i, 0, tactical_gain + 150, alpha);
            best = std::max(best, futility_value);
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
        int s = -quiescence<NT>(-beta, -alpha, ply + 1, qply + 1, ss + 1);
        undo_move(ss, m);

        if (state_.stopped) return 0;
        if (s > best) best = s;
        if (s > alpha) {
            alpha = s;
            best_move = m;
        }
        if (s >= beta) {
            TRACE_DECISION(TraceEvent::QsBetaCutoff, ply, 0, m,
                           alpha, beta, stand_pat, -1, correction, VALUE_NONE,
                           i, 0, VALUE_NONE, s);
            tt_store(hash, 0, s, TT_BETA, m, ply, tt_eval, pv_hit);
            return s;
        }
    }

    tt_store(hash, 0, best, TT_ALPHA, best_move, ply, tt_eval, pv_hit);
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
    record_node();
    if ((state_.nodes & 2047) == 0) {
        check_stop();
    }
    if (state_.stopped) return 0;

    if (ply >= MAX_PLY) return evaluator_.evaluate(*state_.board);
    state_.pv_len[ply] = ply;

    if (!is_root && state_.board->is_draw(ply)) return 0;

    bool in_check = state_.board->is_in_check();
    ss->in_check   = in_check;
    ss->move_count = 0;
    ss->stat_score = 0;
    // The grandchildren's record starts this node's children clean: the
    // children share (ss + 1), and each that fails high counts in it.
    (ss + 2)->cutoff_cnt = 0;

    // Check extension: when the side to move is in check, extend by 1 ply.
    // Guard with ss->excluded to prevent stacking with singular extensions.
    if (in_check) DIAG_COUNT(state_.diag.in_check_nodes++);
    // The extension is unconditional: every in-check node gets a ply.
    //
    // There is no per-path cap on the accumulation: check_ext_path_cap
    // defaulted to 0 (disabled) and was the residue of a failed trial
    // (BAS-S16 REJECTED at -3.48 +/- 3.32), not an avenue still open.
    // Reducing extension at checking nodes was measured to fail our WAC floor.
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
        return quiescence<QsType>(alpha, beta, ply, 0, ss, true);
    DIAG_COUNT(state_.diag.interior_nodes++);

    // Mate distance pruning
    if (!is_root) {
        alpha = std::max(alpha, -(MATE_SCORE - ply));
        beta  = std::min(beta,   (MATE_SCORE - ply - 1));
        if (alpha >= beta) return alpha;
    }

    // The reduction the parent applied to reach this node, read once: a
    // re-search of the same move must not see it again.
    const auto& kp = config_.limits.params;
    const int prior_reduction = (ss - 1)->reduction;
    (ss - 1)->reduction = 0;

    // ---- Transposition table lookup ----------------------------------------
    Key hash     = state_.board->position_key();
    TTEntry tte{};
    bool tt_found = shared_.tt.probe_copy(hash, tte);
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
        // across the check; nothing at this node prunes on it.
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
            || (tt_flag == TT_ALPHA && tt_score < static_eval)))
        eval = tt_score;

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
    if (prior_reduction >= 2 && depth >= 2 && ss->eval + (ss - 1)->eval > 47) {
        --depth;
        DIAG_COUNT(++state_.diag.hindsight_down);
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
                    ss - 1, type_of(state_.board->piece_on(to_sq(prev))), Square(to_sq(prev)),
                    -2210);
        }

        // Near the rule-50 limit the stored score may not hold, so no cutoff.
        // Deep enough, a cutoff is checked one ply down: the TT move's child
        // entry must agree that the move fails high (or low), against
        // graph-history errors.
        bool cut = state_.board->rule50_count() < 96;
        if (cut && depth >= 7 && !is_decisive(tt_score) && tt_move_playable(tt_move)) {
            do_move(ss, tt_move);
            TTEntry next{};
            const bool next_found = shared_.tt.probe_copy(state_.board->position_key(), next);
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
        if (!ABLATED(0) && depth <= kp.razor_depth_cap
            && eval < alpha - kp.razor_margin * depth
            && (tt_move == MOVE_NONE || tt_capture) && tt_flag != TT_BETA) {
            const int q = quiescence<NodeType::NonPV>(alpha, beta, ply, 0, ss, true);
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
        if (!ABLATED(1) && !ss->tt_pv && depth <= kp.rfp_depth_cap && eval >= beta
            && (tt_move == MOVE_NONE || tt_capture)
            && beta > -tablebaseWinInMaxPly && eval < tablebaseWinInMaxPly) {
            int margin = kp.rfp_per_ply * depth
                       - kp.rfp_improving * improving
                       - kp.rfp_worsening * opponent_worsening;
            margin += std::abs(correction_raw) / 131072 * kp.rfp_corr_mult / 1024;
            if (eval - margin >= beta) {
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
                return value;
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
            shared_.tt.prefetch(state_.board->position_key());
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
                shared_.tt.prefetch(state_.board->position_key());
                // Quick check via QSearch first
                int val = -quiescence<NodeType::NonPV>(-pc_beta, -pc_beta + 1, ply + 1, 0, ss + 1);
                if (val >= pc_beta)
                    val = -negamax<NodeType::NonPV>(depth - 4, -pc_beta, -pc_beta + 1,
                                   ply + 1, ss + 1, true, true);
                undo_move(ss, m);
                if (state_.stopped) return 0;
                if (val >= pc_beta) {
                    tt_store(hash, depth - 3, pc_beta, TT_BETA, m, ply,
                              tt_static_eval, ss->tt_pv);
                    DIAG_COUNT(state_.diag.probcut_cuts++);
                    TRACE_DECISION(TraceEvent::ProbcutCutoff, ply, depth, m,
                                   alpha, beta, eval, improving, correction, VALUE_NONE,
                                   0, depth - 4, pc_beta - static_eval, pc_beta);
                    return pc_beta;
                }
            }
        }
    }

    if (!in_check)
        improving = improving || ss->eval >= beta;

    // IIR: reduce non-PV nodes when no TT move (or a stale TT entry) guides the search.
    if (!ABLATED(4) && !is_pv && depth >= kp.iir_min_depth
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
                          + cont_hist_score(ss, pt, Square(to_sq(m)))
                          + pawn_hist[pt][to_sq(m)]
                          + (low_ply_hist ? (*low_ply_hist)[from_sq(m)][to_sq(m)] : 0);
        }
#endif

        // ---- Move-loop pruning ---------------------------------------------
        // Not at the root, in check or before a move was searched, not while
        // the best score is a proven loss, and not for a side left with king
        // and pawns. Each test reads the depth the move's reduced search
        // would have, adjusted by the quiet move's history (`lmr_depth`).
        if (!ABLATED(5) && !is_root && !in_check && searched > 0
            && best_score > -tablebaseWinInMaxPly
            && state_.board->has_non_pawn_material(state_.board->turn())) {
            DIAG_COUNT(++state_.diag.moveloop_tested);
            const Square from = Square(from_sq(m));
            const Square to   = Square(to_sq(m));
            const PieceType moved = type_of(state_.board->piece_on(from));

            // Count pruning: past this many moves the picker delivers no
            // more quiets (direct checks excepted while the switch is on).
            if (!picker.quiets_skipped()
                && move_count >= (kp.lmp_base + depth * depth * kp.lmp_square_scale / 16)
                                 / (2 - improving)) {
                picker.skip_quiets(kp.lmp_exempt_checks != 0);
                DIAG_COUNT(++state_.diag.skip_quiets_nodes);
                TRACE_DECISION(TraceEvent::SkipQuiets, ply, depth, m,
                               alpha, beta, eval, improving, correction, VALUE_NONE,
                               move_count, 0, VALUE_NONE, alpha,
                               -1, VALUE_NONE, 1, TraceFamily::SkipQuiets);
            }

            int lmr_depth = depth - 1 - r / 1024;

            if (is_cap || is_promo || move_gives_check()) {
                const PieceType captured = move_type(m) == EN_PASSANT ? PAWN
                                         : type_of(state_.board->piece_on(to));
                const int capt_hist = hist_.capture.entry[moved][to][captured];

                // Capture futility: even the captured piece and a margin
                // that grows with the reduced depth leave the static eval
                // below alpha.
                if (!move_gives_check() && lmr_depth < kp.capt_fut_depth_cap
                    && ss->eval + kp.capt_fut_base + kp.capt_fut_slope * lmr_depth
                           + PIECE_VALUE[captured] + capt_hist / kp.capt_fut_hist_div <= alpha) {
                    DIAG_COUNT(++state_.diag.capture_futility_pruned);
                    TRACE_DECISION(TraceEvent::CaptureFutilityPrune, ply, depth, m,
                                   alpha, beta, eval, improving, correction, capt_hist,
                                   move_count, 0,
                                   kp.capt_fut_base + kp.capt_fut_slope * lmr_depth, alpha,
                                   -1, lmr_depth, picker.quiets_skipped(),
                                   TraceFamily::CaptureFutility);
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
                    TRACE_DECISION(TraceEvent::CaptureSeePrune, ply, depth, m,
                                   alpha, beta, eval, improving, correction, capt_hist,
                                   move_count, 0, -margin, alpha,
                                   -1, lmr_depth, picker.quiets_skipped(),
                                   TraceFamily::CaptureSee);
                    return false;
                }
            } else if (!is_pv) {
                // Continuation-history pruning: the two nearest continuations
                // and the pawn history rate the move this far below zero.
                int history = (ss - 1)->cont_hist->entry[moved][to]
                            + (ss - 2)->cont_hist->entry[moved][to]
                            + pawn_hist[moved][to];
                if (history < -kp.cont_hist_prune_slope * depth) {
                    DIAG_COUNT(++state_.diag.cont_hist_pruned);
                    TRACE_DECISION(TraceEvent::HistoryPrune, ply, depth, m,
                                   alpha, beta, eval, improving, correction, history,
                                   move_count, 0, -kp.cont_hist_prune_slope * depth, alpha,
                                   -1, lmr_depth, picker.quiets_skipped(),
                                   TraceFamily::ContHist);
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
                if (lmr_depth < kp.quiet_fut_depth_cap && futility <= alpha) {
                    if (best_score <= futility && !is_decisive(best_score)
                        && futility < tablebaseWinInMaxPly)
                        best_score = futility;
                    DIAG_COUNT(state_.diag.fut_prunes++);
                    TRACE_DECISION(TraceEvent::FutilityPrune, ply, depth, m,
                                   alpha, beta, eval, improving, correction, history,
                                   move_count, 0, futility - ss->eval, alpha,
                                   -1, lmr_depth, picker.quiets_skipped(),
                                   TraceFamily::QuietFutility);
                    return false;
                }

                // SEE pruning of quiets: the moved piece is lost for more
                // than the square of the reduced depth.
                lmr_depth = std::max(lmr_depth, 0);
                if (!state_.board->see_ge(m, -kp.quiet_see_coeff * lmr_depth * lmr_depth)) {
                    DIAG_COUNT(++state_.diag.quiet_see_pruned);
                    TRACE_DECISION(TraceEvent::QuietSeePrune, ply, depth, m,
                                   alpha, beta, eval, improving, correction, history,
                                   move_count, 0, -kp.quiet_see_coeff * lmr_depth * lmr_depth,
                                   alpha, -1, lmr_depth, picker.quiets_skipped(),
                                   TraceFamily::QuietSee);
                    return false;
                }
            }
        }

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
                // TT move is singular — extend it. Cap stacked
                // 2-ply extensions along this path so a pathological line can't
                // chain unbounded double-extensions.
                bool allow_double = !is_pv
                    && s_val < s_beta - config_.limits.params.singular_double_margin
                    && ss->double_exts < config_.limits.params.double_ext_max;
                // REFUTED: our per-node check extension composes with
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

                // Probe: count the stack, do not change it yet.
                DIAG_COUNT(++state_.diag.sing_fired);
                if (allow_double)   DIAG_COUNT(++state_.diag.sing_double);
                if (did_check_ext)  DIAG_COUNT(++state_.diag.sing_in_check);
                if (allow_double && did_check_ext) DIAG_COUNT(++state_.diag.sing_triple);

                // Relax LMR for this node's remaining moves. Suppressed
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

                // REFUTED: the reference replaces this negative
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
                       + hist_.capture.entry[moved_pt][move_to][captured];
        } else {
            stat_score = (2252 * main_hist[from_sq(m)][move_to]
                          + 1126 * (ss - 1)->cont_hist->entry[moved_pt][move_to]
                          + 1093 * (ss - 2)->cont_hist->entry[moved_pt][move_to]) / 1024;
        }
        ss->stat_score = stat_score;

        const int64_t nodes_before_move = state_.nodes;
        do_move(ss, m);
        shared_.tt.prefetch(state_.board->position_key());
        state_.sel_depth = std::max(state_.sel_depth, ply + 1);

        int new_depth = depth - 1 + extension;
        // Propagate the stacked double-extension count to the
        // child so a chain of singular double-extensions is eventually capped.
        (ss + 1)->double_exts = ss->double_exts + (extension >= 2 ? 1 : 0);

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
        // A singular TT move relaxes its quiet siblings' reductions (the
        // singular search keeps its own form until B.3).
        if (!capture_stage && singular_quiet_lmr)
            r -= kp.lmr_singular_quiet;
        if (all_node)
            r += r * kp.lmr_all_node_scale / (256 * depth + 268);

        int score;
        if (searched == 0) {
            // At a non-PV node a first move whose reduction would be large is
            // searched one or two plies shallower (never at the root or in
            // check), as later moves without LMR are.
            int step = 0;
            if (!is_pv && !ABLATED(7) && !is_root && !in_check) {
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
            // Nothing is reduced at the root or in check.
            const bool reducible = !ABLATED(7) && !is_root && !in_check;
            if (!ABLATED(7) && !is_root) {
                DIAG_COUNT(++state_.diag.lmr_eligible);
                if (depth < 2)     DIAG_COUNT(++state_.diag.lmr_blocked_depth);
                else if (in_check) DIAG_COUNT(++state_.diag.lmr_blocked_in_check);
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
                    update_continuation_histories(ss, moved_pt, move_to, 1334);
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
                // (never at the root or in check).
                if (score < beta && depth > 3 && depth < 12 && !is_decisive(score)
                    && !is_root && !in_check)
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
        if (!is_pv)
            TTMoveHistory::update(hist_.tt_move.value, alpha_move == tt_move ? 918 : -747);
    } else if ((ss - 1)->move != MOVE_NONE && (ss - 1)->move != MOVE_NULL) {
        update_fail_low_parent(ss, depth, best_score);
    }

    best_score = std::min(best_score, tb_cap);


    // Correction training: never in check, under exclusion, from a decisive
    // score or from a node whose best move is a capture. The default
    // admission takes the error only where its sign agrees with the bound:
    // above the static eval exactly when a move raised alpha.
    const bool raised_alpha = best_score > orig_alpha;
    if (!in_check && ss->excluded == MOVE_NONE && static_eval != VALUE_NONE
        && !is_decisive(best_score)
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
    if (ss->excluded == MOVE_NONE && !(is_root && !state_.root_excluded.empty()))
        tt_store(hash, depth, best_score, flag, best_move, ply, tt_static_eval,
                  ss->tt_pv);

    return best_score;
}

// ---- Kernel entry points ---------------------------------------------------

void Searcher::kernel_new_game() {
    hist_.clear();
}

void Searcher::kernel_begin_search() {
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
    return negamax<NodeType::Root>(depth, alpha, beta, 0, state_.stack + STACK_SENTINELS,
                                   true, false);
}
