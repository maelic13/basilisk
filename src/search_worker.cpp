#include "search_worker.h"
#include "constants.h"
#include "syzygy.h"
#include <algorithm>
#include <bit>
#include <format>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <ratio>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

static_assert(TranspositionTable::MATE_SCORE == MATE_SCORE
              && TranspositionTable::MAX_PLY == MAX_PLY,
              "the table's mate-score encoding uses the search's mate band");

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

// ---- Constructor -----------------------------------------------------------

Searcher::Searcher(TranspositionTable& tt,
                   std::atomic_bool& stop_flag,
                   InfoCallback info_cb,
                   std::atomic_bool* ponderhit_flag)
    : shared_{tt, stop_flag, ponderhit_flag, std::move(info_cb), {}}
    , evaluator_()
{
    // hist_ constructs cleared (history.h); clear() also resets the age counter.
    clear();
}

void Searcher::clear() {
    hist_.clear();
    history_age_counter_ = 0;
}

// ---- Time management -------------------------------------------------------

void Searcher::compute_time_limit(const SearchLimits& limits, bool ponder, Color side,
                                  int game_ply) {
    state_.soft_limit = 0.0;
    state_.hard_limit = 0.0;

    if (limits.infinite || ponder) return;
    if (limits.movetime > 0) {
        // Fixed movetime: use the full movetime as the hard limit. GUIs and
        // adjudicators tolerate ~10% over nominal, and movetime games never
        // forfeit on time the way clock play does, so subtracting overhead
        // here only costs depth for no safety gain.
        state_.hard_limit = std::max(1, limits.movetime) / 1000.0;
        return;
    }

    // Phase 5 Step 5.1: logarithmic-time-left, increment-and-ply-aware clock
    // budget, ported from Rarog's Phase 2.2 rewrite (src/time_manager.rs) so both
    // engines share the same proven formula and time-safety reserve.
    const double time     = std::max(0, (side == WHITE) ? limits.wtime : limits.btime);
    const double inc      = (side == WHITE) ? limits.winc : limits.binc;
    const double overhead = limits.overhead;

    if (time <= 0.0 && inc <= 0.0) return;

    const bool   explicit_mtg = limits.movestogo > 0;
    const double mtg = explicit_mtg ? std::min(limits.movestogo, 50) : 50.0;

    // SF: timeLeft = max(1, time + inc*(mtg-1) - overhead*(2+mtg))
    const double time_left = std::max(1.0,
        time + inc * (mtg - 1.0) - overhead * (2.0 + mtg));

    const double ply = static_cast<double>(std::max(0, game_ply));

    double opt_scale, max_scale;
    if (explicit_mtg) {
        // Explicit movestogo branch (SF timeman.cpp)
        opt_scale = std::min((0.88 + ply / 116.4) / mtg, 0.88 * time / time_left);
        max_scale = 1.3 + 0.11 * mtg;
    } else {
        // Sudden death / increment branch (SF timeman.cpp)
        const double log_t     = std::log10(std::max(time_left / 1000.0, 1e-9));
        const double opt_const = std::min(0.0029869 + 0.00033554 * log_t, 0.004905);
        const double max_const = std::max(3.3744 + 3.0608 * log_t, 3.1441);
        opt_scale = std::min(0.012112 + std::pow(std::max(ply + 3.22713, 0.0), 0.46866) * opt_const,
                             0.19404 * time / time_left);
        max_scale = std::min(6.873, max_const + ply / 12.352);
    }

    double optimum_ms = std::max(opt_scale * time_left, 1.0);
    // SF: maximum = max(optimum, min(0.8097*time - overhead, maxScale*optimum))
    double maximum_ms = std::max(
        std::min(0.8097 * time - overhead, max_scale * optimum_ms),
        optimum_ms);

    // Step 5.8: overall SPSA-tunable budget multipliers (default ×1.00 -> no-op).
    optimum_ms *= limits.params.tm_opt_mult / 100.0;
    maximum_ms *= limits.params.tm_max_mult / 100.0;

    // Time-safety reserve (Step 2.9.1, matched here to Rarog's 2.9.1 fix). The
    // SF maximum above leaves only ~19% of the clock plus one move overhead
    // unused; at low remaining time that slack is just a few ms. check_stop()
    // polls every 2048 nodes, but the wall time the GUI actually charges also
    // includes the latency before our clock starts (command-queue dispatch,
    // Syzygy root probing, thread-pool setup) and the latency for bestmove to
    // reach the GUI — none of which `overhead` alone covers. Reserve an
    // absolute 2x move-overhead margin on top of the raw clock value; this
    // only binds in a genuine low-time scramble and leaves normal allocation
    // (the Elo from the SF-style formula above) untouched.
    // 9.4(c): the reserve above assumes the clock poll lands promptly. It does
    // at Threads=1, where our two poll sites fire every 2048 nodes — under a
    // millisecond. Under multi-thread scheduler contention that same interval
    // stretches to tens of milliseconds, and 2 x overhead is only ~20 ms at the
    // default Move Overhead of 10, so the hard cap can be overrun by the
    // difference. This is the exact configuration in which Rarog measured 10
    // time forfeits in 240 games at Threads=4; their fix — an extra flat 30 ms
    // once Threads > 1 — took it to 0 in 103. `timeBeginPeriod(1)` was measured
    // NOT to be the fix, so this is not a timer-resolution problem.
    //
    // Checked against our own numbers rather than transcribed: 2 x 10 ms = 20 ms
    // of reserve against a poll that can stretch to 50-100 ms leaves 30-80 ms of
    // exposure; +30 ms covers the common case and cuts the tail. It binds only
    // in a genuine low-time scramble.
    //
    // Threads=1 is BYTE-IDENTICAL: the term is gated on thread_count > 1, so no
    // single-thread game's budget moves by a microsecond.
    const double smp_reserve  = (config_.thread_count > 1) ? 30.0 : 0.0;
    const double reserve      = 2.0 * overhead + smp_reserve;
    const double hard_ceiling = std::max(time - reserve, 1.0);
    maximum_ms = std::min(maximum_ms, hard_ceiling);
    optimum_ms = std::min(optimum_ms, maximum_ms);

    state_.soft_limit = optimum_ms / 1000.0;
    state_.hard_limit = maximum_ms / 1000.0;
}

