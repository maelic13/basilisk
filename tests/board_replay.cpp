/// Search-replay board benchmark.
///
/// Replays the board operations a real search made, in its order and with
/// its arguments, as a BASILISK_BOARD_TRACE build recorded them
/// (src/board_trace.cpp), on ONE production Board that does nothing else.
/// The positions are search nodes, the mix is the search's, the board stays
/// hot and the branch history is real: this prices the search's own board
/// work, which a microbenchmark column on cold game positions underprices.
///
///   board_replay <trace> --verify [--plant OP]
///   board_replay <trace> [--classes LIST] [--passes N] [--stub] [--nodes N]
///
/// --verify replays every record and compares every recorded result (the key
/// after each make, each generated count, each Boolean answer, each check
/// mask); the first mismatch is exit 1. --plant OP first corrupts the
/// recorded result of the first record of OP (MAKE, GEN_CAPS, SEE, ...), so
/// that a check can be shown to fail.
///
/// A timed run first keeps only the records of the chosen classes (make,
/// unmake and null moves always run), takes one warm-up pass, then prints
/// every pass with its checksum and the median. LIST is a comma list of gen,
/// see, gives_check, check_squares, is_legal, draw, misc (is_in_check,
/// has_non_pawn_material); `all` (the default) or `none`. --nodes is the
/// recorded search's node count, for figures per node.
///
/// --stub is the control: the same records through the same switch, each
/// case an empty out-of-line call, so its time is the replay loop's own
/// (record loads, the unpredictable dispatch, the checksum). Board cost is a
/// timed run less its stub. The stub over-corrects, since a real call hides
/// part of the next misprediction, so the two figures bracket the truth; an
/// A/B of two boards on one trace needs no correction.
///
/// tools/diag/board_replay_run.py records the trace and runs this.

#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "board_trace_format.h"
#include "zobrist.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace board_trace;

namespace {

#if defined(__GNUC__) || defined(__clang__)
template <typename T>
inline void keep(T const& value) { asm volatile("" : : "r,m"(value) : "memory"); }
#define REPLAY_NOINLINE __attribute__((noinline))
#else
template <typename T>
inline void keep(T const& value) { (void)value; }
#define REPLAY_NOINLINE
#endif

inline uint64_t mix(uint64_t cs, uint64_t v) { return ((cs << 1) | (cs >> 63)) ^ v; }

enum Mode { TIMING, VERIFY, STUB };

// The stub control's cases: distinct out-of-line functions that fold the
// record and touch no board.
#define STUB_FN(N, K) \
    REPLAY_NOINLINE uint64_t stub_##N(uint64_t cs, Rec r) { \
        return mix(cs, uint64_t(r.move) * K + uint64_t(uint32_t(r.arg)) + r.aux); }
STUB_FN(0, 3) STUB_FN(1, 5) STUB_FN(2, 7) STUB_FN(3, 11) STUB_FN(4, 13) STUB_FN(5, 17) STUB_FN(6, 19)
STUB_FN(7, 23) STUB_FN(8, 29) STUB_FN(9, 31) STUB_FN(10, 37) STUB_FN(11, 41) STUB_FN(12, 43)
STUB_FN(13, 47) STUB_FN(14, 53) STUB_FN(15, 59) STUB_FN(16, 61) STUB_FN(17, 67) STUB_FN(18, 71)
#undef STUB_FN

struct Fen { std::string text; size_t history; };

struct Mismatch {
    bool   failed = false;
    size_t index = 0;
    uint64_t expected = 0, actual = 0;
    const char* what = "";
};

