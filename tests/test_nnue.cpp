/// NNUE conformance tests.
///
/// Loads the committed test network (from the shared net_trainer repo's
/// models/test/) and checks that Basilisk's .mnn inference reproduces the
/// reference implementation's evaluations INTEGER-EXACTLY on the committed
/// vectors. Any drift here means the C++ port diverged from the contract
/// (net_trainer docs/mnn_format.md).
///
/// Usage: test_nnue <test_h16.mnn> <nnue_vectors.txt>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "Board.h"
#include "attacks.h"
#include "bitboard.h"
#include "eval.h"
#include "nnue.h"
#include "test_harness.h"
#include "zobrist.h"

static void test_lifecycle(const std::string& net_path) {
    begin_section("lifecycle: disabled without a net");
    EXPECT(!nnue::available());
    EXPECT(!nnue::set_enabled(true));   // refuses without a net
    EXPECT(!nnue::enabled());
    end_section();

    begin_section("lifecycle: load + enable");
    EXPECT(!nnue::load_file(net_path + ".does_not_exist"));
    EXPECT(nnue::load_file(net_path));
    EXPECT(nnue::available());
    EXPECT(!nnue::enabled());           // loading does not auto-enable
    EXPECT(nnue::set_enabled(true));
    EXPECT(nnue::enabled());
    end_section();
}

static void test_vectors(const std::string& vectors_path) {
    std::ifstream f(vectors_path);
    begin_section("conformance vectors: file opens");
    EXPECT(bool(f));
    end_section();

    int count = 0;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        const auto bar = line.find('|');
        if (bar == std::string::npos)
            continue;
        const int expected = std::stoi(line.substr(0, bar));
        const std::string fen = line.substr(bar + 1).find_first_not_of(' ') != std::string::npos
            ? line.substr(line.find_first_not_of(' ', bar + 1))
            : "";

        Board b;
        b.set_fen(fen);
        begin_section(("vector: " + fen).c_str());
        EXPECT_EQ(nnue::evaluate(b), expected);
        end_section();
        ++count;
    }

    begin_section("conformance vectors: all present");
    EXPECT(count >= 10);
    end_section();
}

static void test_perspective_symmetry() {
    // The startpos evaluation must be identical for white-to-move and
    // black-to-move (stm-relative net, symmetric position).
    Board w, bl;
    w.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    bl.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    begin_section("perspective symmetry at startpos");
    EXPECT_EQ(nnue::evaluate(w), nnue::evaluate(bl));
    end_section();
}

static void test_evaluator_dispatch() {
    // Evaluator::evaluate must route to the net when enabled and back to the
    // HCE when disabled.
    Board b;
    b.set_fen("8/8/8/8/8/5k2/8/4K2R w K - 0 1");
    Evaluator ev;
    nnue::set_enabled(true);
    const int with_net = ev.evaluate(b);
    nnue::set_enabled(false);
    const int with_hce = ev.evaluate(b);

    begin_section("Evaluator dispatch: NNUE when enabled, HCE when not");
    EXPECT_EQ(with_net, nnue::evaluate(b));
    EXPECT(with_net != with_hce);  // the tiny test net will not match the HCE
    end_section();
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("usage: test_nnue <net.mnn> <vectors.txt>\n");
        return 1;
    }
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables(g_eval_params);

    test_lifecycle(argv[1]);
    test_vectors(argv[2]);
    test_perspective_symmetry();
    test_evaluator_dispatch();

    return harness_summary();
}
