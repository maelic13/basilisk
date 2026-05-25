#include "syzygy.h"

#include "bitboard.h"
#include "Constants.h"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <mutex>
#include <system_error>

extern "C" {
#include "tbprobe.h"
}

namespace {

std::mutex g_init_mutex;
std::atomic_bool g_enabled{false};
std::atomic_int g_largest{0};
std::atomic_int g_wdl_files{0};
std::atomic_int g_dtz_files{0};
std::string g_path;

struct TbPosition {
    uint64_t white = 0;
    uint64_t black = 0;
    uint64_t kings = 0;
    uint64_t queens = 0;
    uint64_t rooks = 0;
    uint64_t bishops = 0;
    uint64_t knights = 0;
    uint64_t pawns = 0;
    unsigned ep = 0;
    bool turn = true;
};

TbPosition to_tb_position(const Board& board) {
    TbPosition pos;
    pos.white = board.occupancy[WHITE];
    pos.black = board.occupancy[BLACK];
    pos.kings = board.pieces[WHITE][KING] | board.pieces[BLACK][KING];
    pos.queens = board.pieces[WHITE][QUEEN] | board.pieces[BLACK][QUEEN];
    pos.rooks = board.pieces[WHITE][ROOK] | board.pieces[BLACK][ROOK];
    pos.bishops = board.pieces[WHITE][BISHOP] | board.pieces[BLACK][BISHOP];
    pos.knights = board.pieces[WHITE][KNIGHT] | board.pieces[BLACK][KNIGHT];
    pos.pawns = board.pieces[WHITE][PAWN] | board.pieces[BLACK][PAWN];
    pos.ep = board.ep_sq == SQ_NONE ? 0u : static_cast<unsigned>(board.ep_sq);
    pos.turn = board.side_to_move == WHITE;
    return pos;
}

std::optional<Syzygy::Wdl> map_wdl(unsigned result) {
    switch (result) {
        case TB_LOSS: return Syzygy::Wdl::Loss;
        case TB_BLESSED_LOSS: return Syzygy::Wdl::BlessedLoss;
        case TB_DRAW: return Syzygy::Wdl::Draw;
        case TB_CURSED_WIN: return Syzygy::Wdl::CursedWin;
        case TB_WIN: return Syzygy::Wdl::Win;
        default: return std::nullopt;
    }
}

PieceType promoted_piece_from_tb(unsigned promotes) {
    switch (promotes) {
        case TB_PROMOTES_QUEEN: return QUEEN;
        case TB_PROMOTES_ROOK: return ROOK;
        case TB_PROMOTES_BISHOP: return BISHOP;
        case TB_PROMOTES_KNIGHT: return KNIGHT;
        default: return NO_PIECE_TYPE;
    }
}

Move move_from_tb(const Board& board, TbMove tb_move) {
    const Square from = Square(TB_MOVE_FROM(tb_move));
    const Square to = Square(TB_MOVE_TO(tb_move));
    const Piece moving = board.board_sq[from];
    const unsigned promotes = TB_MOVE_PROMOTES(tb_move);

    if (moving == NO_PIECE)
        return MOVE_NONE;

    if (promotes != TB_PROMOTES_NONE) {
        const PieceType pt = promoted_piece_from_tb(promotes);
        return pt == NO_PIECE_TYPE ? MOVE_NONE : make_promotion(from, to, pt);
    }

    if (type_of(moving) == PAWN && board.ep_sq == to
        && board.board_sq[to] == NO_PIECE && file_of(from) != file_of(to)) {
        return make_ep(from, to);
    }

    return make_move(from, to);
}

bool is_legal_root_move(const Board& board, Move move) {
    if (move == MOVE_NONE)
        return false;

    MoveList legal;
    board.gen_legal(legal);
    for (Move candidate : legal)
        if (candidate == move)
            return true;
    return false;
}

int normalize_root_score(int score, int rank, bool use_rule50) {
    if (use_rule50 && rank > -900 && rank < 900)
        return 0;
    if (score > 1000)
        return tablebaseWinScore;
    if (score < -1000)
        return -tablebaseWinScore;
    return std::clamp(score, -tablebaseWinScore, tablebaseWinScore);
}

std::vector<std::string> split_paths(const std::string& paths) {
    std::vector<std::string> out;
#ifdef _WIN32
    constexpr char sep = ';';
#else
    constexpr char sep = ':';
#endif
    size_t begin = 0;
    while (begin <= paths.size()) {
        const size_t end = paths.find(sep, begin);
        std::string part = paths.substr(begin, end == std::string::npos
                                               ? std::string::npos
                                               : end - begin);
        if (!part.empty())
            out.push_back(std::move(part));
        if (end == std::string::npos)
            break;
        begin = end + 1;
    }
    return out;
}

std::pair<int, int> count_tablebase_files(const std::string& paths) {
    int wdl = 0;
    int dtz = 0;

    for (const std::string& path : split_paths(paths)) {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec))
            continue;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(
                 path, std::filesystem::directory_options::skip_permission_denied, ec)) {
            if (ec)
                break;
            if (!entry.is_regular_file(ec))
                continue;
            const std::string ext = entry.path().extension().string();
            if (ext == ".rtbw")
                ++wdl;
            else if (ext == ".rtbz")
                ++dtz;
        }
    }

    return {wdl, dtz};
}

