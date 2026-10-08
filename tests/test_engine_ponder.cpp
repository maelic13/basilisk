/// Engine-level ponder protocol tests.

#include "engine.h"
#include "engine_command.h"
#include "uci_output.h"
#include "attacks.h"
#include "bitboard.h"
#include "eval.h"
#include "test_harness.h"
#include "zobrist.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

namespace {

int count_bestmove_lines(const std::string& text) {
    std::istringstream input(text);
    std::string line;
    int count = 0;
    while (std::getline(input, line)) {
        if (line.rfind("bestmove ", 0) == 0)
            ++count;
    }
    return count;
}

class EngineSession {
public:
    EngineSession()
        : engine_(queue_, stop_requested_, ponderhit_requested_,
                  searching_, control_epoch_) {
        // A rejected `position` ends the process in production; here it is
        // recorded and ends only the engine loop. Installed before the engine
        // thread starts.
        engine_.set_fatal_handler([this](const std::string&) {
            fatal_count_.fetch_add(1, std::memory_order_acq_rel);
        });
        old_out_ = std::cout.rdbuf(output_.rdbuf());
        thread_ = std::thread(&Engine::start, &engine_);
    }

    ~EngineSession() {
        quit();
        std::cout.rdbuf(old_out_);
    }

    void set_option(const std::string& args) {
        queue_.push(EngineCommand{EngineCommandType::SetOption, args, nullptr, 0});
    }

    void new_game() {
        queue_.push(EngineCommand{EngineCommandType::NewGame, {}, nullptr, 0});
    }

    void position(const std::string& args) {
        queue_.push(EngineCommand{EngineCommandType::Position, args, nullptr, 0});
    }

    void go(const std::string& args) {
        const uint64_t epoch =
            control_epoch_.fetch_add(1, std::memory_order_acq_rel) + 1;
        if (searching_.exchange(true, std::memory_order_acq_rel))
            stop_requested_.store(true, std::memory_order_seq_cst);
        // Mirrors UciProtocol::cmdGo: a new `go` starts a new ponderhit lifetime.
        ponderhit_requested_.store(false, std::memory_order_release);
        queue_.push(EngineCommand{EngineCommandType::Go, args, nullptr, epoch});
    }

    void stop() {
        const uint64_t epoch =
            control_epoch_.fetch_add(1, std::memory_order_acq_rel) + 1;
        stop_requested_.store(true, std::memory_order_release);
        queue_.push(EngineCommand{EngineCommandType::Stop, {}, nullptr, epoch});
    }

    void ponderhit() {
        ponderhit_requested_.store(true, std::memory_order_release);
        queue_.push(EngineCommand{EngineCommandType::PonderHit, {}, nullptr, 0});
    }

    void sync() {
        auto ack = std::make_shared<std::promise<void>>();
        auto done = ack->get_future();
        queue_.push(EngineCommand{EngineCommandType::Ready, {}, ack});
        done.wait();
    }

    void quit() {
        if (joined_)
            return;
        stop_requested_.store(true, std::memory_order_release);
        queue_.push(EngineCommand{
            EngineCommandType::Quit, {}, nullptr,
            control_epoch_.load(std::memory_order_acquire)});
        if (thread_.joinable())
            thread_.join();
        joined_ = true;
    }

    std::string output() const {
        std::lock_guard lock(uci_output_mutex());
        return output_.str();
    }

    // `info depth` lines printed so far; a ponder search that has finished its
    // depth limit has printed them and must then wait.
    int info_depth_lines() const {
        std::istringstream input(output());
        std::string line;
        int count = 0;
        while (std::getline(input, line))
            count += line.rfind("info depth", 0) == 0;
        return count;
    }

