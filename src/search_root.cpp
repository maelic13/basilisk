#include "search_root.h"
#include <algorithm>
#include <cstdlib>
#include <format>
#include <string>

bool move_in_root_moves(Move move, const std::vector<Move>& root_moves) {
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

bool is_legal_move_on_board(const Board& board, Move move) {
    if (move == MOVE_NONE)
        return false;

    const Square from = from_sq(move);
    const Piece piece = board.piece_on(from);
    if (piece == NO_PIECE || color_of(piece) != board.turn())
        return false;

    MoveList legal;
    board.gen_legal(legal);
    for (Move candidate : legal) {
        if (candidate == move)
            return true;
    }
    return false;
}

Move first_legal_move(const Board& board) {
    MoveList legal;
    board.gen_legal(legal);
    return legal.size() == 0 ? MOVE_NONE : legal[0];
}

int root_tablebase_ordering_score(const std::vector<Syzygy::RootMoveInfo>& root_moves, Move move) {
    for (const auto& entry : root_moves) {
        if (entry.bestmove == move) {
            return 8'000'000
                 + std::clamp(entry.rank, -2000, 2000) * 1000
                 + std::clamp(entry.score, -tablebaseValue, tablebaseValue);
        }
    }
    return 0;
}


SearchResult sanitize_search_result(const Board& root_board, SearchResult result) {
    if (!is_legal_move_on_board(root_board, result.bestmove)) {
        result.bestmove = first_legal_move(root_board);
        result.pondermove = MOVE_NONE;
        result.pv.clear();
        return result;
    }
    if (!result.pv.empty() && result.pv.front() != result.bestmove)
        result.pv.clear();

    if (result.pondermove != MOVE_NONE) {
        Board ponder_board = root_board;
        ponder_board.make_move(result.bestmove);
        if (!is_legal_move_on_board(ponder_board, result.pondermove))
            result.pondermove = MOVE_NONE;
        if (result.pondermove == MOVE_NONE && result.pv.size() > 1)
            result.pv.resize(1);
    }

    return result;
}

std::string format_info_line(int depth, int seldepth, int multipv, int score,
                             std::string_view bound, int64_t nodes, double elapsed,
                             int64_t tbhits, int hashfull, const std::vector<Move>& pv) {
    const bool mate = std::abs(score) >= MATE_SCORE - MAX_PLY;
    const int mate_in = (MATE_SCORE - std::abs(score) + 1) / 2;
    // A tablebase result n plies of tablebase play away shows as cp 20000 - n.
    if (is_tablebase_decisive(score))
        score = (score > 0 ? 1 : -1) * (tablebaseWinScore - (tablebaseValue - std::abs(score)));
    const int64_t nps = int64_t(double(nodes) / std::max(elapsed, 0.001));
    std::string line = std::format(
        "info depth {} seldepth {} multipv {} score {} {}{}{} nodes {} nps {} hashfull {} "
        "tbhits {} time {}",
        depth, seldepth, multipv, mate ? "mate" : "cp",
        mate ? (score > 0 ? mate_in : -mate_in) : score,
        bound.empty() ? "" : " ", bound, nodes, nps, hashfull, tbhits,
        int64_t(elapsed * 1000));

    if (!pv.empty()) {
        line += " pv";
        for (Move pv_move : pv)
            line += ' ' + move_to_uci(pv_move);
    }
    return line;
}

std::vector<Move> legal_line(const Board& root, const std::vector<Move>& line) {
    std::vector<Move> legal;
    Board board = root;
    for (Move move : line) {
        if (!is_legal_move_on_board(board, move))
            break;
        legal.push_back(move);
        board.make_move(move);
    }
    return legal;
}

bool needs_pool_line(const SearchResult& merged, const SearchResult& main_thread) {
    return merged.bestmove != MOVE_NONE
        && (merged.bestmove != main_thread.bestmove || merged.depth != main_thread.depth);
}