std::vector<Syzygy::RootMoveInfo> collect_root_moves(const Board& board,
                                                     const TbRootMoves& moves,
                                                     bool used_dtz,
                                                     Move preferred_move,
                                                     bool use_rule50) {
    std::vector<Syzygy::RootMoveInfo> out;
    out.reserve(moves.size);

    for (unsigned i = 0; i < moves.size; ++i) {
        const TbRootMove& tb_move = moves.moves[i];
        const Move move = move_from_tb(board, tb_move.move);
        if (!is_legal_root_move(board, move))
            continue;

        Syzygy::RootMoveInfo info;
        info.bestmove = move;
        info.rank = static_cast<int>(tb_move.tbRank);
        info.score = normalize_root_score(static_cast<int>(tb_move.tbScore),
                                          info.rank, use_rule50);
        info.used_dtz = used_dtz;
        info.pv.push_back(move);
        out.push_back(std::move(info));
    }

    std::stable_sort(out.begin(), out.end(),
        [preferred_move](const Syzygy::RootMoveInfo& a,
                         const Syzygy::RootMoveInfo& b) {
            if (a.rank != b.rank)
                return a.rank > b.rank;
            if (a.score != b.score)
                return a.score > b.score;
            if (a.bestmove == preferred_move && b.bestmove != preferred_move)
                return true;
            if (a.bestmove != preferred_move && b.bestmove == preferred_move)
                return false;
            return move_to_uci(a.bestmove) < move_to_uci(b.bestmove);
        });

    return out;
}

Move root_result_move(const Board& board, unsigned result) {
    if (result == TB_RESULT_FAILED
        || result == TB_RESULT_CHECKMATE
        || result == TB_RESULT_STALEMATE) {
        return MOVE_NONE;
    }

    const TbMove tb_move = static_cast<TbMove>(
        (TB_GET_PROMOTES(result) << 12)
        | (TB_GET_FROM(result) << 6)
        | TB_GET_TO(result));
    const Move move = move_from_tb(board, tb_move);
    return is_legal_root_move(board, move) ? move : MOVE_NONE;
}

int effective_probe_limit(int probe_limit) {
    if (probe_limit <= 0)
        return 0;
    return std::min(probe_limit, g_largest.load(std::memory_order_acquire));
}

} // namespace

