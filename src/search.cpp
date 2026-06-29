#include "search.h"
#include "Constants.h"
#include "syzygy.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <exception>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>

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
    for (int d = 1; d < 64; d++)
        for (int m = 1; m < 64; m++)
            lmr_table_[d][m] = int(base + std::log(d) * std::log(m) / divisor);
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

    std::lock_guard lock(mutex_);
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

    std::lock_guard lock(mutex_);
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
    std::lock_guard lock(mutex_);
    for (const Entry& entry : entries_) {
        if (entry.bestmove == move)
            return true;
    }
    return false;
}

Move RootMoveTable::fallback_move() const {
    std::lock_guard lock(mutex_);
    return entries_.empty() ? MOVE_NONE : entries_.front().bestmove;
}

int RootMoveTable::ordering_score(Move move) const {
    std::lock_guard lock(mutex_);
    for (const Entry& entry : entries_) {
        if (entry.bestmove == move && entry.depth > 0) {
            return 7'000'000 + entry.depth * 10'000
                 + std::clamp(entry.score, -MATE_SCORE, MATE_SCORE);
        }
    }
    return 0;
}

SearchResult RootMoveTable::best_result() const {
    std::lock_guard lock(mutex_);

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
    : evaluator()
    , tt_(tt)
    , stop_(stop_flag)
    , ponderhit_(ponderhit_flag)
    , info_cb_(info_cb)
    , board_ptr_(nullptr)
    , nodes_(0)
    , tb_hits_(0)
    , nodes_limit_(0)
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
    cont_hist1_ = std::make_unique<ContHistTable>();
    cont_hist2_ = std::make_unique<ContHistTable>();
    cont_hist4_ = std::make_unique<ContHistTable>();
    pawn_hist_  = std::make_unique<PawnHistTable>();
    clear();
}

void Searcher::clear() {
    std::memset(main_hist_,    0, sizeof(main_hist_));
    std::memset(cap_hist_,     0, sizeof(cap_hist_));
    std::memset(cont_hist1_->data, 0, sizeof(cont_hist1_->data));
    std::memset(cont_hist2_->data, 0, sizeof(cont_hist2_->data));
    std::memset(cont_hist4_->data, 0, sizeof(cont_hist4_->data));
    std::memset(pawn_hist_->data, 0, sizeof(pawn_hist_->data));
    std::memset(low_ply_hist_, 0, sizeof(low_ply_hist_));
    std::memset(countermove_,  0, sizeof(countermove_));
    std::memset(pawn_corr_hist_, 0, sizeof(pawn_corr_hist_));
    std::memset(minor_corr_hist_, 0, sizeof(minor_corr_hist_));
    std::memset(nonpawn_corr_hist_, 0, sizeof(nonpawn_corr_hist_));
    std::memset(cont_corr_hist_, 0, sizeof(cont_corr_hist_));
    history_age_counter_ = 0;
}

static void blend_history_value(int16_t& dst, int16_t src) {
    const int value = std::clamp(int(dst) + int(src) / 4, -16384, 16384);
    dst = static_cast<int16_t>(value);
}

