#include "search.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <sstream>
#include <string>

// ---- LMR table initialization ----------------------------------------------

int  Searcher::LMR_TABLE[64][64];
bool Searcher::lmr_init_ = false;

void Searcher::init_lmr() {
    if (lmr_init_) return;
    for (int depth = 1; depth < 64; depth++)
        for (int moves = 1; moves < 64; moves++)
            LMR_TABLE[depth][moves] = int(0.75 + std::log(depth) * std::log(moves) / 2.25);
    lmr_init_ = true;
}

// ---- Constructor -----------------------------------------------------------

Searcher::Searcher(TranspositionTable& tt,
                   std::atomic_bool& stop_flag,
                   std::function<void(const std::string&)> info_cb)
    : evaluator()
    , tt_(tt)
    , stop_(stop_flag)
    , info_cb_(info_cb)
    , board_ptr_(nullptr)
    , nodes_(0)
    , sel_depth_(0)
    , stopped_(false)
    , time_limit_(0.0)
{
    init_lmr();
    std::memset(killers_,     0, sizeof(killers_));
    std::memset(history_,     0, sizeof(history_));
    std::memset(countermove_, 0, sizeof(countermove_));
    std::memset(pv_table_,    0, sizeof(pv_table_));
    std::memset(pv_len_,      0, sizeof(pv_len_));
    std::memset(eval_stack_,  0, sizeof(eval_stack_));
}

// ---- Time management -------------------------------------------------------

void Searcher::compute_time_limit(const SearchLimits& limits, Color side) {
    if (limits.infinite || limits.ponder) { time_limit_ = 0.0; return; }
    if (limits.movetime > 0) { time_limit_ = std::max(1, limits.movetime - 50) / 1000.0; return; }

    int remaining = (side == WHITE) ? limits.wtime : limits.btime;
    int inc       = (side == WHITE) ? limits.winc  : limits.binc;

    if (remaining <= 0 && inc <= 0) { time_limit_ = 0.0; return; }

    int base;
    if (limits.movestogo > 0)
        base = remaining / (limits.movestogo + 5);
    else
        base = remaining / 30;

    int time_ms  = base + (inc * 3) / 4;
    int hard_ms  = (remaining < 1000) ? remaining * 15 / 100
                 : (remaining < 5000) ? remaining * 25 / 100
                                      : remaining * 40 / 100;

    time_limit_ = std::max(50, std::min(time_ms, hard_ms)) / 1000.0;
}

double Searcher::elapsed_seconds() const {
    using namespace std::chrono;
    return duration<double>(steady_clock::now() - start_time_).count();
}

bool Searcher::check_stop() {
    if (stopped_) return true;
    if (stop_.load()) { stopped_ = true; return true; }
    if (time_limit_ > 0.0 && elapsed_seconds() >= time_limit_) {
        stopped_ = true;
        return true;
    }
    return false;
}

// ---- History ---------------------------------------------------------------

void Searcher::update_history(Color c, Square from, Square to, int bonus) {
    int& h = history_[c][from][to];
    h += bonus - h * std::abs(bonus) / MAX_HISTORY;
}

void Searcher::history_bonus(Color c, Square from, Square to, int depth) {
    update_history(c, from, to, depth * depth);
}

void Searcher::history_malus(Color c, Square from, Square to, int depth) {
    update_history(c, from, to, -depth * depth);
}

void Searcher::age_history() {
    for (int c = 0; c < NCOLORS; c++)
        for (int f = 0; f < SQUARE_NB; f++)
            for (int t = 0; t < SQUARE_NB; t++)
                history_[c][f][t] /= 2;
}

// ---- Move ordering ---------------------------------------------------------