namespace Syzygy {

bool init(const std::string& path) {
    std::lock_guard lock(g_init_mutex);

    tb_free();
    g_path = path;
    g_enabled.store(false, std::memory_order_release);
    g_largest.store(0, std::memory_order_release);
    g_wdl_files.store(0, std::memory_order_release);
    g_dtz_files.store(0, std::memory_order_release);

    if (path.empty())
        return true;

    if (!tb_init(path.c_str())) {
        g_path.clear();
        return false;
    }

    const auto [wdl, dtz] = count_tablebase_files(path);
    g_wdl_files.store(wdl, std::memory_order_release);
    g_dtz_files.store(dtz, std::memory_order_release);
    g_largest.store(static_cast<int>(TB_LARGEST), std::memory_order_release);
    g_enabled.store(TB_LARGEST > 0, std::memory_order_release);
    return true;
}

void clear() {
    std::lock_guard lock(g_init_mutex);
    tb_free();
    g_path.clear();
    g_enabled.store(false, std::memory_order_release);
    g_largest.store(0, std::memory_order_release);
    g_wdl_files.store(0, std::memory_order_release);
    g_dtz_files.store(0, std::memory_order_release);
}

bool enabled() {
    return g_enabled.load(std::memory_order_acquire);
}

int largest() {
    return g_largest.load(std::memory_order_acquire);
}

int wdl_file_count() {
    return g_wdl_files.load(std::memory_order_acquire);
}

int dtz_file_count() {
    return g_dtz_files.load(std::memory_order_acquire);
}

std::string path() {
    std::lock_guard lock(g_init_mutex);
    return g_path;
}

bool can_probe_root(const Board& board, int probe_limit) {
    const int limit = effective_probe_limit(probe_limit);
    return enabled()
        && limit > 0
        && board.castling_rights == NO_CASTLING
        && popcount(board.all_occ) <= limit;
}

bool can_probe_wdl(const Board& board, int probe_limit, bool use_rule50) {
    if (!can_probe_root(board, probe_limit))
        return false;
    return !use_rule50 || board.halfmove_clock == 0;
}

std::optional<Wdl> probe_wdl(const Board& board, int probe_limit, bool use_rule50) {
    if (!can_probe_wdl(board, probe_limit, use_rule50))
        return std::nullopt;

    const TbPosition pos = to_tb_position(board);
    const unsigned result = tb_probe_wdl(pos.white, pos.black, pos.kings, pos.queens,
                                         pos.rooks, pos.bishops, pos.knights,
                                         pos.pawns, 0, 0, pos.ep, pos.turn);
    if (result == TB_RESULT_FAILED)
        return std::nullopt;
    return map_wdl(result);
}

std::vector<RootMoveInfo> probe_root_moves(const Board& board, bool use_rule50,
                                           int probe_limit, bool rank_dtz) {
    if (!can_probe_root(board, probe_limit))
        return {};

    const TbPosition pos = to_tb_position(board);
    const unsigned rule50 = use_rule50 ? static_cast<unsigned>(board.halfmove_clock) : 0u;
    const unsigned dtz_best = tb_probe_root(pos.white, pos.black, pos.kings, pos.queens,
                                            pos.rooks, pos.bishops, pos.knights,
                                            pos.pawns, rule50, 0, pos.ep, pos.turn,
                                            nullptr);
    const Move preferred_move = root_result_move(board, dtz_best);

    TbRootMoves moves{};
    if (rank_dtz) {
        const int dtz_ok = tb_probe_root_dtz(pos.white, pos.black, pos.kings, pos.queens,
                                             pos.rooks, pos.bishops, pos.knights,
                                             pos.pawns, rule50, 0, pos.ep, pos.turn,
                                             false, use_rule50, &moves);
        if (dtz_ok) {
            auto out = collect_root_moves(board, moves, true, preferred_move, use_rule50);
            if (!out.empty())
                return out;
        }
    }

    moves = TbRootMoves{};
    const int wdl_ok = tb_probe_root_wdl(pos.white, pos.black, pos.kings, pos.queens,
                                         pos.rooks, pos.bishops, pos.knights,
                                         pos.pawns, rule50, 0, pos.ep, pos.turn,
                                         use_rule50, &moves);
    if (!wdl_ok)
        return {};

    return collect_root_moves(board, moves, false, preferred_move, use_rule50);
}

std::optional<RootProbeResult> probe_root(const Board& board, bool use_rule50,
                                          int probe_limit) {
    auto moves = probe_root_moves(board, use_rule50, probe_limit, true);
    if (moves.empty())
        return std::nullopt;

    RootProbeResult result;
    result.bestmove = moves.front().bestmove;
    result.score = moves.front().score;
    result.rank = moves.front().rank;
    result.used_dtz = moves.front().used_dtz;
    return result;
}

std::vector<Move> extend_pv(const Board& root, const std::vector<Move>& initial_pv,
                            bool use_rule50, int probe_limit, int max_plies) {
    Board board = root;
    std::vector<Move> pv;
    pv.reserve(static_cast<size_t>(std::max(0, max_plies)));

    for (Move move : initial_pv) {
        if (static_cast<int>(pv.size()) >= max_plies || !is_legal_root_move(board, move))
            return pv;
        pv.push_back(move);
        board.make_move(move);
        if (board.is_draw())
            return pv;
    }

    while (static_cast<int>(pv.size()) < max_plies && can_probe_root(board, probe_limit)) {
        auto moves = probe_root_moves(board, use_rule50, probe_limit, true);
        if (moves.empty())
            break;
        Move move = moves.front().bestmove;
        if (!is_legal_root_move(board, move))
            break;
        pv.push_back(move);
        board.make_move(move);
        if (board.is_draw())
            break;
    }

    return pv;
}

} // namespace Syzygy
