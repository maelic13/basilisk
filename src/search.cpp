#include "search.h"
#include "constants.h"
#include "syzygy.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <utility>

// ---- LMR table -------------------------------------------------------------

static constexpr int TB_WIN_SCORE = tablebaseWinScore;

static int score_from_syzygy_wdl(Syzygy::Wdl wdl) {
    switch (wdl) {
        case Syzygy::Wdl::Win:
            return TB_WIN_SCORE;
        case Syzygy::Wdl::CursedWin:
            return 2;
        case Syzygy::Wdl::Draw:
            return 0;
        case Syzygy::Wdl::BlessedLoss:
            return -2;
        case Syzygy::Wdl::Loss:
            return -TB_WIN_SCORE;
    }
    return 0;
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

// ---- Shared root move table ------------------------------------------------

static bool move_in_root_moves(Move move, const std::vector<Move>& root_moves) {
    return root_moves.empty()
        || std::find(root_moves.begin(), root_moves.end(), move) != root_moves.end();
}

static bool move_in_syzygy_root_moves(Move move,
                                      const std::vector<Syzygy::RootMoveInfo>& root_moves) {
    if (root_moves.empty())
        return true;
    for (const auto& entry : root_moves)
        if (entry.bestmove == move)
            return true;
    return false;
}

void RootMoveTable::reset(const Board& board,
                          const std::vector<Move>& root_moves,
                          const std::vector<Syzygy::RootMoveInfo>& syzygy_root_moves) {
    MoveList legal;
    board.gen_legal(legal);

    std::scoped_lock lock(mutex_);
    entries_.clear();
    entries_.reserve(static_cast<size_t>(legal.size()));
    sequence_ = 0;

    for (Move move : legal) {
        if (!move_in_root_moves(move, root_moves)
            || !move_in_syzygy_root_moves(move, syzygy_root_moves)) {
            continue;
        }
        Entry entry;
        entry.bestmove = move;
        entries_.push_back(entry);
    }
}

void RootMoveTable::update(Move bestmove, Move pondermove, int depth, int score) {
    if (bestmove == MOVE_NONE) return;

    std::scoped_lock lock(mutex_);
    for (Entry& entry : entries_) {
        if (entry.bestmove != bestmove) continue;

        if (depth > entry.depth || (depth == entry.depth && score > entry.score)) {
            entry.pondermove = pondermove;
            entry.depth = depth;
            entry.score = score;
            entry.sequence = ++sequence_;
        }
        return;
    }
}

bool RootMoveTable::contains(Move move) const {
    std::scoped_lock lock(mutex_);
    for (const Entry& entry : entries_) {
        if (entry.bestmove == move)
            return true;
    }
    return false;
}

Move RootMoveTable::fallback_move() const {
    std::scoped_lock lock(mutex_);
    return entries_.empty() ? MOVE_NONE : entries_.front().bestmove;
}

int RootMoveTable::ordering_score(Move move) const {
    std::scoped_lock lock(mutex_);
    for (const Entry& entry : entries_) {
        if (entry.bestmove == move && entry.depth > 0) {
            return 7'000'000 + entry.depth * 10'000
                 + std::clamp(entry.score, -MATE_SCORE, MATE_SCORE);
        }
    }
    return 0;
}


SearchResult RootMoveTable::best_result() const {
    std::scoped_lock lock(mutex_);

    SearchResult best;
    int best_sequence = -1;

    for (const Entry& entry : entries_) {
        if (entry.depth <= 0 || entry.bestmove == MOVE_NONE)
            continue;
        const bool entry_mates = entry.score >= MATE_SCORE - MAX_PLY;
        const bool best_mates = best.score >= MATE_SCORE - MAX_PLY;

        if (best.bestmove == MOVE_NONE
            || (entry_mates && (!best_mates || entry.score > best.score))
            || (!best_mates && entry.depth > best.depth)
            || (!best_mates && entry.depth == best.depth && entry.score > best.score)
            || (!best_mates && entry.depth == best.depth && entry.score == best.score
                && entry.sequence > best_sequence)) {
            best.bestmove = entry.bestmove;
            best.pondermove = entry.pondermove;
            best.depth = entry.depth;
            best.score = entry.score;
            best_sequence = entry.sequence;
        }
    }

    return best;
}

static bool is_legal_move_on_board(const Board& board, Move move) {
    if (move == MOVE_NONE)
        return false;

    const Square from = from_sq(move);
    const Piece piece = board.board_sq[from];
    if (piece == NO_PIECE || color_of(piece) != board.side_to_move)
        return false;

    MoveList legal;
    board.gen_legal(legal);
    for (Move candidate : legal) {
        if (candidate == move)
            return true;
    }
    return false;
}

static Move first_legal_move(const Board& board) {
    MoveList legal;
    board.gen_legal(legal);
    return legal.size() == 0 ? MOVE_NONE : legal[0];
}

SearchResult sanitize_search_result(const Board& root_board, SearchResult result) {
    if (!is_legal_move_on_board(root_board, result.bestmove)) {
        result.bestmove = first_legal_move(root_board);
        result.pondermove = MOVE_NONE;
        return result;
    }

    if (result.pondermove != MOVE_NONE) {
        Board ponder_board = root_board;
        ponder_board.make_move(result.bestmove);
        if (!is_legal_move_on_board(ponder_board, result.pondermove))
            result.pondermove = MOVE_NONE;
    }

    return result;
}

// ---- Constructor -----------------------------------------------------------

Searcher::Searcher(TranspositionTable& tt,
                   std::atomic_bool& stop_flag,
                   std::function<void(const std::string&)> info_cb,
                   std::atomic_bool* ponderhit_flag)
    : evaluator_()
    , tt_(tt)
    , stop_(stop_flag)
    , ponderhit_(ponderhit_flag)
    , info_cb_(std::move(info_cb))
    , board_ptr_(nullptr)
    , nodes_(0)
    , tb_hits_(0)
    , nodes_limit_(0)
    , shared_nodes_flushed_(0)
    , shared_nodes_total_(0)
    , sel_depth_(0)
    , stopped_(false)
    , root_filter_index_(-1)
    , root_filter_count_(1)
    , thread_id_(0)
    , root_table_(nullptr)
    , pondering_(false)
    , root_side_(WHITE)
    , root_depth_nodes_(0)
    , root_best_nodes_(0)
    , root_best_effort_(0)
    , history_age_counter_(0)
    , time_limit_(0.0)
    , soft_limit_(0.0)
    , hard_limit_(0.0)
{
    // hist_ constructs cleared (history.h); clear() also resets the age counter.
    clear();
}

void Searcher::clear() {
    hist_.clear();
    history_age_counter_ = 0;
}

// ---- Time management -------------------------------------------------------

void Searcher::compute_time_limit(const SearchLimits& limits, Color side, int game_ply) {
    soft_limit_ = 0.0;
    hard_limit_ = 0.0;

    if (limits.infinite || limits.ponder) return;
    if (limits.movetime > 0) {
        // Fixed movetime: use the full movetime as the hard limit. GUIs and
        // adjudicators tolerate ~10% over nominal, and movetime games never
        // forfeit on time the way clock play does, so subtracting overhead
        // here only costs depth for no safety gain.
        hard_limit_ = std::max(1, limits.movetime) / 1000.0;
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
    const double smp_reserve  = (limits.thread_count > 1) ? 30.0 : 0.0;
    const double reserve      = 2.0 * overhead + smp_reserve;
    const double hard_ceiling = std::max(time - reserve, 1.0);
    maximum_ms = std::min(maximum_ms, hard_ceiling);
    optimum_ms = std::min(optimum_ms, maximum_ms);

    soft_limit_ = optimum_ms / 1000.0;
    hard_limit_ = maximum_ms / 1000.0;

    // Legacy: keep time_limit_ pointing at hard for check_stop()
    time_limit_ = hard_limit_;
}

double Searcher::elapsed_seconds() const {
    using namespace std::chrono;
    return duration<double>(steady_clock::now() - start_time_).count();
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
    ++diag_.tt_stores;
    if (tt_.store(key, depth, score, flag, m, ply, static_eval))
        ++diag_.tt_stores_same_key;
}

void Searcher::flush_shared_nodes() {
    if (!active_limits_.shared_nodes)
        return;
    const int64_t pending = nodes_ - shared_nodes_flushed_;
    if (pending <= 0)
        return;
    shared_nodes_flushed_ = nodes_;
    shared_nodes_total_ =
        active_limits_.shared_nodes->fetch_add(pending, std::memory_order_relaxed) + pending;
}

int64_t Searcher::record_node() {
    ++nodes_;
    if (active_limits_.shared_nodes) {
        if ((nodes_ & (sharedNodeBatch - 1)) == 0)
            flush_shared_nodes();
        // The node LIMIT is still checked every node, against the last
        // published pool total plus this thread's unpublished nodes. That
        // estimate is a lower bound on the true total (other threads hold
        // unpublished nodes of their own) and is exact immediately after a
        // flush — so `go nodes N` keeps roughly its old accuracy, which
        // checking only at batch boundaries would NOT: a limit smaller than
        // one batch would have been missed entirely and overshot ~10x.
        // The comparison is local arithmetic; the atomic is what got batched.
        const int64_t estimate = shared_nodes_total_ + (nodes_ - shared_nodes_flushed_);
        if (nodes_limit_ > 0 && estimate >= nodes_limit_)
            stopped_ = true;
        return estimate;
    }
    if (nodes_limit_ > 0 && nodes_ >= nodes_limit_)
        stopped_ = true;
    return nodes_;
}

void Searcher::record_tbhit(int64_t count) {
    if (count <= 0)
        return;
    tb_hits_ += count;
    if (active_limits_.shared_tbhits)
        active_limits_.shared_tbhits->fetch_add(count, std::memory_order_relaxed);
}

int64_t Searcher::current_nodes() const {
    return active_limits_.shared_nodes
        ? active_limits_.shared_nodes->load(std::memory_order_relaxed)
        : nodes_;
}

int64_t Searcher::current_tbhits() const {
    return active_limits_.shared_tbhits
        ? active_limits_.shared_tbhits->load(std::memory_order_relaxed)
        : tb_hits_;
}

bool Searcher::check_stop() {
    if (stopped_) return true;

    if (stop_.load(std::memory_order_acquire)) {
        stopped_ = true;
        return true;
    }

    if (pondering_ && ponderhit_ && ponderhit_->load(std::memory_order_acquire)) {
        pondering_ = false;
        SearchLimits normal_limits = active_limits_;
        normal_limits.ponder = false;
        const int game_ply = board_ptr_
            ? 2 * (board_ptr_->fullmove_number - 1) + (board_ptr_->side_to_move == BLACK ? 1 : 0)
            : 0;
        compute_time_limit(normal_limits, root_side_, game_ply);
        if (soft_limit_ > 0.0 && elapsed_seconds() >= soft_limit_) {
            stopped_ = true;
            return true;
        }
    }

    if (hard_limit_ > 0.0 && elapsed_seconds() >= hard_limit_) {
        stopped_ = true;
        return true;
    }
    if (nodes_limit_ > 0 && current_nodes() >= nodes_limit_) {
        stopped_ = true;
        return true;
    }
    return false;
}

// ---- History ---------------------------------------------------------------

void Searcher::update_quiet(Color stm, Square from, Square to, int bonus) {
    hist_update<HistoryTables::MAX_MAIN_HIST>(hist_.main[stm][from][to], bonus);
}

void Searcher::update_cap(PieceType pt, Square to, PieceType cap, int bonus) {
    hist_update<HistoryTables::MAX_CAP_HIST>(hist_.capture[pt][to][cap], bonus);
}

void Searcher::update_cont(HistoryTables::ContHistTable& tbl,
                           PieceType ppt, Square pto,
                           PieceType cpt, Square cto, int bonus) {
    hist_update<HistoryTables::MAX_CONT_HIST>(tbl.data[ppt][pto][cpt][cto], bonus);
}

void Searcher::update_pawn_hist(Key pawn_key, PieceType pt, Square to, int bonus) {
    hist_update<HistoryTables::MAX_PAWN_HIST>(hist_.pawn->data[pawn_key & (HistoryTables::PAWN_HIST_SIZE - 1)][pt][to], bonus);
}

void Searcher::update_low_ply(int ply, Square from, Square to, int bonus) {
    if (ply < HistoryTables::LOW_PLY_HISTORY_SIZE)
        hist_update<HistoryTables::MAX_LOW_HIST>(hist_.low_ply[ply][from][to], bonus);
}

// Phase 6.3 bonus/malus shape: bonus = min(quad*d^2/64 + lin*d, max), malus
// mirrored with its own knobs. Defaults reproduce the legacy min(d*d, 2048).
int Searcher::history_bonus_value(int depth) const {
    const auto& p = active_limits_.params;
    return std::min(p.hist_bonus_quad * depth * depth / 64 + p.hist_bonus_lin * depth,
                    p.hist_bonus_max);
}

int Searcher::history_malus_value(int depth) const {
    const auto& p = active_limits_.params;
    return -std::min(p.hist_malus_quad * depth * depth / 64 + p.hist_malus_lin * depth,
                     p.hist_malus_max);
}

void Searcher::update_cont_for_move(SearchStack* ss, PieceType pt, Square to, int bonus) {
    if ((ss-1)->moved_piece != NO_PIECE_TYPE && (ss-1)->move != MOVE_NONE
        && (ss-1)->move != MOVE_NULL) {
        update_cont(*hist_.cont1, (ss-1)->moved_piece,
                    Square(to_sq((ss-1)->move)), pt, to, bonus);
    }
    if ((ss-2)->moved_piece != NO_PIECE_TYPE && (ss-2)->move != MOVE_NONE
        && (ss-2)->move != MOVE_NULL) {
        update_cont(*hist_.cont2, (ss-2)->moved_piece,
                    Square(to_sq((ss-2)->move)), pt, to, bonus);
    }
    if ((ss-4)->moved_piece != NO_PIECE_TYPE && (ss-4)->move != MOVE_NONE
        && (ss-4)->move != MOVE_NULL) {
        update_cont(*hist_.cont4, (ss-4)->moved_piece,
                    Square(to_sq((ss-4)->move)), pt, to, bonus / 2);
    }
}

int Searcher::cont_hist_score(const SearchStack* ss, PieceType pt, Square to) const {
    int score = 0;
    // 1-ply back
    if ((ss-1)->move != MOVE_NONE && (ss-1)->move != MOVE_NULL
        && (ss-1)->moved_piece != NO_PIECE_TYPE) {
        score += hist_.cont1->data[(ss-1)->moved_piece][to_sq((ss-1)->move)][pt][to];
    }
    // 2-ply back
    if ((ss-2)->move != MOVE_NONE && (ss-2)->move != MOVE_NULL
        && (ss-2)->moved_piece != NO_PIECE_TYPE) {
        score += hist_.cont2->data[(ss-2)->moved_piece][to_sq((ss-2)->move)][pt][to];
    }
    // 4-ply back keeps useful quiet continuations across one full move pair.
    if ((ss-4)->move != MOVE_NONE && (ss-4)->move != MOVE_NULL
        && (ss-4)->moved_piece != NO_PIECE_TYPE) {
        score += hist_.cont4->data[(ss-4)->moved_piece][to_sq((ss-4)->move)][pt][to] / 2;
    }
    return score;
}

// (8.6.6: first statement counts the update event by type)
void Searcher::update_all_histories(Move best, bool best_is_tt,
                                    const Move* quiets, int quiet_count,
                                    const Move* bad_caps, int bad_cap_count,
                                    Color stm, int depth, SearchStack* ss,
                                    bool reward_only, int bonus_scale) {
    (reward_only ? diag_.hist_reward_updates : diag_.hist_cutoff_updates)++;
    // 8.5.10(e): bonus_scale (percent) lets the caller boost the reward when the
    // cutoff was "surprising" (static eval below beta -- the search saw a good
    // move the eval did not). Default 100 = unchanged.
    int bonus = history_bonus_value(depth) * bonus_scale / 100
              + (best_is_tt ? active_limits_.params.hist_ttmove_bonus : 0);
    int malus = history_malus_value(depth);

    bool best_is_cap   = (board_ptr_->board_sq[to_sq(best)] != NO_PIECE)
                      || (move_type(best) == EN_PASSANT);
    bool best_is_promo = (move_type(best) == PROMOTION);

    if (!best_is_cap && !best_is_promo) {
        Square from  = Square(from_sq(best));
        Square to    = Square(to_sq(best));
        PieceType pt = type_of(board_ptr_->board_sq[from]);

        // Quiet history
        update_quiet(stm, from, to, bonus);
        update_pawn_hist(board_ptr_->pawn_key, pt, to, bonus);
        update_low_ply(static_cast<int>(ss - (ss_arr_ + 4)), from, to, bonus);

        // Killers / countermove are cutoff semantics ("this move refuted the
        // node"). In reward_only mode (exact/PV nodes) the best move improved
        // alpha but did not refute anything, so we boost only the graded history
        // tables and leave the categorical killer/countermove slots alone.
        if (!reward_only) {
            // Killers
            if (ss->killers[0] != best) {
                ss->killers[1] = ss->killers[0];
                ss->killers[0] = best;
            }

            // Countermove
            Move prev = (ss-1)->move;
            if (prev != MOVE_NONE && prev != MOVE_NULL)
                hist_.countermove[from_sq(prev)][to_sq(prev)] = best;
        }

        // Continuation history
        update_cont_for_move(ss, pt, to, bonus);

        // Malus for other searched quiets
        if (!reward_only)
        for (int i = 0; i < quiet_count; ++i) {
            Move m = quiets[i];
            if (m == best) continue;
            Square mf = Square(from_sq(m)), mt = Square(to_sq(m));
            PieceType mpt = type_of(board_ptr_->board_sq[mf]);
            update_quiet(stm, mf, mt, malus);
            update_pawn_hist(board_ptr_->pawn_key, mpt, mt, malus);
            update_low_ply(static_cast<int>(ss - (ss_arr_ + 4)), mf, mt, malus);
            update_cont_for_move(ss, mpt, mt, malus);
        }
    } else if (best_is_cap) {
        // Best was a capture (not a quiet promotion)
        PieceType atk = type_of(board_ptr_->board_sq[from_sq(best)]);
        PieceType cap = (move_type(best) == EN_PASSANT)
                      ? PAWN : type_of(board_ptr_->board_sq[to_sq(best)]);
        update_cap(atk, Square(to_sq(best)), cap, bonus);
    }
    // Quiet promotions: no history update (too rare to matter)

    // Malus for bad captures searched before best
    if (!reward_only)
    for (int i = 0; i < bad_cap_count; ++i) {
        Move m = bad_caps[i];
        if (m == best) continue;
        PieceType atk = type_of(board_ptr_->board_sq[from_sq(m)]);
        PieceType cap = (move_type(m) == EN_PASSANT)
                      ? PAWN : type_of(board_ptr_->board_sq[to_sq(m)]);
        update_cap(atk, Square(to_sq(m)), cap, malus);
    }
}

static void update_correction_slot(int16_t& slot, int diff, int depth) {
    static constexpr int MAX_CORR = 1024;
    int w = std::min(depth + 1, 16);
    int updated = std::clamp((int(slot) * (256 - w) + diff * w) / 256, -MAX_CORR, MAX_CORR);
    slot = static_cast<int16_t>(updated);
}

void Searcher::update_correction(Color stm, const Board& board, SearchStack* ss, int diff, int depth) {
    update_correction_slot(hist_.pawn_corr[stm][board.pawn_key & (HistoryTables::CORR_SIZE - 1)], diff, depth);
    update_correction_slot(hist_.minor_corr[stm][board.minor_key & (HistoryTables::CORR_SIZE - 1)], diff, depth);
    update_correction_slot(hist_.nonpawn_corr[stm][WHITE][board.nonpawn_key[WHITE] & (HistoryTables::CORR_SIZE - 1)],
                           diff, depth);
    update_correction_slot(hist_.nonpawn_corr[stm][BLACK][board.nonpawn_key[BLACK] & (HistoryTables::CORR_SIZE - 1)],
                           diff, depth);

    if ((ss-1)->move != MOVE_NONE && (ss-1)->move != MOVE_NULL
        && (ss-1)->moved_piece != NO_PIECE_TYPE) {
        update_correction_slot(hist_.cont_corr[stm][(ss-1)->moved_piece][to_sq((ss-1)->move)],
                               diff, depth);
    }
}

int Searcher::correction_value(Color stm, const Board& board, const SearchStack* ss) const {
    const int pawn = hist_.pawn_corr[stm][board.pawn_key & (HistoryTables::CORR_SIZE - 1)];
    const int minor = hist_.minor_corr[stm][board.minor_key & (HistoryTables::CORR_SIZE - 1)];
    const int own = hist_.nonpawn_corr[stm][stm][board.nonpawn_key[stm] & (HistoryTables::CORR_SIZE - 1)];
    const int opp = hist_.nonpawn_corr[stm][~stm][board.nonpawn_key[~stm] & (HistoryTables::CORR_SIZE - 1)];

    int cont = 0;
    if ((ss-1)->move != MOVE_NONE && (ss-1)->move != MOVE_NULL
        && (ss-1)->moved_piece != NO_PIECE_TYPE) {
        cont = hist_.cont_corr[stm][(ss-1)->moved_piece][to_sq((ss-1)->move)];
    }

    return (pawn + minor + own + opp + cont) / 5;
}

void Searcher::age_history() {
    hist_.age();
}

// ---- Move ordering ---------------------------------------------------------

static constexpr int PIECE_VALUE[PIECE_TYPE_NB] = {0, 100, 300, 300, 500, 900, 20000};
static constexpr int MAX_TRACKED_QUIETS = 64;
static constexpr int MAX_TRACKED_BAD_CAPS = 32;

void Searcher::score_moves(ScoredMove* moves, int n, SearchStack* ss,
                           bool is_root, int ply) const {
    const Board& b = *board_ptr_;

    // 9.6: these history-table dimensions are constant for every quiet move at
    // this node. Hoist them beside the continuation rows so the move loop only
    // indexes its varying piece/from/to dimensions. Keeping the additions in
    // the same order preserves the fixed-depth bench exactly.
    const auto& main_hist = hist_.main[b.side_to_move];
    const auto& pawn_hist = hist_.pawn->data[
        b.pawn_key & (HistoryTables::PAWN_HIST_SIZE - 1)];
    const auto* low_ply_hist = ply < HistoryTables::LOW_PLY_HISTORY_SIZE
                             ? &hist_.low_ply[ply] : nullptr;

    Move cm = MOVE_NONE;
    Move prev = (ss-1)->move;
    if (prev != MOVE_NONE && prev != MOVE_NULL)
        cm = hist_.countermove[from_sq(prev)][to_sq(prev)];

    std::array<Bitboard, PIECE_TYPE_NB> check_squares{};
    std::array<bool, PIECE_TYPE_NB> check_squares_ready{};
    auto checks_for = [&](PieceType pt) {
        const auto idx = static_cast<size_t>(pt);
        if (!check_squares_ready[idx]) {
            check_squares[idx] = b.check_squares(pt, b.side_to_move);
            check_squares_ready[idx] = true;
        }
        return check_squares[idx];
    };

    // 8.7.6(b): hoist the continuation-history row bases ONCE per node. The
    // (ss-1/2/4) piece/square indices are constant across every scored move,
    // so recomputing the guards and the first two array dimensions (two index
    // multiplies each) per quiet — cont_hist_score(ss, ...) — is pure waste.
    // With the rows hoisted, the per-move cost is three [pt][to] loads. The
    // computed sum is identical (same terms, same order, same cont4/2 integer
    // divide), so bench stays 11,941,440. Standard SF conthist pattern.
    const int16_t (*ch1)[SQUARE_NB] = nullptr;
    const int16_t (*ch2)[SQUARE_NB] = nullptr;
    const int16_t (*ch4)[SQUARE_NB] = nullptr;
    if ((ss-1)->move != MOVE_NONE && (ss-1)->move != MOVE_NULL
        && (ss-1)->moved_piece != NO_PIECE_TYPE)
        ch1 = hist_.cont1->data[(ss-1)->moved_piece][to_sq((ss-1)->move)];
    if ((ss-2)->move != MOVE_NONE && (ss-2)->move != MOVE_NULL
        && (ss-2)->moved_piece != NO_PIECE_TYPE)
        ch2 = hist_.cont2->data[(ss-2)->moved_piece][to_sq((ss-2)->move)];
    if ((ss-4)->move != MOVE_NONE && (ss-4)->move != MOVE_NULL
        && (ss-4)->moved_piece != NO_PIECE_TYPE)
        ch4 = hist_.cont4->data[(ss-4)->moved_piece][to_sq((ss-4)->move)];

    for (int i = 0; i < n; i++) {
        Move m = moves[i].move;

        bool is_cap   = (b.board_sq[to_sq(m)] != NO_PIECE) || (move_type(m) == EN_PASSANT);
        bool is_promo = (move_type(m) == PROMOTION);

        if (is_cap) {
            PieceType atk = type_of(b.board_sq[from_sq(m)]);
            PieceType cap = (move_type(m) == EN_PASSANT) ? PAWN : type_of(b.board_sq[to_sq(m)]);
            moves[i].score = 6'000'000 + PIECE_VALUE[cap] * 16 - PIECE_VALUE[atk]
                                       + hist_.capture[atk][to_sq(m)][cap];
        } else if (is_promo) {
            moves[i].score = (promo_type(m) == QUEEN) ? 5'500'000 : -100;
        } else {
            // Quiet
            const Square from = Square(from_sq(m));
            const Square to = Square(to_sq(m));
            const PieceType pt = type_of(b.board_sq[from]);
            int hist = main_hist[from][to];
            if (ch1) hist += ch1[pt][to];
            if (ch2) hist += ch2[pt][to];
            if (ch4) hist += ch4[pt][to] / 2;
            hist += pawn_hist[pt][to];
            if (low_ply_hist) hist += (*low_ply_hist)[from][to];

            if (checks_for(pt) & sq_bb(to))
                hist += 32'000;

            if      (m == ss->killers[0]) moves[i].score = 4'000'000;
            else if (m == ss->killers[1]) moves[i].score = 3'900'000;
            else if (m == cm)             moves[i].score = 3'800'000;
            else                          moves[i].score = hist;
        }

        if (is_root && !root_tb_moves_.empty())
            moves[i].score += root_tablebase_ordering_score(m);

        if (is_root && root_table_)
            moves[i].score += root_table_->ordering_score(m);
    }
}

Move Searcher::pick_next(ScoredMove* moves, int idx, int n) {
    ScoredMove* const first = moves + idx;
    ScoredMove* best = first;
    // 8.7.6(d): keep the running best SCORE in a register instead of reloading
    // best->score on every comparison. Selection order is unchanged (strict >,
    // first-wins on ties), so bench stays identical.
    int best_score = first->score;
    for (ScoredMove* it = first + 1, *end = moves + n; it != end; ++it)
        if (it->score > best_score) {
            best = it;
            best_score = it->score;
        }

    if (best != first)
        std::swap(*first, *best);
    return first->move;
}

class Searcher::MovePicker {
public:
    MovePicker(Searcher& searcher, Move tt_move, Move excluded, SearchStack* ss,
               bool is_root, int ply, ScoredMove* tactical_buffer, ScoredMove* bad_buffer)
        : searcher_(searcher)
        , tt_move_(tt_move)
        , excluded_(excluded)
        , ss_(ss)
        , is_root_(is_root)
        , ply_(ply)
        , scored_(tactical_buffer)
        , bad_(bad_buffer) {}

    Move next() {
        last_see_ = VALUE_NONE;   // 8.7.5(a): reset the per-move SEE verdict
        last_src_ = Src::None;    // 5.2: reset the per-move picker source
        while (true) {
            switch (stage_) {
                case Stage::TT:
                    stage_ = Stage::TacticalsInit;
                    if (tt_move_ != MOVE_NONE && tt_move_ != excluded_) {
                        Piece p = searcher_.board_ptr_->board_sq[from_sq(tt_move_)];
                        if (p != NO_PIECE
                            && color_of(p) == searcher_.board_ptr_->side_to_move
                            && searcher_.board_ptr_->is_legal(tt_move_)) {
                            tt_searched_ = true;
                            last_src_ = Src::TT;
                            return tt_move_;
                        }
                    }
                    break;

                case Stage::TacticalsInit:
                    fill_tacticals();
                    stage_ = Stage::GoodTacticals;
                    break;

                case Stage::GoodTacticals:
                    while (idx_ < n_) {
                        Move move = Searcher::pick_next(scored_, idx_++, n_);
                        if (is_bad_tactical(move)) {
                            bad_[bad_count_++] = {move, scored_[idx_ - 1].score};
                            continue;
                        }
                        // 8.7.5(a): a good tactical that is a non-promo capture
                        // passed is_bad_tactical == false, i.e. see_ge(m,0) was
                        // TRUE — memoize see_score = 0 so search_one need not
                        // recompute the identical see_ge. Promotions carry no
                        // SEE verdict (search skips SEE for them).
                        last_see_ = is_nonpromo_capture(move) ? 0 : VALUE_NONE;
                        last_src_ = Src::GoodTactical;
                        return move;
                    }
                    stage_ = Stage::QuietsInit;
                    break;

                case Stage::QuietsInit:
                    fill_quiets();
                    stage_ = Stage::Quiets;
                    break;

                case Stage::Quiets:
                    if (idx_ < n_) {
                        last_src_ = Src::Quiet;
                        return Searcher::pick_next(scored_, idx_++, n_);
                    }
                    stage_ = Stage::BadTacticals;
                    bad_idx_ = 0;
                    break;

                case Stage::BadTacticals:
                    if (bad_idx_ < bad_count_) {
                        // 8.7.5(a): the bad-tactical buffer holds only non-promo
                        // captures with see_ge(m,0) == FALSE → see_score = -1.
                        last_see_ = -1;
                        last_src_ = Src::BadTactical;
                        return Searcher::pick_next(bad_, bad_idx_++, bad_count_);
                    }
                    stage_ = Stage::Done;
                    break;

                case Stage::Done:
                    return MOVE_NONE;
            }
        }
    }

private:
    enum class Stage {
        TT,
        TacticalsInit,
        GoodTacticals,
        QuietsInit,
        Quiets,
        BadTacticals,
        Done
    };

    void fill_tacticals() {
        MoveList moves;
        searcher_.board_ptr_->gen_legal_captures(moves);
        fill_from(moves);
    }

    void fill_quiets() {
        MoveList moves;
        searcher_.board_ptr_->gen_legal_quiets(moves);
        fill_from(moves);
    }

    void fill_from(const MoveList& moves) {
        n_ = 0;
        idx_ = 0;
        for (Move move : moves) {
            if (move == excluded_ || (tt_searched_ && move == tt_move_))
                continue;
            scored_[n_++] = {move, 0};
        }
        searcher_.score_moves(scored_, n_, ss_, is_root_, ply_);
    }

    bool is_bad_tactical(Move move) const {
        if (move_type(move) == PROMOTION)
            return false;
        const Board& board = *searcher_.board_ptr_;
        const bool is_cap = board.board_sq[to_sq(move)] != NO_PIECE || move_type(move) == EN_PASSANT;
        return is_cap && !board.see_ge(move, 0);
    }

    // 8.7.5(a): matches search_one's `is_cap && !is_promo` — the exact class
    // for which see_score = see_ge(m,0)?0:-1 is computed downstream.
    bool is_nonpromo_capture(Move move) const {
        if (move_type(move) == PROMOTION)
            return false;
        const Board& board = *searcher_.board_ptr_;
        return board.board_sq[to_sq(move)] != NO_PIECE || move_type(move) == EN_PASSANT;
    }

public:
    // The SEE verdict for the move next() just returned: 0 (good capture),
    // -1 (bad capture), or VALUE_NONE (TT move / promo / quiet / not a capture)
    // — lets search_one skip recomputing the identical see_ge(m,0).
    int last_see_score() const { return last_see_; }
    // 5.2: which stage produced the move just returned. Recorded at each
    // return rather than read from stage_, because the TT and tactical
    // stages advance stage_ before returning.
    enum class Src { None, TT, GoodTactical, Quiet, BadTactical };
    Src last_source() const { return last_src_; }
private:

    Searcher& searcher_;
    Move tt_move_;
    Move excluded_;
    SearchStack* ss_;
    bool is_root_;
    int ply_;
    bool tt_searched_ = false;
    Stage stage_ = Stage::TT;
    ScoredMove* scored_;
    ScoredMove* bad_;
    int n_ = 0;
    int idx_ = 0;
    int bad_count_ = 0;
    int bad_idx_ = 0;
    int last_see_ = VALUE_NONE;   // 8.7.5(a): SEE verdict of the last move returned
    Src last_src_ = Src::None;    // 5.2: picker stage of the last move returned
};

// ---- UCI info ---------------------------------------------------------------

// 8.6.6: end-of-search diagnostic dump (UCI `Diag`, TUNE builds). One line per
// family; shares are of interior (negamax) nodes. Never a gate — these size
// candidates and verify mechanisms (check_exts must be 0 once 8.6.7 lands).
void Searcher::print_diag() const {
    if (!info_cb_) return;
    const auto& d = diag_;
    auto pct = [](int64_t a, int64_t b) {
        return b > 0 ? 100.0 * double(a) / double(b) : 0.0;
    };
    // 5.6: 256 was too small once the kv mirror grew. snprintf truncates
    // silently and always loses the TAIL field, so the corruption scales
    // with the counter magnitudes - it produced a non-monotonic threshold
    // series that is arithmetically impossible. Sized with headroom, and
    // the history probe moved to its own line below.
    char buf[512];
    auto emit = [&](const char* text) { info_cb_(std::string("info string diag ") + text); };
    std::snprintf(buf, sizeof(buf),
        "nodes interior %lld qsearch %lld | in_check %lld (%.2f%%) check_ext %lld tt_pv %lld (%.2f%%)",
        (long long)d.interior_nodes, (long long)d.qs_nodes,
        (long long)d.in_check_nodes, pct(d.in_check_nodes, d.interior_nodes),
        (long long)d.check_exts,
        (long long)d.tt_pv_nodes, pct(d.tt_pv_nodes, d.interior_nodes));
    emit(buf);
    std::snprintf(buf, sizeof(buf),
        "tt probes %lld hits %lld (%.2f%%) cutoffs %lld",
        (long long)d.tt_probes, (long long)d.tt_hits, pct(d.tt_hits, d.tt_probes),
        (long long)d.tt_cutoffs);
    emit(buf);
    std::snprintf(buf, sizeof(buf),
        "prune rfp %lld razor %lld null %lld/%lld probcut %lld/%lld fut %lld lmp %lld hist %lld see %lld",
        (long long)d.rfp_cuts, (long long)d.razor_cuts,
        (long long)d.null_cuts, (long long)d.null_tries,
        (long long)d.probcut_cuts, (long long)d.probcut_tries,
        (long long)d.fut_prunes, (long long)d.lmp_prunes,
        (long long)d.hist_prunes, (long long)d.see_prunes);
    emit(buf);
    std::snprintf(buf, sizeof(buf),
        "lmr applied %lld researched %lld (%.2f%%) | hist updates cutoff %lld reward %lld | qs evasion %lld",
        (long long)d.lmr_applied, (long long)d.lmr_researched,
        pct(d.lmr_researched, d.lmr_applied),
        (long long)d.hist_cutoff_updates, (long long)d.hist_reward_updates,
        (long long)d.qs_evasion_nodes);
    emit(buf);
    // ---- 5.2 differential harness (BAS-O03) --------------------------------
    // Read these against the oracle's tree shape, not in isolation.
    std::snprintf(buf, sizeof(buf),
        "order fail_highs %lld first %lld (%.2f%%) mean_idx %.3f | src tt %lld goodcap %lld quiet %lld badcap %lld",
        (long long)d.fail_highs, (long long)d.fail_high_first,
        pct(d.fail_high_first, d.fail_highs),
        d.fail_highs > 0 ? double(d.fail_high_index_sum) / double(d.fail_highs) : 0.0,
        (long long)d.cutoff_src_tt, (long long)d.cutoff_src_good_tactical,
        (long long)d.cutoff_src_quiet, (long long)d.cutoff_src_bad_tactical);
    emit(buf);
    std::snprintf(buf, sizeof(buf),
        "lmrgate eligible %lld applied %lld (%.2f%%) mean_r %.3f clamp0 %lld",
        (long long)d.lmr_eligible, (long long)d.lmr_applied,
        pct(d.lmr_applied, d.lmr_eligible),
        d.lmr_applied > 0 ? double(d.lmr_reduction_plies) / double(d.lmr_applied) : 0.0,
        (long long)d.lmr_clamped_zero);
    emit(buf);
    std::snprintf(buf, sizeof(buf),
        "lmrblock depth %lld searched %lld in_check %lld movetype %lld gives_check %lld",
        (long long)d.lmr_blocked_depth, (long long)d.lmr_blocked_searched,
        (long long)d.lmr_blocked_in_check, (long long)d.lmr_blocked_movetype,
        (long long)d.lmr_blocked_gives_check);
    emit(buf);
    // Machine-readable mirror. The lines above are shaped for a human reading
    // one search; the harness aggregates over a 107-position suite and must not
    // have to reverse-engineer prose, percentages or floats to do it. Integers
    // only, canonical names, one token per counter — derived ratios are the
    // consumer's job, since summing a percentage across positions is wrong.
    {
        std::snprintf(buf, sizeof(buf),
            "kv fail_highs=%lld fail_high_first=%lld fail_high_index_sum=%lld "
            "cutoff_src_tt=%lld cutoff_src_goodcap=%lld cutoff_src_quiet=%lld "
            "cutoff_src_badcap=%lld",
            (long long)d.fail_highs, (long long)d.fail_high_first,
            (long long)d.fail_high_index_sum,
            (long long)d.cutoff_src_tt, (long long)d.cutoff_src_good_tactical,
            (long long)d.cutoff_src_quiet, (long long)d.cutoff_src_bad_tactical);
        emit(buf);
        std::snprintf(buf, sizeof(buf),
            "kv lmr_eligible=%lld lmr_applied=%lld lmr_researched=%lld "
            "lmr_reduction_plies=%lld lmr_clamped_zero=%lld "
            "lmr_blocked_depth=%lld lmr_blocked_searched=%lld "
            "lmr_blocked_in_check=%lld lmr_blocked_movetype=%lld "
            "lmr_blocked_gives_check=%lld lmr_clamped_high=%lld",
            (long long)d.lmr_eligible, (long long)d.lmr_applied,
            (long long)d.lmr_researched, (long long)d.lmr_reduction_plies,
            (long long)d.lmr_clamped_zero,
            (long long)d.lmr_blocked_depth, (long long)d.lmr_blocked_searched,
            (long long)d.lmr_blocked_in_check, (long long)d.lmr_blocked_movetype,
            (long long)d.lmr_blocked_gives_check, (long long)d.lmr_clamped_high);
        emit(buf);
        std::snprintf(buf, sizeof(buf),
            "kv interior_nodes=%lld qs_nodes=%lld tt_probes=%lld tt_hits=%lld "
            "tt_cutoffs=%lld in_check_nodes=%lld check_exts=%lld",
            (long long)d.interior_nodes, (long long)d.qs_nodes,
            (long long)d.tt_probes, (long long)d.tt_hits, (long long)d.tt_cutoffs,
            (long long)d.in_check_nodes, (long long)d.check_exts);
        emit(buf);
        std::snprintf(buf, sizeof(buf),
            "kv rfp_cuts=%lld razor_cuts=%lld null_tries=%lld null_cuts=%lld "
            "probcut_tries=%lld probcut_cuts=%lld fut_prunes=%lld lmp_prunes=%lld "
            "hist_prunes=%lld see_prunes=%lld",
            (long long)d.rfp_cuts, (long long)d.razor_cuts,
            (long long)d.null_tries, (long long)d.null_cuts,
            (long long)d.probcut_tries, (long long)d.probcut_cuts,
            (long long)d.fut_prunes, (long long)d.lmp_prunes,
            (long long)d.hist_prunes, (long long)d.see_prunes);
        emit(buf);
        std::snprintf(buf, sizeof(buf),
            "kv hist_prune_tested=%lld hist_below_half=%lld "
            "hist_below_quarter=%lld hist_below_eighth=%lld",
            (long long)d.hist_prune_tested, (long long)d.hist_below_half,
            (long long)d.hist_below_quarter, (long long)d.hist_below_eighth);
        emit(buf);
    }
    // 8.7.1(c) speed telemetry — the numbers Phase 8.7 steps read before
    // touching anything: eval rate (8.7.7), pawn-cache hit rate (8.7.8),
    // full-gives_check rate (8.7.3), SEE calls per node (8.7.5).
    {
        const int64_t total_nodes = d.interior_nodes + d.qs_nodes;
        std::snprintf(buf, sizeof(buf),
            "speed eval %lld (%.2f%%/node) pawncache %lld/%lld (%.2f%% hit) "
            "gives_check %lld (%.2f%%/node) see_ge %lld (%.3f/node)",
            (long long)evaluator_.eval_calls, pct(evaluator_.eval_calls, total_nodes),
            (long long)evaluator_.pawn_hits, (long long)evaluator_.pawn_probes,
            pct(evaluator_.pawn_hits, evaluator_.pawn_probes),
            (long long)d.gives_check_calls, pct(d.gives_check_calls, total_nodes),
            (long long)d.see_ge_calls,
            total_nodes > 0 ? double(d.see_ge_calls) / double(total_nodes) : 0.0);
        emit(buf);
        std::snprintf(buf, sizeof(buf),
            "kv eval_calls=%lld pawn_probes=%lld pawn_hits=%lld "
            "gives_check_calls=%lld see_ge_calls=%lld",
            (long long)evaluator_.eval_calls,
            (long long)evaluator_.pawn_probes, (long long)evaluator_.pawn_hits,
            (long long)d.gives_check_calls, (long long)d.see_ge_calls);
        emit(buf);
    }
#ifdef BASILISK_TUNE
    {
        const auto& e = evaluator_.endgame_occurrence;
        std::snprintf(buf, sizeof(buf),
            "endgames <=7men %lld (%.3f%% eval, %.3f%% node)",
            (long long)e.classified,
            pct(e.classified, evaluator_.eval_calls),
            pct(e.classified, d.interior_nodes + d.qs_nodes));
        emit(buf);
        std::snprintf(buf, sizeof(buf),
            "kv eg_classified=%lld eg_krpkr=%lld eg_krpkb=%lld eg_kpsk=%lld "
            "eg_kpk=%lld eg_krkp=%lld eg_kbpsk=%lld eg_kpkp=%lld",
            (long long)e.classified, (long long)e.krpkr, (long long)e.krpkb,
            (long long)e.kpsk, (long long)e.kpk, (long long)e.krkp,
            (long long)e.kbpsk, (long long)e.kpkp);
        emit(buf);
        std::snprintf(buf, sizeof(buf),
            "kv eg_kqkp=%lld eg_kbpkb=%lld eg_kbppkb=%lld eg_krkn=%lld "
            "eg_krkb=%lld eg_kbpkn=%lld eg_knnkp=%lld eg_knnk=%lld",
            (long long)e.kqkp, (long long)e.kbpkb, (long long)e.kbppkb,
            (long long)e.krkn, (long long)e.krkb, (long long)e.kbpkn,
            (long long)e.knnkp, (long long)e.knnk);
        emit(buf);
        std::snprintf(buf, sizeof(buf),
            "kv eg_kqkr=%lld eg_kqkrps=%lld eg_krppkrp=%lld eg_kxk=%lld eg_kbnk=%lld",
            (long long)e.kqkr, (long long)e.kqkrps, (long long)e.krppkrp,
            (long long)e.kxk, (long long)e.kbnk);
        emit(buf);
    }
#endif
    {
        char b[256];
        std::snprintf(b, sizeof(b),
            "aspiration windows %lld fail_low %lld fail_high %lld researches %lld giveup %lld",
            (long long)diag_.asp_windows, (long long)diag_.asp_fail_low,
            (long long)diag_.asp_fail_high, (long long)diag_.asp_researches,
            (long long)diag_.asp_giveup);
        emit(b);
    }

    {
        char b[256];
        std::snprintf(b, sizeof(b),
            "singular fired %lld double %lld in_check %lld triple %lld ttbeta %lld (%.2f%% of fired)",
            (long long)diag_.sing_fired, (long long)diag_.sing_double,
            (long long)diag_.sing_in_check, (long long)diag_.sing_triple,
            (long long)diag_.sing_ttbeta,
            diag_.sing_fired ? 100.0 * double(diag_.sing_in_check) / double(diag_.sing_fired) : 0.0);
        emit(b);
    }

    if (evaluator_.lazy_fires > 0) {
        std::snprintf(buf, sizeof(buf),
            "lazy fires %lld sign_flips %lld crossings %lld absdelta mean %.1f max %lld",
            (long long)evaluator_.lazy_fires, (long long)evaluator_.lazy_sign_flips,
            (long long)evaluator_.lazy_margin_crossings,
            double(evaluator_.lazy_absdelta_sum) / double(evaluator_.lazy_fires),
            (long long)evaluator_.lazy_absdelta_max);
        emit(buf);
    }
}

// 9.3(c): the pool section. Printed by thread 0 AFTER the join (main's own
// search — and its per-thread diag lines — finish before the helpers do), so
// this appends rather than replacing anything. Emitted only at Threads>1.
void Searcher::print_pool_diag(const std::vector<std::unique_ptr<Searcher>>& pool,
                               int thread_count,
                               const std::vector<int>& completed_depths) const {
    if (!info_cb_ || !active_limits_.diag || thread_count <= 1) return;

    DiagCounters total;
    const int counted = std::min<int>(thread_count, static_cast<int>(pool.size()));
    for (int i = 0; i < counted; ++i)
        total.add(pool[static_cast<size_t>(i)]->diag_);

    auto pct = [](int64_t a, int64_t b) {
        return b > 0 ? 100.0 * double(a) / double(b) : 0.0;
    };
    char buf[256];
    auto emit = [&](const char* text) { info_cb_(std::string("info string diag ") + text); };

    const int64_t pool_nodes = total.interior_nodes + total.qs_nodes;
    const int64_t main_nodes = diag_.interior_nodes + diag_.qs_nodes;
    std::snprintf(buf, sizeof(buf),
        "pool threads %d nodes %lld (main %lld = %.1f%%) | main tt %lld/%lld (%.2f%% hit) "
        "pool tt %lld/%lld (%.2f%% hit)",
        thread_count, (long long)pool_nodes, (long long)main_nodes,
        pct(main_nodes, pool_nodes),
        (long long)diag_.tt_hits, (long long)diag_.tt_probes,
        pct(diag_.tt_hits, diag_.tt_probes),
        (long long)total.tt_hits, (long long)total.tt_probes,
        pct(total.tt_hits, total.tt_probes));
    emit(buf);

    // Same-key share: how much of the pool's TT traffic updates an entry for a
    // position the table already holds, versus evicting a different one. This
    // is the quantity 9.5's coordination work moves; read it as a share, never
    // as an absolute.
    std::snprintf(buf, sizeof(buf),
        "pool tt_stores %lld same_key %lld (%.2f%%) | main stores %lld same_key %lld (%.2f%%)",
        (long long)total.tt_stores, (long long)total.tt_stores_same_key,
        pct(total.tt_stores_same_key, total.tt_stores),
        (long long)diag_.tt_stores, (long long)diag_.tt_stores_same_key,
        pct(diag_.tt_stores_same_key, diag_.tt_stores));
    emit(buf);

    // Per-thread completed depth. ⚠ A DIAGNOSTIC, never a verdict: its
    // rep-to-rep spread at fixed time is ~±2 iterations, the same size as any
    // effect worth measuring. Likewise divide aspiration/re-search counts by
    // the thread count before comparing across thread counts.
    std::string depths = "pool depths";
    for (size_t i = 0; i < completed_depths.size(); ++i) {
        depths += (i == 0 ? " main=" : " t" + std::to_string(i) + "=");
        depths += std::to_string(completed_depths[i]);
    }
    depths += "  (diagnostic only: +/-2 iterations rep-to-rep at fixed time)";
    emit(depths.c_str());
}

void Searcher::send_info(int depth, int score, int64_t total_nodes, double elapsed) const {
    std::string line = "info depth " + std::to_string(depth)
        + " seldepth " + std::to_string(sel_depth_)
        + " score ";

    if (std::abs(score) >= MATE_SCORE - MAX_PLY) {
        int mtm = (MATE_SCORE - std::abs(score) + 1) / 2;
        line += "mate " + std::to_string(score > 0 ? mtm : -mtm);
    } else {
        line += "cp " + std::to_string(score);
    }

    int64_t nps = elapsed > 0.0 ? int64_t(double(total_nodes) / elapsed) : 0;
    line += " nodes " + std::to_string(total_nodes)
         + " nps "   + std::to_string(nps)
         + " time "  + std::to_string(int64_t(elapsed * 1000))
         + " tbhits " + std::to_string(current_tbhits())
         + " hashfull " + std::to_string(tt_.hashfull());

    if (pv_len_[0] > 0) {
        Board pv_board = *board_ptr_;
        std::vector<Move> pv_moves;
        int pv_count = std::clamp(pv_len_[0], 0, MAX_PLY);
        for (int i = 0; i < pv_count; i++) {
            Move pv_move = pv_table_[0][i];
            if (!is_legal_move_on_board(pv_board, pv_move))
                break;
            pv_moves.push_back(pv_move);
            pv_board.make_move(pv_move);
        }

        if (!pv_moves.empty()) {
            std::vector<Move> tb_pv = root_tablebase_pv(pv_moves.front());
            if (!tb_pv.empty())
                pv_moves = std::move(tb_pv);
        }

        if (!pv_moves.empty()) {
            line += " pv";
            for (Move pv_move : pv_moves)
                line += ' ' + move_to_uci(pv_move);
        }
    }

    if (info_cb_) info_cb_(line);
}

void Searcher::init_root_tablebase_scores(const Board& board) {
    (void) board;
    root_tb_moves_ = active_limits_.syzygy_root_moves;
    if (!root_tb_moves_.empty() && thread_id_ == 0)
        record_tbhit(static_cast<int64_t>(root_tb_moves_.size()));
}

int Searcher::root_tablebase_score(Move move) const {
    for (const auto& entry : root_tb_moves_) {
        if (entry.bestmove == move)
            return entry.score;
    }
    return VALUE_NONE;
}

int Searcher::root_tablebase_ordering_score(Move move) const {
    for (const auto& entry : root_tb_moves_) {
        if (entry.bestmove == move) {
            return 8'000'000
                 + std::clamp(entry.rank, -2000, 2000) * 1000
                 + std::clamp(entry.score, -tablebaseWinScore, tablebaseWinScore);
        }
    }
    return 0;
}

std::vector<Move> Searcher::root_tablebase_pv(Move move) const {
    for (const auto& entry : root_tb_moves_) {
        if (entry.bestmove == move)
            return entry.pv;
    }
    return {};
}

bool Searcher::root_tablebase_allows(Move move) const {
    if (root_tb_moves_.empty())
        return true;
    return root_tablebase_score(move) != VALUE_NONE;
}

Move Searcher::ponder_from_tt(const Board& root, Move bestmove) const {
    if (!is_legal_move_on_board(root, bestmove))
        return MOVE_NONE;

    Board child = root;
    child.make_move(bestmove);

    TTEntry entry{};
    if (!tt_.probe_copy(child.hash, entry))
        return MOVE_NONE;

    const Move ponder = move_from_tt(entry.move16);
    return is_legal_move_on_board(child, ponder) ? ponder : MOVE_NONE;
}

// ---- Quiescence search -----------------------------------------------------

int Searcher::quiescence(int alpha, int beta, int ply, int qply, SearchStack* ss) {
    record_node();
    if ((nodes_ & 2047) == 0) check_stop();
    if (stopped_) return 0;
    if (ply >= MAX_PLY) return evaluator_.evaluate(*board_ptr_);
    if (board_ptr_->is_draw(ply)) return 0;

    bool in_check = board_ptr_->is_in_check();
    diag_.qs_nodes++;

    // TT probe
    Key hash = board_ptr_->hash;
    TTEntry tte{};
    bool tt_found = tt_.probe_copy(hash, tte);
    diag_.tt_probes++;
    if (tt_found) diag_.tt_hits++;
    Move tt_move = MOVE_NONE;
    int  tt_score = VALUE_NONE;       // hoisted (Step 6.1) for the stand-pat tighten
    TTFlag tt_flag = TT_NONE;
    if (tt_found) {
        tt_move = move_from_tt(tte.move16);
        tt_score = TranspositionTable::score_from_tt(tte.score, ply, board_ptr_->halfmove_clock);
        tt_flag = TTFlag(tte.flag_age & 3);
        if (tt_flag == TT_EXACT) return tt_score;
        if (tt_flag == TT_ALPHA && tt_score <= alpha) return tt_score;
        if (tt_flag == TT_BETA  && tt_score >= beta)  return tt_score;
    }

    if (in_check) {
        // No qsearch-depth cap for evasion nodes: a static eval of an
        // in-check position is not a valid bound and can mask mates in long
        // check chains, poisoning parent TT stores (search audit 7 / 8.1e).
        // The ply >= MAX_PLY guard at the top of quiescence() remains the
        // termination backstop; check chains cannot exceed it.
        MoveList legal;
        board_ptr_->gen_legal(legal);
        int best = -INF_SCORE;
        diag_.qs_evasion_nodes++;
        bool has_legal = false;
        for (Move m : legal) {
            has_legal = true;
            do_move(ss, m);
            int s = -quiescence(-beta, -alpha, ply + 1, qply + 1, ss + 1);
            undo_move(ss, m);
            if (stopped_) return 0;
            if (s > best) best = s;
            if (s > alpha) alpha = s;
            if (alpha >= beta) { best = s; break; }
        }
        return has_legal ? best : -(MATE_SCORE - ply);
    }

    // Stand-pat evaluation
    int raw_eval;
    if (tt_found && tte.static_eval != TranspositionTable::INF_EVAL)
        raw_eval = tte.static_eval;
    else
        raw_eval = evaluator_.evaluate(*board_ptr_);

    int stand_pat = raw_eval;
    stand_pat += correction_value(board_ptr_->side_to_move, *board_ptr_, ss);
    stand_pat = std::clamp(stand_pat, -(MATE_SCORE - 1), MATE_SCORE - 1);

    // Step 6.1 mirror: tighten the stand-pat with the TT bound when it proves a
    // better estimate (a fail-high above it / fail-low below it). The raw eval
    // stored as the TT static_eval (raw_eval) is unchanged.
    if (tt_found && tt_score != VALUE_NONE
        && ((tt_flag == TT_BETA  && tt_score > stand_pat)
            || (tt_flag == TT_ALPHA && tt_score < stand_pat)))
        stand_pat = tt_score;

    if (stand_pat >= beta) {
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
    if (stand_pat < alpha - PIECE_VALUE[QUEEN] - 200)
        return alpha;

    if (stand_pat > alpha) alpha = stand_pat;

    if (qply >= MAX_QSEARCH_PLY) return stand_pat;  // fail-soft (8.1f)

    MoveList captures;
    board_ptr_->gen_legal_captures(captures);

    // Score captures: MVV + cap_hist; prefer TT move
    ScoredMove* sm = move_buffers_[ply][0];
    int nm = 0;
    for (Move m : captures) {
        PieceType atk = type_of(board_ptr_->board_sq[from_sq(m)]);
        PieceType cap = (move_type(m) == EN_PASSANT) ? PAWN : type_of(board_ptr_->board_sq[to_sq(m)]);
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
        const Piece target = board_ptr_->board_sq[to_sq(m)];
        const int captured_value = (mt == EN_PASSANT) ? PIECE_VALUE[PAWN]
                                 : (target != NO_PIECE) ? PIECE_VALUE[type_of(target)]
                                 : 0;
        const int promotion_gain = is_promo ? PIECE_VALUE[promo_type(m)] - PIECE_VALUE[PAWN] : 0;
        const int tactical_gain = captured_value + promotion_gain;

        bool gives_check_known = false;
        bool gives_check = false;
        auto move_gives_check = [&]() {
            if (!gives_check_known) {
                gives_check = board_ptr_->gives_check(m);
                gives_check_known = true;
            }
            return gives_check;
        };

        if (!is_promo
            && stand_pat + tactical_gain + 150 <= alpha
            && !move_gives_check())
            continue;

        const int see_threshold = std::clamp(alpha - stand_pat - 200, -800, 200);
        if (!board_ptr_->see_ge(m, see_threshold))
            continue;

        if (!is_promo && i >= 6 && !board_ptr_->see_ge(m, -50) && !move_gives_check())
            continue;

        do_move(ss, m);
        int s = -quiescence(-beta, -alpha, ply + 1, qply + 1, ss + 1);
        undo_move(ss, m);

        if (stopped_) return 0;
        if (s > alpha) {
            alpha = s;
            best_move = m;
        }
        if (s >= beta) {
            tt_store(hash, 0, s, TT_BETA, m, ply, raw_eval);
            return s;
        }
    }

    // Qsearch quiet checks (Step 6.8): captures didn't cut off. At qply==0
    // only, try quiet checking moves filtered by SEE>=0, capped at
    // qsearch_check_cap (0 = off; the hcefinal SPSA kept it there, and
    // current SF restricts qsearch to captures/evasions -- kept as inert
    // infrastructure only).
    if (qply == 0 && active_limits_.params.qsearch_check_cap > 0) {
        MoveList checks;
        board_ptr_->gen_quiet_checks(checks);
        int tried = 0;
        for (Move m : checks) {
            if (tried >= active_limits_.params.qsearch_check_cap) break;
            if (!board_ptr_->see_ge(m, 0)) continue;
            tried++;

            do_move(ss, m);
            int s = -quiescence(-beta, -alpha, ply + 1, qply + 1, ss + 1);
            undo_move(ss, m);

            if (stopped_) return 0;
            if (s > alpha) {
                alpha = s;
                best_move = m;
            }
            if (s >= beta) {
                tt_store(hash, 0, s, TT_BETA, m, ply, raw_eval);
                return s;
            }
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

int Searcher::negamax(int depth, int alpha, int beta, int ply,
                      SearchStack* ss, bool is_pv, bool allow_null, bool cut_node) {
    record_node();
    if ((nodes_ & 2047) == 0) {
        check_stop();
    }
    if (stopped_) return 0;

    if (ply >= MAX_PLY) return evaluator_.evaluate(*board_ptr_);
    pv_len_[ply] = ply;

    bool is_root = (ply == 0);

    if (!is_root && board_ptr_->is_draw(ply)) return 0;

    if (!is_root && ss->excluded == MOVE_NONE
        && root_tb_moves_.empty()
        && active_limits_.syzygy_probe_depth > 0
        && (ply == 1 || depth >= active_limits_.syzygy_probe_depth)) {
        if (auto wdl = Syzygy::probe_wdl(*board_ptr_,
                                         active_limits_.syzygy_probe_limit,
                                         active_limits_.syzygy_50_move_rule)) {
            record_tbhit();
            const int tb_score = score_from_syzygy_wdl(*wdl);
            tt_store(board_ptr_->hash, depth, tb_score, TT_EXACT, MOVE_NONE, ply,
                      TranspositionTable::INF_EVAL);
            return tb_score;
        }
    }

    bool in_check = board_ptr_->is_in_check();

    // Check extension: when the side to move is in check, extend by 1 ply.
    // Guard with ss->excluded to prevent stacking with singular extensions.
    diag_.interior_nodes++;
    if (in_check) diag_.in_check_nodes++;
    // The extension is unconditional: every in-check node gets a ply.
    //
    // 5.7.6 removed check_ext_path_cap, which bounded the accumulation per path
    // and defaulted to 0 (disabled). It was added inert for 5.4.4, and that
    // cluster closed with BAS-S16 REJECTED at -3.48 +/- 3.32 -- so it was the
    // residue of a failed trial, not an avenue still open. 5.7.3 separately
    // measured that reducing extension at checking nodes fails our WAC floor.
    bool did_check_ext = false;
    if (in_check && ss->excluded == MOVE_NONE && ply < MAX_PLY - 2) {
        depth++;
        did_check_ext = true;
        diag_.check_exts++;
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
    Key hash     = board_ptr_->hash;
    TTEntry tte{};
    bool tt_found = tt_.probe_copy(hash, tte);
    diag_.tt_probes++;
    if (tt_found) diag_.tt_hits++;

    Move  tt_move  = MOVE_NONE;
    int   tt_score = VALUE_NONE;
    int   tt_depth = 0;
    TTFlag tt_flag  = TT_NONE;

    if (tt_found) {
        tt_move  = move_from_tt(tte.move16);
        tt_score = TranspositionTable::score_from_tt(tte.score, ply, board_ptr_->halfmove_clock);
        // depth is int8_t with a deliberate -1 sentinel; tidy's suggested
        // unsigned cast would corrupt it.
        // NOLINTNEXTLINE(bugprone-signed-char-misuse)
        tt_depth = tte.depth;
        tt_flag  = TTFlag(tte.flag_age & 3);

        if (!is_pv && ss->excluded == MOVE_NONE && tt_depth >= depth) {
            if (tt_flag == TT_EXACT
                || (tt_flag == TT_ALPHA && tt_score <= alpha)
                || (tt_flag == TT_BETA  && tt_score >= beta)) {
                diag_.tt_cutoffs++;
                return tt_score;
            }
        }
    }

    ss->tt_pv = is_pv || (tt_found && tt_flag == TT_EXACT && tt_depth >= depth - 1);
    if (ss->tt_pv) diag_.tt_pv_nodes++;

    // Phase 6.7: is the TT move a capture? (LMR input, lmr_tt_capture)
    const bool tt_capture = tt_move != MOVE_NONE
        && (board_ptr_->board_sq[to_sq(tt_move)] != NO_PIECE
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
            raw_static_eval = evaluator_.evaluate(*board_ptr_);

        // TT stores the raw static eval; correction is applied at probe time.
        static_eval = raw_static_eval;
        static_eval += correction_value(board_ptr_->side_to_move, *board_ptr_, ss);
        static_eval  = std::clamp(static_eval, -(MATE_SCORE - 1), MATE_SCORE - 1);
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
        && std::abs(tt_score) < MATE_SCORE - MAX_PLY
        && (tt_flag == TT_EXACT
            || (tt_flag == TT_BETA  && tt_score > static_eval)
            || (tt_flag == TT_ALPHA && tt_score < static_eval)))
        eval = tt_score;

    // Improving: eval is better than 2 plies ago
    bool improving = !in_check && ply >= 2
                   && (ss-2)->eval != VALUE_NONE
                   && static_eval > (ss-2)->eval;

    // ---- Non-PV pruning (skip if in check, in PV, or singular search) ------
    if (!is_pv && !in_check && ss->excluded == MOVE_NONE
        && static_eval != VALUE_NONE) {

        // Reverse futility pruning
        if (depth <= 9) {
            const auto& p = active_limits_.params;
            int margin = p.rfp_coeff * depth - (improving ? p.rfp_improving : 0);
            if (eval - margin >= beta) {
                diag_.rfp_cuts++;
                return eval;
            }
        }

        // Razoring
        if (depth <= 3 && eval + active_limits_.params.razor_coeff * depth <= alpha) {
            int q = quiescence(alpha, beta, ply, 0, ss);
            if (q <= alpha) {
                diag_.razor_cuts++;
                return q;
            }
        }

        // Null-move pruning
        if (allow_null && depth >= 3
            && eval >= beta
            && board_ptr_->has_non_pawn_material(board_ptr_->side_to_move)
            && (ss-1)->move != MOVE_NULL) {

            int r = active_limits_.params.null_base + depth / 4
                  + std::min((eval - beta) / active_limits_.params.null_eval_div, 3);
            diag_.null_tries++;
            do_null_move(ss);
            tt_.prefetch(board_ptr_->hash);   // 8.7.6(c)
            int null_score = -negamax(std::max(0, depth - r), -beta, -(beta - 1),
                                      ply + 1, ss + 1, false, false, true);
            undo_null_move(ss);
            if (stopped_) return 0;
            if (null_score >= beta) {
                if (null_score >= MATE_SCORE - MAX_PLY) null_score = beta;
                bool verified = true;
                if (depth >= 10) {
                    const int verify_depth = std::max(1, depth - r);
                    const int verify_score = negamax(verify_depth, beta - 1, beta,
                                                     ply, ss, false, false, false);
                    if (stopped_) return 0;
                    verified = verify_score >= beta;
                }
                if (verified) {
                    diag_.null_cuts++;
                    return null_score;
                }
            }
        }

        // ProbCut: if a capture is likely to fail high at reduced depth
        if (depth >= 5 && std::abs(beta) < MATE_SCORE - MAX_PLY) {
            int pc_beta = std::min(beta + active_limits_.params.probcut_margin,
                                   MATE_SCORE - MAX_PLY - 1);
            MoveList pcaps;
            board_ptr_->gen_legal_captures(pcaps);
            for (Move m : pcaps) {
                if (m == ss->excluded) continue;
                if (!board_ptr_->see_ge(m, pc_beta - static_eval)) continue;

                diag_.probcut_tries++;
                do_move(ss, m);
                tt_.prefetch(board_ptr_->hash);   // 8.7.6(c)
                // Quick check via QSearch first
                int val = -quiescence(-pc_beta, -pc_beta + 1, ply + 1, 0, ss + 1);
                if (val >= pc_beta)
                    val = -negamax(depth - 4, -pc_beta, -pc_beta + 1,
                                   ply + 1, ss + 1, false, true, true);
                undo_move(ss, m);
                if (stopped_) return 0;
                if (val >= pc_beta) {
                    tt_store(hash, depth - 3, pc_beta, TT_BETA, m, ply,
                              raw_static_eval == VALUE_NONE
                                  ? TranspositionTable::INF_EVAL : raw_static_eval);
                    diag_.probcut_cuts++;
                    return pc_beta;
                }
            }
        }
    }

    // IIR: reduce non-PV nodes when no TT move (or a stale TT entry) guides the search.
    if (!is_pv && depth >= 4 && (tt_move == MOVE_NONE || (tt_found && tt_depth < depth - 3)))
        depth--;

    int  orig_alpha  = alpha;
    Move best_move   = MOVE_NONE;
    int  best_score  = -INF_SCORE;
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
    const auto& main_hist = hist_.main[board_ptr_->side_to_move];
    const auto& pawn_hist = hist_.pawn->data[
        board_ptr_->pawn_key & (HistoryTables::PAWN_HIST_SIZE - 1)];
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
        if (is_root && !move_in_root_moves(m, active_limits_.root_moves))
            return false;
        if (is_root && root_filter_index_ >= 0) {
            const int ordinal = root_ordinal++;
            if ((ordinal % root_filter_count_) != root_filter_index_)
                return false;
        }
        if (is_root && !root_tablebase_allows(m))
            return false;

        bool is_cap   = (board_ptr_->board_sq[to_sq(m)] != NO_PIECE)
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
            assert(see_score == (board_ptr_->see_ge(m, 0) ? 0 : -1)
                   && "8.7.5(a) memoized see_score disagrees with a fresh see_ge");
#endif
        bool gives_check_known = false;
        bool gives_check = false;
        auto move_gives_check = [&]() {
            if (!gives_check_known) {
                gives_check = board_ptr_->gives_check(m);
                gives_check_known = true;
            }
            return gives_check;
        };

        // ---- Late-move pruning / futility ----------------------------------
        if (!is_root && searched > 0 && best_score > -(MATE_SCORE - MAX_PLY)) {

            // Reduction-aware depth for the shallow-pruning heuristics (Step
            // 6.5): the base LMR-table reduction, matching SF/Ethereal's use of
            // lmrDepth here. The history/pv refinements of the real reduction
            // are a second-order effect on the pruning decision, so the cheap
            // base estimate is enough (and avoids hoisting the full reduction).
            int base_r = lmr_table_[std::min(depth, 63)][std::min(searched, 63)] >> 10;  // 1024ths -> plies (6.7)
            int lmr_depth = std::clamp(depth - base_r, 0, depth);

            if (is_quiet) {
                // Futility pruning
                if (!is_pv && !in_check && depth <= 6
                    && eval != VALUE_NONE
                    && eval + active_limits_.params.futility_base
                            + active_limits_.params.futility_coeff * depth <= alpha
                    && !move_gives_check()) {
                    diag_.fut_prunes++;
                    return false;
                }

                // Late move pruning (LMP) — never in PV
                if (!is_pv && !in_check && depth <= 6 && searched >= lmp_thresh
                    && !move_gives_check()) {
                    diag_.lmp_prunes++;
                    return false;
                }

                // History pruning: skip moves with very bad combined history
                if (!is_pv && depth <= 6) {
                    PieceType pt = type_of(board_ptr_->board_sq[from_sq(m)]);
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
                            int64_t(active_limits_.params.hist_prune_coeff) * depth;
                        ++diag_.hist_prune_tested;
                        if (hist < -(thr / 2)) ++diag_.hist_below_half;
                        if (hist < -(thr / 4)) ++diag_.hist_below_quarter;
                        if (hist < -(thr / 8)) ++diag_.hist_below_eighth;
                    }
                    if (hist < -active_limits_.params.hist_prune_coeff * depth && !move_gives_check()) {
                        diag_.hist_prunes++;
                        return false;
                    }
                }

                // SEE pruning of quiet moves (Step 6.5): skip quiets that lose
                // material by SEE, margin scaling with lmr_depth². EXPOSED BUT
                // INERT — quiet_see_depth defaults to 0 so `depth <= 0` never
                // fires (a naive base-table lmr_depth broke KBNK; needs SF's
                // history-aware lmr_depth, deferred to 6.9). See SearchParams.h.
                if (!is_pv && depth <= active_limits_.params.quiet_see_depth
                    && !move_gives_check()
                    && !board_ptr_->see_ge(
                           m, -active_limits_.params.quiet_see_coeff * lmr_depth * lmr_depth))
                    return false;
            } else if (is_cap) {
                // Capture futility pruning (Step 6.5): if even winning the
                // captured piece cannot lift the static eval to alpha, skip the
                // capture at shallow lmr_depth. Good captures (high cap_hist)
                // are spared via the capture-history term. EXPOSED BUT INERT —
                // cap_fut_depth defaults to 0 so `lmr_depth < 0` never fires
                // (SPRT'd active at -2.78 Elo, reverted; re-enable in 6.9).
                if (!is_pv && eval != VALUE_NONE && lmr_depth < active_limits_.params.cap_fut_depth
                    && !move_gives_check()) {
                    PieceType atk = type_of(board_ptr_->board_sq[from_sq(m)]);
                    PieceType captured = (move_type(m) == EN_PASSANT)
                                       ? PAWN : type_of(board_ptr_->board_sq[to_sq(m)]);
                    int fut = eval + active_limits_.params.cap_fut_base
                            + active_limits_.params.cap_fut_coeff * lmr_depth
                            + PIECE_VALUE[captured]
                            + hist_.capture[atk][to_sq(m)][captured] / 32;
                    if (fut <= alpha)
                        return false;
                }

                // SEE pruning for bad captures
                if (!is_pv && depth <= 8 && !is_promo) {
                    if (!board_ptr_->see_ge(m, -depth * active_limits_.params.see_prune_coeff) && !move_gives_check()) {
                        diag_.see_prunes++;
                        return false;
                    }
                }
            }
        }

        if (is_cap && !is_promo && depth >= 2 && searched >= 2 && see_score == VALUE_NONE)
            see_score = board_ptr_->see_ge(m, 0) ? 0 : -1;

        // ---- Extensions -------------------------------------------------------
        int extension = 0;

        // ---- Singular extension (only for TT move) -------------------------
        if (!is_root && m == tt_move && ss->excluded == MOVE_NONE
            && depth >= active_limits_.params.singular_min_depth
            && tt_found && tt_depth >= depth - 3
            && (tt_flag == TT_BETA || tt_flag == TT_EXACT)
            && std::abs(tt_score) < MATE_SCORE - MAX_PLY) {

            int s_beta  = tt_score - active_limits_.params.singular_beta_mult * depth;
            int s_depth = (depth - 1) / 2;

            ss->excluded = m;
            int s_val = negamax(s_depth, s_beta - 1, s_beta, ply, ss, false, false, true);
            ss->excluded = MOVE_NONE;

            if (stopped_) {
                immediate_return = true;
                immediate_score = 0;
                return true;
            }

            if (s_val < s_beta) {
                // TT move is singular — extend it. Phase 6.4 rider: cap stacked
                // 2-ply extensions along this path so a pathological line can't
                // chain unbounded double-extensions.
                bool allow_double = !is_pv
                    && s_val < s_beta - active_limits_.params.singular_double_margin
                    && ss->double_exts < active_limits_.params.double_ext_max;
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

                // 5.7.3 probe: count the stack, do not change it yet.
                ++diag_.sing_fired;
                if (allow_double)   ++diag_.sing_double;
                if (did_check_ext)  ++diag_.sing_in_check;
                if (allow_double && did_check_ext) ++diag_.sing_triple;

                // 5.7.2: relax LMR for this node's remaining moves. Suppressed
                // when the TT move is a capture -- a singular capture says the
                // tactics are forced, not that the quiet alternatives are
                // delicate, and `lmr_tt_capture` already raises r for exactly
                // that case. Letting both fire would have them cancel.
                singular_quiet_lmr = !tt_capture;
            } else if (s_beta >= beta) {
                // Multicut: likely to fail high without this move too
                immediate_return = true;
                immediate_score = s_beta;
                return true;
            } else if (tt_score >= beta) {
                ++diag_.sing_ttbeta;

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

        PieceType moved_pt = type_of(board_ptr_->board_sq[from_sq(m)]);
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
        const int64_t nodes_before_move = nodes_;
        do_move(ss, m);
        tt_.prefetch(board_ptr_->hash);
        sel_depth_ = std::max(sel_depth_, ply + 1);

        int new_depth = depth - 1 + extension;
        // Phase 6.4 rider: propagate the stacked double-extension count to the
        // child so a chain of singular double-extensions is eventually capped.
        (ss + 1)->double_exts = ss->double_exts + (extension >= 2 ? 1 : 0);

        int score;
        if (searched == 0) {
            score = -negamax(new_depth, -beta, -alpha, ply + 1, ss + 1, is_pv, true, false);
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
            ++diag_.lmr_eligible;
            const bool lmr_type_ok = is_quiet || (is_cap && !is_promo && see_score < 0);
            if (depth < 2)          ++diag_.lmr_blocked_depth;
            else if (searched < 2)  ++diag_.lmr_blocked_searched;
            else if (in_check)      ++diag_.lmr_blocked_in_check;
            else if (!lmr_type_ok)  ++diag_.lmr_blocked_movetype;
            // Checking moves are never reduced. 5.7.6 removed the
            // lmr_allow_check switch that could have relaxed this: it was added
            // inert for 5.4.4, which closed rejected (BAS-S16).
            else if (move_gives_check())
                ++diag_.lmr_blocked_gives_check;
            // LMR applies to: quiets, and bad captures — but NOT promotions
            else {
                // Phase 6.7: accumulate the reduction in 1024ths of a ply, then
                // shift back at the end. Behaviour-identical at default knobs
                // (adjustments are the old integer values ×1024; history stays
                // integer-quantised via the ×1024-after-divide form).
                int r = lmr_table_[std::min(depth, 63)][std::min(searched, 63)];

                if (is_quiet) {
                    const auto& p = active_limits_.params;
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
                if ((r >> 10) > new_depth - 1) ++diag_.lmr_clamped_high;
                reduction = std::clamp(r >> 10, 0, new_depth - 1);
                // 5.2: the gate passed but the computed reduction was zero —
                // distinct from being blocked, and a different repair. Counted
                // here so that
                //   eligible = applied + clamped_zero + sum(blocked_*)
                // holds exactly, which is what makes the breakdown auditable.
                if (reduction == 0) ++diag_.lmr_clamped_zero;
            }
            ss->reduction = reduction;
            if (reduction > 0) {
                diag_.lmr_applied++;
                // Mean reduction over applied = reduction_plies / applied. A
                // timid-LMR hypothesis is decided by this number, not by how
                // often LMR fired.
                diag_.lmr_reduction_plies += reduction;
            }

            score = -negamax(new_depth - reduction, -alpha - 1, -alpha,
                             ply + 1, ss + 1, false, true, true);
            // Re-search at full depth if LMR didn't fail low
            if (reduction > 0 && score > alpha && !stopped_) {
                diag_.lmr_researched++;
                score = -negamax(new_depth, -alpha - 1, -alpha,
                                 ply + 1, ss + 1, false, true, !cut_node);

                // Post-LMR continuation-history nudge (Step 6.4, Weiss form,
                // reusing the 6.3 bonus/malus formulas, scaled by
                // post_lmr_hist_scale -- see SearchParams.h for why it
                // defaults to 0/provably inert). Reward or punish this quiet
                // move's continuation history based on whether the
                // confirmation score actually held up against the original
                // window.
                if (is_quiet && !stopped_ && active_limits_.params.post_lmr_hist_scale > 0) {
                    int scale = active_limits_.params.post_lmr_hist_scale;
                    if (score >= beta)
                        update_cont_for_move(ss, moved_pt, Square(to_sq(m)),
                                             history_bonus_value(depth) * scale / 100);
                    else if (score <= alpha)
                        update_cont_for_move(ss, moved_pt, Square(to_sq(m)),
                                             history_malus_value(depth) * scale / 100);
                }
            }
            // Re-search as PV if score is within window
            if (is_pv && score > alpha && score < beta && !stopped_)
                score = -negamax(new_depth, -beta, -alpha,
                                 ply + 1, ss + 1, true, true, false);
        }

        undo_move(ss, m);

        if (stopped_)
            return true;

        searched++;
        const int64_t move_nodes = nodes_ - nodes_before_move;
        if (is_root) {
            root_depth_nodes_ += std::max<int64_t>(0, move_nodes);
            // 8.6.10e bookkeeping (no consumer yet — see RootMoveStat).
            RootMoveStat& rs = root_stat(m);
            rs.nodes   += std::max<int64_t>(0, move_nodes);
            rs.seldepth = std::max(rs.seldepth, sel_depth_);
            rs.exact    = score > alpha && score < beta;
            rs.add_sample(score);
        }

        // Track for history updates
        if (is_cap && !is_promo) {
            if (see_score == VALUE_NONE)
                see_score = board_ptr_->see_ge(m, 0) ? 0 : -1;
            if (see_score < 0 && bad_caps_count < MAX_TRACKED_BAD_CAPS)
                bad_caps_searched[bad_caps_count++] = m;
        } else if (is_quiet && quiets_count < MAX_TRACKED_QUIETS) {
            quiets_searched[quiets_count++] = m;
        }

        if (score > best_score) {
            if (is_root)
                root_best_nodes_ = std::max<int64_t>(0, move_nodes);
            best_score = score;
            best_move  = m;
            if (score > alpha) {
                alpha = score;
                // Update PV
                pv_table_[ply][ply] = m;
                int child_pv_len = std::clamp(pv_len_[ply + 1], ply + 1, MAX_PLY);
                for (int k = ply + 1; k < child_pv_len; k++)
                    pv_table_[ply][k] = pv_table_[ply + 1][k];
                pv_len_[ply] = child_pv_len;
                if (is_root)
                    root_stat(m).pv.assign(&pv_table_[0][0],
                                           &pv_table_[0][0] + pv_len_[0]);
            }
        }

        if (alpha >= beta) {
            // 5.2 (BAS-O03): ordering quality at the point it costs something.
            // `searched` was incremented above, so the cutting move's index is
            // searched - 1. A cutoff on index 0 costs one move's search; on
            // index n it costs n+1, so the mean index is a direct multiplier on
            // tree width — the quantity separating our 2.20 EBF from ~1.61.
            // cutoff_src says which picker stage to fix rather than merely that
            // ordering is imperfect.
            ++diag_.fail_highs;
            diag_.fail_high_index_sum += searched - 1;
            if (searched == 1) ++diag_.fail_high_first;
            switch (picker_src) {
                case MovePicker::Src::TT:           ++diag_.cutoff_src_tt; break;
                case MovePicker::Src::GoodTactical: ++diag_.cutoff_src_good_tactical; break;
                case MovePicker::Src::Quiet:        ++diag_.cutoff_src_quiet; break;
                case MovePicker::Src::BadTactical:  ++diag_.cutoff_src_bad_tactical; break;
                case MovePicker::Src::None:         break;
            }
            // 8.5.10(e): boost the bonus when the cutoff was "surprising" -- the
            // node's static eval was below beta, so the search found a good move
            // the eval did not credit.
            const int es = (static_eval != VALUE_NONE && static_eval < beta) ? 125 : 100;
            update_all_histories(m, m == tt_move, quiets_searched, quiets_count,
                                 bad_caps_searched, bad_caps_count,
                                 board_ptr_->side_to_move, depth, ss,
                                 /*reward_only=*/false, /*bonus_scale=*/es);
            return true;
        }

        return false;
    };

    // ---- Staged move picking -----------------------------------------------
    // TT move first, then tactical moves, then quiet moves. Quiet generation and
    // scoring are delayed until captures/promotions fail to produce a cutoff.
    MovePicker picker(*this, tt_move, ss->excluded, ss, is_root, ply,
                      move_buffers_[ply][0], move_buffers_[ply][1]);
    while (true) {
        Move move = picker.next();
        if (move == MOVE_NONE)
            break;
        if (search_one(move, picker.last_see_score(), picker.last_source()))
            break;
    }

    if (immediate_return)
        return immediate_score;

    if (stopped_)
        return (is_root && best_move != MOVE_NONE) ? best_score : 0;

    // No legal moves
    if (searched == 0)
        return in_check ? -(MATE_SCORE - ply) : 0;

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
                             board_ptr_->side_to_move, depth, ss,
                             /*reward_only=*/true);
    }

    // Update correction history with search result
    if (!in_check && ss->excluded == MOVE_NONE && static_eval != VALUE_NONE
        && std::abs(best_score) < MATE_SCORE - MAX_PLY
        && (best_score >= beta || best_score > orig_alpha)) {
        update_correction(board_ptr_->side_to_move, *board_ptr_, ss,
                          best_score - static_eval, depth);
    }

    // Store to TT
    TTFlag flag = (best_score >= beta)    ? TT_BETA
                : (best_score > orig_alpha) ? TT_EXACT
                :                             TT_ALPHA;
    if (ss->excluded == MOVE_NONE)
        tt_store(hash, depth, best_score, flag, best_move, ply,
                  raw_static_eval == VALUE_NONE ? TranspositionTable::INF_EVAL : raw_static_eval);

    return best_score;
}

// ---- Iterative deepening ---------------------------------------------------

SearchResult Searcher::search(Board board, const SearchLimits& limits) {
    board_ptr_    = &board;
    nodes_        = 0;
    tb_hits_      = 0;
    nodes_limit_  = limits.nodes;
    shared_nodes_flushed_ = 0;   // 9.3(b): per-search batching state
    shared_nodes_total_   = 0;
    sel_depth_    = 0;
    stopped_      = false;
    root_filter_count_ = std::max(1, limits.root_filter_count);
    root_filter_index_ = (limits.root_filter_index >= 0
                          && limits.root_filter_index < root_filter_count_)
                       ? limits.root_filter_index
                       : -1;
    thread_id_    = std::max(0, limits.thread_id);
    root_table_   = limits.root_table;
    pondering_    = limits.ponder;
    active_limits_ = limits;
    root_side_    = board.side_to_move;

    init_lmr(static_cast<float>(active_limits_.params.lmr_base)    / 100.0f,
             static_cast<float>(active_limits_.params.lmr_divisor) / 100.0f);

    // Step 5.4: start the clock at the `go`-receipt instant (captured in
    // UciProtocol::cmdGo and threaded via SearchLimits.go_recv_time), not here at
    // the worker's search entry. This makes elapsed_seconds() account for the
    // command-dispatch + thread-handoff latency the GUI already charges (5.3
    // measured up to ~20 ms at bullet under load) instead of giving it away,
    // tightening the engine against the GUI clock. Falls back to now() for
    // internal/bench calls where go_recv_time is unset (so bench is unaffected).
    start_time_ = (limits.go_recv_time.time_since_epoch().count() != 0)
                ? limits.go_recv_time
                : std::chrono::steady_clock::now();
    const int game_ply = 2 * (board.fullmove_number - 1) + (board.side_to_move == BLACK ? 1 : 0);
    compute_time_limit(limits, board.side_to_move, game_ply);

    if (limits.update_tt_age)
        tt_.new_search();
    if (++history_age_counter_ >= 2) {
        age_history();
        history_age_counter_ = 0;
    }

    // Initialize search stack sentinels
    for (auto& s : ss_arr_) s = SearchStack{};
    for (int i = 0; i < 4; i++) {
        ss_arr_[i].move        = MOVE_NONE;
        ss_arr_[i].moved_piece = NO_PIECE_TYPE;
        ss_arr_[i].eval        = VALUE_NONE;
    }
    SearchStack* ss = ss_arr_ + 4; // root at offset 4

    std::memset(pv_len_, 0, sizeof(pv_len_));
    init_root_tablebase_scores(board);

    SearchResult result;
    int prev_score      = 0;
    Move prev_best      = MOVE_NONE;
    int  best_stability = 0;   // how many consecutive depths best move hasn't changed
    double best_move_changes = 0.0;  // 8.5.12: decaying count of root best-move flips

    int max_depth = limits.infinite ? MAX_SEARCH_DEPTH
                  : std::min(limits.depth, MAX_SEARCH_DEPTH);

    int start_depth = 1;
    root_stats_.clear();   // fresh records per `go` (8.6.10e)
    diag_.reset();         // fresh diagnostic counters per `go` (8.6.6)
    evaluator_.diag_lazy = active_limits_.diag;
    evaluator_.lazy_fires = evaluator_.lazy_sign_flips = 0;
    evaluator_.lazy_margin_crossings = 0;
    evaluator_.lazy_absdelta_sum = evaluator_.lazy_absdelta_max = 0;
    // 8.7.1(c) speed telemetry: fresh per `go`. The Board counters are reset
    // through board_ptr_ because the Board arrived by value from a caller
    // whose own counters may be stale.
    evaluator_.eval_calls = 0;
    evaluator_.pawn_probes = evaluator_.pawn_hits = 0;
#ifdef BASILISK_TUNE
    evaluator_.diag_endgames = active_limits_.diag;
    evaluator_.endgame_occurrence.reset();
#endif
    if (board_ptr_) {
        board_ptr_->diag_see_ge_calls = 0;
        board_ptr_->diag_gives_check_calls = 0;
    }

    if (thread_id_ > 0 && max_depth > 2)
        start_depth = 1 + (thread_id_ % 2);

    for (int depth = start_depth; depth <= max_depth && !stopped_; depth++) {
        pv_len_[0] = 0;
        root_depth_nodes_ = 0;
        root_best_nodes_ = 0;
        root_best_effort_ = 0;
        int score;

        if (depth <= 3 || std::abs(prev_score) >= MATE_SCORE - MAX_PLY) {
            score = negamax(depth, -INF_SCORE, INF_SCORE, 0, ss, true, true, false);
        } else {
            int delta = active_limits_.params.aspiration_delta;
            int asp_a = prev_score - delta;
            int asp_b = prev_score + delta;
            ++diag_.asp_windows;
            while (true) {
                // 5.8.5 REFUTED: the reference re-searches SHALLOWER after each
                // fail-high (failedHighCnt). Measured: WAC 137 -> 119 against a
                // floor of 130, and re-searches ROSE 1305 -> 1450. A root
                // failing high is often a tactical shot, and searching it
                // shallower misses it. Full depth every time. (BAS-D17)
                score = negamax(depth, asp_a, asp_b, 0, ss, true, true, false);
                if (stopped_) break;
                if (score <= asp_a) {
                    ++diag_.asp_fail_low;
                    ++diag_.asp_researches;
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
                    ++diag_.asp_fail_high;
                    ++diag_.asp_researches;
                    asp_b  = std::min(score + delta, INF_SCORE);
                    delta += delta / 2;
                } else {
                    break;
                }
                if (delta >= 900) {
                    ++diag_.asp_giveup;
                    ++diag_.asp_researches;
                    asp_a = -INF_SCORE;
                    asp_b =  INF_SCORE;
                    score = negamax(depth, asp_a, asp_b, 0, ss, true, true, false);
                    break;
                }
            }
        }

        if (stopped_ && depth > 1) break;

        if (root_depth_nodes_ > 0)
            root_best_effort_ = static_cast<int>(
                std::min<int64_t>(100, root_best_nodes_ * 100 / root_depth_nodes_));

        // Track best-move stability for adaptive soft time limit
        Move cur_best = (pv_len_[0] > 0) ? pv_table_[0][0] : MOVE_NONE;

        int reported_score = score;
        if (cur_best != MOVE_NONE) {
            const int tb_score = root_tablebase_score(cur_best);
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

        if (pv_len_[0] > 0) {
            result.bestmove   = pv_table_[0][0];
            result.pondermove = (pv_len_[0] > 1) ? pv_table_[0][1] : MOVE_NONE;
            std::vector<Move> tb_pv = root_tablebase_pv(result.bestmove);
            if (tb_pv.size() > 1)
                result.pondermove = tb_pv[1];
            if (result.pondermove == MOVE_NONE)
                result.pondermove = ponder_from_tt(board, result.bestmove);
        }
        result.score = reported_score;
        result.depth = depth;

        // 5.8.6: the table is given the RAW `score`, not the tablebase-
        // corrected `reported_score` that goes out over UCI. That is deliberate
        // and not a bug: the table's scores exist to ORDER root moves for the
        // next iteration and for helper threads, and a TB-corrected value is a
        // fixed mate/draw verdict that carries no ordering information. Stating
        // it here because the asymmetry two lines apart reads as an oversight.
        if (root_table_ && result.bestmove != MOVE_NONE)
            root_table_->update(result.bestmove, result.pondermove, depth, score);

        double elapsed = elapsed_seconds();
        send_info(depth, reported_score, current_nodes(), elapsed);

        // Adaptive soft time limit:
        // The more stable the best move, the less time we need to confirm it.
        // stability=0 → 100% of soft, stability=6+ → ~64% of soft
        // A significant score drop signals instability — extend time budget.
        // 9.4(a): ONLY the main thread owns a clock. Helpers run until `stop_`,
        // which the pool sets when main returns. Keep this gate outside the
        // whole time-management calculation so the restored 9.4 baseline is
        // source- and code-shape-identical in this path.
        if (soft_limit_ > 0.0 && !pondering_ && thread_id_ == 0) {
            // Step 5.8: the scaling constants below are SPSA-tunable
            // (active_limits_.params, defaults == the baked values).
            const SearchParams& tp = active_limits_.params;
            double stability_scale = 1.0 - (tp.tm_stability / 1000.0) * std::min(best_stability, 6);
            // Score-based time extension: if score dropped enough, take more time
            int score_drop = prev_score_saved - score;
            double score_scale = (depth > 4 && score_drop > tp.tm_scoredrop_thr)
                               ? 1.0 + std::min(score_drop - tp.tm_scoredrop_thr, 120)
                                       / static_cast<double>(tp.tm_scoredrop_div)
                               : 1.0;
            double effort_scale = (depth > 5 && root_best_effort_ >= tp.tm_effort_hi) ? tp.tm_effort_hi_mult / 100.0
                                : (depth > 5 && root_best_effort_ <= tp.tm_effort_lo) ? tp.tm_effort_lo_mult / 100.0
                                : 1.0;
            // 8.5.12: instability extension — a thrashing root best move raises
            // the threshold (buys more time), complementing stability_scale.
            double instability_scale = 1.0 + std::min(best_move_changes, 2.0) * (tp.tm_instability / 100.0);
            if (elapsed >= soft_limit_ * stability_scale * score_scale
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
    // 8.7.1(c): harvest the Board-side speed counters BEFORE board_ptr_ is
    // dropped — print_diag() runs after this point.
    diag_.see_ge_calls      = board.diag_see_ge_calls;
    diag_.gives_check_calls = board.diag_gives_check_calls;
    board_ptr_ = nullptr;
    root_table_ = nullptr;
    pondering_ = false;
    root_tb_moves_.clear();
    result.nodes      = nodes_;
    result.tbhits     = tb_hits_;
    result.elapsed_ms = int64_t(elapsed_seconds() * 1000.0);

    // Step 5.3 diagnostic: one line per move with the time budget, the actual
    // elapsed, and the go-receipt -> search-start dispatch latency the GUI
    // charges but elapsed_seconds() (clock starts at start_time_) does not yet
    // count. Emitted only on the reporting thread (info_cb_ set) and only with
    // the hidden TM_Debug option on, so play/bench are unaffected when off.
    if (info_cb_ && active_limits_.diag)
        print_diag();

    if (info_cb_ && active_limits_.tm_debug) {
        long long dispatch_ms = -1;
        if (active_limits_.go_recv_time.time_since_epoch().count() != 0)
            dispatch_ms = int64_t(std::chrono::duration<double, std::milli>(
                              start_time_ - active_limits_.go_recv_time).count());
        info_cb_("info string tm soft_ms=" + std::to_string(int64_t(soft_limit_ * 1000.0))
               + " hard_ms="     + std::to_string(int64_t(hard_limit_ * 1000.0))
               + " elapsed_ms="  + std::to_string(int64_t(elapsed_seconds() * 1000.0))
               + " dispatch_ms=" + std::to_string(dispatch_ms));
    }
    return result;
}

// ---- Persistent Lazy SMP thread pool ---------------------------------------

SearchThreadPool::SearchThreadPool(TranspositionTable& tt,
                                   std::atomic_bool& stop_flag,
                                   std::function<void(const std::string&)> info_cb,
                                   std::atomic_bool* ponderhit_flag)
    : tt_(tt)
    , stop_(stop_flag)
    , ponderhit_(ponderhit_flag)
    , info_cb_(std::move(info_cb)) {
    resize_threads(1);
}

SearchThreadPool::~SearchThreadPool() {
    {
        std::scoped_lock lock(mutex_);
        shutdown_ = true;
        ++epoch_;
    }
    work_cv_.notify_all();

    for (std::thread& worker : workers_) {
        if (worker.joinable())
            worker.join();
    }
}

int SearchThreadPool::ensure_threads(int count) {
    return resize_threads(count);
}

// The ONE definition of the Threads cap (declared in constants.h). Flat 1024,
// as Stockfish does it — see constants.h for why the machine is not consulted.
// What 9.3(a) actually repaired is that this existed TWICE, computed
// independently in two files as `max(1024, 4*hw)` where `min` was meant, so
// the advertisement and the pool's real limit could drift apart. That is the
// part worth keeping; the value itself is a policy choice.
int max_search_threads() {
    return maxSearchThreads;
}

int SearchThreadPool::normalize_thread_count(int count) {
    count = std::max(1, count);
    return std::min(count, max_search_threads());
}

int SearchThreadPool::active_thread_count() const {
    std::scoped_lock lock(mutex_);
    return static_cast<int>(searchers_.size());
}

int SearchThreadPool::resize_threads(int count) {
    count = normalize_thread_count(count);

    bool already_exact = false;
    {
        std::scoped_lock lock(mutex_);
        already_exact = !shutdown_
            && std::cmp_equal(searchers_.size(), count)
            && static_cast<int>(workers_.size()) + 1 == count;
    }
    if (already_exact)
        return count;

    {
        std::scoped_lock lock(mutex_);
        shutdown_ = true;
        ++epoch_;
    }
    work_cv_.notify_all();

    for (std::thread& worker : workers_) {
        if (worker.joinable())
            worker.join();
    }

    {
        std::scoped_lock lock(mutex_);
        workers_.clear();
        searchers_.clear();
        job_results_ = nullptr;
        job_root_table_ = nullptr;
        requested_helpers_ = 0;
        active_helpers_ = 0;
        shutdown_ = false;
        epoch_ = 0;
    }

    while (std::cmp_less(searchers_.size(), count)) {
        const bool emit_info = searchers_.empty();
        auto cb = emit_info ? info_cb_ : std::function<void(const std::string&)>();
        searchers_.push_back(std::make_unique<Searcher>(tt_, stop_, std::move(cb), ponderhit_));
    }

    while (static_cast<int>(workers_.size()) + 1 < count) {
        const int helper_slot = static_cast<int>(workers_.size());
        try {
            workers_.emplace_back(&SearchThreadPool::worker_loop, this, helper_slot);
        } catch (const std::system_error& e) {
            if (info_cb_) {
                info_cb_("info string Threads reduced to "
                         + std::to_string(static_cast<int>(workers_.size()) + 1)
                         + " after worker creation failed: " + e.what());
            }
            break;
        }
    }

    const int active_count = std::min<int>(count, static_cast<int>(workers_.size()) + 1);
    if (std::cmp_greater(searchers_.size(), active_count))
        searchers_.resize(static_cast<size_t>(active_count));
    return active_count;
}

void SearchThreadPool::clear() {
    for (auto& searcher : searchers_)
        searcher->clear();
}

SearchLimits SearchThreadPool::limits_for_thread(const SearchLimits& limits,
                                                 int thread_id,
                                                 int thread_count,
                                                 RootMoveTable& root_table) const {
    SearchLimits worker_limits = limits;
    worker_limits.update_tt_age = false;
    worker_limits.thread_id = thread_id;
    worker_limits.thread_count = thread_count;
    worker_limits.root_table = &root_table;
    worker_limits.root_filter_count = 1;
    worker_limits.root_filter_index = -1;

    // 9.4(b): helpers must not inherit the DEPTH limit. `limits` was copied
    // wholesale, so under `go depth N` every helper stopped at N and then idled
    // instead of continuing to widen the shared TT for the main thread. The
    // main thread keeps the depth contract — it is the one whose result is
    // reported — so the answer to `go depth N` is unchanged; only the helpers'
    // idle time becomes useful work. Clock-limited games are unaffected (they
    // carry no depth limit), and 1T never reaches this function.
    if (thread_id > 0)
        worker_limits.depth = infiniteDepth;

    return worker_limits;
}

SearchResult SearchThreadPool::merge_results(const std::vector<SearchResult>& results,
                                             int count,
                                             const RootMoveTable& root_table,
                                             int64_t elapsed_ms) const {
    SearchResult best = root_table.best_result();
    int64_t total_nodes = 0;
    int64_t total_tbhits = 0;

    for (int i = 0; i < count; ++i) {
        const SearchResult& result = results[static_cast<size_t>(i)];
        total_nodes += result.nodes;
        total_tbhits += result.tbhits;

        if (result.bestmove == MOVE_NONE || !root_table.contains(result.bestmove))
            continue;

        const bool result_mates = result.score >= MATE_SCORE - MAX_PLY;
        const bool best_mates = best.score >= MATE_SCORE - MAX_PLY;
        if (best.bestmove == MOVE_NONE
            || (result_mates && (!best_mates || result.score > best.score))
            || (!best_mates && result.depth > best.depth)
            || (!best_mates && result.depth == best.depth && result.score > best.score)) {
            best = result;
        }
    }

    if (best.bestmove == MOVE_NONE)
        best.bestmove = root_table.fallback_move();

    best.nodes = total_nodes;
    best.tbhits = total_tbhits;
    best.elapsed_ms = elapsed_ms;
    return best;
}

SearchResult SearchThreadPool::search(Board board, const SearchLimits& limits, int thread_count) {
    thread_count = resize_threads(thread_count);
    const Board root_board = board;

    if (thread_count <= 1) {
        SearchLimits worker_limits = limits;
        worker_limits.shared_nodes = nullptr;
        worker_limits.shared_tbhits = nullptr;
        worker_limits.update_tt_age = true;
        worker_limits.thread_id = 0;
        worker_limits.thread_count = 1;
        worker_limits.root_table = nullptr;
        return sanitize_search_result(root_board, searchers_[0]->search(std::move(board), worker_limits));
    }

    tt_.new_search();

    RootMoveTable root_table;
    root_table.reset(board, limits.root_moves, limits.syzygy_root_moves);

    std::vector<SearchResult> results(static_cast<size_t>(thread_count));
    // 9.3(b): these were adjacent stack atomics, i.e. the same cache line, so
    // every tbhit publish invalidated the node counter's line for every thread
    // and vice versa. One cache line each. (Batching in record_node() is what
    // makes the traffic rare; this makes what remains non-interfering.)
    alignas(64) std::atomic<int64_t> shared_nodes{0};
    alignas(64) std::atomic<int64_t> shared_tbhits{0};
    SearchLimits shared_limits = limits;
    shared_limits.shared_nodes = &shared_nodes;
    shared_limits.shared_tbhits = &shared_tbhits;
    const auto wall_start = std::chrono::steady_clock::now();

    {
        std::scoped_lock lock(mutex_);
        job_board_ = board;
        job_limits_ = shared_limits;
        job_results_ = &results;
        job_root_table_ = &root_table;
        requested_helpers_ = thread_count - 1;
        active_helpers_ = requested_helpers_;
        ++epoch_;
    }
    work_cv_.notify_all();

    SearchLimits main_limits = limits_for_thread(shared_limits, 0, thread_count, root_table);
    results[0] = searchers_[0]->search(std::move(board), main_limits);

    while (!stop_.load(std::memory_order_acquire) && (limits.ponder || limits.infinite)) {
        if (limits.ponder && ponderhit_ && ponderhit_->load(std::memory_order_acquire))
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    stop_.store(true, std::memory_order_release);

    {
        std::unique_lock lock(mutex_);
        done_cv_.wait(lock, [&] { return active_helpers_ == 0; });
        job_results_ = nullptr;
        job_root_table_ = nullptr;
        requested_helpers_ = 0;
    }

    // 9.3(c): the pool aggregate, printed after the join because that is the
    // first moment the helpers' counters are complete. Thread 0 owns info_cb_,
    // so it does the printing. Gated here as well as inside, so normal play
    // does not build the depth vector on every multi-thread search.
    if (limits.diag) {
        std::vector<int> completed_depths;
        completed_depths.reserve(static_cast<size_t>(thread_count));
        for (int i = 0; i < thread_count; ++i)
            completed_depths.push_back(results[static_cast<size_t>(i)].depth);
        searchers_[0]->print_pool_diag(searchers_, thread_count, completed_depths);
    }

    const int64_t elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - wall_start).count();

    return sanitize_search_result(root_board, merge_results(results, thread_count, root_table, elapsed_ms));
}

void SearchThreadPool::worker_loop(int helper_slot) {
    uint64_t seen_epoch = 0;

    while (true) {
        Board board;
        SearchLimits limits;
        RootMoveTable* root_table = nullptr;
        std::vector<SearchResult>* results = nullptr;
        int thread_id = helper_slot + 1;
        int thread_count = 1;

        {
            std::unique_lock lock(mutex_);
            work_cv_.wait(lock, [&] { return shutdown_ || epoch_ != seen_epoch; });
            if (shutdown_)
                return;

            seen_epoch = epoch_;
            if (helper_slot >= requested_helpers_ || !job_results_ || !job_root_table_)
                continue;

            board = job_board_;
            root_table = job_root_table_;
            results = job_results_;
            thread_count = requested_helpers_ + 1;
            limits = limits_for_thread(job_limits_, thread_id, thread_count, *root_table);
        }

        SearchResult result = searchers_[static_cast<size_t>(thread_id)]->search(std::move(board), limits);

        {
            std::scoped_lock lock(mutex_);
            if (results && std::cmp_less(thread_id, results->size()))
                (*results)[static_cast<size_t>(thread_id)] = result;

            if (active_helpers_ > 0)
                --active_helpers_;
            if (active_helpers_ == 0)
                done_cv_.notify_one();
        }
    }
}