void Searcher::score_moves(ScoredMove* moves, int n, Move tt_move, int ply, Move prev_move) const {
    static constexpr int SEE_VALUES[PIECE_TYPE_NB] = {0, 100, 300, 300, 500, 900, 20000};

    const Board& b = *board_ptr_;
    Move cm = (prev_move && prev_move != MOVE_NULL)
              ? countermove_[from_sq(prev_move)][to_sq(prev_move)]
              : MOVE_NONE;

    for (int i = 0; i < n; i++) {
        Move m = moves[i].move;

        if (m == tt_move) {
            moves[i].score = 10'000'000;
            continue;
        }

        bool is_cap = (b.board_sq[to_sq(m)] != NO_PIECE) || (move_type(m) == EN_PASSANT);
        bool is_promo = (move_type(m) == PROMOTION);

        if (is_cap) {
            int cap_val = (move_type(m) == EN_PASSANT) ? SEE_VALUES[PAWN]
                                                       : SEE_VALUES[type_of(b.board_sq[to_sq(m)])];
            int atk_val = SEE_VALUES[type_of(b.board_sq[from_sq(m)])];
            int see_val = b.see(m);
            if (see_val >= 0)
                moves[i].score = 6'000'000 + cap_val * 16 - atk_val;
            else
                moves[i].score = 2'000'000 + see_val;
        } else if (is_promo) {
            if (promo_type(m) == QUEEN)
                moves[i].score = 5'000'000;
            else
                moves[i].score = -100;
        } else {
            if (m == killers_[ply][0])      moves[i].score = 4'000'000;
            else if (m == killers_[ply][1]) moves[i].score = 3'900'000;
            else if (m == cm)               moves[i].score = 3'800'000;
            else                            moves[i].score = history_[b.side_to_move][from_sq(m)][to_sq(m)];
        }
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

// ---- UCI info output -------------------------------------------------------

void Searcher::send_info(int depth, int score, int64_t total_nodes, double elapsed) const {
    std::string line = "info depth " + std::to_string(depth)
        + " seldepth " + std::to_string(sel_depth_)
        + " score ";

    if (std::abs(score) >= MATE_SCORE - MAX_PLY) {
        int moves_to_mate = (MATE_SCORE - std::abs(score) + 1) / 2;
        line += "mate " + std::to_string(score > 0 ? moves_to_mate : -moves_to_mate);
    } else {
        line += "cp " + std::to_string(score);
    }

    int64_t nps = elapsed > 0.0 ? int64_t(double(total_nodes) / elapsed) : 0;
    line += " nodes " + std::to_string(total_nodes)
         + " nps "   + std::to_string(nps)
         + " time "  + std::to_string(int64_t(elapsed * 1000))
         + " hashfull " + std::to_string(tt_.hashfull());

    // PV
    if (pv_len_[0] > 0) {
        line += " pv";
        for (int i = 0; i < pv_len_[0]; i++)
            line += ' ' + move_to_uci(pv_table_[0][i]);
    }

    if (info_cb_) info_cb_(line);
}

// ---- Quiescence search -----------------------------------------------------

int Searcher::quiescence(int alpha, int beta, int ply) {
    nodes_++;
    if ((nodes_ & 2047) == 0) check_stop();

    if (stopped_) return 0;
    if (board_ptr_->is_draw()) return 0;
    if (ply >= MAX_PLY) return evaluator.evaluate(*board_ptr_);

    bool in_check = board_ptr_->is_in_check();

    if (in_check) {
        // Generate all legal moves as evasions
        std::vector<Move> pseudo;
        board_ptr_->gen_pseudo_legal(pseudo);

        bool has_legal = false;
        int best = -INF_SCORE;

        for (Move m : pseudo) {
            if (!board_ptr_->is_legal(m)) continue;
            has_legal = true;
            board_ptr_->make_move(m);
            int score = -quiescence(-beta, -alpha, ply + 1);
            board_ptr_->unmake_move(m);
            if (stopped_) return 0;
            if (score > best) best = score;
            if (score > alpha) alpha = score;
            if (alpha >= beta) return beta;
        }
        if (!has_legal)
            return -(MATE_SCORE - ply);
        return best;
    }

    int stand_pat = evaluator.evaluate(*board_ptr_);
    if (stand_pat >= beta) return beta;
    if (stand_pat < alpha - 900) return alpha; // delta pruning
    if (stand_pat > alpha) alpha = stand_pat;

    std::vector<Move> captures;
    board_ptr_->gen_pseudo_legal_captures(captures);

    // Score by MVV-LVA
    static constexpr int SEE_VALUES[PIECE_TYPE_NB] = {0, 100, 300, 300, 500, 900, 20000};
    std::vector<ScoredMove> sm;
    sm.reserve(captures.size());
    for (Move m : captures) {
        int cap_val = (move_type(m) == EN_PASSANT) ? SEE_VALUES[PAWN]
                    : (board_ptr_->board_sq[to_sq(m)] != NO_PIECE
                        ? SEE_VALUES[type_of(board_ptr_->board_sq[to_sq(m)])] : 0);
        int atk_val = SEE_VALUES[type_of(board_ptr_->board_sq[from_sq(m)])];
        sm.push_back({m, cap_val * 100 - atk_val});
    }

    for (int i = 0; i < (int)sm.size(); i++) {
        Move m = pick_next(sm.data(), i, (int)sm.size());
        if (!board_ptr_->is_legal(m)) continue;
        if (board_ptr_->see(m) < 0) continue;

        board_ptr_->make_move(m);
        int score = -quiescence(-beta, -alpha, ply + 1);
        board_ptr_->unmake_move(m);

        if (stopped_) return 0;
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

    return alpha;
}

// ---- Negamax search --------------------------------------------------------

int Searcher::negamax(int depth, int alpha, int beta, int ply,
                      bool is_pv, bool allow_null, Move prev_move) {
    nodes_++;
    if ((nodes_ & 2047) == 0) check_stop();
    if (stopped_) return 0;

    // PV table init
    pv_len_[ply] = ply;

    // Draw check
    if (ply > 0 && board_ptr_->is_draw()) return 0;
    if (ply >= MAX_PLY) return evaluator.evaluate(*board_ptr_);

    bool in_check = board_ptr_->is_in_check();

    // Drop into quiescence
    if (depth <= 0 && !in_check)
        return quiescence(alpha, beta, ply);

    // Mate distance pruning
    {
        int mda = -(MATE_SCORE - ply);
        int mdb =  (MATE_SCORE - ply - 1);
        if (mda > alpha) alpha = mda;
        if (mdb < beta)  beta  = mdb;
        if (alpha >= beta) return alpha;
    }

    // TT probe
    bool tt_found = false;
    TTEntry* tte  = tt_.probe(board_ptr_->hash, tt_found);
    Move tt_move  = MOVE_NONE;
    int static_eval = TranspositionTable::INF_EVAL;

    if (tt_found) {
        tt_move = move_from_tt(tte->move16);
        if (tte->static_eval != TranspositionTable::INF_EVAL)
            static_eval = tte->static_eval;

        if (!is_pv && tte->depth >= depth) {
            int tt_score = TranspositionTable::score_from_tt(tte->score, ply);
            TTFlag flag  = TTFlag(tte->flag_age & 3);
            if (flag == TT_EXACT) return tt_score;
            if (flag == TT_ALPHA && tt_score <= alpha) return tt_score;
            if (flag == TT_BETA  && tt_score >= beta)  return tt_score;
        }
    }

    // Compute static eval
    if (static_eval == TranspositionTable::INF_EVAL && !in_check)
        static_eval = evaluator.evaluate(*board_ptr_);
    eval_stack_[ply] = in_check ? TranspositionTable::INF_EVAL : static_eval;

    // Improving flag
    bool improving = false;
    if (!in_check && ply >= 2 && eval_stack_[ply-2] != TranspositionTable::INF_EVAL)
        improving = static_eval > eval_stack_[ply-2];

    // ---- Pruning (skip in PV / in-check / root) ----------------------------
    if (!is_pv && !in_check && depth > 0) {

        // Reverse futility pruning
        if (depth <= 9 && static_eval != TranspositionTable::INF_EVAL) {
            int rfp = 120 * depth + (improving ? 0 : 80);
            if (static_eval - rfp >= beta) return static_eval;
        }

        // Razoring
        if (depth <= 3 && static_eval + 300 * depth <= alpha) {
            int q = quiescence(alpha, beta, ply);
            if (q <= alpha) return q;
        }

        // Null-move pruning
        if (allow_null && depth >= 3
            && static_eval >= beta
            && board_ptr_->has_non_pawn_material(board_ptr_->side_to_move)) {
            int r = 4 + depth / 4;
            board_ptr_->make_null_move();
            int null_score = -negamax(std::max(0, depth - 1 - r), -beta, -beta + 1,
                                      ply + 1, false, false, MOVE_NULL);
            board_ptr_->unmake_null_move();
            if (stopped_) return 0;
            if (null_score >= beta) return beta;
        }
    }

    // IIR: reduce depth when no TT move
    if (tt_move == MOVE_NONE && depth >= 4) depth--;

    // ---- Generate and score moves ------------------------------------------
    std::vector<Move> pseudo;
    pseudo.reserve(64);
    board_ptr_->gen_pseudo_legal(pseudo);

    std::vector<ScoredMove> move_list;
    move_list.reserve(pseudo.size());
    for (Move m : pseudo) move_list.push_back({m, 0});
    score_moves(move_list.data(), (int)move_list.size(), tt_move, ply, prev_move);

    int orig_alpha = alpha;
    Move best_move = MOVE_NONE;
    int best_score = -INF_SCORE;
    int searched   = 0;

    std::vector<Move> searched_quiets;
    searched_quiets.reserve(32);

    int futility_base = INF_SCORE;
    if (!is_pv && !in_check && depth <= 6 && static_eval != TranspositionTable::INF_EVAL)
        futility_base = static_eval + 160 * depth + (improving ? 0 : 100);

    int lmp_threshold = improving ? (3 + depth * depth) : (2 + depth * depth / 2);

    for (int i = 0; i < (int)move_list.size(); i++) {
        Move m = pick_next(move_list.data(), i, (int)move_list.size());
        if (!board_ptr_->is_legal(m)) continue;

        bool is_cap   = (board_ptr_->board_sq[to_sq(m)] != NO_PIECE) || (move_type(m) == EN_PASSANT);
        bool is_promo = (move_type(m) == PROMOTION);
        bool is_quiet = !is_cap && !is_promo;

        // Late-move pruning (not PV, not in-check, not near mate)
        if (searched > 0 && best_score > -(MATE_SCORE - MAX_PLY)) {
            if (futility_base != INF_SCORE && is_quiet && futility_base <= alpha)
                continue;
            if (!is_pv && !in_check && is_quiet && depth <= 5 && searched >= lmp_threshold)
                continue;
            if (!is_pv && !in_check && is_cap && depth <= 6 && board_ptr_->see(m) < -depth * 80)
                continue;
            if (!is_pv && !in_check && is_quiet && depth <= 4 && searched > 3) {
                if (history_[board_ptr_->side_to_move][from_sq(m)][to_sq(m)] < -(depth * depth * 64))
                    continue;
            }
        }

        board_ptr_->make_move(m);
        sel_depth_ = std::max(sel_depth_, ply + 1);

        bool gives_check = board_ptr_->is_in_check();
        int  extension   = (gives_check && depth <= 8) ? 1 : 0;
        int  new_depth   = depth - 1 + extension;

        int score;
        if (searched == 0) {
            score = -negamax(new_depth, -beta, -alpha, ply + 1, is_pv, true, m);
        } else {
            // LMR
            int reduction = 0;
            if (searched >= 3 && depth >= 3 && !in_check && !gives_check && is_quiet) {
                reduction = LMR_TABLE[std::min(depth, 63)][std::min(searched, 63)];
                if (!is_pv)   reduction++;
                if (!improving) reduction++;
                reduction = std::min(reduction, new_depth - 1);
                reduction = std::max(reduction, 0);
            }

            score = -negamax(new_depth - reduction, -alpha - 1, -alpha, ply + 1, false, true, m);
            if (reduction > 0 && score > alpha && !stopped_)
                score = -negamax(new_depth, -alpha - 1, -alpha, ply + 1, false, true, m);
            if (score > alpha && score < beta && !stopped_)
                score = -negamax(new_depth, -beta, -alpha, ply + 1, true, true, m);
        }

        board_ptr_->unmake_move(m);

        if (stopped_) {
            // Return best we have so far, but only at ply > 0
            return (ply == 0 && best_move != MOVE_NONE) ? best_score : 0;
        }

        searched++;
        if (is_quiet) searched_quiets.push_back(m);

        if (score > best_score) {
            best_score = score;
            best_move  = m;
            // Update triangular PV
            pv_table_[ply][ply] = m;
            for (int k = ply + 1; k < pv_len_[ply + 1]; k++)
                pv_table_[ply][k] = pv_table_[ply + 1][k];
            pv_len_[ply] = pv_len_[ply + 1];
        }

        if (score > alpha) alpha = score;

        if (alpha >= beta) {
            // Beta cutoff
            if (is_quiet) {
                if (killers_[ply][0] != m) {
                    killers_[ply][1] = killers_[ply][0];
                    killers_[ply][0] = m;
                }
                history_bonus(board_ptr_->side_to_move, from_sq(m), to_sq(m), depth);
                for (Move q : searched_quiets) {
                    if (q != m)
                        history_malus(board_ptr_->side_to_move, from_sq(q), to_sq(q), depth);
                }
                if (prev_move && prev_move != MOVE_NULL)
                    countermove_[from_sq(prev_move)][to_sq(prev_move)] = m;
            }
            tt_.store(board_ptr_->hash, depth, beta, TT_BETA, m, ply, static_eval);
            return beta;
        }
    }

    // No legal moves
    if (searched == 0) {
        return in_check ? -(MATE_SCORE - ply) : 0;
    }

    TTFlag flag = (best_score > orig_alpha) ? TT_EXACT : TT_ALPHA;
    tt_.store(board_ptr_->hash, depth, best_score, flag, best_move, ply, static_eval);
    return best_score;
}

// ---- Iterative deepening ---------------------------------------------------

SearchResult Searcher::search(Board board, const SearchLimits& limits) {
    board_ptr_ = &board;
    nodes_     = 0;
    sel_depth_ = 0;
    stopped_   = false;

    start_time_ = std::chrono::steady_clock::now();
    compute_time_limit(limits, board.side_to_move);

    tt_.new_search();
    age_history();

    // Clear per-search state
    std::memset(killers_, 0, sizeof(killers_));
    std::memset(pv_len_,  0, sizeof(pv_len_));
    std::memset(eval_stack_, 0, sizeof(eval_stack_));

    SearchResult result;
    int prev_score = 0;

    int max_depth = limits.infinite ? MAX_SEARCH_DEPTH
                  : std::min(limits.depth, MAX_SEARCH_DEPTH);

    for (int depth = 1; depth <= max_depth && !stopped_; depth++) {
        pv_len_[0] = 0;
        int score;

        if (depth <= 3) {
            score = negamax(depth, -INF_SCORE, INF_SCORE, 0, true, true, MOVE_NONE);
        } else {
            // Aspiration windows
            constexpr int ASP_WIN   = 30;
            int delta = ASP_WIN;
            int a = prev_score - delta;
            int b = prev_score + delta;

            while (true) {
                score = negamax(depth, a, b, 0, true, true, MOVE_NONE);
                if (stopped_) break;
                if (score <= a) {
                    a -= delta;
                    delta *= 4;
                } else if (score >= b) {
                    b += delta;
                    delta *= 4;
                } else {
                    break;
                }
            }
        }

        if (stopped_ && depth > 1) break;

        prev_score = score;

        if (pv_len_[0] > 0) {
            result.bestmove   = pv_table_[0][0];
            result.pondermove = (pv_len_[0] > 1) ? pv_table_[0][1] : MOVE_NONE;
        }
        result.score = score;
        result.depth = depth;

        double elapsed = elapsed_seconds();
        send_info(depth, score, nodes_, elapsed);

        // Time management: if we've used > 50% of allocated time, stop
        if (time_limit_ > 0.0 && elapsed >= time_limit_ * 0.5 && !limits.ponder)
            break;

        // Early exit on forced mate
        if (std::abs(score) >= MATE_SCORE - MAX_PLY)
            break;
    }

    board_ptr_ = nullptr;
    return result;
}
