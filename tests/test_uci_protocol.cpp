/// UCI protocol command parsing tests.
///
/// These tests exercise UciProtocol without starting a full Engine thread.
/// They intentionally avoid "isready", because that command waits for an
/// engine-side acknowledgement.

#include "engine_command.h"
#include "parameters.h"
#include "attacks.h"
#include "bitboard.h"
#include "zobrist.h"
#include "uci_protocol.h"
#include "test_harness.h"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct ProtocolRun {
    std::vector<EngineCommand> commands;
    std::string output;
    bool stop_requested = false;
    bool ponderhit_requested = false;
    bool searching = false;
    uint64_t epoch = 0;
};

class StreamRedirect {
public:
    StreamRedirect(const std::string& input, std::ostringstream& output)
        : input_(input)
        , old_in_(std::cin.rdbuf(input_.rdbuf()))
        , old_out_(std::cout.rdbuf(output.rdbuf())) {}

    ~StreamRedirect() {
        std::cin.rdbuf(old_in_);
        std::cout.rdbuf(old_out_);
    }

private:
    std::istringstream input_;
    std::streambuf* old_in_;
    std::streambuf* old_out_;
};

static ProtocolRun run_protocol(const std::string& input, int expected_commands) {
    EngineCommandQueue queue;
    std::atomic_bool stop_requested{false};
    std::atomic_bool ponderhit_requested{false};
    std::atomic_bool searching{false};
    std::atomic_uint64_t control_epoch{0};
    std::ostringstream output;

    {
        StreamRedirect redirect(input, output);
        UciProtocol protocol(queue, stop_requested, ponderhit_requested,
                             searching, control_epoch);
        protocol.UciLoop();
    }

    ProtocolRun run;
    for (int i = 0; i < expected_commands; ++i)
        run.commands.push_back(queue.wait_pop());
    run.output = output.str();
    run.stop_requested = stop_requested.load(std::memory_order_acquire);
    run.ponderhit_requested = ponderhit_requested.load(std::memory_order_acquire);
    run.searching = searching.load(std::memory_order_acquire);
    run.epoch = control_epoch.load(std::memory_order_acquire);
    return run;
}

static void test_quit_keeps_prior_go_order() {
    ProtocolRun run = run_protocol(
        "position startpos\n"
        "go movetime 100\n"
        "quit\n",
        3);

    begin_section("uci protocol: position queued before go");
    EXPECT(run.commands[0].type == EngineCommandType::Position);
    EXPECT_STR(run.commands[0].args, "startpos");
    EXPECT_EQ(static_cast<int>(run.commands[0].epoch), 0);
    end_section();

    begin_section("uci protocol: go queued before quit");
    EXPECT(run.commands[1].type == EngineCommandType::Go);
    EXPECT_STR(run.commands[1].args, "movetime 100");
    EXPECT_EQ(static_cast<int>(run.commands[1].epoch), 1);
    EXPECT(run.commands[2].type == EngineCommandType::Quit);
    EXPECT_EQ(static_cast<int>(run.commands[2].epoch), 1);
    end_section();

    begin_section("uci protocol: quit stops active search without advancing epoch");
    EXPECT(run.stop_requested);
    EXPECT(run.searching);
    EXPECT_EQ(static_cast<int>(run.epoch), 1);
    end_section();
}

static void test_go_ponder_queued() {
    ProtocolRun run = run_protocol(
        "go ponder wtime 1000 btime 1000\n"
        "quit\n",
        2);

    begin_section("uci protocol: go ponder queued with limits");
    EXPECT(run.commands[0].type == EngineCommandType::Go);
    EXPECT_STR(run.commands[0].args, "ponder wtime 1000 btime 1000");
    EXPECT_EQ(static_cast<int>(run.commands[0].epoch), 1);
    EXPECT(run.searching);
    end_section();
}

static void test_quit_keeps_prior_bench_order() {
    ProtocolRun run = run_protocol(
        "bench 13\n"
        "quit\n",
        2);

    begin_section("uci protocol: bench queued before quit");
    EXPECT(run.commands[0].type == EngineCommandType::Bench);
    EXPECT_STR(run.commands[0].args, "13");
    EXPECT_EQ(static_cast<int>(run.commands[0].epoch), 1);
    EXPECT(run.commands[1].type == EngineCommandType::Quit);
    EXPECT_EQ(static_cast<int>(run.commands[1].epoch), 1);
    end_section();
}

static void test_eof_enqueues_quit() {
    ProtocolRun run = run_protocol("position startpos\n", 2);

    begin_section("uci protocol: EOF queues quit after prior command");
    EXPECT(run.commands[0].type == EngineCommandType::Position);
    EXPECT(run.commands[1].type == EngineCommandType::Quit);
    EXPECT_EQ(static_cast<int>(run.commands[1].epoch), 0);
    EXPECT(run.stop_requested);
    end_section();
}