double Searcher::elapsed_seconds() const {
    using namespace std::chrono;
    return duration<double>(steady_clock::now() - state_.start_time).count();
}

// 9.3(b): publish the shared node count in batches instead of one atomic
// fetch_add per node per thread on a single cache line. The batch is a power of
// two so the test is a mask, and the node LIMIT is now granular to the batch —
// accepted, documented, and what every engine that does this accepts.
// Single-thread searches leave `shared_nodes` null and never enter this path.
static constexpr int64_t sharedNodeBatch = 1024;
static_assert((sharedNodeBatch & (sharedNodeBatch - 1)) == 0,
              "sharedNodeBatch must be a power of two (the flush test is a mask)");

void Searcher::tt_store(Key key, int depth, int score, TTFlag flag, Move m,
                        int ply, int static_eval) {
    DIAG_COUNT(++state_.diag.tt_stores);
    if (shared_.tt.store(key, depth, score, flag, m, ply, static_eval))
        DIAG_COUNT(++state_.diag.tt_stores_same_key);
}

void Searcher::flush_shared_nodes() {
    if (!shared_.pool.nodes)
        return;
    const int64_t pending = state_.nodes - state_.shared_nodes_flushed;
    if (pending <= 0)
        return;
    state_.shared_nodes_flushed = state_.nodes;
    state_.shared_nodes_total =
        shared_.pool.nodes->fetch_add(pending, std::memory_order_relaxed) + pending;
}

int64_t Searcher::record_node() {
    ++state_.nodes;
    if (shared_.pool.nodes) {
        if ((state_.nodes & (sharedNodeBatch - 1)) == 0)
            flush_shared_nodes();
        // The node LIMIT is still checked every node, against the last
        // published pool total plus this thread's unpublished nodes. That
        // estimate is a lower bound on the true total (other threads hold
        // unpublished nodes of their own) and is exact immediately after a
        // flush — so `go nodes N` keeps roughly its old accuracy, which
        // checking only at batch boundaries would NOT: a limit smaller than
        // one batch would have been missed entirely and overshot ~10x.
        // The comparison is local arithmetic; the atomic is what got batched.
        const int64_t estimate = state_.shared_nodes_total + (state_.nodes - state_.shared_nodes_flushed);
        if (state_.nodes_limit > 0 && estimate >= state_.nodes_limit)
            state_.stopped = true;
        return estimate;
    }
    if (state_.nodes_limit > 0 && state_.nodes >= state_.nodes_limit)
        state_.stopped = true;
    return state_.nodes;
}

