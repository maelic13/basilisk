/// Engine-level threading protocol tests.

#include "engine.h"
#include "engine_command.h"
#include "constants.h"
#include "parameters.h"
#include "uci_output.h"
#include "attacks.h"
#include "bitboard.h"
#include "eval.h"
#include "test_harness.h"
#include "zobrist.h"

#include <algorithm>
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

bool contains_line_fragment(const std::string& text, const std::string& fragment) {
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        if (line.find(fragment) != std::string::npos)
            return true;
    }
    return false;
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
            stop_requested_.store(true, std::memory_order_release);
        queue_.push(EngineCommand{EngineCommandType::Go, args, nullptr, epoch});
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

    bool wait_for_fragment(const std::string& fragment, int timeout_ms) const {
        const auto deadline = std::chrono::steady_clock::now()
                            + std::chrono::milliseconds(timeout_ms);
        while (std::chrono::steady_clock::now() < deadline) {
            if (contains_line_fragment(output(), fragment))
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return contains_line_fragment(output(), fragment);
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

void test_threads_setoption_resizes_before_ready() {
    const int max_threads = Parameters::max_threads();
    if (max_threads < 2) {
        begin_section("engine threads: host exposes one thread");
        EXPECT(true);
        end_section();
        return;
    }

    EngineSession session;
    session.set_option("name Threads value 2");
    session.sync();

    begin_section("engine threads: setoption grows before isready returns");
    EXPECT(contains_line_fragment(session.output(), "info string Using 2 threads"));
    end_section();

    session.set_option("name Threads value 1");
    session.sync();

    begin_section("engine threads: setoption shrinks before isready returns");
    EXPECT(contains_line_fragment(session.output(), "info string Using 1 thread"));
    end_section();
}

// 9.3(a): the Threads cap must be ONE value, not two independently computed
// ones. It used to be `max(1024, 4*hw)` in both parameters.cpp and search.cpp
// — `min` was meant — so the advertisement and the pool's real limit could
// drift apart on a large enough machine. The VALUE is a flat 1024, matching
// Stockfish's Option(1, 1, 1024): picking a sane thread count is the
// operator's job, and hardware_concurrency() under-reports in exactly the
// containerised environments where a machine-derived cap would bite.
void test_threads_maximum_is_the_single_definition() {
    const int advertised = Parameters::max_threads();

    begin_section("engine threads: advertised maximum is the flat cap");
    EXPECT_EQ(advertised, maxSearchThreads);
    EXPECT_EQ(advertised, 1024);
    end_section();

    // The invariant that actually matters and survives any policy change:
    // an option that claims a maximum the pool silently clamps is a protocol
    // lie, so the advertised string must quote the same number the pool honours.
    begin_section("engine threads: uci advertisement matches the real cap");
    const std::string options = Parameters::uci_options();
    const std::string expected =
        "option name Threads type spin default 1 min 1 max " + std::to_string(advertised);
    EXPECT(options.find(expected) != std::string::npos);
    end_section();
}

void test_threaded_go_nodes_returns_one_bestmove() {
    const int max_threads = Parameters::max_threads();

    EngineSession session;
    if (max_threads >= 2) {
        session.set_option("name Threads value 2");
        session.sync();
    }
    session.position("startpos");
    session.go("nodes 1000");

    begin_section("engine threads: go nodes returns one bestmove");
    EXPECT(session.wait_for_bestmoves(1, 2000));
    EXPECT_EQ(count_bestmove_lines(session.output()), 1);
    end_section();

    begin_section("engine threads: threaded search emits node info");
    EXPECT(contains_line_fragment(session.output(), " nodes "));
    end_section();
}

// 9.5: the SMP machinery must be INERT on a single thread. Every Phase-9
// mechanism is gated on thread_count > 1 or on root_table != nullptr (which
// the 1T path never sets), so a single-thread search must be exactly as
// deterministic as it was before any of it existed. This also catches the
// subtler failure: multi-thread state leaking ACROSS searches — a stale stop
// ballot (9.5c) or unflushed shared node count (9.3b) would show up here as a
// 1T search that no longer reproduces itself after a 4-thread one.
void test_smp_machinery_is_inert_on_a_single_thread() {
    auto fixed_depth_nodes = [](EngineSession& s) {
        // ucinewgame first: the TT and the history tables persist across
        // searches BY DESIGN, so without clearing them this would measure
        // ordinary carryover rather than anything about the SMP machinery.
        s.new_game();
        s.sync();
        // Bestmove counting is CUMULATIVE over the session, so wait for one
        // MORE than we have already seen — waiting for "1" would return
        // instantly on a previous search's bestmove and parse its nodes.
        const int already = count_bestmove_lines(s.output());
        s.position("startpos");
        s.go("depth 10");
        s.wait_for_bestmoves(already + 1, 20000);
        // The last "nodes N" reported for this search.
        std::istringstream in(s.output());
        std::string line, last;
        while (std::getline(in, line))
            if (line.find(" nodes ") != std::string::npos) last = line;
        const size_t at = last.find(" nodes ");
        return at == std::string::npos ? std::string()
                                       : last.substr(at + 7, last.find(' ', at + 7) - at - 7);
    };

    std::string first, again, after_mt;
    {
        EngineSession session;
        session.set_option("name Threads value 1");
        session.sync();
        first = fixed_depth_nodes(session);
    }
    {
        EngineSession session;
        session.set_option("name Threads value 1");
        session.sync();
        again = fixed_depth_nodes(session);
    }

    begin_section("smp: single-thread fixed-depth search is deterministic");
    EXPECT(!first.empty());
    EXPECT(first == again);
    end_section();

    // Same engine instance: 4 threads, then back to 1. The 1T result must be
    // the SAME node count as a process that never ran a multi-thread search.
    if (Parameters::max_threads() >= 4) {
        EngineSession session;
        session.set_option("name Threads value 4");
        session.sync();
        session.position("startpos");
        session.go("depth 10");
        session.wait_for_bestmoves(1, 30000);
        session.set_option("name Threads value 1");
        session.sync();
        after_mt = fixed_depth_nodes(session);

        begin_section("smp: 1T is unaffected by a preceding multi-thread search");
        EXPECT(after_mt == first);
        end_section();
    }
}

void test_go_perft_returns_nodes_without_bestmove() {
    EngineSession session;
    session.position("startpos");
    session.go("perft 1");

    begin_section("engine uci: go perft returns node count");
    EXPECT(session.wait_for_fragment("Nodes searched: 20", 1000));
    end_section();

    begin_section("engine uci: go perft does not emit bestmove");
    EXPECT_EQ(count_bestmove_lines(session.output()), 0);
    end_section();
}

void test_go_searchmoves_restricts_root_move() {
    EngineSession session;
    session.position("startpos");
    session.go("searchmoves e2e4 depth 1");

    begin_section("engine uci: searchmoves restricts bestmove");
    EXPECT(session.wait_for_bestmoves(1, 1000));
    EXPECT(contains_line_fragment(session.output(), "bestmove e2e4"));
    end_section();
}

} // namespace

// ---------------------------------------------------------------------------
// 8.6.3a: malformed-input survival contract at the ENGINE level.
//
// Contract decision recorded 2026-07-20 (Rarog 9.5-A style; its 11-engine
// survey): current Stockfish dev exits(1)+diagnostic on both a bad FEN and an
// illegal move in `moves` — the reference implementation is tightening — but
// we deliberately stay with REJECT-AND-RETAIN (majority practice: Critter,
// SaberTooth, Hydra, us): print an info string, keep the previous valid
// board, keep answering. Residual risk, on record: a GUI that ignores the
// info string gets a legal move for the PREVIOUS board. Any flip to
// SF-style hard-exit must be a deliberate decision that changes this test,
// never a silent change.
// ---------------------------------------------------------------------------

void test_malformed_input_survival() {
    begin_section("engine: survives garbage input, answers, retains board");
    {
        EngineSession session;
        session.position("startpos");
        session.position("fen not-a-fen at all");           // garbage FEN
        session.position("fen");                             // bare `position fen`
        session.position("");                                // no args at all
        session.position("kentucky");                        // unknown sub-token
        session.position("startpos moves e2e5");             // illegal move
        session.set_option("name NoSuchOption value 42");    // unknown option
        session.set_option("garbage without name token");    // malformed setoption
        session.set_option("name Hash");                     // missing value
        session.sync();                                      // isready → must return

        session.go("depth 1");
        EXPECT(session.wait_for_bestmoves(1, 5000));

        // The retained board must be the STARTING position: the bestmove has
        // to be one of the 20 legal startpos moves.
        Board b;
        MoveList legal;
        b.gen_legal(legal);
        const std::string out = session.output();
        std::string bm;
        std::istringstream input(out);
        std::string line;
        while (std::getline(input, line))
            if (line.rfind("bestmove ", 0) == 0)
                bm = line.substr(9, 4);
        bool found = false;
        for (Move m : legal)
            if (move_to_uci(m).rfind(bm, 0) == 0) { found = true; break; }
        EXPECT(!bm.empty());
        EXPECT(found);
        // And the rejections were reported, not swallowed silently.
        EXPECT(contains_line_fragment(out, "info string"));
    }
    end_section();
}

int main() {
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables();

    std::printf("Engine threading tests\n");
    std::printf("%s\n", std::string(62, '=').c_str());

    std::printf("\nThreads option\n");
    test_threads_setoption_resizes_before_ready();
    test_threads_maximum_is_the_single_definition();

    std::printf("\nThreaded search\n");
    test_threaded_go_nodes_returns_one_bestmove();
    test_smp_machinery_is_inert_on_a_single_thread();

    std::printf("\nRoot commands\n");
    test_go_perft_returns_nodes_without_bestmove();
    test_go_searchmoves_restricts_root_move();

    std::printf("\nMalformed-input survival (8.6.3a)\n");
    test_malformed_input_survival();

    return harness_summary();
}
