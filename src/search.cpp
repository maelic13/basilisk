#include "search.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <exception>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>

// ---- LMR table -------------------------------------------------------------

int  Searcher::LMR_TABLE[64][64];
bool Searcher::lmr_init_ = false;

void Searcher::init_lmr() {
    if (lmr_init_) return;
    for (int d = 1; d < 64; d++)
        for (int m = 1; m < 64; m++)
            LMR_TABLE[d][m] = int(0.75 + std::log(d) * std::log(m) / 2.25);
    lmr_init_ = true;
}

// ---- Shared root move table ------------------------------------------------

void RootMoveTable::reset(const Board& board) {
    MoveList legal;
    board.gen_legal(legal);

    std::lock_guard lock(mutex_);
    entries_.clear();
    entries_.reserve(static_cast<size_t>(legal.size()));
    sequence_ = 0;

    for (Move move : legal) {
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

    Entry entry;
    entry.bestmove = bestmove;
    entry.pondermove = pondermove;
    entry.depth = depth;
    entry.score = score;
    entry.sequence = ++sequence_;
    entries_.push_back(entry);
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
    , nodes_limit_(0)
    , sel_depth_(0)
    , stopped_(false)
    , root_filter_index_(-1)
    , root_filter_count_(1)
    , thread_id_(0)
    , root_table_(nullptr)
    , pondering_(false)
    , root_side_(WHITE)
    , time_limit_(0.0)
    , soft_limit_(0.0)
    , hard_limit_(0.0)
{
    init_lmr();
    cont_hist1_ = std::make_unique<ContHistTable>();
    cont_hist2_ = std::make_unique<ContHistTable>();
    clear();
}

void Searcher::clear() {
    std::memset(main_hist_,    0, sizeof(main_hist_));
    std::memset(cap_hist_,     0, sizeof(cap_hist_));
    std::memset(cont_hist1_->data, 0, sizeof(cont_hist1_->data));
    std::memset(cont_hist2_->data, 0, sizeof(cont_hist2_->data));
    std::memset(countermove_,  0, sizeof(countermove_));
    std::memset(corr_hist_,    0, sizeof(corr_hist_));
}

// ---- Time management -------------------------------------------------------

void Searcher::compute_time_limit(const SearchLimits& limits, Color side) {
    soft_limit_ = 0.0;
    hard_limit_ = 0.0;

    if (limits.infinite || limits.ponder) return;
    if (limits.movetime > 0) {
        // Fixed movetime: hard limit only, no soft limit
        hard_limit_ = std::max(1, limits.movetime - limits.overhead) / 1000.0;
        return;
    }

    int remaining = (side == WHITE) ? limits.wtime : limits.btime;
    int inc       = (side == WHITE) ? limits.winc  : limits.binc;

    remaining = std::max(0, remaining - limits.overhead);

    if (remaining <= 0 && inc <= 0) return;

    // Soft limit: target time per move
    int base;
    if (limits.movestogo > 0)
        base = remaining / (limits.movestogo + 3);
    else
        base = remaining / 25;  // assume ~25 moves left

    int soft_ms = base + (inc * 3) / 4;

    // Hard limit: maximum we ever spend
    int hard_ms = (remaining < 1000) ? remaining * 20 / 100
                : (remaining < 5000) ? remaining * 30 / 100
                                     : remaining * 50 / 100;
    hard_ms = std::max(soft_ms, hard_ms);  // hard >= soft always

    soft_limit_ = std::max(50, soft_ms) / 1000.0;
    hard_limit_ = std::max(50, hard_ms) / 1000.0;

    // Legacy: keep time_limit_ pointing at hard for check_stop()
    time_limit_ = hard_limit_;
}

double Searcher::elapsed_seconds() const {
    using namespace std::chrono;
    return duration<double>(steady_clock::now() - start_time_).count();
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
        start_time_ = std::chrono::steady_clock::now();
        compute_time_limit(normal_limits, root_side_);
    }

    if (hard_limit_ > 0.0 && elapsed_seconds() >= hard_limit_) {
        stopped_ = true;
        return true;
    }
    if (nodes_limit_ > 0 && nodes_ >= nodes_limit_) {
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
    return score;
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

        // Malus for other searched quiets
        for (int i = 0; i < quiet_count; ++i) {
            Move m = quiets[i];
            if (m == best) continue;
            Square mf = Square(from_sq(m)), mt = Square(to_sq(m));
            PieceType mpt = type_of(board_ptr_->board_sq[mf]);
            update_quiet(stm, mf, mt, malus);
            if ((ss-1)->moved_piece != NO_PIECE_TYPE && (ss-1)->move != MOVE_NONE
                && (ss-1)->move != MOVE_NULL)
                update_cont(*cont_hist1_, (ss-1)->moved_piece,
                            Square(to_sq((ss-1)->move)), mpt, mt, malus);
            if ((ss-2)->moved_piece != NO_PIECE_TYPE && (ss-2)->move != MOVE_NONE
                && (ss-2)->move != MOVE_NULL)
                update_cont(*cont_hist2_, (ss-2)->moved_piece,
                            Square(to_sq((ss-2)->move)), mpt, mt, malus);
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

void Searcher::update_correction(Color stm, Key pawn_key, int diff, int depth) {
    static constexpr int MAX_CORR = 1024;
    int16_t& slot = corr_hist_[stm][pawn_key & (CORR_SIZE - 1)];
    int w = std::min(depth + 1, 16);
    int updated = std::clamp((int(slot) * (256 - w) + diff * w) / 256, -MAX_CORR, MAX_CORR);
    slot = static_cast<int16_t>(updated);
}

int Searcher::correction_value(Color stm, Key pawn_key) const {
    return corr_hist_[stm][pawn_key & (CORR_SIZE - 1)];
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
}

// ---- Move ordering ---------------------------------------------------------

static constexpr int PIECE_VALUE[PIECE_TYPE_NB] = {0, 100, 300, 300, 500, 900, 20000};
static constexpr int MAX_TRACKED_QUIETS = 64;
static constexpr int MAX_TRACKED_BAD_CAPS = 32;

void Searcher::score_moves(ScoredMove* moves, int n, Move tt_move, SearchStack* ss, bool is_root) const {
    const Board& b = *board_ptr_;

    Move cm = MOVE_NONE;
    Move prev = (ss-1)->move;
    if (prev != MOVE_NONE && prev != MOVE_NULL)
        cm = countermove_[from_sq(prev)][to_sq(prev)];

    for (int i = 0; i < n; i++) {
        Move m = moves[i].move;

        if (m == tt_move) { moves[i].score = 10'000'000; continue; }

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
            PieceType pt = type_of(b.board_sq[from_sq(m)]);
            int hist = main_hist_[b.side_to_move][from_sq(m)][to_sq(m)];
            hist += cont_hist_score(ss, pt, Square(to_sq(m)));

            if      (m == ss->killers[0]) moves[i].score = 4'000'000;
            else if (m == ss->killers[1]) moves[i].score = 3'900'000;
            else if (m == cm)             moves[i].score = 3'800'000;
            else                          moves[i].score = hist;
        }

        if (is_root && root_table_)
            moves[i].score += root_table_->ordering_score(m);
    }
}

Move Searcher::pick_next(ScoredMove* moves, int idx, int n) {
    int best = idx;
    for (int i = idx + 1; i < n; i++)
        if (moves[i].score > moves[best].score)
            best = i;
    std::swap(moves[idx], moves[best]);
    return moves[idx].move;
}

class Searcher::MovePicker {
public:
    MovePicker(Searcher& searcher, Move tt_move, Move excluded, SearchStack* ss, bool is_root)
        : searcher_(searcher)
        , tt_move_(tt_move)
        , excluded_(excluded)
        , ss_(ss)
        , is_root_(is_root) {}

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
                    stage_ = Stage::Tacticals;
                    break;

                case Stage::Tacticals:
                    if (idx_ < n_)
                        return Searcher::pick_next(scored_, idx_++, n_);
                    stage_ = Stage::QuietsInit;
                    break;

                case Stage::QuietsInit:
                    fill_quiets();
                    stage_ = Stage::Quiets;
                    break;

                case Stage::Quiets:
                    if (idx_ < n_)
                        return Searcher::pick_next(scored_, idx_++, n_);
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
        Tacticals,
        QuietsInit,
        Quiets,
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
        searcher_.score_moves(scored_, n_, MOVE_NONE, ss_, is_root_);
    }

    Searcher& searcher_;
    Move tt_move_;
    Move excluded_;
    SearchStack* ss_;
    bool is_root_;
    bool tt_searched_ = false;
    Stage stage_ = Stage::TT;
    ScoredMove scored_[MoveList::CAPACITY];
    int n_ = 0;
    int idx_ = 0;
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
         + " hashfull " + std::to_string(tt_.hashfull());

    if (pv_len_[0] > 0) {
        line += " pv";
        for (int i = 0; i < pv_len_[0]; i++)
            line += ' ' + move_to_uci(pv_table_[0][i]);
    }

    if (info_cb_) info_cb_(line);
}

// ---- Quiescence search -----------------------------------------------------

int Searcher::quiescence(int alpha, int beta, int ply, int qply, SearchStack* ss) {
    nodes_++;
    if ((nodes_ & 2047) == 0) check_stop();
    if (stopped_) return 0;
    if (board_ptr_->is_draw()) return 0;
    if (ply >= MAX_PLY) return evaluator.evaluate(*board_ptr_);

    bool in_check = board_ptr_->is_in_check();

    // TT probe
    Key hash = board_ptr_->hash;
    TTEntry tte{};
    bool tt_found = tt_.probe_copy(hash, tte);
    Move tt_move = MOVE_NONE;
    if (tt_found) {
        tt_move = move_from_tt(tte.move16);
        int tt_score = TranspositionTable::score_from_tt(tte.score, ply);
        TTFlag tt_flag = TTFlag(tte.flag_age & 3);
        if (tt_flag == TT_EXACT) return tt_score;
        if (tt_flag == TT_ALPHA && tt_score <= alpha) return tt_score;
        if (tt_flag == TT_BETA  && tt_score >= beta)  return tt_score;
    }

    if (in_check) {
        // Full-move search while in check (up to 1 level)
        if (qply >= 1) return evaluator.evaluate(*board_ptr_);
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
            if (alpha >= beta) { best = beta; break; }
        }
        return has_legal ? best : -(MATE_SCORE - ply);
    }

    // Stand-pat evaluation
    int stand_pat;
    if (tt_found && tte.static_eval != TranspositionTable::INF_EVAL)
        stand_pat = tte.static_eval;
    else
        stand_pat = evaluator.evaluate(*board_ptr_);
    stand_pat += correction_value(board_ptr_->side_to_move, board_ptr_->pawn_key);
    stand_pat = std::clamp(stand_pat, -(MATE_SCORE - 1), MATE_SCORE - 1);

    if (stand_pat >= beta) {
        tt_.store(hash, 0, stand_pat, TT_BETA, MOVE_NONE, ply, stand_pat);
        return beta;
    }

    // Delta pruning: skip if even capturing the best possible piece can't raise alpha
    if (stand_pat < alpha - PIECE_VALUE[QUEEN] - 200) return alpha;

    if (stand_pat > alpha) alpha = stand_pat;

    if (qply >= MAX_QSEARCH_PLY) return alpha;

    MoveList captures;
    board_ptr_->gen_legal_captures(captures);

    // Score captures: MVV + cap_hist; prefer TT move
    ScoredMove sm[MoveList::CAPACITY];
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

        if (board_ptr_->see(m) < 0) continue;

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
            tt_.store(hash, 0, s, TT_BETA, m, ply, stand_pat);
            return beta;
        }
    }

    TTFlag flag = (alpha > orig_alpha) ? TT_EXACT : TT_ALPHA;
    tt_.store(hash, 0, alpha, flag, best_move, ply, stand_pat);
    return alpha;
}