void Searcher::record_tbhit(int64_t count) {
    if (count <= 0)
        return;
    state_.tb_hits += count;
    if (shared_.pool.tbhits)
        shared_.pool.tbhits->fetch_add(count, std::memory_order_relaxed);
}

int64_t Searcher::current_nodes() const {
    return shared_.pool.nodes
        ? shared_.pool.nodes->load(std::memory_order_relaxed)
        : state_.nodes;
}

int64_t Searcher::current_tbhits() const {
    return shared_.pool.tbhits
        ? shared_.pool.tbhits->load(std::memory_order_relaxed)
        : state_.tb_hits;
}

bool Searcher::check_stop() {
    if (state_.stopped) return true;

    if (shared_.stop.load(std::memory_order_acquire)) {
        state_.stopped = true;
        return true;
    }

    if (state_.pondering && shared_.ponderhit && shared_.ponderhit->load(std::memory_order_acquire)) {
        state_.pondering = false;
        const int game_ply = state_.board
            ? 2 * (state_.board->fullmove() - 1) + (state_.board->turn() == BLACK ? 1 : 0)
            : 0;
        compute_time_limit(config_.limits, false, state_.root_side, game_ply);
        if (state_.soft_limit > 0.0 && elapsed_seconds() >= state_.soft_limit) {
            state_.stopped = true;
            return true;
        }
    }

    if (state_.hard_limit > 0.0 && elapsed_seconds() >= state_.hard_limit) {
        state_.stopped = true;
        return true;
    }
    if (state_.nodes_limit > 0 && current_nodes() >= state_.nodes_limit) {
        state_.stopped = true;
        return true;
    }
    return false;
}

// ---- Move loop bookkeeping ----------------------------------------------

static constexpr int MAX_TRACKED_QUIETS = 64;
static constexpr int MAX_TRACKED_BAD_CAPS = 32;

// ---- UCI info ---------------------------------------------------------------

// 9.3(c): the pool section. Printed by thread 0 AFTER the join (main's own
// search — and its per-thread diag lines — finish before the helpers do), so
// this appends rather than replacing anything. Emitted only at Threads>1.
void Searcher::print_pool_diag(const std::vector<std::unique_ptr<Searcher>>& pool,
                               int thread_count,
                               const std::vector<int>& completed_depths) const {
    if (!shared_.info || !config_.limits.diag || thread_count <= 1) return;

    DiagCounters total;
    const int counted = std::min<int>(thread_count, static_cast<int>(pool.size()));
    for (int i = 0; i < counted; ++i)
        total.add(pool[static_cast<size_t>(i)]->state_.diag);
    ::print_pool_diag(state_.diag, total, thread_count, completed_depths, shared_.info);
}

void Searcher::send_info(int depth, int multipv, int score, const std::vector<Move>& line,
                         int64_t total_nodes, double elapsed) const {
    std::vector<Move> pv_moves = legal_line(*state_.board, line);

    // Stockfish-style tablebase PV extension. With no clock or movetime it
    // runs on every line, unbounded, as in Stockfish's analysis mode. Under
    // time control only the final line is extended (Engine), once per move and
    // within half the Move Overhead, because Basilisk prints a line at every
    // depth. A line that ends in a fifty-move draw shows the draw.
    if (!pv_moves.empty() && is_tablebase_decisive(score) && Syzygy::enabled()
        && config_.limits.movetime <= 0 && config_.limits.wtime <= 0
        && config_.limits.btime <= 0) {
        Syzygy::PvExtension ext = Syzygy::extend_pv(
            *state_.board, pv_moves, config_.limits.syzygy_50_move_rule,
            config_.limits.syzygy_probe_limit, [] { return false; });
        pv_moves = std::move(ext.pv);
        if (ext.ends_in_draw)
            score = 0;
    }

    if (shared_.info)
        shared_.info(format_info_line(depth, state_.sel_depth, multipv, score, {}, total_nodes, elapsed,
                                  current_tbhits(), shared_.tt.hashfull(), pv_moves));
}

