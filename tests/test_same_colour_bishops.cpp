/// Bishops on one colour against a bare king (F6).
///
/// However many bishops a side has, if they all stand on one colour they can
/// never mate a bare king: the position is dead, a draw. The evaluator scores
/// it as a material win (its mating rules need a bishop of each colour), and
/// the search's dead-position rule covers only KB-KB and a lone minor. B.9.3's
/// read: all 765,152 quiet KBB-K positions with both bishops on one colour
/// score 300 or more, and the tables call every one a draw.
///
///     test_same_colour_bishops [--expect-defect]
///
/// The contract: such a position is a draw to the search's rule or scores 0.
/// With --expect-defect it passes only while the head breaks the contract.

#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "eval.h"
#include "zobrist.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

int main(int argc, char** argv) {
    const bool expect_defect = argc > 1 && std::strcmp(argv[1], "--expect-defect") == 0;
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables();

    // Light-squared pairs and a triple, each side to move, either colour.
    const char* fens[] = {
        "8/8/8/4k3/8/8/2B5/K2B4 w - - 0 1",   // c2, d1: both light
        "8/8/8/4k3/8/8/2B5/K2B4 b - - 0 1",
        "8/3B4/8/4k3/8/8/2B5/K2B4 w - - 0 1", // d7 as well
        "k2b4/2b5/8/8/4K3/8/8/8 b - - 0 1",   // the same, colours reversed
        "1b6/b7/8/8/4K3/8/8/k7 w - - 0 1",    // b8, a7: both dark
    };
    const auto ev = std::make_unique<Evaluator>();
    int broken = 0;
    for (const char* fen : fens) {
        Board b;
        if (!b.try_set_fen(fen, /*validate_legal_position=*/true)) {
            std::printf("HARNESS ERROR: illegal %s\n", fen);
            return 2;
        }
        const int score = ev->evaluate_undamped(b);
        const bool dead = b.is_insufficient_material();
        if (!dead && score != 0) {
            ++broken;
            std::printf("  %s: not dead, scores %d\n", fen, score);
        }
    }

    const bool holds = broken == 0;
    if (expect_defect) {
        if (holds) {
            std::printf("DEFECT GONE: every position is a draw. Drop --expect-defect.\n");
            return 1;
        }
        std::printf("KNOWN DEFECT REPRODUCED: %d of %zu same-coloured bishop positions read as a win\n",
                    broken, sizeof(fens) / sizeof(fens[0]));
        return 0;
    }
    std::printf("%s: %d of %zu read as a win\n", holds ? "ok" : "FAILED", broken,
                sizeof(fens) / sizeof(fens[0]));
    return holds ? 0 : 1;
}
