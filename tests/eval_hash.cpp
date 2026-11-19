/// A digest of the evaluator over a position file: the proof that a
/// restructure of the evaluation left every score where it was.
///
///     eval_hash FILE [--lazy-audit]
///
/// FILE is a CSV whose header names a `fen` column, or one position per line
/// with anything after a ';' ignored. One warm evaluator reads the positions in
/// order, as a search would, so the pawn and imbalance caches are on the path.
///
/// Printed, one `key: value` line each:
///   positions      positions read (a malformed line is a harness error)
///   served_hash    FNV-1a over evaluate() of every position, side to move
///   verdict_hash   FNV-1a over draw_verdict() of every position
///   trace_hash     TEXEL_TRACE builds only: FNV-1a over the phase and every
///                  middlegame and endgame trace slot of every position
///
/// --lazy-audit (Tune and Diag builds) reads the file a second time with the
/// lazy audit on; its served hash must equal the first, and its counters are
/// printed. Exit status: 0 on success, 1 when the audit changed a served
/// score, 2 on a harness error.

#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "eval.h"
#include "zobrist.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Fnv {
    uint64_t h = 0xcbf29ce484222325ull;
    void add(int64_t v) {
        for (int i = 0; i < 8; ++i) {
            h ^= static_cast<uint64_t>(v >> (8 * i)) & 0xffu;
            h *= 0x100000001b3ull;
        }
    }
};

bool read_fens(const std::string& path, std::vector<std::string>& out) {
    std::ifstream in(path);
    if (!in) {
        std::fprintf(stderr, "cannot open %s\n", path.c_str());
        return false;
    }
    std::string line;
    int fen_column = -1;
    bool first = true;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty() || line[0] == '#')
            continue;
        if (first) {
            first = false;
            std::stringstream header(line);
            std::string cell;
            for (int col = 0; std::getline(header, cell, ','); ++col)
                if (cell == "fen")
                    fen_column = col;
            if (fen_column >= 0)
                continue;
        }
        if (fen_column >= 0) {
            std::stringstream row(line);
            std::string cell;
            for (int col = 0; std::getline(row, cell, ','); ++col)
                if (col == fen_column)
                    break;
            out.push_back(cell);
        } else {
            out.push_back(line.substr(0, line.find(';')));
        }
    }
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: eval_hash FILE [--lazy-audit]\n");
        return 2;
    }
    const bool lazy_audit = argc > 2 && std::string(argv[2]) == "--lazy-audit";

    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables();

    std::vector<std::string> fens;
    if (!read_fens(argv[1], fens))
        return 2;
    std::vector<Board> boards(fens.size());
    for (size_t i = 0; i < fens.size(); ++i) {
        if (auto r = boards[i].try_set_fen(fens[i]); !r) {
            std::fprintf(stderr, "bad FEN at position %zu: %s\n", i, fens[i].c_str());
            return 2;
        }
    }

    auto ev = std::make_unique<Evaluator>();
    Fnv served, verdict;
#ifdef TEXEL_TRACE
    Fnv trace;
#endif
    for (const Board& b : boards) {
#ifdef TEXEL_TRACE
        g_trace = {};
#endif
        served.add(ev->evaluate(b));
#ifdef TEXEL_TRACE
        trace.add(g_trace.phase);
        for (int i = 0; i < EVAL_PARAM_FLAT_SIZE; ++i) {
            trace.add(g_trace.mg[i]);
            trace.add(g_trace.eg[i]);
        }
#endif
        verdict.add(ev->draw_verdict(b) ? 1 : 0);
    }

    std::printf("positions: %zu\n", boards.size());
    std::printf("served_hash: %016llx\n", static_cast<unsigned long long>(served.h));
    std::printf("verdict_hash: %016llx\n", static_cast<unsigned long long>(verdict.h));
#ifdef TEXEL_TRACE
    std::printf("trace_hash: %016llx\n", static_cast<unsigned long long>(trace.h));
#endif

    if (!lazy_audit)
        return 0;
#if (defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)) && !defined(TEXEL_TRACE)
    auto audited = std::make_unique<Evaluator>();
    audited->diag_lazy = true;
    Fnv again;
    for (const Board& b : boards)
        again.add(audited->evaluate(b));
    std::printf("audited_served_hash: %016llx\n", static_cast<unsigned long long>(again.h));
    std::printf("lazy_fires: %lld\n", static_cast<long long>(audited->lazy_fires));
    std::printf("lazy_sign_flips: %lld\n", static_cast<long long>(audited->lazy_sign_flips));
    std::printf("lazy_margin_crossings: %lld\n", static_cast<long long>(audited->lazy_margin_crossings));
    std::printf("lazy_absdelta_sum: %lld\n", static_cast<long long>(audited->lazy_absdelta_sum));
    std::printf("lazy_absdelta_max: %lld\n", static_cast<long long>(audited->lazy_absdelta_max));
    return again.h == served.h ? 0 : 1;
#else
    std::fprintf(stderr, "--lazy-audit needs a Tune or Diag build\n");
    return 2;
#endif
}