    bool wait_for_info_depth_lines(int expected, int timeout_ms) const {
        const auto deadline = std::chrono::steady_clock::now()
                            + std::chrono::milliseconds(timeout_ms);
        while (info_depth_lines() < expected && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        return info_depth_lines() >= expected;
    }

    bool wait_for_bestmoves(int expected, int timeout_ms) const {
        const auto deadline = std::chrono::steady_clock::now()
                            + std::chrono::milliseconds(timeout_ms);
        while (std::chrono::steady_clock::now() < deadline) {
            if (count_bestmove_lines(output()) >= expected)
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return count_bestmove_lines(output()) >= expected;
    }

private:
    EngineCommandQueue queue_;
    std::atomic_bool stop_requested_{false};
    std::atomic_bool ponderhit_requested_{false};
    std::atomic_bool searching_{false};
    std::atomic_uint64_t control_epoch_{0};
    Engine engine_;
    mutable std::ostringstream output_;
    std::streambuf* old_out_ = nullptr;
    std::thread thread_;
    bool joined_ = false;
    std::atomic_int fatal_count_{0};
};

// A search that wrongly answered after its depth limit would print bestmove
// immediately after the depth's info line; this grace leaves room for that.
constexpr int kPrematureGraceMs = 25;

void configure_two_threads(EngineSession& session) {
    session.set_option("name Threads value 2");
    session.sync();
}

void test_ponder_depth_waits_for_stop() {
    begin_section("engine ponder: completed depth waits for stop");
    EngineSession session;
    configure_two_threads(session);
    session.position("startpos");
    session.go("ponder depth 1");
    EXPECT(session.wait_for_info_depth_lines(1, 2000));
    EXPECT(!session.wait_for_bestmoves(1, kPrematureGraceMs));
    session.stop();
    EXPECT(session.wait_for_bestmoves(1, 1000));
    EXPECT_EQ(count_bestmove_lines(session.output()), 1);
    end_section();
}

void test_ponder_depth_waits_for_ponderhit() {
    begin_section("engine ponder: completed depth waits for ponderhit");
    EngineSession session;
    configure_two_threads(session);
    session.position("startpos");
    session.go("ponder depth 1");
    EXPECT(session.wait_for_info_depth_lines(1, 2000));
    EXPECT(!session.wait_for_bestmoves(1, kPrematureGraceMs));
    session.ponderhit();
    EXPECT(session.wait_for_bestmoves(1, 1000));
    EXPECT_EQ(count_bestmove_lines(session.output()), 1);
    end_section();
}

void test_stale_stop_does_not_poison_next_ponder() {
    begin_section("engine ponder: previous stop state is cleared");
    EngineSession session;
    configure_two_threads(session);
    session.position("startpos");
    session.go("depth 1");
    EXPECT(session.wait_for_bestmoves(1, 1000));

    const int before = session.info_depth_lines();
    session.position("startpos moves e2e4 e7e5");
    session.go("ponder depth 1");
    EXPECT(session.wait_for_info_depth_lines(before + 1, 2000));
    EXPECT(!session.wait_for_bestmoves(2, kPrematureGraceMs));
    session.stop();
    EXPECT(session.wait_for_bestmoves(2, 1000));
    EXPECT_EQ(count_bestmove_lines(session.output()), 2);
    end_section();
}

// A new Hash size is applied when the queued `setoption` is processed. Queuing
// one ahead of `go` keeps the engine thread busy long enough that a
// back-to-back `ponderhit` reliably arrives before start_search clears
// anything -- the ordering in which 1.10.0 cleared the flag and forfeited on
// time (Grand Blitz 4T, rounds 1-2, opponent replying in ~1 ms). The old code never answers, so the generous
// timeouts only absorb slow Debug/sanitizer builds, where the pending hash
// resize alone can take longer than a second.
void configure_two_threads_pending_hash(EngineSession& session) {
    configure_two_threads(session);
    session.set_option("name Hash value 256");
}

void test_immediate_ponderhit_with_clock() {
    begin_section("engine ponder: ponderhit during search setup is kept (clock)");
    EngineSession session;
    configure_two_threads_pending_hash(session);
    session.position("startpos moves e2e4 e7e5");
    session.go("ponder wtime 1000 btime 1000 winc 0 binc 0");
    session.ponderhit();
    EXPECT(session.wait_for_bestmoves(1, 10000));
    EXPECT_EQ(count_bestmove_lines(session.output()), 1);
    end_section();
}

void test_immediate_ponderhit_after_depth_cap() {
    begin_section("engine ponder: ponderhit during search setup is kept (depth cap)");
    EngineSession session;
    configure_two_threads_pending_hash(session);
    session.position("startpos moves e2e4 e7e5");
    session.go("ponder depth 1");
    session.ponderhit();
    EXPECT(session.wait_for_bestmoves(1, 10000));
    EXPECT_EQ(count_bestmove_lines(session.output()), 1);
    end_section();
}

void test_stale_ponderhit_does_not_poison_next_ponder() {
    begin_section("engine ponder: previous ponderhit does not end the next ponder");
    EngineSession session;
    configure_two_threads(session);
    session.position("startpos");
    session.go("ponder depth 1");
    EXPECT(session.wait_for_info_depth_lines(1, 2000));
    session.ponderhit();
    EXPECT(session.wait_for_bestmoves(1, 1000));

    const int before = session.info_depth_lines();
    session.position("startpos moves e2e4 e7e5");
    session.go("ponder depth 1");
    EXPECT(session.wait_for_info_depth_lines(before + 1, 2000));
    EXPECT(!session.wait_for_bestmoves(2, kPrematureGraceMs));
    session.stop();
    EXPECT(session.wait_for_bestmoves(2, 1000));
    EXPECT_EQ(count_bestmove_lines(session.output()), 2);
    end_section();
}

// Hash sizing and clearing belong to `setoption` / `ucinewgame`, finished
// before the GUI's readyok, not to the first search after `go`, whose clock
// is already running: 1.10.1-rc spent ~270 ms of the first move allocating
// and clearing a 512 MB table (the test fails on that code). With the work
// done at configure time the first search costs what any other shallow
// search does.
void test_hash_setup_is_off_the_clock() {
    EngineSession session;
    session.set_option("name Hash value 512");
    session.new_game();
    session.position("startpos");
    session.sync();

    const auto start = std::chrono::steady_clock::now();
    session.go("depth 1");
    const bool answered = session.wait_for_bestmoves(1, 10000);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();

    begin_section("engine setup: hash resize and clear happen before go");
    EXPECT(answered);
    EXPECT(elapsed < 100);
    end_section();
}

} // namespace

int main() {
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables();

    std::printf("Engine ponder tests\n");
    std::printf("%s\n", std::string(62, '=').c_str());

    std::printf("\nPonder lifecycle\n");
    test_ponder_depth_waits_for_stop();
    test_ponder_depth_waits_for_ponderhit();
    test_stale_stop_does_not_poison_next_ponder();
    test_immediate_ponderhit_with_clock();
    test_immediate_ponderhit_after_depth_cap();
    test_stale_ponderhit_does_not_poison_next_ponder();
    test_hash_setup_is_off_the_clock();

    return harness_summary();
}
