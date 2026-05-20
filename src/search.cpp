#include "search.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <sstream>
#include <string>

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
    // stop_ is the "go" flag: false means the GUI sent "stop"
    if (!stop_.load()) { stopped_ = true; return true; }
    if (hard_limit_ > 0.0 && elapsed_seconds() >= hard_limit_) {
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
                                    const std::vector<Move>& quiets,
                                    const std::vector<Move>& bad_caps,
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
        for (Move m : quiets) {
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
    for (Move m : bad_caps) {
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

void Searcher::score_moves(ScoredMove* moves, int n, Move tt_move, SearchStack* ss) const {
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
            int see = b.see(m);
            if (see >= 0)
                moves[i].score = 6'000'000 + PIECE_VALUE[cap] * 16 - PIECE_VALUE[atk]
                                           + cap_hist_[atk][to_sq(m)][cap];
            else
                moves[i].score = 2'000'000 + see;
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

    if (in_check) {
        // One level of in-check full-move search to avoid missing forced mates.
        if (qply >= 1) return evaluator.evaluate(*board_ptr_);
        std::vector<Move> pseudo;
        board_ptr_->gen_pseudo_legal(pseudo);
        int best = -INF_SCORE;
        bool has_legal = false;
        for (Move m : pseudo) {
            if (!board_ptr_->is_legal(m)) continue;
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
            if (alpha >= beta) return beta;
        }
        return has_legal ? best : -(MATE_SCORE - ply);
    }

    // Stand-pat evaluation
    int stand_pat = evaluator.evaluate(*board_ptr_);
    Key pk = board_ptr_->pieces[WHITE][PAWN] ^ board_ptr_->pieces[BLACK][PAWN];
    stand_pat += correction_value(board_ptr_->side_to_move, pk);
    stand_pat = std::clamp(stand_pat, -(MATE_SCORE - 1), MATE_SCORE - 1);

    if (stand_pat >= beta) return beta;

    // Delta pruning: skip if even capturing the best possible piece can't raise alpha
    if (stand_pat < alpha - PIECE_VALUE[QUEEN] - 200) return alpha;

    if (stand_pat > alpha) alpha = stand_pat;

    if (qply >= MAX_QSEARCH_PLY) return alpha;

    std::vector<Move> captures;
    board_ptr_->gen_pseudo_legal_captures(captures);

    // Score captures: MVV + cap_hist
    std::vector<ScoredMove> sm;
    sm.reserve(captures.size());
    for (Move m : captures) {
        PieceType atk = type_of(board_ptr_->board_sq[from_sq(m)]);
        PieceType cap = (move_type(m) == EN_PASSANT) ? PAWN : type_of(board_ptr_->board_sq[to_sq(m)]);
        int score = PIECE_VALUE[cap] * 16 - PIECE_VALUE[atk]
                  + cap_hist_[atk][to_sq(m)][cap];
        sm.push_back({m, score});
    }

    for (int i = 0; i < (int)sm.size(); i++) {
        Move m = pick_next(sm.data(), i, (int)sm.size());
        if (!board_ptr_->is_legal(m)) continue;

        if (board_ptr_->see(m) < 0) continue;

        ss->move = m;
        ss->moved_piece = type_of(board_ptr_->board_sq[from_sq(m)]);
        board_ptr_->make_move(m);
        int s = -quiescence(-beta, -alpha, ply + 1, qply + 1, ss + 1);
        board_ptr_->unmake_move(m);
        ss->move = MOVE_NONE;

        if (stopped_) return 0;
        if (s >= beta) return beta;
        if (s > alpha) alpha = s;
    }

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
    bool tt_found = false;
    TTEntry* tte  = tt_.probe(hash, tt_found);

    Move  tt_move  = MOVE_NONE;
    int   tt_score = VALUE_NONE;
    int   tt_depth = 0;
    TTFlag tt_flag  = TT_NONE;

    if (tt_found) {
        tt_move  = move_from_tt(tte->move16);
        tt_score = TranspositionTable::score_from_tt(tte->score, ply);
        tt_depth = tte->depth;
        tt_flag  = TTFlag(tte->flag_age & 3);

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
        if (tt_found && tte->static_eval != TranspositionTable::INF_EVAL)
            static_eval = tte->static_eval;
        else
            static_eval = evaluator.evaluate(*board_ptr_);

        // Pawn-structure correction
        Key pawn_key = board_ptr_->pieces[WHITE][PAWN] ^ board_ptr_->pieces[BLACK][PAWN];
        static_eval += correction_value(board_ptr_->side_to_move, pawn_key);
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
            std::vector<Move> pcaps;
            board_ptr_->gen_pseudo_legal_captures(pcaps);
            for (Move m : pcaps) {
                if (m == ss->excluded || !board_ptr_->is_legal(m)) continue;
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

    // IIR: reduce depth when no TT move to guide the search
    if (tt_move == MOVE_NONE && depth >= 4) depth--;

    // ---- Generate and score moves ------------------------------------------
    std::vector<Move> pseudo;
    pseudo.reserve(64);
    board_ptr_->gen_pseudo_legal(pseudo);

    std::vector<ScoredMove> move_list;
    move_list.reserve(pseudo.size());
    for (Move m : pseudo)
        if (m != ss->excluded)
            move_list.push_back({m, 0});

    score_moves(move_list.data(), (int)move_list.size(), tt_move, ss);

    int  orig_alpha  = alpha;
    Move best_move   = MOVE_NONE;
    int  best_score  = -INF_SCORE;
    int  searched    = 0;

    std::vector<Move> quiets_searched;
    std::vector<Move> bad_caps_searched;
    quiets_searched.reserve(32);
    bad_caps_searched.reserve(8);

    int lmp_thresh = improving ? (3 + depth * depth) : (2 + depth * depth / 2);

    for (int i = 0; i < (int)move_list.size(); i++) {
        Move m      = pick_next(move_list.data(), i, (int)move_list.size());
        int  mscore = move_list[i].score; // score after pick_next

        if (!board_ptr_->is_legal(m)) continue;

        bool is_cap   = (board_ptr_->board_sq[to_sq(m)] != NO_PIECE)
                     || (move_type(m) == EN_PASSANT);
        bool is_promo = (move_type(m) == PROMOTION);
        bool is_quiet = !is_cap && !is_promo;

        // ---- Late-move pruning / futility ----------------------------------
        if (!is_root && searched > 0 && best_score > -(MATE_SCORE - MAX_PLY)) {

            if (is_quiet) {
                // Futility pruning
                if (!is_pv && !in_check && depth <= 6
                    && static_eval != VALUE_NONE
                    && static_eval + 150 + 110 * depth <= alpha)
                    continue;

                // Late move pruning (LMP) — never in PV
                if (!is_pv && !in_check && depth <= 6 && searched >= lmp_thresh)
                    continue;

                // History pruning: skip moves with very bad combined history
                if (!is_pv && depth <= 6) {
                    PieceType pt = type_of(board_ptr_->board_sq[from_sq(m)]);
                    int hist = main_hist_[board_ptr_->side_to_move][from_sq(m)][to_sq(m)]
                             + cont_hist_score(ss, pt, Square(to_sq(m)));
                    if (hist < -3500 * depth)
                        continue;
                }
            } else if (is_cap) {
                // SEE pruning for bad captures
                if (!is_pv && depth <= 8 && mscore < 6'000'000) {
                    int embedded_see = mscore - 2'000'000;
                    if (embedded_see < -depth * 80) continue;
                }
            }
        }

        // ---- Singular extension (only for TT move) -------------------------
        int extension = 0;
        if (!is_root && m == tt_move && ss->excluded == MOVE_NONE
            && depth >= 8 && tt_found && tt_depth >= depth - 3
            && (tt_flag == TT_BETA || tt_flag == TT_EXACT)
            && std::abs(tt_score) < MATE_SCORE - MAX_PLY) {

            int s_beta  = tt_score - 2 * depth;
            int s_depth = (depth - 1) / 2;

            ss->excluded = m;
            int s_val = negamax(s_depth, s_beta - 1, s_beta, ply, ss, false, false);
            ss->excluded = MOVE_NONE;

            if (stopped_) return 0;

            if (s_val < s_beta) {
                // TT move is singular — extend it
                extension = (!is_pv && s_val < s_beta - 20) ? 2 : 1;
            } else if (s_beta >= beta) {
                // Multicut: likely to fail high without this move too
                return s_beta;
            } else if (tt_score >= beta) {
                extension = -1; // Negative extension: not clearly best
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
                && (is_quiet || (is_cap && !is_promo && mscore < 6'000'000))) {
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
            return (is_root && best_move != MOVE_NONE) ? best_score : 0;

        searched++;

        // Track for history updates
        if (is_cap && mscore < 6'000'000)
            bad_caps_searched.push_back(m);
        else if (is_quiet)
            quiets_searched.push_back(m);

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
            update_all_histories(m, quiets_searched, bad_caps_searched,
                                 board_ptr_->side_to_move, depth, ss);
            break;
        }
    }

    // No legal moves
    if (searched == 0)
        return in_check ? -(MATE_SCORE - ply) : 0;

    // Update correction history with search result
    if (!in_check && ss->excluded == MOVE_NONE && static_eval != VALUE_NONE
        && std::abs(best_score) < MATE_SCORE - MAX_PLY
        && (best_score >= beta || best_score > orig_alpha)) {
        Key pawn_key = board_ptr_->pieces[WHITE][PAWN] ^ board_ptr_->pieces[BLACK][PAWN];
        update_correction(board_ptr_->side_to_move, pawn_key,
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
    board_ptr_ = &board;
    nodes_     = 0;
    sel_depth_ = 0;
    stopped_   = false;

    start_time_ = std::chrono::steady_clock::now();
    compute_time_limit(limits, board.side_to_move);

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

    for (int depth = 1; depth <= max_depth && !stopped_; depth++) {
        pv_len_[0] = 0;
        int score;

        if (depth <= 3) {
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

        double elapsed = elapsed_seconds();
        send_info(depth, score, nodes_, elapsed);

        // Adaptive soft time limit:
        // The more stable the best move, the less time we need to confirm it.
        // stability=0 → 100% of soft, stability=6+ → ~64% of soft
        if (soft_limit_ > 0.0 && !limits.ponder) {
            double stability_scale = 1.0 - 0.06 * std::min(best_stability, 6);
            if (elapsed >= soft_limit_ * stability_scale)
                break;
        }

        if (std::abs(score) >= MATE_SCORE - MAX_PLY)
            break;
    }

    board_ptr_ = nullptr;
    return result;
}
