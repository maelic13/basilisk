/// Engine-level ponder protocol tests.

#include "Engine.h"
#include "EngineCommand.h"
#include "UciOutput.h"
#include "attacks.h"
#include "bitboard.h"
#include "eval.h"
#include "test_harness.h"
#include "zobrist.h"

#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
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

    void position(const std::string& args) {
        queue_.push(EngineCommand{EngineCommandType::Position, args, nullptr, 0});
    }

    void go(const std::string& args) {
        const uint64_t epoch =
            control_epoch_.fetch_add(1, std::memory_order_acq_rel) + 1;
        if (searching_.exchange(true, std::memory_order_acq_rel))
            stop_requested_.store(true, std::memory_order_release);
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
};

void configure_two_threads(EngineSession& session) {
    session.set_option("name Threads value 2");
    session.sync();
}

void test_ponder_depth_waits_for_stop() {
    EngineSession session;
    configure_two_threads(session);
    session.position("startpos");
    session.go("ponder depth 1");

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    begin_section("engine ponder: completed depth waits for stop");
    EXPECT_EQ(count_bestmove_lines(session.output()), 0);
    session.stop();
    EXPECT(session.wait_for_bestmoves(1, 1000));
    EXPECT_EQ(count_bestmove_lines(session.output()), 1);
    end_section();
}

void test_ponder_depth_waits_for_ponderhit() {
    EngineSession session;
    configure_two_threads(session);
    session.position("startpos");
    session.go("ponder depth 1");

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    begin_section("engine ponder: completed depth waits for ponderhit");
    EXPECT_EQ(count_bestmove_lines(session.output()), 0);
    session.ponderhit();
    EXPECT(session.wait_for_bestmoves(1, 1000));
    EXPECT_EQ(count_bestmove_lines(session.output()), 1);
    end_section();
}

void test_stale_stop_does_not_poison_next_ponder() {
    EngineSession session;
    configure_two_threads(session);
    session.position("startpos");
    session.go("depth 1");
    EXPECT(session.wait_for_bestmoves(1, 1000));

    session.position("startpos moves e2e4 e7e5");
    session.go("ponder depth 1");
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    begin_section("engine ponder: previous stop state is cleared");
    EXPECT_EQ(count_bestmove_lines(session.output()), 1);
    session.stop();
    EXPECT(session.wait_for_bestmoves(2, 1000));
    EXPECT_EQ(count_bestmove_lines(session.output()), 2);
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

    return harness_summary();
}