static void test_ponderhit_signal() {
    ProtocolRun run = run_protocol(
        "ponderhit\n"
        "quit\n",
        2);

    begin_section("uci protocol: ponderhit sets signal");
    EXPECT(run.ponderhit_requested);
    EXPECT(run.commands[0].type == EngineCommandType::PonderHit);
    EXPECT(run.commands[1].type == EngineCommandType::Quit);
    end_section();
}

static void test_uci_output() {
    ProtocolRun run = run_protocol(
        "uci\n"
        "quit\n",
        1);

    begin_section("uci protocol: uci command emits engine id");
    EXPECT(run.output.find("id name Basilisk") != std::string::npos);
    EXPECT(run.output.find("id author") != std::string::npos);
    end_section();

    begin_section("uci protocol: uci command emits options and uciok");
    EXPECT(run.output.find("option name Threads") != std::string::npos);
    EXPECT(run.output.find("option name Ponder type check default false") != std::string::npos);
    EXPECT(run.output.find("uciok") != std::string::npos);
    end_section();
}


// ---------------------------------------------------------------------------
// 15.0.d: malformed input and counter boundaries.
//
// Contract: malformed input produces a DIAGNOSTIC and leaves the engine in a
// LEGAL state -- never a crash, never a silently corrupted board. These drive
// Parameters directly: UciProtocol only enqueues the raw argument string, so
// the parsing that can actually be malformed lives one layer down.
// ---------------------------------------------------------------------------

template <class Fn>
static std::string capture(Fn&& fn) {
    std::ostringstream out;
    std::streambuf* old = std::cout.rdbuf(out.rdbuf());
    fn();
    std::cout.rdbuf(old);
    return out.str();
}

