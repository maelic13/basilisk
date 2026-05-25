#include "syzygy.h"

#include "bitboard.h"

#include <algorithm>
#include <atomic>
#include <mutex>

extern "C" {
#include "tbprobe.h"
}

namespace {

std::mutex g_init_mutex;
std::atomic_bool g_enabled{false};
std::atomic_int g_largest{0};
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

std::optional<Syzygy::RootProbeResult> pick_root_move(const Board& board,
                                                      const TbRootMoves& moves,
                                                      bool used_dtz) {
    Syzygy::RootProbeResult best;
    bool found = false;

    for (unsigned i = 0; i < moves.size; ++i) {
        const TbRootMove& tb_move = moves.moves[i];
        const Move move = move_from_tb(board, tb_move.move);
        if (!is_legal_root_move(board, move))
            continue;

        if (!found || tb_move.tbRank > best.rank
            || (tb_move.tbRank == best.rank && tb_move.tbScore > best.score)) {
            best.bestmove = move;
            best.score = std::clamp(static_cast<int>(tb_move.tbScore), -31999, 31999);
            best.rank = static_cast<int>(tb_move.tbRank);
            best.used_dtz = used_dtz;
            found = true;
        }
    }

    if (!found)
        return std::nullopt;
    return best;
}

} // namespace

namespace Syzygy {

bool init(const std::string& path) {
    std::lock_guard lock(g_init_mutex);

    tb_free();
    g_path = path;
    g_enabled.store(false, std::memory_order_release);
    g_largest.store(0, std::memory_order_release);

    if (path.empty())
        return true;

    if (!tb_init(path.c_str())) {
        g_path.clear();
        return false;
    }

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
}

bool enabled() {
    return g_enabled.load(std::memory_order_acquire);
}

int largest() {
    return g_largest.load(std::memory_order_acquire);
}

std::string path() {
    std::lock_guard lock(g_init_mutex);
    return g_path;
}

bool can_probe_root(const Board& board) {
    return enabled()
        && board.castling_rights == NO_CASTLING
        && popcount(board.all_occ) <= largest();
}

bool can_probe_wdl(const Board& board) {
    return can_probe_root(board) && board.halfmove_clock == 0;
}

std::optional<Wdl> probe_wdl(const Board& board) {
    if (!can_probe_wdl(board))
        return std::nullopt;

    const TbPosition pos = to_tb_position(board);
    const unsigned result = tb_probe_wdl(pos.white, pos.black, pos.kings, pos.queens,
                                         pos.rooks, pos.bishops, pos.knights,
                                         pos.pawns, 0, 0, pos.ep, pos.turn);
    if (result == TB_RESULT_FAILED)
        return std::nullopt;
    return map_wdl(result);
}

std::optional<RootProbeResult> probe_root(const Board& board, bool use_rule50) {
    if (!can_probe_root(board))
        return std::nullopt;

    const TbPosition pos = to_tb_position(board);
    const unsigned rule50 = use_rule50 ? static_cast<unsigned>(board.halfmove_clock) : 0u;

    TbRootMoves moves{};
    const int dtz_ok = tb_probe_root_dtz(pos.white, pos.black, pos.kings, pos.queens,
                                         pos.rooks, pos.bishops, pos.knights,
                                         pos.pawns, rule50, 0, pos.ep, pos.turn,
                                         false, use_rule50, &moves);
    if (dtz_ok) {
        if (auto result = pick_root_move(board, moves, true))
            return result;
    }

    moves = TbRootMoves{};
    const int wdl_ok = tb_probe_root_wdl(pos.white, pos.black, pos.kings, pos.queens,
                                         pos.rooks, pos.bishops, pos.knights,
                                         pos.pawns, rule50, 0, pos.ep, pos.turn,
                                         use_rule50, &moves);
    if (!wdl_ok)
        return std::nullopt;

    return pick_root_move(board, moves, false);
}

} // namespace Syzygy