void Searcher::init_root_tablebase_scores(const Board& board) {
    (void) board;
    state_.root_tb_moves = config_.limits.syzygy_root_moves;
    state_.tb_probe_in_search = state_.root_tb_moves.empty()
        || (!state_.root_tb_moves.front().used_dtz && state_.root_tb_moves.front().score > 0);
    if (!state_.root_tb_moves.empty() && config_.thread_id == 0)
        record_tbhit(static_cast<int64_t>(state_.root_tb_moves.size()));
}

int Searcher::root_tablebase_score(Move move) const {
    for (const auto& entry : state_.root_tb_moves) {
        if (entry.bestmove == move)
            return entry.score;
    }
    return VALUE_NONE;
}

int Searcher::root_tablebase_display(Move move) const {
    for (const auto& entry : state_.root_tb_moves) {
        if (entry.bestmove == move)
            return entry.display;
    }
    return VALUE_NONE;
}

bool Searcher::root_tablebase_allows(Move move) const {
    if (state_.root_tb_moves.empty())
        return true;
    return root_tablebase_score(move) != VALUE_NONE;
}

Move Searcher::ponder_from_tt(const Board& root, Move bestmove) const {
    if (!is_legal_move_on_board(root, bestmove))
        return MOVE_NONE;

    Board child = root;
    child.make_move(bestmove);

    TTEntry entry{};
    if (!shared_.tt.probe_copy(child.position_key(), entry))
        return MOVE_NONE;

    const Move ponder = move_from_tt(entry.move16);
    return is_legal_move_on_board(child, ponder) ? ponder : MOVE_NONE;
}

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

// ---- Iterative deepening ---------------------------------------------------

SearchResult Searcher::search(Board board, const SearchLimits& limits) {
    SearchConfig config;
    config.limits = limits;
    return search(std::move(board), std::move(config), PoolResources{});
}