static bool contains(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

static const char* START_FEN =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static void test_malformed_position_moves() {
    // (a) Non-ASCII move token. Rarog panicked here slicing UTF-8 (RAR-M26).
    // Basilisk compares whole tokens against generated legal moves, so the
    // token matches nothing -- but the contract still demands the diagnostic
    // and an unchanged, legal board.
    {
        Parameters p;
        const std::string out = capture([&] {
            p.set_position("startpos moves e2e4 \xE2\x99\x9A" "e7e5");
        });
        begin_section("15.0.d: non-ASCII move token is rejected, not sliced");
        EXPECT(contains(out, "Illegal move:"));
        // A rejected move list leaves the PREVIOUS position standing, not a
        // half-applied one.
        EXPECT_STR(p.board.get_fen(), std::string(START_FEN));
        end_section();
    }

    // (b) Stray UTF-8 continuation bytes are just bytes to a whole-token
    // comparison, and must stay that way.
    {
        Parameters p;
        const std::string out = capture([&] {
            p.set_position("startpos moves \x80\x80\x80\x80");
        });
        begin_section("15.0.d: stray UTF-8 continuation bytes are rejected");
        EXPECT(contains(out, "Illegal move:"));
        EXPECT_STR(p.board.get_fen(), std::string(START_FEN));
        end_section();
    }

    // (c) A move list longer than HISTORY_RESERVE (2048). The undo history is
    // a growable vector (8.6.10a), so this must simply work: no clamp, no
    // truncation, no crash. 3,000 plies of knight shuffling returns to the
    // start position with the halfmove clock advanced.
    {
        std::string cmd = "startpos moves";
        for (int i = 0; i < 750; ++i)
            cmd += " g1f3 g8f6 f3g1 f6g8";
        Parameters p;
        const std::string out = capture([&] { p.set_position(cmd); });
        begin_section("15.0.d: move list far beyond the history reservation");
        EXPECT(!contains(out, "Illegal move:"));
        EXPECT(contains(p.board.get_fen(),
                        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -"));
        end_section();
    }

    // (d) Truncated and nonsense position headers.
    {
        Parameters p;
        const std::string a = capture([&] { p.set_position(""); });
        const std::string b = capture([&] { p.set_position("fen"); });
        const std::string c = capture([&] { p.set_position("banana"); });
        const std::string d = capture([&] { p.set_position("startpos junk"); });
        begin_section("15.0.d: malformed position headers each diagnose");
        EXPECT(contains(a, "Incorrect position format."));
        EXPECT(contains(b, "Missing FEN fields."));
        EXPECT(contains(c, "Incorrect position format."));
        EXPECT(contains(d, "Incorrect position format."));
        EXPECT_STR(p.board.get_fen(), std::string(START_FEN));
        end_section();
    }
}

static void test_fen_counter_boundaries() {
    Parameters p;
    const std::string over = capture([&] {
        p.set_position("fen 4k3/8/8/8/8/8/8/4K3 w - - 0 100001");
    });
    begin_section("15.0.d: fullmove above the 100000 bound is refused");
    EXPECT(contains(over, "Invalid fullmove number."));
    EXPECT_STR(p.board.get_fen(), std::string(START_FEN));
    end_section();

    const std::string at = capture([&] {
        p.set_position("fen 4k3/8/8/8/8/8/8/4K3 w - - 0 100000");
    });
    begin_section("15.0.d: fullmove at the bound is accepted");
    EXPECT(at.empty());
    EXPECT_STR(p.board.get_fen(), std::string("4k3/8/8/8/8/8/8/4K3 w - - 0 100000"));
    end_section();

    Parameters q;
    const std::string neg = capture([&] {
        q.set_position("fen 4k3/8/8/8/8/8/8/4K3 w - - -1 1");
    });
    begin_section("15.0.d: negative halfmove clock is refused");
    EXPECT(!neg.empty());
    EXPECT_STR(q.board.get_fen(), std::string(START_FEN));
    end_section();
}

static void test_malformed_go() {
    Parameters p;

    capture([&] { p.set_search_parameters("depth"); });
    begin_section("15.0.d: go with a missing value still leaves a usable depth");
    EXPECT(p.depth >= 1);
    end_section();

    capture([&] { p.set_search_parameters("depth banana"); });
    begin_section("15.0.d: go with a non-numeric value does not corrupt depth");
    EXPECT(p.depth >= 1);
    end_section();

    capture([&] { p.set_search_parameters("depth 99999999999999999999"); });
    begin_section("15.0.d: go depth overflowing int64 is ignored, not wrapped");
    EXPECT(p.depth >= 1);
    end_section();

    capture([&] { p.set_search_parameters("nodes -5 movetime -1000 movestogo -3"); });
    begin_section("15.0.d: negative go values clamp to zero, never negative");
    EXPECT(p.nodes >= 0);
    EXPECT(p.move_time >= 0);
    EXPECT(p.movestogo >= 0);
    end_section();

    capture([&] { p.set_search_parameters(""); });
    begin_section("15.0.d: bare go sets the default depth");
    EXPECT(p.depth >= 1);
    end_section();

    const std::string sm = capture([&] {
        p.set_search_parameters("searchmoves \xE2\x99\x9A");
    });
    begin_section("15.0.d: non-ASCII searchmoves token diagnoses, no move stored");
    EXPECT(contains(sm, "Invalid searchmoves move:"));
    EXPECT(p.search_moves.empty());
    end_section();
}

static void test_unknown_setoption() {
    Parameters p;
    const std::string a = capture([&] { p.set_option("name NoSuchOption value 5"); });
    begin_section("15.0.d: unknown setoption name is diagnosed");
    EXPECT(contains(a, "NoSuchOption"));
    end_section();

    const std::string b = capture([&] { p.set_option("garbage"); });
    begin_section("15.0.d: setoption without a name is diagnosed");
    EXPECT(contains(b, "Incorrect setoption format."));
    end_section();

    const int before = p.hash_mb;
    const std::string c = capture([&] { p.set_option("name Hash"); });
    begin_section("15.0.d: known option missing its value is diagnosed, value kept");
    EXPECT(contains(c, "requires a value"));
    EXPECT_EQ(p.hash_mb, before);
    end_section();

    const std::string d = capture([&] { p.set_option("name Hash value banana"); });
    begin_section("15.0.d: known option with a non-numeric value is diagnosed");
    EXPECT(contains(d, "Invalid value for option"));
    EXPECT_EQ(p.hash_mb, before);
    end_section();
}

int main() {
    // 15.0.d drives Parameters, which builds real Boards and generates legal
    // moves; the protocol-only tests above never needed the attack tables.
    init_bitboards();
    init_attacks();
    Zobrist::init();
    std::printf("UCI protocol tests\n");
    std::printf("%s\n", std::string(62, '=').c_str());

    std::printf("\nGo / quit ordering\n");
    test_quit_keeps_prior_go_order();
    test_go_ponder_queued();

    std::printf("\nBench / quit ordering\n");
    test_quit_keeps_prior_bench_order();

    std::printf("\nEOF handling\n");
    test_eof_enqueues_quit();

    std::printf("\nPonderhit\n");
    test_ponderhit_signal();

    std::printf("\nUCI output\n");
    test_uci_output();
    test_malformed_position_moves();
    test_fen_counter_boundaries();
    test_malformed_go();
    test_unknown_setoption();

    return harness_summary();
}
