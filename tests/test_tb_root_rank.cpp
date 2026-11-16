/// The root's tablebase ranking after a repetition in the game.
///
/// Once the position has repeated since the last zeroing move, every winning
/// root move may no longer rank alike: a won game that drifts can be drawn by
/// threefold (BAS-S39's game 4965 shuffled a won rook, knight and pawn against
/// a queen into one). Stockfish then ranks the wins by DTZ, so that only the
/// moves nearest a zeroing move stay in the best rank.
///
/// 1. On the KQvK fixture, after Qd2 Ke4 Qc2 Ke5 Qd2 Ke4 Qd1 Ke5 the root has
///    occurred twice, and of its 20 winning moves the tables put five at the
///    shortest DTZ (16 against 18): Qh5, Qf3, Qf1, Kb2 and Ka2.
/// 2. The DTZ ranking alone still lost game 4965: its shortest win, 77...Ke5,
///    made a position for the third time. A win after which the position, or
///    the one after any reply, occurs for the third time ranks as the draw it
///    is. From the same root reached through Qg2-f3 Ke6 Qd1 Ke5 Qf3 Ke6 Qd1
///    Ke5, Qf3 is such a move (its position stood twice), and leaves the rank.
///
///     test_tb_root_rank

#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "move.h"
#include "syzygy.h"
#include "syzygy_fixture.h"
#include "zobrist.h"

#include <algorithm>
#include <cstdio>
#include <initializer_list>
#include <set>
#include <string>
#include <vector>

namespace {

bool play(Board& board, const std::string& uci) {
    MoveList legal;
    board.gen_legal(legal);
    for (Move move : legal) {
        if (move_to_uci(move) == uci) {
            board.make_move(move);
            return true;
        }
    }
    return false;
}

// The best rank's moves after `moves` from `fen`; empty on a harness error.
std::set<std::string> best_rank_after(const char* fen, std::initializer_list<const char*> moves) {
    Board board;
    board.set_fen(fen);
    for (const char* uci : moves) {
        if (!play(board, uci)) {
            std::printf("HARNESS ERROR: %s is not legal\n", uci);
            return {};
        }
    }
    const std::vector<Syzygy::RootMoveInfo> ranked = Syzygy::probe_root_moves(board, true, 7, true);
    if (ranked.size() != 23) {
        std::printf("HARNESS ERROR: %zu root moves ranked, expected 23\n", ranked.size());
        return {};
    }
    const int best = std::max_element(ranked.begin(), ranked.end(),
                                       [](const auto& a, const auto& b) { return a.rank < b.rank; })->rank;
    std::set<std::string> best_rank;
    for (const auto& move : ranked)
        if (move.rank == best)
            best_rank.insert(move_to_uci(move.bestmove));
    return best_rank;
}

std::string joined(const std::set<std::string>& moves) {
    std::string out;
    for (const auto& uci : moves)
        out += " " + uci;
    return out;
}

}  // namespace

int main() {
    init_bitboards();
    init_attacks();
    Zobrist::init();

    if (!Syzygy::init(syzygy_fixture_path().string()) || Syzygy::largest() != 3) {
        std::printf("HARNESS ERROR: the KQvK fixture did not load\n");
        return 2;
    }

    const auto by_dtz = best_rank_after("8/8/8/4k3/8/8/8/K2Q4 w - - 0 1",
        {"d1d2", "e5e4", "d2c2", "e4e5", "c2d2", "e5e4", "d2d1", "e4e5"});
    const auto threefold = best_rank_after("8/8/8/4k3/8/8/6Q1/K7 w - - 0 1",
        {"g2f3", "e5e6", "f3d1", "e6e5", "d1f3", "e5e6", "f3d1", "e6e5"});
    if (by_dtz.empty() || threefold.empty())
        return 2;

    const std::set<std::string> nearest{"d1h5", "d1f3", "d1f1", "a1b2", "a1a2"};
    const std::set<std::string> nearest_but_f3{"d1h5", "d1f1", "a1b2", "a1a2"};
    const bool ok_dtz = by_dtz == nearest;
    const bool ok_threefold = threefold == nearest_but_f3;
    std::printf("%s: best rank after a repetition:%s\n", ok_dtz ? "ok" : "FAILED", joined(by_dtz).c_str());
    std::printf("%s: best rank when Qf3 repeats a position a third time:%s\n",
                ok_threefold ? "ok" : "FAILED", joined(threefold).c_str());
    return ok_dtz && ok_threefold ? 0 : 1;
}
