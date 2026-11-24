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
///
/// --rest (TEXEL_TRACE builds) reports the tuner's untraced remainder, the
/// served score less the trace's reconstruction, by source: each frozen term
/// tapered alone, the taper's rounding, apply_endgame, and the rule-50
/// damping, one `rest.<source>: <positions nonzero> <sum |value|> <max |value|>`
/// line each. `rest.unattributed` is what the running sum holds beyond the
/// traced terms and the frozen ones; exit status 1 when it is not zero.

#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "eval.h"
#include "zobrist.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
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

struct RestStat {
    const char* name;
    int64_t nonzero = 0, abs_sum = 0, abs_max = 0;
    void add(int64_t v) {
        const int64_t a = v < 0 ? -v : v;
        nonzero += a != 0;
        abs_sum += a;
        abs_max = a > abs_max ? a : abs_max;
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
    [[maybe_unused]] const bool rest_report = argc > 2 && std::string(argv[2]) == "--rest";

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
    RestStat rest_total{"total"}, rest_passer{"passer_rank"}, rest_mate{"mate_drive"},
             rest_rounding{"taper_rounding"}, rest_endgame{"apply_endgame"},
             rest_damping{"rule50_damping"}, rest_unattributed{"unattributed"};
#endif
    for (const Board& b : boards) {
#ifdef TEXEL_TRACE
        g_trace = {};
        g_rest = {};
#endif
        const int score = ev->evaluate(b);
        served.add(score);
#ifdef TEXEL_TRACE
        trace.add(g_trace.phase);
        for (int i = 0; i < EVAL_PARAM_FLAT_SIZE; ++i) {
            trace.add(g_trace.mg[i]);
            trace.add(g_trace.eg[i]);
        }
        if (rest_report) {
            // The traced terms' sums, as reconstruct() forms them.
            int64_t mg_dot = 0, eg_dot = 0;
#define X(name, member, len) \
            { const auto w = eval_param_cspan(g_eval_params.member, (len)); \
              const int base = eval_param_offset(EPG_##name); \
              for (size_t i = 0; i < w.size(); ++i) { \
                  mg_dot += g_trace.mg[base + int(i)] * w[i]; \
                  eg_dot += g_trace.eg[base + int(i)] * w[i]; \
              } }
            EVAL_PARAM_LIST(X)
#undef X
            const int phase = g_trace.phase;
            auto taper = [&](int64_t mg, int64_t eg) { return (mg * phase + eg * (24 - phase)) / 24; };
            const int white = b.turn() == WHITE ? score : -score;
            const int64_t linear = reconstruct(g_trace, g_eval_params);
            const int64_t passer = taper(g_rest.mg[REST_PASSER_RANK], g_rest.eg[REST_PASSER_RANK]);
            const int64_t mate = taper(g_rest.mg[REST_MATE_DRIVE], g_rest.eg[REST_MATE_DRIVE]);
            int64_t frozen_mg = 0, frozen_eg = 0;
            for (int s = 0; s < REST_SOURCE_NB; ++s) {
                frozen_mg += g_rest.mg[s];
                frozen_eg += g_rest.eg[s];
            }
            rest_total.add(white - linear);
            rest_passer.add(passer);
            rest_mate.add(mate);
            rest_rounding.add(g_rest.tapered - linear - passer - mate);
            rest_endgame.add(g_rest.endgame - g_rest.tapered);
            rest_damping.add(white - g_rest.endgame);
            rest_unattributed.add(std::abs(g_rest.sum_mg - mg_dot - frozen_mg)
                                  + std::abs(g_rest.sum_eg - eg_dot - frozen_eg));
        }
#endif
        verdict.add(ev->draw_verdict(b) ? 1 : 0);
    }

    std::printf("positions: %zu\n", boards.size());
    std::printf("served_hash: %016llx\n", static_cast<unsigned long long>(served.h));
    std::printf("verdict_hash: %016llx\n", static_cast<unsigned long long>(verdict.h));
#ifdef TEXEL_TRACE
    std::printf("trace_hash: %016llx\n", static_cast<unsigned long long>(trace.h));
    if (rest_report) {
        for (const RestStat* s : {&rest_total, &rest_passer, &rest_mate, &rest_rounding,
                                  &rest_endgame, &rest_damping, &rest_unattributed})
            std::printf("rest.%s: %lld %lld %lld\n", s->name, static_cast<long long>(s->nonzero),
                        static_cast<long long>(s->abs_sum), static_cast<long long>(s->abs_max));
        return rest_unattributed.nonzero == 0 ? 0 : 1;
    }
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