// Replays recs[0..n) and returns the checksum. `en[op]` says whether a query
// op runs (root, make, unmake and null moves always do). VERIFY stops at the
// first mismatch and fills `mm`, and counts records by op in `by_op`.
template <Mode M>
uint64_t run(const Rec* recs, size_t n, const std::vector<Fen>& fens, Board& b, MoveList& ml,
             const bool* en, Mismatch* mm, uint64_t* by_op) {
    uint64_t cs = 0;
    for (size_t i = 0; i < n; ++i) {
        const Rec r = recs[i];
        if constexpr (M == VERIFY) ++by_op[r.op];
        if constexpr (M == STUB) {
            switch (r.op) {
            case ROOT: cs = stub_0(cs, r); break;
            case MAKE: cs = stub_1(cs, r); break;
            case UNMAKE: cs = stub_2(cs, r); break;
            case NULL_MAKE: cs = stub_3(cs, r); break;
            case NULL_UNMAKE: cs = stub_4(cs, r); break;
            case GEN_LEGAL: cs = stub_5(cs, r); break;
            case GEN_CAPS: cs = stub_6(cs, r); break;
            case GEN_QUIETS: cs = stub_7(cs, r); break;
            case GEN_QUIET_CHECKS: cs = stub_8(cs, r); break;
            case SEE: cs = stub_9(cs, r); break;
            case GIVES_CHECK: cs = stub_10(cs, r); break;
            case IS_LEGAL: cs = stub_11(cs, r); break;
            case IS_DRAW_PLY: cs = stub_12(cs, r); break;
            case IS_REPETITION: cs = stub_13(cs, r); break;
            case UPCOMING_REPETITION: cs = stub_14(cs, r); break;
            case IN_CHECK: cs = stub_15(cs, r); break;
            case HAS_NPM: cs = stub_16(cs, r); break;
            case CHECK_SQUARES: cs = stub_17(cs, r); break;
            default: cs = stub_18(cs, r); break;
            }
            continue;
        }
#define BAD(WHAT, EXP, ACT) \
    do { mm->failed = true; mm->index = i; mm->what = WHAT; mm->expected = uint64_t(EXP); \
         mm->actual = uint64_t(ACT); return cs; } while (0)
        switch (r.op) {
        case ROOT:
            b.set_fen(fens[size_t(r.arg)].text);
            if constexpr (M == VERIFY) {
                if (b.history_size() != fens[size_t(r.arg)].history)
                    BAD("history_size after set_fen", fens[size_t(r.arg)].history, b.history_size());
            }
            cs = mix(cs, b.position_key());
            break;
        case MAKE:
            b.make_move(r.move);
            if constexpr (M == VERIFY) {
                if (uint32_t(b.position_key()) != uint32_t(r.arg))
                    BAD("position_key low 32 after make_move", uint32_t(r.arg), uint32_t(b.position_key()));
            }
            cs = mix(cs, b.position_key());
            keep(b);
            break;
        case UNMAKE:
            b.unmake_move(r.move);
            cs = mix(cs, b.position_key());
            keep(b);
            break;
        case NULL_MAKE:
            b.make_null_move();
            cs = mix(cs, b.position_key());
            keep(b);
            break;
        case NULL_UNMAKE:
            b.unmake_null_move();
            cs = mix(cs, b.position_key());
            keep(b);
            break;
#define GEN_CASE(OP, CALL) \
        case OP: \
            if (!en[OP]) break; \
            ml.reset(); \
            b.CALL(ml); \
            keep(ml); \
            if constexpr (M == VERIFY) { \
                if (ml.size() != r.arg) BAD(#OP " move count", r.arg, ml.size()); \
            } \
            cs = mix(cs, uint64_t(ml.size())); \
            break;
        GEN_CASE(GEN_LEGAL, gen_legal)
        GEN_CASE(GEN_CAPS, gen_legal_captures)
        GEN_CASE(GEN_QUIETS, gen_legal_quiets)
        GEN_CASE(GEN_QUIET_CHECKS, gen_quiet_checks)
#define BOOL_CASE(OP, EXPR) \
        case OP: { \
            if (!en[OP]) break; \
            const bool res = (EXPR); \
            keep(res); \
            if constexpr (M == VERIFY) { \
                if (uint8_t(res) != (r.aux & 1)) BAD(#OP " result", r.aux & 1, res); \
            } \
            cs = mix(cs, res); \
            break; }
        BOOL_CASE(SEE, b.see_ge(r.move, r.arg))
        BOOL_CASE(GIVES_CHECK, b.gives_check(r.move))
        BOOL_CASE(IS_LEGAL, b.is_legal(r.move))
        BOOL_CASE(IS_DRAW_PLY, b.is_draw(r.arg))
        BOOL_CASE(IS_REPETITION, b.is_repetition(r.arg))
        BOOL_CASE(UPCOMING_REPETITION, b.upcoming_repetition(r.arg))
        BOOL_CASE(IN_CHECK, b.is_in_check())
        BOOL_CASE(HAS_NPM, b.has_non_pawn_material(Color(r.aux >> 7)))
        case CHECK_SQUARES: {
            if (!en[CHECK_SQUARES]) break;
            const Bitboard bb = b.check_squares(PieceType(r.aux & 7), Color(r.aux >> 7));
            keep(bb);
            const int32_t fold = int32_t(uint32_t(bb) ^ uint32_t(bb >> 32));
            if constexpr (M == VERIFY) {
                if (fold != r.arg) BAD("CHECK_SQUARES fold", uint32_t(r.arg), uint32_t(fold));
            }
            cs = mix(cs, uint32_t(fold));
            break; }
        default:
            if constexpr (M == VERIFY) BAD("unknown op", 0, r.op);
            break;
        }
#undef BOOL_CASE
#undef GEN_CASE
#undef BAD
    }
    return cs;
}

[[noreturn]] void fail(const char* what, const std::string& detail) {
    std::fprintf(stderr, "%s %s\n", what, detail.c_str());
    std::exit(2);
}

std::vector<Rec> load_trace(const char* path) {
    std::error_code ec;
    const uintmax_t bytes = std::filesystem::file_size(path, ec);
    if (ec) fail("cannot read", path);
    if (bytes == 0 || bytes % sizeof(Rec) != 0) fail("bad trace size:", path);
    std::FILE* f = std::fopen(path, "rb");
    if (!f) fail("cannot read", path);
    std::vector<Rec> recs(size_t(bytes / sizeof(Rec)));
    if (std::fread(recs.data(), sizeof(Rec), recs.size(), f) != recs.size()) fail("short read:", path);
    std::fclose(f);
    return recs;
}

std::vector<Fen> load_fens(const std::string& path) {
    std::ifstream in(path);
    if (!in) fail("cannot read", path);
    std::vector<Fen> fens;
    for (std::string line; std::getline(in, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const size_t tab = line.find('\t');
        Fen f;
        f.text = line.substr(0, tab);
        f.history = tab == std::string::npos ? 0 : size_t(std::strtoull(line.c_str() + tab + 1, nullptr, 10));
        fens.push_back(std::move(f));
    }
    return fens;
}

// Corrupts the recorded result of the first record of `op`; returns its index.
size_t plant(std::vector<Rec>& recs, unsigned op) {
    for (size_t i = 0; i < recs.size(); ++i) {
        Rec& r = recs[i];
        if (r.op != op) continue;
        switch (op) {
        case MAKE: case CHECK_SQUARES: r.arg ^= 1; break;
        case GEN_LEGAL: case GEN_CAPS: case GEN_QUIETS: case GEN_QUIET_CHECKS: r.arg += 1; break;
        case SEE: case GIVES_CHECK: case IS_LEGAL: case IS_DRAW_PLY: case IS_REPETITION:
        case UPCOMING_REPETITION: case IN_CHECK: case HAS_NPM: r.aux ^= 1; break;
        default: fail("no result to plant in", op_name(op));
        }
        return i;
    }
    fail("the trace holds no record of", op_name(op));
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: board_replay <trace> --verify [--plant OP]\n"
                             "       board_replay <trace> [--classes LIST] [--passes N] [--stub] [--nodes N]\n");
        return 2;
    }
    bool verify = false, stub = false;
    int passes = 5;
    long long nodes = 0;
    std::string classes = "all", plant_op;
    for (int a = 2; a < argc; ++a) {
        if (!std::strcmp(argv[a], "--verify")) verify = true;
        else if (!std::strcmp(argv[a], "--stub")) stub = true;
        else if (!std::strcmp(argv[a], "--classes") && a + 1 < argc) classes = argv[++a];
        else if (!std::strcmp(argv[a], "--passes") && a + 1 < argc) passes = std::atoi(argv[++a]);
        else if (!std::strcmp(argv[a], "--nodes") && a + 1 < argc) nodes = std::atoll(argv[++a]);
        else if (!std::strcmp(argv[a], "--plant") && a + 1 < argc) plant_op = argv[++a];
        else fail("unknown argument", argv[a]);
    }
    if (passes < 1) fail("need at least one pass,", std::to_string(passes));
    if (!plant_op.empty() && !verify) fail("--plant needs", "--verify");
    if (verify && (stub || classes != "all")) fail("--verify replays every record;", "no --stub or --classes");

    bool en[OP_COUNT] = {};
    {
        static const char* const known[] = {"all", "none", "gen", "see", "gives_check",
                                            "check_squares", "is_legal", "draw", "misc"};
        bool has[9] = {};
        for (size_t p = 0; p <= classes.size();) {
            size_t q = classes.find(',', p);
            if (q == std::string::npos) q = classes.size();
            const std::string tok = classes.substr(p, q - p);
            bool ok = false;
            for (int k = 0; k < 9; ++k)
                if (tok == known[k]) has[k] = ok = true;
            if (!ok) fail("unknown class", "'" + tok + "'");
            p = q + 1;
        }
        const bool all = has[0];
        if (all || has[2]) en[GEN_LEGAL] = en[GEN_CAPS] = en[GEN_QUIETS] = en[GEN_QUIET_CHECKS] = true;
        if (all || has[3]) en[SEE] = true;
        if (all || has[4]) en[GIVES_CHECK] = true;
        if (all || has[5]) en[CHECK_SQUARES] = true;
        if (all || has[6]) en[IS_LEGAL] = true;
        if (all || has[7]) en[IS_DRAW_PLY] = en[IS_REPETITION] = en[UPCOMING_REPETITION] = true;
        if (all || has[8]) en[IN_CHECK] = en[HAS_NPM] = true;
    }

    // Everything is loaded before anything is timed.
    std::vector<Rec> recs = load_trace(argv[1]);
    const std::vector<Fen> fens = load_fens(std::string(argv[1]) + ".fens");
    init_bitboards();
    init_attacks();
    Zobrist::init();
    Board board;
    MoveList ml;
    const size_t recorded = recs.size();

    if (verify) {
        size_t planted = SIZE_MAX;
        if (!plant_op.empty()) {
            unsigned op = 0;
            for (unsigned o = 1; o < OP_COUNT; ++o)
                if (plant_op == op_name(o)) op = o;
            if (!op) fail("unknown op", plant_op);
            planted = plant(recs, op);
            std::printf("planted: a wrong %s result at record %zu\n", plant_op.c_str(), planted);
        }
        std::printf("trace: %s  records: %zu  roots: %zu  [verify]\n", argv[1], recorded, fens.size());
        uint64_t by_op[256] = {};
        Mismatch mm;
        run<VERIFY>(recs.data(), recs.size(), fens, board, ml, en, &mm, by_op);
        if (mm.failed) {
            const Rec& r = recs[mm.index];
            std::printf("verify: FAIL at record %zu (op %s): %s: recorded %llu, replayed %llu\n",
                        mm.index, op_name(r.op), mm.what,
                        (unsigned long long)mm.expected, (unsigned long long)mm.actual);
            std::printf("  record: op=%u aux=%u move=%u arg=%d\n", r.op, r.aux, r.move, r.arg);
            std::printf("  board fen: %s\n", board.get_fen().c_str());
            return 1;
        }
        std::printf("verify: PASS\n");
        for (unsigned op = 1; op < OP_COUNT; ++op)
            std::printf("  %-20s %12llu\n", op_name(op), (unsigned long long)by_op[op]);
        return 0;
    }

    // Keep only the records this run executes, so a class left out costs
    // nothing in the timed loop.
    size_t w = 0;
    for (size_t i = 0; i < recs.size(); ++i) {
        const uint8_t op = recs[i].op;
        if (op == ROOT || op == MAKE || op == UNMAKE || op == NULL_MAKE || op == NULL_UNMAKE || en[op])
            recs[w++] = recs[i];
    }
    recs.resize(w);
    const size_t n = recs.size();
    std::printf("trace: %s  records: %zu of %zu  roots: %zu  classes: %s%s\n", argv[1], n, recorded,
                fens.size(), classes.c_str(), stub ? "  [stub]" : "");

    auto pass = [&]() -> uint64_t {
        return stub ? run<STUB>(recs.data(), n, fens, board, ml, en, nullptr, nullptr)
                    : run<TIMING>(recs.data(), n, fens, board, ml, en, nullptr, nullptr);
    };
    const uint64_t cs = pass();
    std::printf("warm-up checksum: %016llx\n", (unsigned long long)cs);

    auto per_node = [&](double ns) { return nodes > 0 ? ns / double(nodes) : 0.0; };
    std::vector<double> totals;
    bool differs = false;
    for (int p = 0; p < passes; ++p) {
        const auto t0 = std::chrono::steady_clock::now();
        const uint64_t c = pass();
        const auto t1 = std::chrono::steady_clock::now();
        keep(c);
        differs = differs || c != cs;
        const double ns = double(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        totals.push_back(ns);
        std::printf("pass %d: total %.0f ns  %.3f ns/record  %.3f ns/node  checksum %016llx%s\n", p + 1,
                    ns, ns / double(n), per_node(ns), (unsigned long long)c, c == cs ? "" : "  (DIFFERS)");
    }
    std::sort(totals.begin(), totals.end());
    const double med = totals.size() % 2 ? totals[totals.size() / 2]
                                         : 0.5 * (totals[totals.size() / 2 - 1] + totals[totals.size() / 2]);
    std::printf("median: total %.0f ns  %.3f ns/record  %.3f ns/node  (nodes %lld)\n", med,
                med / double(n), per_node(med), nodes);
    if (differs) {
        std::fprintf(stderr, "a pass's checksum differs from the warm-up's\n");
        return 1;
    }
    return 0;
}