SearchResult Searcher::search(Board board, SearchConfig config, const PoolResources& pool) {
    config.root_filter_count = std::max(1, config.root_filter_count);
    if (config.root_filter_index < 0 || config.root_filter_index >= config.root_filter_count)
        config.root_filter_index = -1;
    config.thread_id = std::max(0, config.thread_id);
    config_ = std::move(config);
    shared_.pool = pool;
    const SearchLimits& limits = config_.limits;

    state_.board        = &board;
    state_.nodes        = 0;
    state_.tb_hits      = 0;
    state_.nodes_limit  = limits.nodes;
    state_.shared_nodes_flushed = 0;   // 9.3(b): per-search batching state
    state_.shared_nodes_total   = 0;
    state_.sel_depth    = 0;
    state_.stopped      = false;
    state_.pondering    = limits.ponder;
    state_.root_side    = board.turn();
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
    state_.trace.start(limits.decision_trace && limits.diag && shared_.info
                       && config_.thread_count == 1 && config_.thread_id == 0
                       && limits.root_moves.size() == 1);
    if (limits.decision_trace && shared_.info && !state_.trace.enabled())
        shared_.info("info string trace error requires Diag=true Threads=1 and exactly one searchmoves root");
#endif

    init_lmr(static_cast<float>(config_.limits.params.lmr_base)    / 100.0f,
             static_cast<float>(config_.limits.params.lmr_divisor) / 100.0f);

    // Step 5.4: start the clock at the `go`-receipt instant (captured in
    // UciProtocol::cmdGo and threaded via SearchLimits.go_recv_time), not here at
    // the worker's search entry. This makes elapsed_seconds() account for the
    // command-dispatch + thread-handoff latency the GUI already charges (5.3
    // measured up to ~20 ms at bullet under load) instead of giving it away,
    // tightening the engine against the GUI clock. Falls back to now() for
    // internal/bench calls where go_recv_time is unset (so bench is unaffected).
    state_.start_time = (limits.go_recv_time.time_since_epoch().count() != 0)
                ? limits.go_recv_time
                : std::chrono::steady_clock::now();
    const int game_ply = 2 * (board.fullmove() - 1) + (board.turn() == BLACK ? 1 : 0);
    compute_time_limit(limits, limits.ponder, board.turn(), game_ply);

    if (config_.update_tt_age)
        shared_.tt.new_search();
    if (++history_age_counter_ >= 2) {
        age_history();
        history_age_counter_ = 0;
    }

    // Initialize search stack sentinels
    for (auto& s : state_.stack) s = SearchStack{};
    for (int i = 0; i < 4; i++) {
        state_.stack[i].move        = MOVE_NONE;
        state_.stack[i].moved_piece = NO_PIECE_TYPE;
        state_.stack[i].eval        = VALUE_NONE;
    }
    SearchStack* ss = state_.stack + 4; // root at offset 4

    std::memset(state_.pv_len, 0, sizeof(state_.pv_len));
    init_root_tablebase_scores(board);
    state_.root_ordering = RootOrdering{&state_.root_tb_moves, shared_.pool.root_table};

    SearchResult result;
    int prev_score      = 0;
    Move prev_best      = MOVE_NONE;
    int  best_stability = 0;   // how many consecutive depths best move hasn't changed
    double best_move_changes = 0.0;  // 8.5.12: decaying count of root best-move flips

    int max_depth = limits.infinite ? MAX_SEARCH_DEPTH
                  : std::min(limits.depth, MAX_SEARCH_DEPTH);

    // A root with no legal move is decided: report it once, as Stockfish does,
    // and search nothing. `bestmove` (0000) still waits for `stop` under
    // `infinite` or `ponder`; that is the caller's.
    {
        MoveList root_legal;
        board.gen_legal(root_legal);
        if (root_legal.size() == 0) {
            const bool mated = board.is_in_check();
            result.score = mated ? -MATE_SCORE : 0;
            if (shared_.info)
                shared_.info(mated ? "info depth 0 score mate 0" : "info depth 0 score cp 0");
            max_depth = 0;
        }
    }

    // MultiPV: the main thread reports up to `multipv` lines, never more than
    // the root moves the search may play; helpers search one.
    state_.multipv_lines = 1;
    state_.root_excluded.clear();
    if (limits.multipv > 1 && config_.thread_id == 0) {
        MoveList root_legal;
        board.gen_legal(root_legal);
        int allowed = 0;
        for (Move m : root_legal)
            if (move_in_root_moves(m, limits.root_moves) && root_tablebase_allows(m))
                ++allowed;
        state_.multipv_lines = std::clamp(allowed, 1, limits.multipv);
    }

    int start_depth = 1;
    state_.diag.reset();         // fresh diagnostic counters per `go` (8.6.6)
    evaluator_.diag_lazy = config_.limits.diag;
    evaluator_.lazy_fires = evaluator_.lazy_sign_flips = 0;
    evaluator_.lazy_margin_crossings = 0;
    evaluator_.lazy_absdelta_sum = evaluator_.lazy_absdelta_max = 0;
    // 8.7.1(c) speed telemetry: fresh per `go`. The Board counters are reset
    // through state_.board because the Board arrived by value from a caller
    // whose own counters may be stale.
    evaluator_.eval_calls = 0;
    evaluator_.pawn_probes = evaluator_.pawn_hits = 0;
#ifdef BASILISK_TUNE
    evaluator_.diag_endgames = config_.limits.diag;
    evaluator_.endgame_occurrence.reset();
#endif
    if (state_.board)
        state_.board->reset_diag_counters();

    if (config_.thread_id > 0 && max_depth > 2)
        start_depth = 1 + (config_.thread_id % 2);

    for (int depth = start_depth; depth <= max_depth && !state_.stopped; depth++) {
        state_.pv_len[0] = 0;
        state_.sel_depth = 0;
        state_.root_depth_nodes = 0;
        state_.root_best_nodes = 0;
        state_.root_best_effort = 0;
        int score;

        if (depth <= 3 || std::abs(prev_score) >= MATE_SCORE - MAX_PLY) {
            score = negamax<NodeType::Root>(depth, -INF_SCORE, INF_SCORE, 0, ss, true, false);
        } else {
            int delta = config_.limits.params.aspiration_delta;
            int asp_a = prev_score - delta;
            int asp_b = prev_score + delta;
            // A long iteration whose window fails reports the bound it proved,
            // with the line that failed high, or the previous line on a fail low.
            auto bound_line = [&](int bound_score, bool lower) {
                if (!shared_.info || elapsed_seconds() <= kBoundLineAfterSeconds)
                    return;
                std::vector<Move> line = result.pv;
                if (lower && state_.pv_len[0] > 0)
                    line.assign(state_.pv_table[0], state_.pv_table[0] + std::clamp(state_.pv_len[0], 0, MAX_PLY));
                shared_.info(format_info_line(depth, state_.sel_depth, 1, bound_score,
                                          lower ? "lowerbound" : "upperbound", current_nodes(),
                                          elapsed_seconds(), current_tbhits(), shared_.tt.hashfull(),
                                          legal_line(*state_.board, line)));
            };
            DIAG_COUNT(++state_.diag.asp_windows);
            while (true) {
                // 5.8.5 REFUTED: the reference re-searches SHALLOWER after each
                // fail-high (failedHighCnt). Measured: WAC 137 -> 119 against a
                // floor of 130, and re-searches ROSE 1305 -> 1450. A root
                // failing high is often a tactical shot, and searching it
                // shallower misses it. Full depth every time. (BAS-D17)
                score = negamax<NodeType::Root>(depth, asp_a, asp_b, 0, ss, true, false);
                if (state_.stopped) break;
                if (score <= asp_a) {
                    bound_line(score, false);
                    DIAG_COUNT(++state_.diag.asp_fail_low);
                    DIAG_COUNT(++state_.diag.asp_researches);
                    // 5.8.3 REFUTED: the reference also pulls beta to the
                    // window midpoint here, reasoning that a fail-low proves the
                    // standing beta far too generous. Measured, it makes things
                    // WORSE in the way that matters: re-searches ROSE 1305 ->
                    // 1342, because a tighter window simply fails again, and the
                    // depth split was 18 better / 19 worse -- no direction.
                    // 5.8.4 REFUTED with it: the reference's slower delta growth
                    // (delta/4 + 5 against our delta/2) measured -0.243 ply,
                    // 20 better / 33 worse. Ours escalates faster and that is
                    // the better trade here. (BAS-D16)
                    asp_a  = std::max(score - delta, -INF_SCORE);
                    delta += delta / 2;
                } else if (score >= asp_b) {
                    bound_line(score, true);
                    DIAG_COUNT(++state_.diag.asp_fail_high);
                    // Past the optimum, a fail-high on the move the last
                    // iteration chose only proves that move better than the
                    // window expected: play it rather than spend the rest of
                    // the budget measuring by how much. A rising score in a won
                    // ending otherwise re-searches at full depth until the hard
                    // maximum. The iteration is discarded as on any stop, so
                    // the move played is that iteration's, and it is this one.
                    if (state_.soft_limit > 0.0 && !state_.pondering && config_.thread_id == 0
                        && state_.pv_len[0] > 0 && state_.pv_table[0][0] == result.bestmove
                        && elapsed_seconds() >= state_.soft_limit) {
                        state_.stopped = true;
                        break;
                    }
                    DIAG_COUNT(++state_.diag.asp_researches);
                    asp_b  = std::min(score + delta, INF_SCORE);
                    delta += delta / 2;
                } else {
                    break;
                }
                if (delta >= 900) {
                    DIAG_COUNT(++state_.diag.asp_giveup);
                    DIAG_COUNT(++state_.diag.asp_researches);
                    asp_a = -INF_SCORE;
                    asp_b =  INF_SCORE;
                    score = negamax<NodeType::Root>(depth, asp_a, asp_b, 0, ss, true, false);
                    break;
                }
            }
        }

        if (state_.stopped && depth > 1) break;

        if (state_.root_depth_nodes > 0)
            state_.root_best_effort = static_cast<int>(
                std::min<int64_t>(100, state_.root_best_nodes * 100 / state_.root_depth_nodes));

        // Track best-move stability for adaptive soft time limit
        Move cur_best = (state_.pv_len[0] > 0) ? state_.pv_table[0][0] : MOVE_NONE;

        int reported_score = score;
        if (cur_best != MOVE_NONE) {
            const int tb_score = root_tablebase_display(cur_best);
            if (tb_score != VALUE_NONE)
                reported_score = tb_score;
        }

        int prev_score_saved = prev_score;
        prev_score = score;
        // 8.5.12: decaying best-move-change signal (SF's totBestMoveChanges).
        // Decays each iteration; a flip adds 1. Used to EXTEND time when the root
        // best move is thrashing -- complementary to stability_scale, which only
        // shrinks time when the move is stable.
        best_move_changes *= 0.5;
        if (cur_best == prev_best)
            best_stability++;
        else {
            best_stability = 0;
            prev_best      = cur_best;
            if (depth > 1)
                best_move_changes += 1.0;
        }

        if (state_.pv_len[0] > 0) {
            result.bestmove   = state_.pv_table[0][0];
            result.pondermove = (state_.pv_len[0] > 1) ? state_.pv_table[0][1] : MOVE_NONE;
            result.pv.assign(state_.pv_table[0], state_.pv_table[0] + std::clamp(state_.pv_len[0], 0, MAX_PLY));
            if (result.pondermove == MOVE_NONE)
                result.pondermove = ponder_from_tt(board, result.bestmove);
        }
        result.score = reported_score;
        result.depth = depth;
        result.seldepth = state_.sel_depth;

        // 5.8.6: the table is given the RAW `score`, not the tablebase-
        // corrected `reported_score` that goes out over UCI. That is deliberate
        // and not a bug: the table's scores exist to ORDER root moves for the
        // next iteration and for helper threads, and a TB-corrected value is a
        // fixed mate/draw verdict that carries no ordering information. Stating
        // it here because the asymmetry two lines apart reads as an oversight.
        if (shared_.pool.root_table && result.bestmove != MOVE_NONE)
            shared_.pool.root_table->update(result.bestmove, result.pondermove, depth, score);

        // The first line's own PV, before any later line overwrites the table.
        const std::vector<Move> first_line(state_.pv_table[0], state_.pv_table[0] + std::clamp(state_.pv_len[0], 0, MAX_PLY));

        // MultiPV lines 2..N: each is the best of the root moves the earlier
        // lines did not play, searched at full width. A line a stop cuts short
        // is not reported; its previous depth stands.
        struct ExtraLine { int score; std::vector<Move> pv; };
        std::vector<ExtraLine> extra_lines;
        if (state_.multipv_lines > 1 && cur_best != MOVE_NONE && !state_.stopped) {
            state_.root_excluded.assign(1, cur_best);
            for (int k = 2; k <= state_.multipv_lines; ++k) {
                state_.pv_len[0] = 0;
                const int line_score = negamax<NodeType::Root>(depth, -INF_SCORE, INF_SCORE, 0, ss, true, false);
                if (state_.stopped || state_.pv_len[0] == 0)
                    break;
                extra_lines.push_back({line_score, std::vector<Move>(
                    state_.pv_table[0], state_.pv_table[0] + std::clamp(state_.pv_len[0], 0, MAX_PLY))});
                state_.root_excluded.push_back(state_.pv_table[0][0]);
            }
            state_.root_excluded.clear();
        }

        double elapsed = elapsed_seconds();
        if (extra_lines.empty()) {
            send_info(depth, 1, reported_score, first_line, current_nodes(), elapsed);
        } else {
            // Report the lines best first, as Stockfish sorts its root moves, so
            // `bestmove` is always line 1: a later line searched without the
            // first can score above it.
            std::vector<ExtraLine> lines;
            lines.push_back({reported_score, first_line});
            for (ExtraLine& line : extra_lines) {
                const int tb_display = root_tablebase_display(line.pv.front());
                lines.push_back({tb_display != VALUE_NONE ? tb_display : line.score,
                                 std::move(line.pv)});
            }
            std::stable_sort(lines.begin(), lines.end(),
                             [](const ExtraLine& a, const ExtraLine& b) { return a.score > b.score; });
            if (lines.front().pv.front() != result.bestmove) {
                result.bestmove   = lines.front().pv.front();
                result.pv         = lines.front().pv;
                result.pondermove = result.pv.size() > 1 ? result.pv[1]
                                  : ponder_from_tt(board, result.bestmove);
                result.score      = lines.front().score;
            }
            for (size_t k = 0; k < lines.size(); ++k)
                send_info(depth, static_cast<int>(k) + 1, lines[k].score, lines[k].pv,
                          current_nodes(), elapsed);
        }

        // Adaptive soft time limit:
        // The more stable the best move, the less time we need to confirm it.
        // stability=0 → 100% of soft, stability=6+ → ~64% of soft
        // A significant score drop signals instability — extend time budget.
        // 9.4(a): ONLY the main thread owns a clock. Helpers run until `shared_.stop`,
        // which the pool sets when main returns. Keep this gate outside the
        // whole time-management calculation so the restored 9.4 baseline is
        // source- and code-shape-identical in this path.
        if (state_.soft_limit > 0.0 && !state_.pondering && config_.thread_id == 0) {
            // Step 5.8: the scaling constants below are SPSA-tunable
            // (config_.limits.params, defaults == the baked values).
            const SearchParams& tp = config_.limits.params;
            double stability_scale = 1.0 - (tp.tm_stability / 1000.0) * std::min(best_stability, 6);
            // Score-based time extension: if score dropped enough, take more time
            int score_drop = prev_score_saved - score;
            double score_scale = (depth > 4 && score_drop > tp.tm_scoredrop_thr)
                               ? 1.0 + std::min(score_drop - tp.tm_scoredrop_thr, 120)
                                       / static_cast<double>(tp.tm_scoredrop_div)
                               : 1.0;
            double effort_scale = (depth > 5 && state_.root_best_effort >= tp.tm_effort_hi) ? tp.tm_effort_hi_mult / 100.0
                                : (depth > 5 && state_.root_best_effort <= tp.tm_effort_lo) ? tp.tm_effort_lo_mult / 100.0
                                : 1.0;
            // 8.5.12: instability extension — a thrashing root best move raises
            // the threshold (buys more time), complementing stability_scale.
            double instability_scale = 1.0 + std::min(best_move_changes, 2.0) * (tp.tm_instability / 100.0);
            if (elapsed >= state_.soft_limit * stability_scale * score_scale
                         * effort_scale * instability_scale)
                break;
        }

        // Do not stop at the first forced mate. A shallow iteration can find a
        // longer checking mate before a deeper iteration sees a shorter quiet
        // mating net. Only mate-in-1 is impossible to improve.
        if (limits.mate > 0 && std::abs(score) >= MATE_SCORE - MAX_PLY) {
            const int mate_in = (MATE_SCORE - std::abs(score) + 1) / 2;
            if (mate_in > 0 && mate_in <= limits.mate)
                break;
        }
        if (score >= MATE_SCORE - 1)
            break;
    }

    result = sanitize_search_result(board, result);
    // 9.3(b): publish whatever this thread accumulated since the last batch
    // boundary, so the shared total is exact once the pool joins.
    flush_shared_nodes();
    // 8.7.1(c): harvest the Board-side speed counters BEFORE state_.board is
    // dropped — print_search_diag() runs after this point.
    state_.diag.see_ge_calls      = board.see_ge_call_count();
    state_.diag.gives_check_calls = board.gives_check_call_count();
    state_.board = nullptr;
    shared_.pool = PoolResources{};
    state_.root_ordering = RootOrdering{};
    state_.pondering = false;
    state_.root_tb_moves.clear();
    result.nodes      = state_.nodes;
    result.tbhits     = state_.tb_hits;
    result.elapsed_ms = int64_t(elapsed_seconds() * 1000.0);

    // Step 5.3 diagnostic: one line per move with the time budget, the actual
    // elapsed, and the go-receipt -> search-start dispatch latency the GUI
    // charges but elapsed_seconds() (clock starts at state_.start_time) does not yet
    // count. Emitted only on the reporting thread (shared_.info set) and only with
    // the hidden TM_Debug option on, so play/bench are unaffected when off.
    if (shared_.info && config_.limits.diag)
        print_search_diag(state_.diag, evaluator_, shared_.info);
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
    if (config_.limits.decision_trace)
        state_.trace.print(shared_.info, config_.limits.root_moves);
#endif

    if (shared_.info && config_.limits.tm_debug) {
        long long dispatch_ms = -1;
        if (config_.limits.go_recv_time.time_since_epoch().count() != 0)
            dispatch_ms = int64_t(std::chrono::duration<double, std::milli>(
                              state_.start_time - config_.limits.go_recv_time).count());
        shared_.info("info string tm soft_ms=" + std::to_string(int64_t(state_.soft_limit * 1000.0))
               + " hard_ms="     + std::to_string(int64_t(state_.hard_limit * 1000.0))
               + " elapsed_ms="  + std::to_string(int64_t(elapsed_seconds() * 1000.0))
               + " dispatch_ms=" + std::to_string(dispatch_ms));
    }
    return result;
}

