/// Board microbenchmark: each board primitive timed alone.
///
/// Profile basilisk-board-micro-v1. The positions are game positions
/// (tests/data/board_micro_positions.fen, frozen from a gate's games by
/// tools/diag/board_micro_positions.py), so a primitive is timed on the
/// population the search meets, not on the five perft roots of
/// tests/board_performance.cpp, which stays as it is for cross-engine
/// comparison.
///
/// Every input -- the boards, the move lists, the SEE thresholds, the table
/// moves to validate -- is built before any timing, and each column then
/// runs only its primitive over those inputs. Each column's result is folded
/// into a checksum that is printed: it keeps the compiler from eliding the
/// work, and it is the column's semantic signature, so a rewrite that keeps
/// behaviour keeps it (the same binary inputs give the same checksum on any
/// host). Columns are sampled round-robin, so drift spreads over all of them,
/// and every sample is printed.
///
///   board_micro_bench <positions.fen> [--samples N] [--sample-ms MS] [--columns a,b,...]
///                     [--preflight-only]
///
/// Use tools/diag/board_micro_run.py to run it: it records the binary, the
/// host's load and the output.

#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "history_core.h"
#include "move_picker_core.h"
#include "zobrist.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

#if defined(__GNUC__) || defined(__clang__)
template <typename T>
inline void keep(T const& value) { asm volatile("" : : "r,m"(value) : "memory"); }
#else
template <typename T>
inline void keep(T const& value) { (void)value; }
#endif

// ---- Inputs, built once ----------------------------------------------------

struct Inputs {
    std::vector<Board> boards;
    // Per board: its legal moves, and the slice of each list below that
    // belongs to it.
    std::vector<MoveList> legal;
    std::vector<std::pair<uint32_t, Move>> all_moves;   // every legal move
    std::vector<std::pair<uint32_t, Move>> captures;    // the capture stage's moves
    std::vector<std::pair<uint32_t, Move>> quiets;      // the quiet moves
    std::vector<std::pair<uint32_t, Move>> table_moves; // candidates for the table-move test
};

// The file's python-chess counts, which the board must reproduce.
struct Oracle { uint64_t positions = 0, legal = 0, tactical = 0, checking = 0; bool present = false; };