void Searcher::blend_history_from(const Searcher& other) {
    for (Color c : {WHITE, BLACK})
        for (int from = 0; from < SQUARE_NB; ++from)
            for (int to = 0; to < SQUARE_NB; ++to)
                blend_history_value(main_hist_[c][from][to],
                                    other.main_hist_[c][from][to]);

    for (int pt = 0; pt < PIECE_TYPE_NB; ++pt)
        for (int to = 0; to < SQUARE_NB; ++to)
            for (int cap = 0; cap < PIECE_TYPE_NB; ++cap)
                blend_history_value(cap_hist_[pt][to][cap],
                                    other.cap_hist_[pt][to][cap]);

    for (int p_pt = 0; p_pt < PIECE_TYPE_NB; ++p_pt) {
        for (int p_to = 0; p_to < SQUARE_NB; ++p_to) {
            for (int c_pt = 0; c_pt < PIECE_TYPE_NB; ++c_pt) {
                for (int c_to = 0; c_to < SQUARE_NB; ++c_to) {
                    blend_history_value(cont_hist1_->data[p_pt][p_to][c_pt][c_to],
                                        other.cont_hist1_->data[p_pt][p_to][c_pt][c_to]);
                    blend_history_value(cont_hist2_->data[p_pt][p_to][c_pt][c_to],
                                        other.cont_hist2_->data[p_pt][p_to][c_pt][c_to]);
                    blend_history_value(cont_hist4_->data[p_pt][p_to][c_pt][c_to],
                                        other.cont_hist4_->data[p_pt][p_to][c_pt][c_to]);
                }
            }
        }
    }

    for (int key = 0; key < PAWN_HIST_SIZE; ++key)
        for (int pt = 0; pt < PIECE_TYPE_NB; ++pt)
            for (int to = 0; to < SQUARE_NB; ++to)
                blend_history_value(pawn_hist_->data[key][pt][to],
                                    other.pawn_hist_->data[key][pt][to]);

    for (int ply = 0; ply < LOW_PLY_HISTORY_SIZE; ++ply)
        for (int from = 0; from < SQUARE_NB; ++from)
            for (int to = 0; to < SQUARE_NB; ++to)
                blend_history_value(low_ply_hist_[ply][from][to],
                                    other.low_ply_hist_[ply][from][to]);
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

    // Phase 6 Step 6.1: Stockfish-style increment-and-ply-aware clock budget,
    // ported from Rarog's Phase 2.2 rewrite (src/time_manager.rs) so both
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
    const double reserve      = 2.0 * overhead;
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

int64_t Searcher::record_node() {
    ++nodes_;
    if (active_limits_.shared_nodes) {
        const int64_t total =
            active_limits_.shared_nodes->fetch_add(1, std::memory_order_relaxed) + 1;
        if (nodes_limit_ > 0 && total >= nodes_limit_)
            stopped_ = true;
        return total;
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
    hist_update<MAX_MAIN_HIST>(main_hist_[stm][from][to], bonus);
}

void Searcher::update_cap(PieceType pt, Square to, PieceType cap, int bonus) {
    hist_update<MAX_CAP_HIST>(cap_hist_[pt][to][cap], bonus);
}

void Searcher::update_cont(ContHistTable& tbl,
                           PieceType ppt, Square pto,
                           PieceType cpt, Square cto, int bonus) {
    hist_update<MAX_CONT_HIST>(tbl.data[ppt][pto][cpt][cto], bonus);
}

void Searcher::update_pawn_hist(Key pawn_key, PieceType pt, Square to, int bonus) {
    hist_update<MAX_PAWN_HIST>(pawn_hist_->data[pawn_key & (PAWN_HIST_SIZE - 1)][pt][to], bonus);
}

void Searcher::update_low_ply(int ply, Square from, Square to, int bonus) {
    if (ply < LOW_PLY_HISTORY_SIZE)
        hist_update<MAX_LOW_HIST>(low_ply_hist_[ply][from][to], bonus);
}

int Searcher::cont_hist_score(const SearchStack* ss, PieceType pt, Square to) const {
    int score = 0;
    // 1-ply back
    if ((ss-1)->move != MOVE_NONE && (ss-1)->move != MOVE_NULL
        && (ss-1)->moved_piece != NO_PIECE_TYPE) {
        score += cont_hist1_->data[(ss-1)->moved_piece][to_sq((ss-1)->move)][pt][to];
    }
    // 2-ply back
    if ((ss-2)->move != MOVE_NONE && (ss-2)->move != MOVE_NULL
        && (ss-2)->moved_piece != NO_PIECE_TYPE) {
        score += cont_hist2_->data[(ss-2)->moved_piece][to_sq((ss-2)->move)][pt][to];
    }
    // 4-ply back keeps useful quiet continuations across one full move pair.
    if ((ss-4)->move != MOVE_NONE && (ss-4)->move != MOVE_NULL
        && (ss-4)->moved_piece != NO_PIECE_TYPE) {
        score += cont_hist4_->data[(ss-4)->moved_piece][to_sq((ss-4)->move)][pt][to] / 2;
    }
    return score;
}

int Searcher::pawn_hist_score(Key pawn_key, PieceType pt, Square to) const {
    return pawn_hist_->data[pawn_key & (PAWN_HIST_SIZE - 1)][pt][to];
}

int Searcher::low_ply_score(int ply, Square from, Square to) const {
    return ply < LOW_PLY_HISTORY_SIZE ? low_ply_hist_[ply][from][to] : 0;
}

void Searcher::update_all_histories(Move best,
                                    const Move* quiets, int quiet_count,
                                    const Move* bad_caps, int bad_cap_count,
                                    Color stm, int depth, SearchStack* ss) {
    int bonus = std::min(depth * depth, 2048);
    int malus = -std::min(depth * depth, 2048);

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

        // Killers
        if (ss->killers[0] != best) {
            ss->killers[1] = ss->killers[0];
            ss->killers[0] = best;
        }

        // Countermove
        Move prev = (ss-1)->move;
        if (prev != MOVE_NONE && prev != MOVE_NULL)
            countermove_[from_sq(prev)][to_sq(prev)] = best;

        // Continuation history
        if ((ss-1)->moved_piece != NO_PIECE_TYPE && (ss-1)->move != MOVE_NONE
            && (ss-1)->move != MOVE_NULL) {
            update_cont(*cont_hist1_, (ss-1)->moved_piece,
                        Square(to_sq((ss-1)->move)), pt, to, bonus);
        }
        if ((ss-2)->moved_piece != NO_PIECE_TYPE && (ss-2)->move != MOVE_NONE
            && (ss-2)->move != MOVE_NULL) {
            update_cont(*cont_hist2_, (ss-2)->moved_piece,
                        Square(to_sq((ss-2)->move)), pt, to, bonus);
        }
        if ((ss-4)->moved_piece != NO_PIECE_TYPE && (ss-4)->move != MOVE_NONE
            && (ss-4)->move != MOVE_NULL) {
            update_cont(*cont_hist4_, (ss-4)->moved_piece,
                        Square(to_sq((ss-4)->move)), pt, to, bonus / 2);
        }

        // Malus for other searched quiets
        for (int i = 0; i < quiet_count; ++i) {
            Move m = quiets[i];
            if (m == best) continue;
            Square mf = Square(from_sq(m)), mt = Square(to_sq(m));
            PieceType mpt = type_of(board_ptr_->board_sq[mf]);
            update_quiet(stm, mf, mt, malus);
            update_pawn_hist(board_ptr_->pawn_key, mpt, mt, malus);
            update_low_ply(static_cast<int>(ss - (ss_arr_ + 4)), mf, mt, malus);
            if ((ss-1)->moved_piece != NO_PIECE_TYPE && (ss-1)->move != MOVE_NONE
                && (ss-1)->move != MOVE_NULL)
                update_cont(*cont_hist1_, (ss-1)->moved_piece,
                            Square(to_sq((ss-1)->move)), mpt, mt, malus);
            if ((ss-2)->moved_piece != NO_PIECE_TYPE && (ss-2)->move != MOVE_NONE
                && (ss-2)->move != MOVE_NULL)
                update_cont(*cont_hist2_, (ss-2)->moved_piece,
                            Square(to_sq((ss-2)->move)), mpt, mt, malus);
            if ((ss-4)->moved_piece != NO_PIECE_TYPE && (ss-4)->move != MOVE_NONE
                && (ss-4)->move != MOVE_NULL)
                update_cont(*cont_hist4_, (ss-4)->moved_piece,
                            Square(to_sq((ss-4)->move)), mpt, mt, malus / 2);
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
    update_correction_slot(pawn_corr_hist_[stm][board.pawn_key & (CORR_SIZE - 1)], diff, depth);
    update_correction_slot(minor_corr_hist_[stm][board.minor_key & (CORR_SIZE - 1)], diff, depth);
    update_correction_slot(nonpawn_corr_hist_[stm][WHITE][board.nonpawn_key[WHITE] & (CORR_SIZE - 1)],
                           diff, depth);
    update_correction_slot(nonpawn_corr_hist_[stm][BLACK][board.nonpawn_key[BLACK] & (CORR_SIZE - 1)],
                           diff, depth);

    if ((ss-1)->move != MOVE_NONE && (ss-1)->move != MOVE_NULL
        && (ss-1)->moved_piece != NO_PIECE_TYPE) {
        update_correction_slot(cont_corr_hist_[stm][(ss-1)->moved_piece][to_sq((ss-1)->move)],
                               diff, depth);
    }
}

int Searcher::correction_value(Color stm, const Board& board, const SearchStack* ss) const {
    const int pawn = pawn_corr_hist_[stm][board.pawn_key & (CORR_SIZE - 1)];
    const int minor = minor_corr_hist_[stm][board.minor_key & (CORR_SIZE - 1)];
    const int own = nonpawn_corr_hist_[stm][stm][board.nonpawn_key[stm] & (CORR_SIZE - 1)];
    const int opp = nonpawn_corr_hist_[stm][~stm][board.nonpawn_key[~stm] & (CORR_SIZE - 1)];

    int cont = 0;
    if ((ss-1)->move != MOVE_NONE && (ss-1)->move != MOVE_NULL
        && (ss-1)->moved_piece != NO_PIECE_TYPE) {
        cont = cont_corr_hist_[stm][(ss-1)->moved_piece][to_sq((ss-1)->move)];
    }

    return (pawn + minor + own + opp + cont) / 5;
}

void Searcher::age_history() {
    // Halve all history values to preserve inter-search learning while decaying stale info
    for (auto& a : main_hist_)
        for (auto& b : a)
            for (auto& c : b) c /= 2;

    for (auto& a : cap_hist_)
        for (auto& b : a)
            for (auto& c : b) c /= 2;

    for (auto& a : cont_hist1_->data)
        for (auto& b : a)
            for (auto& c : b)
                for (auto& d : c) d /= 2;

    for (auto& a : cont_hist2_->data)
        for (auto& b : a)
            for (auto& c : b)
                for (auto& d : c) d /= 2;

    for (auto& a : cont_hist4_->data)
        for (auto& b : a)
            for (auto& c : b)
                for (auto& d : c) d /= 2;

    for (auto& a : pawn_hist_->data)
        for (auto& b : a)
            for (auto& c : b) c /= 2;

    for (auto& a : low_ply_hist_)
        for (auto& b : a)
            for (auto& c : b) c /= 2;

    for (auto& a : pawn_corr_hist_)
        for (auto& b : a) b /= 2;
    for (auto& a : minor_corr_hist_)
        for (auto& b : a) b /= 2;
    for (auto& a : nonpawn_corr_hist_)
        for (auto& b : a)
            for (auto& c : b) c /= 2;
    for (auto& a : cont_corr_hist_)
        for (auto& b : a)
            for (auto& c : b) c /= 2;
}

// ---- Move ordering ---------------------------------------------------------

static constexpr int PIECE_VALUE[PIECE_TYPE_NB] = {0, 100, 300, 300, 500, 900, 20000};
static constexpr int MAX_TRACKED_QUIETS = 64;
static constexpr int MAX_TRACKED_BAD_CAPS = 32;

void Searcher::score_moves(ScoredMove* moves, int n, SearchStack* ss,
                           bool is_root, int ply) const {
    const Board& b = *board_ptr_;

    Move cm = MOVE_NONE;
    Move prev = (ss-1)->move;
    if (prev != MOVE_NONE && prev != MOVE_NULL)
        cm = countermove_[from_sq(prev)][to_sq(prev)];

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

    for (int i = 0; i < n; i++) {
        Move m = moves[i].move;

        bool is_cap   = (b.board_sq[to_sq(m)] != NO_PIECE) || (move_type(m) == EN_PASSANT);
        bool is_promo = (move_type(m) == PROMOTION);

        if (is_cap) {
            PieceType atk = type_of(b.board_sq[from_sq(m)]);
            PieceType cap = (move_type(m) == EN_PASSANT) ? PAWN : type_of(b.board_sq[to_sq(m)]);
            moves[i].score = 6'000'000 + PIECE_VALUE[cap] * 16 - PIECE_VALUE[atk]
                                       + cap_hist_[atk][to_sq(m)][cap];
        } else if (is_promo) {
            moves[i].score = (promo_type(m) == QUEEN) ? 5'500'000 : -100;
        } else {
            // Quiet
            const Square from = Square(from_sq(m));
            const Square to = Square(to_sq(m));
            const PieceType pt = type_of(b.board_sq[from]);
            int hist = main_hist_[b.side_to_move][from][to];
            hist += cont_hist_score(ss, pt, to);
            hist += pawn_hist_score(b.pawn_key, pt, to);
            hist += low_ply_score(ply, from, to);

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
    for (ScoredMove* it = first + 1, *end = moves + n; it != end; ++it)
        if (it->score > best->score)
            best = it;

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
                        return move;
                    }
                    stage_ = Stage::QuietsInit;
                    break;

                case Stage::QuietsInit:
                    fill_quiets();
                    stage_ = Stage::Quiets;
                    break;

                case Stage::Quiets:
                    if (idx_ < n_)
                        return Searcher::pick_next(scored_, idx_++, n_);
                    stage_ = Stage::BadTacticals;
                    bad_idx_ = 0;
                    break;

                case Stage::BadTacticals:
                    if (bad_idx_ < bad_count_)
                        return Searcher::pick_next(bad_, bad_idx_++, bad_count_);
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
};

// ---- UCI info ---------------------------------------------------------------

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
    if (ply >= MAX_PLY) return evaluator.evaluate(*board_ptr_);
    if (board_ptr_->is_draw(ply)) return 0;

    bool in_check = board_ptr_->is_in_check();

    // TT probe
    Key hash = board_ptr_->hash;
    TTEntry tte{};
    bool tt_found = tt_.probe_copy(hash, tte);
    Move tt_move = MOVE_NONE;
    if (tt_found) {
        tt_move = move_from_tt(tte.move16);
        int tt_score = TranspositionTable::score_from_tt(tte.score, ply, board_ptr_->halfmove_clock);
        TTFlag tt_flag = TTFlag(tte.flag_age & 3);
        if (tt_flag == TT_EXACT) return tt_score;
        if (tt_flag == TT_ALPHA && tt_score <= alpha) return tt_score;
        if (tt_flag == TT_BETA  && tt_score >= beta)  return tt_score;
    }

    if (in_check) {
        if (qply >= MAX_QSEARCH_PLY) {
            int eval = evaluator.evaluate(*board_ptr_);
            eval += correction_value(board_ptr_->side_to_move, *board_ptr_, ss);
            return std::clamp(eval, -(MATE_SCORE - 1), MATE_SCORE - 1);
        }
        MoveList legal;
        board_ptr_->gen_legal(legal);
        int best = -INF_SCORE;
        bool has_legal = false;
        for (Move m : legal) {
            has_legal = true;
            ss->move = m;
            ss->moved_piece = type_of(board_ptr_->board_sq[from_sq(m)]);
            board_ptr_->make_move(m);
            int s = -quiescence(-beta, -alpha, ply + 1, qply + 1, ss + 1);
            board_ptr_->unmake_move(m);
            ss->move = MOVE_NONE;
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
        raw_eval = evaluator.evaluate(*board_ptr_);

    int stand_pat = raw_eval;
    stand_pat += correction_value(board_ptr_->side_to_move, *board_ptr_, ss);
    stand_pat = std::clamp(stand_pat, -(MATE_SCORE - 1), MATE_SCORE - 1);

    if (stand_pat >= beta) {
        tt_.store(hash, 0, stand_pat, TT_BETA, MOVE_NONE, ply, raw_eval);
        return stand_pat;
    }

    // Delta pruning: skip if even capturing the best possible piece can't raise alpha
    if (stand_pat < alpha - PIECE_VALUE[QUEEN] - 200) return alpha;

    if (stand_pat > alpha) alpha = stand_pat;

    if (qply >= MAX_QSEARCH_PLY) return alpha;

    MoveList captures;
    board_ptr_->gen_legal_captures(captures);

    // Score captures: MVV + cap_hist; prefer TT move
    ScoredMove* sm = move_buffers_[ply][0];
    int nm = 0;
    for (Move m : captures) {
        PieceType atk = type_of(board_ptr_->board_sq[from_sq(m)]);
        PieceType cap = (move_type(m) == EN_PASSANT) ? PAWN : type_of(board_ptr_->board_sq[to_sq(m)]);
        int score = (m == tt_move) ? 10'000'000
                  : PIECE_VALUE[cap] * 16 - PIECE_VALUE[atk] + cap_hist_[atk][to_sq(m)][cap];
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

        ss->move = m;
        ss->moved_piece = type_of(board_ptr_->board_sq[from_sq(m)]);
        board_ptr_->make_move(m);
        int s = -quiescence(-beta, -alpha, ply + 1, qply + 1, ss + 1);
        board_ptr_->unmake_move(m);
        ss->move = MOVE_NONE;

        if (stopped_) return 0;
        if (s > alpha) {
            alpha = s;
            best_move = m;
        }
        if (s >= beta) {
            tt_.store(hash, 0, s, TT_BETA, m, ply, raw_eval);
            return s;
        }
    }

    TTFlag flag = (alpha > orig_alpha) ? TT_EXACT : TT_ALPHA;
    tt_.store(hash, 0, alpha, flag, best_move, ply, raw_eval);
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

    if (ply >= MAX_PLY) return evaluator.evaluate(*board_ptr_);
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
            tt_.store(board_ptr_->hash, depth, tb_score, TT_EXACT, MOVE_NONE, ply,
                      TranspositionTable::INF_EVAL);
            return tb_score;
        }
    }

    bool in_check = board_ptr_->is_in_check();

    // Check extension: when the side to move is in check, extend by 1 ply.
    // Guard with ss->excluded to prevent stacking with singular extensions.
    if (in_check && ss->excluded == MOVE_NONE && ply < MAX_PLY - 2)
        depth++;

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

    Move  tt_move  = MOVE_NONE;
    int   tt_score = VALUE_NONE;
    int   tt_depth = 0;
    TTFlag tt_flag  = TT_NONE;

    if (tt_found) {
        tt_move  = move_from_tt(tte.move16);
        tt_score = TranspositionTable::score_from_tt(tte.score, ply, board_ptr_->halfmove_clock);
        tt_depth = tte.depth;
        tt_flag  = TTFlag(tte.flag_age & 3);

        if (!is_pv && ss->excluded == MOVE_NONE && tt_depth >= depth) {
            if (tt_flag == TT_EXACT) return tt_score;
            if (tt_flag == TT_ALPHA && tt_score <= alpha) return tt_score;
            if (tt_flag == TT_BETA  && tt_score >= beta)  return tt_score;
        }
    }

    ss->tt_pv = is_pv || (tt_found && tt_flag == TT_EXACT && tt_depth >= depth - 1);

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
            raw_static_eval = evaluator.evaluate(*board_ptr_);

        // TT stores the raw static eval; correction is applied at probe time.
        static_eval = raw_static_eval;
        static_eval += correction_value(board_ptr_->side_to_move, *board_ptr_, ss);
        static_eval  = std::clamp(static_eval, -(MATE_SCORE - 1), MATE_SCORE - 1);
        ss->eval = static_eval;
    }

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
            if (static_eval - margin >= beta)
                return static_eval;
        }

        // Razoring
        if (depth <= 3 && static_eval + active_limits_.params.razor_coeff * depth <= alpha) {
            int q = quiescence(alpha, beta, ply, 0, ss);
            if (q <= alpha) return q;
        }

        // Null-move pruning
        if (allow_null && depth >= 3
            && static_eval >= beta
            && board_ptr_->has_non_pawn_material(board_ptr_->side_to_move)
            && (ss-1)->move != MOVE_NULL) {

            int r = active_limits_.params.null_base + depth / 4
                  + std::min((static_eval - beta) / active_limits_.params.null_eval_div, 3);
            ss->move        = MOVE_NULL;
            ss->moved_piece = NO_PIECE_TYPE;
            board_ptr_->make_null_move();
            int null_score = -negamax(std::max(0, depth - r), -beta, -(beta - 1),
                                      ply + 1, ss + 1, false, false, true);
            board_ptr_->unmake_null_move();
            ss->move = MOVE_NONE;
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
                if (verified)
                    return null_score;
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

                ss->move        = m;
                ss->moved_piece = type_of(board_ptr_->board_sq[from_sq(m)]);
                board_ptr_->make_move(m);
                // Quick check via QSearch first
                int val = -quiescence(-pc_beta, -pc_beta + 1, ply + 1, 0, ss + 1);
                if (val >= pc_beta)
                    val = -negamax(depth - 4, -pc_beta, -pc_beta + 1,
                                   ply + 1, ss + 1, false, true, true);
                board_ptr_->unmake_move(m);
                ss->move = MOVE_NONE;
                if (stopped_) return 0;
                if (val >= pc_beta) {
                    tt_.store(hash, depth - 3, pc_beta, TT_BETA, m, ply,
                              raw_static_eval == VALUE_NONE
                                  ? TranspositionTable::INF_EVAL : raw_static_eval);
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

    auto search_one = [&](Move m) {
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
        int see_score = VALUE_NONE;
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

            if (is_quiet) {
                // Futility pruning
                if (!is_pv && !in_check && depth <= 6
                    && static_eval != VALUE_NONE
                    && static_eval + active_limits_.params.futility_base
                                   + active_limits_.params.futility_coeff * depth <= alpha
                    && !move_gives_check())
                    return false;

                // Late move pruning (LMP) — never in PV
                if (!is_pv && !in_check && depth <= 6 && searched >= lmp_thresh
                    && !move_gives_check())
                    return false;

                // History pruning: skip moves with very bad combined history
                if (!is_pv && depth <= 6) {
                    PieceType pt = type_of(board_ptr_->board_sq[from_sq(m)]);
                    int hist = main_hist_[board_ptr_->side_to_move][from_sq(m)][to_sq(m)]
                             + cont_hist_score(ss, pt, Square(to_sq(m)))
                             + pawn_hist_score(board_ptr_->pawn_key, pt, Square(to_sq(m)))
                             + low_ply_score(ply, Square(from_sq(m)), Square(to_sq(m)));
                    if (hist < -active_limits_.params.hist_prune_coeff * depth && !move_gives_check())
                        return false;
                }
            } else if (is_cap) {
                // SEE pruning for bad captures
                if (!is_pv && depth <= 8 && !is_promo) {
                    if (!board_ptr_->see_ge(m, -depth * active_limits_.params.see_prune_coeff) && !move_gives_check())
                        return false;
                }
            }
        }

        if (is_cap && !is_promo && depth >= 2 && searched >= 2 && see_score == VALUE_NONE)
            see_score = board_ptr_->see_ge(m, 0) ? 0 : -1;

        // ---- Extensions -------------------------------------------------------
        int extension = 0;

        // ---- Singular extension (only for TT move) -------------------------
        if (!is_root && m == tt_move && ss->excluded == MOVE_NONE
            && depth >= 5 && tt_found && tt_depth >= depth - 3
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
                // TT move is singular — extend it
                extension += (!is_pv && s_val < s_beta - active_limits_.params.singular_double_margin) ? 2 : 1;
            } else if (s_beta >= beta) {
                // Multicut: likely to fail high without this move too
                immediate_return = true;
                immediate_score = s_beta;
                return true;
            } else if (tt_score >= beta) {
                extension--; // Negative extension: not clearly best
            }
        }

        PieceType moved_pt = type_of(board_ptr_->board_sq[from_sq(m)]);
        int move_stat_score = 0;
        if (is_quiet) {
            move_stat_score = main_hist_[board_ptr_->side_to_move][from_sq(m)][to_sq(m)];
            move_stat_score += cont_hist_score(ss, moved_pt, Square(to_sq(m)));
            move_stat_score += pawn_hist_score(board_ptr_->pawn_key, moved_pt, Square(to_sq(m)));
            move_stat_score += low_ply_score(ply, Square(from_sq(m)), Square(to_sq(m)));
        }

        ss->move        = m;
        ss->moved_piece = moved_pt;
        ss->stat_score  = move_stat_score;
        ss->reduction   = 0;
        const int64_t nodes_before_move = nodes_;
        board_ptr_->make_move(m);
        tt_.prefetch(board_ptr_->hash);
        sel_depth_ = std::max(sel_depth_, ply + 1);

        int new_depth = depth - 1 + extension;

        int score;
        if (searched == 0) {
            score = -negamax(new_depth, -beta, -alpha, ply + 1, ss + 1, is_pv, true, false);
        } else {
            // Late Move Reductions
            int reduction = 0;
            // LMR applies to: quiets, and bad captures — but NOT promotions
            if (depth >= 2 && searched >= 2 && !in_check
                && (is_quiet || (is_cap && !is_promo && see_score < 0))
                && !move_gives_check()) {
                reduction = lmr_table_[std::min(depth, 63)][std::min(searched, 63)];

                if (is_quiet) {
                    const auto& p = active_limits_.params;
                    if (!is_pv)     reduction += p.lmr_non_pv_adj;
                    if (cut_node)   reduction += p.lmr_cut_node_adj;
                    if (ss->tt_pv)  reduction -= p.lmr_tt_pv_adj;
                    if (!improving) reduction += p.lmr_not_improving_adj;
                    // History-based adjustment: good moves get reduced less, bad more
                    reduction -= move_stat_score / p.lmr_hist_div;
                } else {
                    // Bad captures get less reduction than quiets
                    reduction = (reduction - 1) / 2;
                }

                reduction = std::clamp(reduction, 0, new_depth - 1);
            }
            ss->reduction = reduction;

            score = -negamax(new_depth - reduction, -alpha - 1, -alpha,
                             ply + 1, ss + 1, false, true, true);
            // Re-search at full depth if LMR didn't fail low
            if (reduction > 0 && score > alpha && !stopped_)
                score = -negamax(new_depth, -alpha - 1, -alpha,
                                 ply + 1, ss + 1, false, true, !cut_node);
            // Re-search as PV if score is within window
            if (is_pv && score > alpha && score < beta && !stopped_)
                score = -negamax(new_depth, -beta, -alpha,
                                 ply + 1, ss + 1, true, true, false);
        }

        board_ptr_->unmake_move(m);
        ss->move = MOVE_NONE;

        if (stopped_)
            return true;

        searched++;
        const int64_t move_nodes = nodes_ - nodes_before_move;
        if (is_root)
            root_depth_nodes_ += std::max<int64_t>(0, move_nodes);

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
            }
        }

        if (alpha >= beta) {
            update_all_histories(m, quiets_searched, quiets_count,
                                 bad_caps_searched, bad_caps_count,
                                 board_ptr_->side_to_move, depth, ss);
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
        if (search_one(move))
            break;
    }

    if (immediate_return)
        return immediate_score;

    if (stopped_)
        return (is_root && best_move != MOVE_NONE) ? best_score : 0;

    // No legal moves
    if (searched == 0)
        return in_check ? -(MATE_SCORE - ply) : 0;

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
        tt_.store(hash, depth, best_score, flag, best_move, ply,
                  raw_static_eval == VALUE_NONE ? TranspositionTable::INF_EVAL : raw_static_eval);

    return best_score;
}

