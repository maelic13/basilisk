/// The root's tablebase ranking after a repetition in the game.
///
/// Once the position has repeated since the last zeroing move, every winning
/// root move may no longer rank alike: a won game that drifts can be drawn by
/// threefold (BAS-S39's game 4965 shuffled a won rook, knight and pawn against
/// a queen into one). Stockfish then ranks the wins by DTZ, so that only the
/// moves nearest a zeroing move stay in the best rank. On the KQvK fixture:
/// after Qd2 Ke4 Qc2 Ke5 Qd2 Ke4 Qd1 Ke5 the root has occurred twice, and of
/// its 20 winning moves the tables put five at the shortest DTZ (16 against
/// 18): Qh5, Qf3, Qf1, Kb2 and Ka2.
///
///     test_tb_root_rank [--expect-defect]
///
/// With --expect-defect it passes only while the head reproduces the defect
/// (every win in the best rank), and fails once the ranking is repaired.

#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "move.h"
#include "syzygy.h"
#include "syzygy_fixture.h"
#include "zobrist.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
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

}  // namespace

int main(int argc, char** argv) {
    const bool expect_defect = argc > 1 && std::strcmp(argv[1], "--expect-defect") == 0;
    init_bitboards();
    init_attacks();
    Zobrist::init();

    if (!Syzygy::init(syzygy_fixture_path().string()) || Syzygy::largest() != 3) {
        std::printf("HARNESS ERROR: the KQvK fixture did not load\n");
        return 2;
    }

    Board board;
    board.set_fen("8/8/8/4k3/8/8/8/K2Q4 w - - 0 1");
    for (const char* uci : {"d1d2", "e5e4", "d2c2", "e4e5", "c2d2", "e5e4", "d2d1", "e4e5"}) {
        if (!play(board, uci)) {
            std::printf("HARNESS ERROR: %s is not legal\n", uci);
            return 2;
        }
    }

    const std::vector<Syzygy::RootMoveInfo> ranked =
        Syzygy::probe_root_moves(board, true, 7, true);
    if (ranked.size() != 23) {
        std::printf("HARNESS ERROR: %zu root moves ranked, expected 23\n", ranked.size());
        return 2;
    }
    const int best = std::max_element(ranked.begin(), ranked.end(),
                                       [](const auto& a, const auto& b) { return a.rank < b.rank; })->rank;
    std::set<std::string> best_rank;
    for (const auto& move : ranked)
        if (move.rank == best)
            best_rank.insert(move_to_uci(move.bestmove));

    const std::set<std::string> nearest{"d1h5", "d1f3", "d1f1", "a1b2", "a1a2"};
    const bool holds = best_rank == nearest;
    std::string got;
    for (const auto& uci : best_rank)
        got += " " + uci;

    if (expect_defect) {
        if (holds) {
            std::printf("DEFECT GONE: the best rank is the five nearest wins. Drop --expect-defect.\n");
            return 1;
        }
        std::printf("KNOWN DEFECT REPRODUCED: after a repetition %zu moves share the best rank:%s\n",
                    best_rank.size(), got.c_str());
        return 0;
    }
    std::printf("%s: best rank after a repetition:%s\n", holds ? "ok" : "FAILED", got.c_str());
    return holds ? 0 : 1;
}