std::vector<std::string> read_fens(const char* path, Oracle& oracle) {
    std::ifstream in(path);
    if (!in) {
        std::fprintf(stderr, "cannot read %s\n", path);
        std::exit(2);
    }
    std::vector<std::string> fens;
    for (std::string line; std::getline(in, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        unsigned long long n[4];
        if (std::sscanf(line.c_str(), "# Oracle: positions %llu legal %llu tactical %llu checking %llu",
                        &n[0], &n[1], &n[2], &n[3]) == 4)
            oracle = {n[0], n[1], n[2], n[3], true};
        if (line.empty() || line[0] == '#') continue;
        fens.push_back(line);
    }
    return fens;
}

bool capture_stage(const Board& b, Move m) { return is_capture_stage(b, m); }

Inputs build_inputs(const std::vector<std::string>& fens) {
    Inputs in;
    in.boards.reserve(fens.size());
    for (const std::string& fen : fens) {
        Board b;
        if (auto ok = b.try_set_fen(fen, true); !ok) {
            std::fprintf(stderr, "bad FEN %s: %s\n", fen.c_str(), ok.error().c_str());
            std::exit(2);
        }
        in.boards.push_back(b);
    }
    in.legal.resize(in.boards.size());
    for (uint32_t i = 0; i < in.boards.size(); ++i) {
        in.boards[i].gen_legal(in.legal[i]);
        for (Move m : in.legal[i]) {
            in.all_moves.emplace_back(i, m);
            (capture_stage(in.boards[i], m) ? in.captures : in.quiets).emplace_back(i, m);
        }
    }
    // The table-move test meets the node's own moves and, after a key
    // collision or a stale entry, another position's: each board is offered
    // its own legal moves and those of the next board in the list.
    for (uint32_t i = 0; i < in.boards.size(); ++i) {
        const uint32_t other = (i + 1) % uint32_t(in.boards.size());
        for (Move m : in.legal[i]) in.table_moves.emplace_back(i, m);
        for (Move m : in.legal[other]) in.table_moves.emplace_back(i, m);
    }
    return in;
}

// ---- Columns -----------------------------------------------------------------
//
// Each returns {operations, checksum} for one pass over its inputs.

struct Pass { uint64_t ops; uint64_t sum; };

Pass legal_generation(Inputs& in) {
    Pass p{0, 0};
    MoveList ml;
    for (const Board& b : in.boards) {
        ml.reset();
        b.gen_legal(ml);
        keep(&ml);
        p.ops += 1;
        p.sum = p.sum * 31 + uint64_t(ml.size()) + (ml.size() ? ml.moves[ml.size() - 1] : 0);
    }
    return p;
}

Pass capture_generation(Inputs& in) {
    Pass p{0, 0};
    MoveList ml;
    for (const Board& b : in.boards) {
        ml.reset();
        b.gen_legal_captures(ml);
        keep(&ml);
        p.ops += 1;
        p.sum = p.sum * 31 + uint64_t(ml.size()) + (ml.size() ? ml.moves[ml.size() - 1] : 0);
    }
    return p;
}

// The main picker's generation: the capture stage, then the quiets.
Pass staged_generation(Inputs& in) {
    Pass p{0, 0};
    MoveList caps, quiets;
    for (const Board& b : in.boards) {
        caps.reset();
        quiets.reset();
        b.gen_legal_captures(caps);
        b.gen_legal_quiets(quiets);
        keep(&caps);
        keep(&quiets);
        p.ops += 1;
        p.sum = p.sum * 31 + uint64_t(caps.size()) * 256 + uint64_t(quiets.size());
    }
    return p;
}

// Quiescence's quiet checks.
Pass quiet_check_generation(Inputs& in) {
    Pass p{0, 0};
    MoveList ml;
    for (const Board& b : in.boards) {
        if (b.is_in_check()) continue;
        ml.reset();
        b.gen_quiet_checks(ml);
        keep(&ml);
        p.ops += 1;
        p.sum = p.sum * 31 + uint64_t(ml.size());
    }
    return p;
}

Pass make_unmake(Inputs& in) {
    Pass p{0, 0};
    for (const auto& [i, m] : in.all_moves) {
        Board& b = in.boards[i];
        b.make_move(m);
        keep(b.position_key());
        p.sum += b.position_key() ^ b.checking_pieces();
        b.unmake_move(m);
        p.ops += 1;
    }
    return p;
}

Pass see_captures(Inputs& in) {
    Pass p{0, 0};
    for (const auto& [i, m] : in.captures) {
        const bool r = in.boards[i].see_ge(m, 0);
        keep(r);
        p.sum = p.sum * 3 + r;
        p.ops += 1;
    }
    return p;
}

// Quiet moves at the picker's check-bonus threshold: an exchange on the
// destination that starts with nothing won.
Pass see_quiets(Inputs& in) {
    Pass p{0, 0};
    for (const auto& [i, m] : in.quiets) {
        const bool r = in.boards[i].see_ge(m, -75);
        keep(r);
        p.sum = p.sum * 3 + r;
        p.ops += 1;
    }
    return p;
}

Pass gives_check(Inputs& in) {
    Pass p{0, 0};
    for (const auto& [i, m] : in.all_moves) {
        const bool r = in.boards[i].gives_check(m);
        keep(r);
        p.sum = p.sum * 3 + r;
        p.ops += 1;
    }
    return p;
}

// What Stockfish's StateInfo holds for a node, in Basilisk's per-call form:
// the squares from which each piece type checks the enemy king, and the
// checkers of the side to move.
Pass check_info(Inputs& in) {
    Pass p{0, 0};
    for (const Board& b : in.boards) {
        const Color us = b.turn();
        Bitboard s = 0;
        for (PieceType pt : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN})
            s ^= b.check_squares(pt, us) * (uint64_t(pt) * 2 + 1);
        s ^= b.attackers_to(b.king_square(us), b.all_pieces(), ~us);
        keep(s);
        p.sum += s;
        p.ops += 1;
    }
    return p;
}

// The test a table or killer move passes before it is played.
Pass table_move_test(Inputs& in) {
    Pass p{0, 0};
    for (const auto& [i, m] : in.table_moves) {
        const bool r = table_move_usable(in.boards[i], m);
        keep(r);
        p.sum = p.sum * 3 + r;
        p.ops += 1;
    }
    return p;
}

// Perft through the search's own path: the staged picker, with ordering
// scores from fixed histories, then make and unmake. Counts leaves.
struct PickerWalk {
    std::unique_ptr<CoreHistoryTables> history = std::make_unique<CoreHistoryTables>();
    const PieceToHistory* cont[6] = {};
    ScoredMove buffers[8][MoveList::CAPACITY];

    PickerWalk() {
        history->clear(true);
        for (int k = 0; k < 6; ++k)
            cont[k] = &history->continuation->row[k & 1][(k >> 1) & 1][KNIGHT + k % 4][(k * 13) % 64];
    }

    uint64_t perft(Board& b, int depth, int ply) {
        CoreMovePicker picker(b, *history, history->pawn->bucket[b.pawn_key_value() & 63], cont,
                              MOVE_NONE, MOVE_NONE, depth, ply, nullptr, buffers[ply]);
        uint64_t leaves = 0;
        for (Move m = picker.next(); m != MOVE_NONE; m = picker.next()) {
            if (depth == 1) {
                ++leaves;
                continue;
            }
            b.make_move(m);
            leaves += perft(b, depth - 1, ply + 1);
            b.unmake_move(m);
        }
        return leaves;
    }
};