// ---- Iterative deepening ---------------------------------------------------

SearchResult Searcher::search(Board board, const SearchLimits& limits) {
    board_ptr_    = &board;
    nodes_        = 0;
    tb_hits_      = 0;
    nodes_limit_  = limits.nodes;
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

    int max_depth = limits.infinite ? MAX_SEARCH_DEPTH
                  : std::min(limits.depth, MAX_SEARCH_DEPTH);

    int start_depth = 1;
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
            while (true) {
                score = negamax(depth, asp_a, asp_b, 0, ss, true, true, false);
                if (stopped_) break;
                if (score <= asp_a) {
                    asp_a  = std::max(score - delta, -INF_SCORE);
                    delta += delta / 2;
                } else if (score >= asp_b) {
                    asp_b  = std::min(score + delta, INF_SCORE);
                    delta += delta / 2;
                } else {
                    break;
                }
                if (delta >= 900) {
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
        if (cur_best == prev_best)
            best_stability++;
        else {
            best_stability = 0;
            prev_best      = cur_best;
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

        if (root_table_ && result.bestmove != MOVE_NONE)
            root_table_->update(result.bestmove, result.pondermove, depth, score);

        double elapsed = elapsed_seconds();
        send_info(depth, reported_score, current_nodes(), elapsed);

        // Adaptive soft time limit:
        // The more stable the best move, the less time we need to confirm it.
        // stability=0 → 100% of soft, stability=6+ → ~64% of soft
        // A significant score drop signals instability — extend time budget.
        if (soft_limit_ > 0.0 && !pondering_) {
            double stability_scale = 1.0 - 0.06 * std::min(best_stability, 6);
            // Score-based time extension: if score dropped by 30+ cp, take more time
            int score_drop = prev_score_saved - score;
            double score_scale = (depth > 4 && score_drop > 30)
                               ? 1.0 + std::min(score_drop - 30, 120) / 100.0
                               : 1.0;
            double effort_scale = (depth > 5 && root_best_effort_ >= 80) ? 0.80
                                : (depth > 5 && root_best_effort_ <= 25) ? 1.20
                                : 1.0;
            if (elapsed >= soft_limit_ * stability_scale * score_scale * effort_scale)
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
        std::lock_guard lock(mutex_);
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

int SearchThreadPool::normalize_thread_count(int count) {
    count = std::max(1, count);
    const unsigned hw = std::thread::hardware_concurrency();
    const int runtime_cap = hw == 0 ? 1024 : static_cast<int>(std::max(1024u, 4u * hw));
    return std::min(count, runtime_cap);
}

int SearchThreadPool::active_thread_count() const {
    std::lock_guard lock(mutex_);
    return static_cast<int>(searchers_.size());
}

int SearchThreadPool::resize_threads(int count) {
    count = normalize_thread_count(count);

    bool already_exact = false;
    {
        std::lock_guard lock(mutex_);
        already_exact = !shutdown_
            && static_cast<int>(searchers_.size()) == count
            && static_cast<int>(workers_.size()) + 1 == count;
    }
    if (already_exact)
        return count;

    {
        std::lock_guard lock(mutex_);
        shutdown_ = true;
        ++epoch_;
    }
    work_cv_.notify_all();

    for (std::thread& worker : workers_) {
        if (worker.joinable())
            worker.join();
    }

    {
        std::lock_guard lock(mutex_);
        workers_.clear();
        searchers_.clear();
        job_results_ = nullptr;
        job_root_table_ = nullptr;
        requested_helpers_ = 0;
        active_helpers_ = 0;
        shutdown_ = false;
        epoch_ = 0;
    }

    while (static_cast<int>(searchers_.size()) < count) {
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
    if (static_cast<int>(searchers_.size()) > active_count)
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
    std::atomic<int64_t> shared_nodes{0};
    std::atomic<int64_t> shared_tbhits{0};
    SearchLimits shared_limits = limits;
    shared_limits.shared_nodes = &shared_nodes;
    shared_limits.shared_tbhits = &shared_tbhits;
    const auto wall_start = std::chrono::steady_clock::now();

    {
        std::lock_guard lock(mutex_);
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

    for (int i = 1; i < thread_count && i < static_cast<int>(searchers_.size()); ++i)
        searchers_[0]->blend_history_from(*searchers_[static_cast<size_t>(i)]);

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
            std::lock_guard lock(mutex_);
            if (results && thread_id < static_cast<int>(results->size()))
                (*results)[static_cast<size_t>(thread_id)] = result;

            if (active_helpers_ > 0)
                --active_helpers_;
            if (active_helpers_ == 0)
                done_cv_.notify_one();
        }
    }
}