// ---- Negamax search --------------------------------------------------------

int Searcher::negamax(int depth, int alpha, int beta, int ply,
                      SearchStack* ss, bool is_pv, bool allow_null) {
    nodes_++;
    if ((nodes_ & 2047) == 0) {
        check_stop();
    }
    if (stopped_) return 0;

    pv_len_[ply] = ply;
    bool is_root = (ply == 0);

    if (!is_root && board_ptr_->is_draw()) return 0;
    if (ply >= MAX_PLY) return evaluator.evaluate(*board_ptr_);

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
        tt_score = TranspositionTable::score_from_tt(tte.score, ply);
        tt_depth = tte.depth;
        tt_flag  = TTFlag(tte.flag_age & 3);

        if (!is_pv && ss->excluded == MOVE_NONE && tt_depth >= depth) {
            if (tt_flag == TT_EXACT) return tt_score;
            if (tt_flag == TT_ALPHA && tt_score <= alpha) return tt_score;
            if (tt_flag == TT_BETA  && tt_score >= beta)  return tt_score;
        }
    }

    // ---- Static evaluation -------------------------------------------------
    int static_eval;
    if (in_check) {
        ss->eval = static_eval = VALUE_NONE;
    } else if (ss->excluded != MOVE_NONE) {
        // Inherit eval from parent to avoid calling evaluate twice
        static_eval = ss->eval;
    } else {
        if (tt_found && tte.static_eval != TranspositionTable::INF_EVAL)
            static_eval = tte.static_eval;
        else
            static_eval = evaluator.evaluate(*board_ptr_);

        // Pawn-structure correction
        static_eval += correction_value(board_ptr_->side_to_move, board_ptr_->pawn_key);
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
            int margin = 120 * depth - (improving ? 60 : 0);
            if (static_eval - margin >= beta)
                return static_eval;
        }

        // Razoring
        if (depth <= 3 && static_eval + 300 * depth <= alpha) {
            int q = quiescence(alpha, beta, ply, 0, ss);
            if (q <= alpha) return q;
        }

        // Null-move pruning
        if (allow_null && depth >= 3
            && static_eval >= beta
            && board_ptr_->has_non_pawn_material(board_ptr_->side_to_move)
            && (ss-1)->move != MOVE_NULL) {

            int r = 4 + depth / 4 + std::min((static_eval - beta) / 200, 3);
            ss->move        = MOVE_NULL;
            ss->moved_piece = NO_PIECE_TYPE;
            board_ptr_->make_null_move();
            int null_score = -negamax(std::max(0, depth - r), -beta, -(beta - 1),
                                      ply + 1, ss + 1, false, false);
            board_ptr_->unmake_null_move();
            ss->move = MOVE_NONE;
            if (stopped_) return 0;
            if (null_score >= beta) {
                if (null_score >= MATE_SCORE - MAX_PLY) null_score = beta;
                return null_score;
            }
        }

        // ProbCut: if a capture is likely to fail high at reduced depth
        if (depth >= 5 && std::abs(beta) < MATE_SCORE - MAX_PLY) {
            int pc_beta = std::min(beta + 200, MATE_SCORE - MAX_PLY - 1);
            MoveList pcaps;
            board_ptr_->gen_legal_captures(pcaps);
            for (Move m : pcaps) {
                if (m == ss->excluded) continue;
                if (board_ptr_->see(m) < pc_beta - static_eval) continue;

                ss->move        = m;
                ss->moved_piece = type_of(board_ptr_->board_sq[from_sq(m)]);
                board_ptr_->make_move(m);
                // Quick check via QSearch first
                int val = -quiescence(-pc_beta, -pc_beta + 1, ply + 1, 0, ss + 1);
                if (val >= pc_beta)
                    val = -negamax(depth - 4, -pc_beta, -pc_beta + 1,
                                   ply + 1, ss + 1, false, true);
                board_ptr_->unmake_move(m);
                ss->move = MOVE_NONE;
                if (stopped_) return 0;
                if (val >= pc_beta) {
                    tt_.store(hash, depth - 3, pc_beta, TT_BETA, m, ply,
                              static_eval == VALUE_NONE
                                  ? TranspositionTable::INF_EVAL : static_eval);
                    return pc_beta;
                }
            }
        }
    }

    // IIR: reduce depth when no TT move (or stale TT entry) to guide the search
    if (depth >= 4 && (tt_move == MOVE_NONE || (tt_found && tt_depth < depth - 3))) depth--;

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
        if (is_root && root_filter_index_ >= 0) {
            const int ordinal = root_ordinal++;
            if ((ordinal % root_filter_count_) != root_filter_index_)
                return false;
        }

        bool is_cap   = (board_ptr_->board_sq[to_sq(m)] != NO_PIECE)
                     || (move_type(m) == EN_PASSANT);
        bool is_promo = (move_type(m) == PROMOTION);
        bool is_quiet = !is_cap && !is_promo;
        int see_score = VALUE_NONE;

        // ---- Late-move pruning / futility ----------------------------------
        if (!is_root && searched > 0 && best_score > -(MATE_SCORE - MAX_PLY)) {

            if (is_quiet) {
                // Futility pruning
                if (!is_pv && !in_check && depth <= 6
                    && static_eval != VALUE_NONE
                    && static_eval + 150 + 110 * depth <= alpha)
                    return false;

                // Late move pruning (LMP) — never in PV
                if (!is_pv && !in_check && depth <= 6 && searched >= lmp_thresh)
                    return false;

                // History pruning: skip moves with very bad combined history
                if (!is_pv && depth <= 6) {
                    PieceType pt = type_of(board_ptr_->board_sq[from_sq(m)]);
                    int hist = main_hist_[board_ptr_->side_to_move][from_sq(m)][to_sq(m)]
                             + cont_hist_score(ss, pt, Square(to_sq(m)));
                    if (hist < -3500 * depth)
                        return false;
                }
            } else if (is_cap) {
                // SEE pruning for bad captures
                if (!is_pv && depth <= 8 && !is_promo) {
                    see_score = board_ptr_->see(m);
                    if (see_score < -depth * 80) return false;
                }
            }
        }

        if (is_cap && !is_promo && depth >= 2 && searched >= 2 && see_score == VALUE_NONE)
            see_score = board_ptr_->see(m);

        // ---- Extensions -------------------------------------------------------
        int extension = 0;

        // ---- Singular extension (only for TT move) -------------------------
        if (!is_root && m == tt_move && ss->excluded == MOVE_NONE
            && depth >= 5 && tt_found && tt_depth >= depth - 3
            && (tt_flag == TT_BETA || tt_flag == TT_EXACT)
            && std::abs(tt_score) < MATE_SCORE - MAX_PLY) {

            int s_beta  = tt_score - 2 * depth;
            int s_depth = (depth - 1) / 2;

            ss->excluded = m;
            int s_val = negamax(s_depth, s_beta - 1, s_beta, ply, ss, false, false);
            ss->excluded = MOVE_NONE;

            if (stopped_) {
                immediate_return = true;
                immediate_score = 0;
                return true;
            }

            if (s_val < s_beta) {
                // TT move is singular — extend it
                extension += (!is_pv && s_val < s_beta - 20) ? 2 : 1;
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
        ss->move        = m;
        ss->moved_piece = moved_pt;
        board_ptr_->make_move(m);
        sel_depth_ = std::max(sel_depth_, ply + 1);

        int new_depth = depth - 1 + extension;

        int score;
        if (searched == 0) {
            score = -negamax(new_depth, -beta, -alpha, ply + 1, ss + 1, is_pv, true);
        } else {
            // Compute a combined history score for this move (used in LMR and pruning)
            int stat_score = 0;
            if (is_quiet) {
                // Use ~side_to_move because make_move already flipped the side
                stat_score = main_hist_[~board_ptr_->side_to_move][from_sq(m)][to_sq(m)];
                stat_score += cont_hist_score(ss, moved_pt, Square(to_sq(m)));
            }

            // Late Move Reductions
            int reduction = 0;
            // LMR applies to: quiets, and bad captures — but NOT promotions
            if (depth >= 2 && searched >= 2 && !in_check
                && (is_quiet || (is_cap && !is_promo && see_score < 0))) {
                reduction = LMR_TABLE[std::min(depth, 63)][std::min(searched, 63)];

                if (is_quiet) {
                    if (!is_pv)     reduction++;
                    if (!improving) reduction++;
                    // History-based adjustment: good moves get reduced less, bad more
                    reduction -= stat_score / 8192;
                } else {
                    // Bad captures get less reduction than quiets
                    reduction = (reduction - 1) / 2;
                }

                reduction = std::clamp(reduction, 0, new_depth - 1);
            }

            score = -negamax(new_depth - reduction, -alpha - 1, -alpha,
                             ply + 1, ss + 1, false, true);
            // Re-search at full depth if LMR didn't fail low
            if (reduction > 0 && score > alpha && !stopped_)
                score = -negamax(new_depth, -alpha - 1, -alpha,
                                 ply + 1, ss + 1, false, true);
            // Re-search as PV if score is within window
            if (is_pv && score > alpha && score < beta && !stopped_)
                score = -negamax(new_depth, -beta, -alpha,
                                 ply + 1, ss + 1, true, true);
        }

        board_ptr_->unmake_move(m);
        ss->move = MOVE_NONE;

        if (stopped_)
            return true;

        searched++;

        // Track for history updates
        if (is_cap && !is_promo) {
            if (see_score == VALUE_NONE)
                see_score = board_ptr_->see(m);
            if (see_score < 0 && bad_caps_count < MAX_TRACKED_BAD_CAPS)
                bad_caps_searched[bad_caps_count++] = m;
        } else if (is_quiet && quiets_count < MAX_TRACKED_QUIETS) {
            quiets_searched[quiets_count++] = m;
        }

        if (score > best_score) {
            best_score = score;
            best_move  = m;
            if (score > alpha) {
                alpha = score;
                // Update PV
                pv_table_[ply][ply] = m;
                for (int k = ply + 1; k < pv_len_[ply + 1]; k++)
                    pv_table_[ply][k] = pv_table_[ply + 1][k];
                pv_len_[ply] = pv_len_[ply + 1];
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
    MovePicker picker(*this, tt_move, ss->excluded, ss, is_root);
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
        update_correction(board_ptr_->side_to_move, board_ptr_->pawn_key,
                          best_score - static_eval, depth);
    }

    // Store to TT
    TTFlag flag = (best_score >= beta)    ? TT_BETA
                : (best_score > orig_alpha) ? TT_EXACT
                :                             TT_ALPHA;
    if (ss->excluded == MOVE_NONE)
        tt_.store(hash, depth, best_score, flag, best_move, ply,
                  static_eval == VALUE_NONE ? TranspositionTable::INF_EVAL : static_eval);

    return best_score;
}

// ---- Iterative deepening ---------------------------------------------------

SearchResult Searcher::search(Board board, const SearchLimits& limits) {
    board_ptr_    = &board;
    nodes_        = 0;
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

    start_time_ = std::chrono::steady_clock::now();
    compute_time_limit(limits, board.side_to_move);

    if (limits.update_tt_age)
        tt_.new_search();
    age_history();

    // Initialize search stack sentinels
    for (auto& s : ss_arr_) s = SearchStack{};
    for (int i = 0; i < 4; i++) {
        ss_arr_[i].move        = MOVE_NONE;
        ss_arr_[i].moved_piece = NO_PIECE_TYPE;
        ss_arr_[i].eval        = VALUE_NONE;
    }
    SearchStack* ss = ss_arr_ + 4; // root at offset 4

    std::memset(pv_len_, 0, sizeof(pv_len_));

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
        int score;

        if (depth <= 3 || std::abs(prev_score) >= MATE_SCORE - MAX_PLY) {
            score = negamax(depth, -INF_SCORE, INF_SCORE, 0, ss, true, true);
        } else {
            int delta = 25;
            int asp_a = prev_score - delta;
            int asp_b = prev_score + delta;
            while (true) {
                score = negamax(depth, asp_a, asp_b, 0, ss, true, true);
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
                    score = negamax(depth, asp_a, asp_b, 0, ss, true, true);
                    break;
                }
            }
        }

        if (stopped_ && depth > 1) break;

        int prev_score_saved = prev_score;
        prev_score = score;

        // Track best-move stability for adaptive soft time limit
        Move cur_best = (pv_len_[0] > 0) ? pv_table_[0][0] : MOVE_NONE;
        if (cur_best == prev_best)
            best_stability++;
        else {
            best_stability = 0;
            prev_best      = cur_best;
        }

        if (pv_len_[0] > 0) {
            result.bestmove   = pv_table_[0][0];
            result.pondermove = (pv_len_[0] > 1) ? pv_table_[0][1] : MOVE_NONE;
        }
        result.score = score;
        result.depth = depth;

        if (root_table_ && result.bestmove != MOVE_NONE)
            root_table_->update(result.bestmove, result.pondermove, depth, score);

        double elapsed = elapsed_seconds();
        send_info(depth, score, nodes_, elapsed);

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
            if (elapsed >= soft_limit_ * stability_scale * score_scale)
                break;
        }

        // Do not stop at the first forced mate. A shallow iteration can find a
        // longer checking mate before a deeper iteration sees a shorter quiet
        // mating net. Only mate-in-1 is impossible to improve.
        if (score >= MATE_SCORE - 1)
            break;
    }

    board_ptr_ = nullptr;
    root_table_ = nullptr;
    pondering_ = false;
    result.nodes      = nodes_;
    result.elapsed_ms = int64_t(elapsed_seconds() * 1000.0);
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
    ensure_threads(1);
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
    count = std::max(1, count);
    const int requested = count;
    const unsigned hw = std::thread::hardware_concurrency();
    const int runtime_cap = hw == 0 ? 1 : std::max(1, static_cast<int>(std::min(4u * hw, 1024u)));
    count = std::min(count, runtime_cap);

    if (requested > count && info_cb_) {
        info_cb_("info string Threads capped at " + std::to_string(count)
                 + " for this process");
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

    return std::min<int>(count, static_cast<int>(workers_.size()) + 1);
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

    if (worker_limits.nodes > 0)
        worker_limits.nodes = std::max<int64_t>(1, worker_limits.nodes / thread_count);

    return worker_limits;
}

