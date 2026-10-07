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
#include "syzygy_fixture.h"
#include "zobrist.h"
#include "bench.h"

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
            stop_requested_.store(true, std::memory_order_release);
        queue_.push(EngineCommand{EngineCommandType::Go, args, nullptr, epoch});
    }

    void stop() {
        const uint64_t epoch =
            control_epoch_.fetch_add(1, std::memory_order_acq_rel) + 1;
        stop_requested_.store(true, std::memory_order_release);
        queue_.push(EngineCommand{EngineCommandType::Stop, {}, nullptr, epoch});
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

    int fatal_count() const { return fatal_count_.load(std::memory_order_acquire); }

    bool wait_for_fatal(int timeout_ms) const {
        const auto deadline = std::chrono::steady_clock::now()
                            + std::chrono::milliseconds(timeout_ms);
        while (fatal_count() == 0 && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        return fatal_count() > 0;
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
    std::atomic_int fatal_count_{0};
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

// The last `info ... pv` line before each `bestmove` names the move played.
// At Threads 8 the merged result is often a helper's: before the pool printed
// it, 20 of 120 short searches over the bench positions ended on a line naming
// another move. Forty searches at 40 ms leave a defect like that nowhere to hide.
void test_threaded_last_line_names_bestmove() {
    EngineSession session;
    session.set_option("name Threads value 8");
    session.sync();

    int searches = 0;
    for (std::string_view fen : bench_fens()) {
        session.position("fen " + std::string(fen));
        session.go("movetime 40");
        ++searches;
        if (!session.wait_for_bestmoves(searches, 5000))
            break;
    }

    std::istringstream input(session.output());
    std::string line;
    std::string last_pv_move;
    int checked = 0;
    int mismatches = 0;
    while (std::getline(input, line)) {
        if (line.rfind("info ", 0) == 0) {
            const size_t pv = line.find(" pv ");
            if (pv != std::string::npos) {
                const size_t start = pv + 4;
                last_pv_move = line.substr(start, line.find(' ', start) - start);
            }
        } else if (line.rfind("bestmove ", 0) == 0) {
            const size_t start = 9;
            const std::string played = line.substr(start, line.find(' ', start) - start);
            ++checked;
            if (played != last_pv_move) {
                ++mismatches;
                std::fprintf(stderr, "  bestmove %s after a last line naming %s\n",
                             played.c_str(), last_pv_move.c_str());
            }
            last_pv_move.clear();
        }
    }

    begin_section("engine threads: every search at Threads 8 answers");
    EXPECT_EQ(checked, searches);
    EXPECT_EQ(searches, 40);
    end_section();

    begin_section("engine threads: the last info line names bestmove at Threads 8");
    EXPECT_EQ(mismatches, 0);
    end_section();
}

// A root with no legal move prints one depth-0 line, Stockfish's, and
// `bestmove 0000`; under `go infinite` the bestmove still waits for `stop`.
void test_terminal_root_reports_once() {
    auto info_lines = [](const std::string& text) {
        std::istringstream input(text);
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(input, line))
            if (line.rfind("info ", 0) == 0 && line.rfind("info string", 0) != 0)
                lines.push_back(line);
        return lines;
    };

    begin_section("engine terminal root: a mated root prints one mate 0 line");
    {
        EngineSession session;
        session.position("fen 7k/6Q1/6K1/8/8/8/8/8 b - - 0 1");
        session.go("depth 5");
        EXPECT(session.wait_for_bestmoves(1, 2000));
        const auto lines = info_lines(session.output());
        EXPECT_EQ(lines.size(), size_t(1));
        EXPECT(!lines.empty() && lines[0] == "info depth 0 score mate 0");
        EXPECT(contains_line_fragment(session.output(), "bestmove 0000"));
    }
    end_section();

    begin_section("engine terminal root: a stalemated root prints one cp 0 line");
    {
        EngineSession session;
        session.position("fen 7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
        session.go("depth 5");
        EXPECT(session.wait_for_bestmoves(1, 2000));
        const auto lines = info_lines(session.output());
        EXPECT_EQ(lines.size(), size_t(1));
        EXPECT(!lines.empty() && lines[0] == "info depth 0 score cp 0");
    }
    end_section();

    begin_section("engine terminal root: go infinite waits for stop");
    {
        EngineSession session;
        session.position("fen 7k/6Q1/6K1/8/8/8/8/8 b - - 0 1");
        session.go("infinite");
        EXPECT(!session.wait_for_bestmoves(1, 200));
        session.stop();
        EXPECT(session.wait_for_bestmoves(1, 2000));
        EXPECT_EQ(info_lines(session.output()).size(), size_t(1));
    }
    end_section();
}

// MultiPV, on the shapes of Rarog's multipv tests: identity at 1, N ordered
// lines of distinct moves per depth, the caps, threads, stop and tablebases.
struct PvLine {
    int depth = 0;
    int multipv = 0;
    int score = 0;          // cp, or +/-(100000 - n) for a mate in n
    std::string move;
};

std::vector<PvLine> pv_lines(const std::string& text) {
    std::vector<PvLine> out;
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        if (line.rfind("info depth ", 0) != 0 || line.find(" pv ") == std::string::npos)
            continue;
        std::istringstream tokens(line);
        std::string tok;
        PvLine pv;
        while (tokens >> tok) {
            if (tok == "depth") tokens >> pv.depth;
            else if (tok == "multipv") tokens >> pv.multipv;
            else if (tok == "cp") tokens >> pv.score;
            else if (tok == "mate") { int n = 0; tokens >> n; pv.score = n > 0 ? 100000 - n : -100000 - n; }
            else if (tok == "pv") { tokens >> pv.move; break; }
        }
        out.push_back(pv);
    }
    return out;
}

std::string strip_speed(const std::string& text) {
    std::istringstream input(text);
    std::string line, out;
    while (std::getline(input, line)) {
        if (line.rfind("info depth ", 0) != 0)
            continue;
        std::istringstream tokens(line);
        std::string tok, kept;
        while (tokens >> tok) {
            if (tok == "nps" || tok == "time") { tokens >> tok; continue; }
            kept += tok + " ";
        }
        out += kept + "\n";
    }
    return out;
}

std::string bestmove_of(const std::string& text) {
    std::istringstream input(text);
    std::string line, move;
    while (std::getline(input, line))
        if (line.rfind("bestmove ", 0) == 0)
            move = line.substr(9, line.find(' ', 9) - 9);
    return move;
}

void test_multipv() {
    const std::string fen = "fen r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3";

    begin_section("multipv: MultiPV 1 reproduces the single-PV search line for line");
    {
        // Sessions redirect std::cout, so they run one after the other.
        auto run = [&](bool set_one) {
            EngineSession s;
            if (set_one)
                s.set_option("name MultiPV value 1");
            s.position(fen);
            s.go("depth 9");
            EXPECT(s.wait_for_bestmoves(1, 10000));
            return s.output();
        };
        const std::string plain = run(false);
        const std::string one = run(true);
        EXPECT(!strip_speed(plain).empty());
        EXPECT_STR(strip_speed(plain), strip_speed(one));
        EXPECT_STR(bestmove_of(plain), bestmove_of(one));
    }
    end_section();

    begin_section("multipv: three ordered lines of distinct moves per depth; bestmove is line 1");
    {
        EngineSession session;
        session.set_option("name MultiPV value 3");
        session.position(fen);
        session.go("depth 8");
        EXPECT(session.wait_for_bestmoves(1, 10000));
        const auto lines = pv_lines(session.output());
        const size_t remainder = lines.size() - (lines.size() / 3) * 3;
        EXPECT_EQ(remainder, size_t(0));
        bool shaped = lines.size() >= 3;
        for (size_t i = 0; shaped && i + 2 < lines.size(); i += 3) {
            shaped = lines[i].multipv == 1 && lines[i + 1].multipv == 2 && lines[i + 2].multipv == 3
                  && lines[i].depth == lines[i + 2].depth
                  && lines[i].move != lines[i + 1].move && lines[i].move != lines[i + 2].move
                  && lines[i + 1].move != lines[i + 2].move
                  && lines[i].score >= lines[i + 1].score && lines[i + 1].score >= lines[i + 2].score;
        }
        EXPECT(shaped);
        EXPECT(!lines.empty() && bestmove_of(session.output()) == lines[lines.size() - 3].move);
    }
    end_section();

    begin_section("multipv: lines are capped by the legal moves and by searchmoves");
    {
        EngineSession legal;   // the king has exactly two moves, Ka7 and Kb8
        legal.set_option("name MultiPV value 5");
        legal.position("fen k7/8/2K5/8/8/8/8/7R b - - 0 1");
        legal.go("depth 4");
        EXPECT(legal.wait_for_bestmoves(1, 5000));
        int max_multipv = 0;
        for (const PvLine& l : pv_lines(legal.output()))
            max_multipv = std::max(max_multipv, l.multipv);
        EXPECT_EQ(max_multipv, 2);
    }
    {
        EngineSession restricted;
        restricted.set_option("name MultiPV value 3");
        restricted.position("startpos");
        restricted.go("depth 5 searchmoves a2a3 h2h3");
        EXPECT(restricted.wait_for_bestmoves(1, 5000));
        bool only_listed = true;
        int top = 0;
        for (const PvLine& l : pv_lines(restricted.output())) {
            only_listed = only_listed && (l.move == "a2a3" || l.move == "h2h3");
            top = std::max(top, l.multipv);
        }
        EXPECT(only_listed);
        EXPECT_EQ(top, 2);
    }
    end_section();

    begin_section("multipv: at Threads 4 the main thread's first line is bestmove");
    {
        EngineSession session;
        session.set_option("name Threads value 4");
        session.set_option("name MultiPV value 3");
        session.sync();
        for (int i = 0; i < 6; ++i) {
            session.position(std::string("fen ") + std::string(bench_fens()[static_cast<size_t>(i)]));
            session.go("movetime 60");
            EXPECT(session.wait_for_bestmoves(i + 1, 5000));
        }
        std::istringstream input(session.output());
        std::string line, last_first_line_move;
        int mismatches = 0;
        while (std::getline(input, line)) {
            if (line.rfind("info depth ", 0) == 0 && line.find(" multipv 1 ") != std::string::npos
                && line.find(" pv ") != std::string::npos) {
                const size_t start = line.find(" pv ") + 4;
                last_first_line_move = line.substr(start, line.find(' ', start) - start);
            } else if (line.rfind("bestmove ", 0) == 0) {
                mismatches += line.substr(9, line.find(' ', 9) - 9) != last_first_line_move;
            }
        }
        EXPECT_EQ(mismatches, 0);
    }
    end_section();

    begin_section("multipv: stop under go infinite answers once, with line 1's move");
    {
        EngineSession session;
        session.set_option("name MultiPV value 4");
        session.position(fen);
        session.go("infinite");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        session.stop();
        EXPECT(session.wait_for_bestmoves(1, 5000));
        EXPECT_EQ(count_bestmove_lines(session.output()), 1);
        const auto lines = pv_lines(session.output());
        std::string last_first;
        for (const PvLine& l : lines)
            if (l.multipv == 1) last_first = l.move;
        EXPECT(!last_first.empty() && bestmove_of(session.output()) == last_first);
    }
    end_section();

    begin_section("multipv: a tablebase root reports only moves of the best rank");
    {
        EngineSession session;
        session.set_option("name SyzygyPath value " + syzygy_fixture_path().string());
        session.set_option("name MultiPV value 8");
        session.position("fen 6k1/8/8/8/8/8/8/6KQ w - - 0 1");
        session.go("depth 4");
        EXPECT(session.wait_for_bestmoves(1, 10000));
        const auto lines = pv_lines(session.output());
        bool decisive = !lines.empty();
        for (const PvLine& l : lines)
            decisive = decisive && l.score >= tablebaseWinScore - 128;   // displayed cp
        EXPECT(decisive);
        Syzygy::clear();
    }
    end_section();
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
// 8.6.3a: malformed-input contract at the ENGINE level.
//
// Recorded 2026-07-20 as reject-and-retain for every command: print an info
// string, keep the previous valid board, keep answering. REVISED 2026-09-27
// (1.10.1, maintainer decision) for `position` only: a rejected position is
// FATAL -- CRITICAL ERROR line, exit status 1 -- as in Stockfish and Rarog.
// The retained board made the next `go` answer with a move for the PREVIOUS
// position; the side to move is the same, so that move was often legal and
// silently played wrong. Malformed `setoption` input still survives.
// ---------------------------------------------------------------------------

void test_malformed_input_survival() {
    begin_section("engine: survives malformed setoption, answers, keeps board");
    {
        EngineSession session;
        session.position("startpos");
        session.set_option("name NoSuchOption value 42");    // unknown option
        session.set_option("garbage without name token");    // malformed setoption
        session.set_option("name Hash");                     // missing value
        session.sync();                                      // isready -> must return

        session.go("depth 1");
        EXPECT(session.wait_for_bestmoves(1, 5000));

        // The board is the STARTING position: the bestmove has to be one of
        // the 20 legal startpos moves.
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
        EXPECT(contains_line_fragment(out, "info string"));
        EXPECT_EQ(session.fatal_count(), 0);
    }
    end_section();

    const char* rejected[] = {
        "fen not-a-fen at all",                          // garbage FEN
        "fen",                                           // bare `position fen`
        "",                                              // no args at all
        "kentucky",                                      // unknown sub-token
        "startpos moves e2e5",                           // illegal move
        "startpos moves e2e4 e2e5",                      // legal prefix, then illegal
        "fen 8/8/8/4k3/8/2BB4/4P3/4K3 w - - 0 1",        // side not to move in check
        "fen 4k3/8/3N4/8/B7/8/8/K3R3 b - - 0 1",         // triple check
    };
    for (const char* args : rejected) {
        const std::string label = std::string("engine: rejected position is fatal: `") + args + "`";
        begin_section(label.c_str());
        EngineSession session;
        session.position("startpos");
        session.position(args);
        session.go("depth 1");                           // must never be answered
        EXPECT(session.wait_for_fatal(5000));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        const std::string out = session.output();
        EXPECT(contains_line_fragment(out, "CRITICAL ERROR"));
        EXPECT_EQ(count_bestmove_lines(out), 0);
        end_section();
    }
}

// 1.10.1: under a clock the final line is extended once (Engine), within
// half the Move Overhead, re-sent as its own info line (no seldepth) and the
// ponder move taken from it. With Move Overhead 0 the box is already spent.
void test_final_tablebase_pv() {
    const std::string path = syzygy_fixture_path().string();
    const std::string fen = "6k1/8/8/8/8/8/8/6KQ w - - 0 1";

    auto run = [&](int overhead, std::string& final_line, std::string& bestmove) {
        EngineSession session;
        session.set_option("name SyzygyPath value " + path);
        session.set_option("name Move Overhead value " + std::to_string(overhead));
        session.position("fen " + fen);
        session.sync();
        session.go("depth 2 wtime 60000 btime 60000");
        EXPECT(session.wait_for_bestmoves(1, 10000));
        // The extended line repeats the final depth after the searched one.
        std::istringstream input(session.output());
        std::string line;
        std::vector<std::string> depth_lines;
        while (std::getline(input, line)) {
            if (line.rfind("info depth", 0) == 0)
                depth_lines.push_back(line);
            if (line.rfind("bestmove", 0) == 0)
                bestmove = line;
        }
        auto depth_of = [](const std::string& text) { return std::stoi(text.substr(11)); };
        const size_t n = depth_lines.size();
        if (n >= 2 && depth_of(depth_lines[n - 1]) == depth_of(depth_lines[n - 2]))
            final_line = depth_lines[n - 1];
    };

    begin_section("engine tb pv: final line extended to mate, ponder from it");
    {
        std::string final_line, bestmove;
        run(1000, final_line, bestmove);
        EXPECT(!final_line.empty());
        std::istringstream pv(final_line.substr(final_line.find(" pv ") + 4));
        Board b;
        b.set_fen(fen);
        std::string tok;
        std::vector<std::string> moves;
        bool legal = true;
        while (pv >> tok) {
            MoveList list;
            b.gen_legal(list);
            Move found = MOVE_NONE;
            for (Move m : list)
                if (move_to_uci(m) == tok) found = m;
            legal = legal && found != MOVE_NONE;
            if (found == MOVE_NONE) break;
            b.make_move(found);
            moves.push_back(tok);
        }
        MoveList replies;
        b.gen_legal(replies);
        EXPECT(legal);
        EXPECT(replies.size() == 0 && b.is_in_check());
        EXPECT(moves.size() > 1);
        EXPECT(moves.size() > 1 && bestmove == "bestmove " + moves[0] + " ponder " + moves[1]);
    }
    end_section();

    begin_section("engine tb pv: the extended line carries the full field set");
    {
        std::string final_line, bestmove;
        run(1000, final_line, bestmove);
        for (const char* field : {" seldepth ", " multipv 1 ", " nps ", " hashfull ", " tbhits ", " time "})
            EXPECT(final_line.find(field) != std::string::npos);
    }
    end_section();

    begin_section("engine tb pv: Move Overhead 0 leaves no time to extend");
    {
        std::string final_line, bestmove;
        run(0, final_line, bestmove);
        EXPECT(final_line.empty());
        EXPECT(!bestmove.empty());
    }
    end_section();
    Syzygy::clear();
}

// Under a clock the final tablebase extension starts only with ten move
// overheads left before the hard ceiling: at Rarog's forfeit, 58 ms of clock
// at the default 10 ms overhead, it does not start.
void test_tablebase_extension_start_rule() {
    begin_section("engine tb pv: the start rule at the forfeit's numbers");
    EXPECT(!tablebase_extension_may_start(1000, 10, 1, 922.0));   // 58 ms left
    EXPECT(tablebase_extension_may_start(1000, 10, 1, 880.0));    // 100 ms left
    EXPECT(!tablebase_extension_may_start(1000, 10, 1, 881.0));
    EXPECT(tablebase_extension_may_start(1000, 10, 4, 850.0));    // helpers reserve 30 ms more
    EXPECT(!tablebase_extension_may_start(1000, 10, 4, 851.0));
    EXPECT(tablebase_extension_may_start(60000, 10, 1, 5000.0));
    end_section();

    begin_section("engine tb pv: a short clock plays the searched line unextended");
    {
        const std::string fen = "6k1/8/8/8/8/8/8/6KQ w - - 0 1";
        EngineSession session;
        session.set_option("name SyzygyPath value " + syzygy_fixture_path().string());
        session.set_option("name Move Overhead value 10");
        session.position("fen " + fen);
        session.sync();
        session.go("depth 2 wtime 100 btime 100");
        EXPECT(session.wait_for_bestmoves(1, 10000));
        std::istringstream input(session.output());
        std::string line;
        std::vector<int> depths;
        while (std::getline(input, line))
            if (line.rfind("info depth ", 0) == 0)
                depths.push_back(std::stoi(line.substr(11)));
        const size_t n = depths.size();
        EXPECT(n >= 1);
        EXPECT(n < 2 || depths[n - 1] != depths[n - 2]);   // no repeated final line
        Syzygy::clear();
    }
    end_section();
}

// A won ending whose score keeps rising: past the optimum, a fail-high on the
// move the last iteration chose ends the search instead of re-searching to the
// hard maximum (15.7 s here; the optimum is 2.3 s). Only an optimised build
// searches fast enough for the clock this test sets.
void test_won_ending_does_not_sink_the_clock() {
    begin_section("engine clock: a rising won ending is played well before the maximum");
#ifdef NDEBUG
    EngineSession session;
    session.position("fen 7r/5R2/8/2k1PB2/8/4K3/8/8 w - - 0 86");
    session.sync();
    const auto start = std::chrono::steady_clock::now();
    session.go("wtime 30000 btime 30000 winc 300 binc 300");
    EXPECT(session.wait_for_bestmoves(1, 20000));
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::fprintf(stderr, "  rec1 at 30000+300 took %.2f s\n", seconds);
    EXPECT(seconds < 8.0);
    EXPECT(contains_line_fragment(session.output(), "bestmove f7d7"));
#else
    EXPECT(true);   // unoptimised builds are too slow for a clock test
#endif
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
    test_threaded_last_line_names_bestmove();
    test_terminal_root_reports_once();
    test_multipv();

    std::printf("\nRoot commands\n");
    test_go_perft_returns_nodes_without_bestmove();
    test_go_searchmoves_restricts_root_move();

    std::printf("\nMalformed-input survival (8.6.3a)\n");
    test_malformed_input_survival();
    test_final_tablebase_pv();
    test_tablebase_extension_start_rule();
    test_won_ending_does_not_sink_the_clock();

    return harness_summary();
}