struct SearchPerft {
    PickerWalk walk;
    std::vector<Board> roots;
    std::vector<int> depths;
};

std::unique_ptr<SearchPerft> search_perft_inputs() {
    auto sp = std::make_unique<SearchPerft>();
    const std::pair<const char*, int> roots[] = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4},
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5},
    };
    for (const auto& [fen, depth] : roots) {
        Board b;
        b.set_fen(fen);
        sp->roots.push_back(b);
        sp->depths.push_back(depth);
    }
    return sp;
}

Pass search_path_perft(SearchPerft& sp) {
    Pass p{0, 0};
    for (std::size_t i = 0; i < sp.roots.size(); ++i) {
        const uint64_t n = sp.walk.perft(sp.roots[i], sp.depths[i], 0);
        p.ops += n;
        p.sum = p.sum * 1000003 + n;
    }
    return p;
}

// The board's own perft over the same roots: gen_legal, make and unmake.
uint64_t plain_perft(Board& b, int depth) {
    MoveList ml;
    b.gen_legal(ml);
    if (depth == 1) return uint64_t(ml.size());
    uint64_t n = 0;
    for (Move m : ml) {
        b.make_move(m);
        n += plain_perft(b, depth - 1);
        b.unmake_move(m);
    }
    return n;
}

Pass board_perft(SearchPerft& sp) {
    Pass p{0, 0};
    for (std::size_t i = 0; i < sp.roots.size(); ++i) {
        const uint64_t n = plain_perft(sp.roots[i], sp.depths[i]);
        p.ops += n;
        p.sum = p.sum * 1000003 + n;
    }
    return p;
}

// ---- Harness -----------------------------------------------------------------

struct Column {
    Column(const char* n, const char* u, std::function<Pass()> f)
        : name(n), unit(u), run(std::move(f)) {}
    const char* name;
    const char* unit;
    std::function<Pass()> run;
    Pass expected{};
    uint64_t passes = 1;  // passes per sample, fixed at calibration
    std::vector<double> ns_per_op;
};

