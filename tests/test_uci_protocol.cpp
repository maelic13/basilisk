/// UCI protocol command parsing tests.
///
/// These tests exercise UciProtocol without starting a full Engine thread.
/// They intentionally avoid "isready", because that command waits for an
/// engine-side acknowledgement.

#include "EngineCommand.h"
#include "UciProtocol.h"
#include "test_harness.h"

#include <atomic>
#include <cstdint>
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

int main() {
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

    return harness_summary();
}