SearchResult SearchThreadPool::merge_results(const std::vector<SearchResult>& results,
                                             int count,
                                             const RootMoveTable& root_table,
                                             int64_t elapsed_ms) const {
    SearchResult best = root_table.best_result();
    int64_t total_nodes = 0;

    for (int i = 0; i < count; ++i) {
        const SearchResult& result = results[static_cast<size_t>(i)];
        total_nodes += result.nodes;

        if (result.bestmove == MOVE_NONE)
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

    if (best.bestmove == MOVE_NONE && !results.empty())
        best = results.front();

    best.nodes = total_nodes;
    best.elapsed_ms = elapsed_ms;
    return best;
}

SearchResult SearchThreadPool::search(Board board, const SearchLimits& limits, int thread_count) {
    thread_count = ensure_threads(thread_count);

    if (thread_count <= 1) {
        SearchLimits worker_limits = limits;
        worker_limits.update_tt_age = true;
        worker_limits.thread_id = 0;
        worker_limits.thread_count = 1;
        worker_limits.root_table = nullptr;
        return searchers_[0]->search(std::move(board), worker_limits);
    }

    tt_.new_search();

    RootMoveTable root_table;
    root_table.reset(board);

    std::vector<SearchResult> results(static_cast<size_t>(thread_count));
    const auto wall_start = std::chrono::steady_clock::now();

    {
        std::lock_guard lock(mutex_);
        job_board_ = board;
        job_limits_ = limits;
        job_results_ = &results;
        job_root_table_ = &root_table;
        requested_helpers_ = thread_count - 1;
        active_helpers_ = requested_helpers_;
        ++epoch_;
    }
    work_cv_.notify_all();

    SearchLimits main_limits = limits_for_thread(limits, 0, thread_count, root_table);
    results[0] = searchers_[0]->search(std::move(board), main_limits);
    stop_.store(true, std::memory_order_release);

    {
        std::unique_lock lock(mutex_);
        done_cv_.wait(lock, [&] { return active_helpers_ == 0; });
        job_results_ = nullptr;
        job_root_table_ = nullptr;
        requested_helpers_ = 0;
    }

    const int64_t elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - wall_start).count();
    return merge_results(results, thread_count, root_table, elapsed_ms);
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