double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    const std::size_t n = v.size();
    return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: board_micro_bench <positions.fen> [--samples N] [--sample-ms MS]"
                             " [--columns a,b] [--preflight-only]\n");
        return 2;
    }
    int samples = 15;
    double sample_ms = 200.0;
    bool preflight_only = false;
    std::string only;
    for (int a = 2; a < argc; ++a) {
        if (!std::strcmp(argv[a], "--samples") && a + 1 < argc) samples = std::atoi(argv[++a]);
        else if (!std::strcmp(argv[a], "--sample-ms") && a + 1 < argc) sample_ms = std::atof(argv[++a]);
        else if (!std::strcmp(argv[a], "--columns") && a + 1 < argc) only = std::string(",") + argv[++a] + ",";
        else if (!std::strcmp(argv[a], "--preflight-only")) preflight_only = true;
        else {
            std::fprintf(stderr, "unknown argument %s\n", argv[a]);
            return 2;
        }
    }
    if (samples < 3 || sample_ms <= 0) {
        std::fprintf(stderr, "need at least 3 samples and a positive sample time\n");
        return 2;
    }

    init_bitboards();
    init_attacks();
    Zobrist::init();

    Oracle oracle;
    Inputs in = build_inputs(read_fens(argv[1], oracle));
    auto sp = search_perft_inputs();

    std::vector<Column> columns = {
        {"legal_gen", "positions", [&] { return legal_generation(in); }},
        {"capture_gen", "positions", [&] { return capture_generation(in); }},
        {"staged_gen", "positions", [&] { return staged_generation(in); }},
        {"quiet_check_gen", "positions", [&] { return quiet_check_generation(in); }},
        {"make_unmake", "moves", [&] { return make_unmake(in); }},
        {"see_captures", "moves", [&] { return see_captures(in); }},
        {"see_quiets", "moves", [&] { return see_quiets(in); }},
        {"gives_check", "moves", [&] { return gives_check(in); }},
        {"check_info", "positions", [&] { return check_info(in); }},
        {"table_move_test", "moves", [&] { return table_move_test(in); }},
        {"search_perft", "leaves", [&] { return search_path_perft(*sp); }},
        {"board_perft", "leaves", [&] { return board_perft(*sp); }},
    };
    if (!only.empty()) {
        std::erase_if(columns, [&](const Column& c) {
            return only.find(std::string(",") + c.name + ",") == std::string::npos;
        });
        if (columns.empty()) {
            std::fprintf(stderr, "no column matches %s\n", only.c_str());
            return 2;
        }
    }

    std::printf("profile: basilisk-board-micro-v1\n");
    std::printf("positions: %zu (%zu legal moves, %zu capture-stage, %zu quiet, %zu table-move candidates)\n",
                in.boards.size(), in.all_moves.size(), in.captures.size(), in.quiets.size(),
                in.table_moves.size());

    // Preflight: two passes of each column must agree on work and checksum,
    // and the search's path must find the board's perft.
    bool ok = true;
    for (Column& c : columns) {
        c.expected = c.run();
        const Pass again = c.run();
        if (again.ops != c.expected.ops || again.sum != c.expected.sum) {
            std::fprintf(stderr, "preflight: %s is not repeatable\n", c.name);
            ok = false;
        }
        std::printf("work %-16s %10llu %-9s checksum %016llx\n", c.name,
                    (unsigned long long)c.expected.ops, c.unit, (unsigned long long)c.expected.sum);
    }
    const Pass sp_pass = search_path_perft(*sp), bp_pass = board_perft(*sp);
    if (sp_pass.ops != bp_pass.ops || sp_pass.sum != bp_pass.sum) {
        std::fprintf(stderr, "preflight: the search's path finds %llu leaves, the board's perft %llu\n",
                     (unsigned long long)sp_pass.ops, (unsigned long long)bp_pass.ops);
        ok = false;
    }
    // Engine-independent semantics, the same lines every engine's harness of
    // the cross-engine contract prints (tools/board_compare/CONTRACT.md).
    uint64_t checking = 0, see_true = 0;
    for (const auto& [i, m] : in.all_moves) checking += in.boards[i].gives_check(m);
    for (const auto& [i, m] : in.captures) see_true += in.boards[i].see_ge(m, 0);
    std::printf("semantic positions %zu\n", in.boards.size());
    std::printf("semantic legal %zu\n", in.all_moves.size());
    std::printf("semantic tactical %zu\n", in.captures.size());
    std::printf("semantic checking %llu\n", (unsigned long long)checking);
    std::printf("semantic see_ge0 %llu\n", (unsigned long long)see_true);
    std::printf("semantic perft %llu\n", (unsigned long long)bp_pass.ops);
    if (!oracle.present) {
        std::fprintf(stderr, "preflight: %s has no oracle line\n", argv[1]);
        ok = false;
    } else if (oracle.positions != in.boards.size() || oracle.legal != in.all_moves.size()
               || oracle.tactical != in.captures.size() || oracle.checking != checking) {
        std::fprintf(stderr, "preflight: the board disagrees with the file's oracle counts\n");
        ok = false;
    }
    if (!ok) return 1;
    std::printf("preflight: PASS\n");
    if (preflight_only) return 0;

    using clock = std::chrono::steady_clock;
    // Warm up for one sample's time each, and fix the passes per sample.
    for (Column& c : columns) {
        const auto t0 = clock::now();
        uint64_t n = 0;
        while (std::chrono::duration<double, std::milli>(clock::now() - t0).count() < sample_ms) {
            keep(c.run().sum);
            ++n;
        }
        c.passes = std::max<uint64_t>(1, n);
    }

    std::printf("samples: %d round-robin, about %.0f ms each\n", samples, sample_ms);
    for (int s = 0; s < samples; ++s) {
        for (Column& c : columns) {
            uint64_t ops = 0, sum = 0;
            const auto t0 = clock::now();
            for (uint64_t k = 0; k < c.passes; ++k) {
                const Pass p = c.run();
                ops += p.ops;
                sum ^= p.sum + k;
            }
            const double ns = std::chrono::duration<double, std::nano>(clock::now() - t0).count();
            keep(sum);
            if (ops != c.expected.ops * c.passes) {
                std::fprintf(stderr, "sample %d: %s did %llu operations, not %llu\n", s, c.name,
                             (unsigned long long)ops, (unsigned long long)(c.expected.ops * c.passes));
                return 1;
            }
            c.ns_per_op.push_back(ns / double(ops));
            std::printf("sample %2d %-16s %10.3f ns/op\n", s, c.name, c.ns_per_op.back());
        }
    }

    std::printf("\n%-16s %12s %10s %8s %12s\n", "column", "median ns/op", "MAD ns", "MAD %", "passes/sample");
    for (Column& c : columns) {
        const double med = median(c.ns_per_op);
        std::vector<double> dev;
        for (double x : c.ns_per_op) dev.push_back(std::fabs(x - med));
        const double mad = median(dev);
        std::printf("%-16s %12.3f %10.3f %7.2f%% %12llu\n", c.name, med, mad, 100.0 * mad / med,
                    (unsigned long long)c.passes);
    }
    return 0;
}
