#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "Board.h"
#include "bench.h"
#include "search.h"
#include "tt.h"
#include "UciOutput.h"

// 40 positions covering openings, middlegames (quiet + tactical), a broad range
// of endgames, mates, and fortresses. In single-thread mode the final "Nodes
// searched" total is a deterministic search fingerprint; the per-position spread
// also feeds the geometric-mean EBF / median / top-share diagnostics below.
//
// Positions 1-16 are the original curated suite. Positions 17-40 are legal
// self-play positions sampled across piece counts (30 down to 8) so no single
// bushy middlegame dominates the node total — the 16-position suite had one
// position at ~35% of all nodes, which made the fingerprint lurch ~15% on
// sub-1-Elo parameter changes. Suite kept identical to sibling engine Rarog's
// (src/bench.rs) so the two bench harnesses match.
static constexpr std::array<std::string_view, 40> BENCH_FENS = {{
    // --- 1-16: original curated suite -----------------------------------------
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
    "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1",
    "8/pp2k3/8/2p5/2P5/1P2K3/P7/8 w - - 0 1",
    "r1bq1r2/pp2n3/4N2k/3pPppP/1b1n2Q1/2N5/PP3PP1/R1B1K2R w KQ g6 0 20",
    "r4rk1/pp1n1pp1/2p1pn1p/q7/3P4/2NB4/PP3PPP/R2QR1K1 w - - 0 1",
    "5k2/5p1p/p3B1p1/Pp6/1P6/5P1P/4K1P1/8 b - - 0 1",
    "6k1/p3q2p/1nr3p1/8/3Q4/7P/PP4P1/4R1K1 b - - 0 1",
    "2r3k1/1q2Rp1p/p2p2p1/1p1P4/1Pp1P3/2Q5/1P4PP/6K1 w - - 0 1",
    "1r3rk1/p4ppp/2p5/3Nb3/1p1bP3/1B4P1/PP3P1P/R2R2K1 b - - 0 1",
    "r2qr1k1/p4ppp/1pn1bn2/2b1p3/4P3/1BN1BN2/PPP2PPP/R2QR1K1 b - - 6 10",
    "r1bqkb1r/pp1p1ppp/2n1pn2/2p5/4P3/2NP1N2/PPP2PPP/R1BQKB1R w KQkq - 0 5",
    "8/8/p1p5/1p5p/1P5P/P1P5/8/K1k5 w - - 0 1",
    "1k6/1b6/8/5p2/p1p2p2/P7/1P3P2/K7 b - - 0 1",
    // --- 17-40: self-play positions, opening/middlegame -> endgame ------------
    "1k1rr3/pb3n2/1pnpqbp1/2pNp2p/2P1P2P/P2Q1P2/1PN1BBP1/1K1RR3 b - - 5 10",
    "3r1rk1/1ppb1pb1/p2npqnp/P5p1/3P4/1BN1BN1P/1PP2PP1/3RQR1K w - - 3 10",
    "1b2qrk1/rp3pp1/2p1p2p/p1Pp4/P2Pn3/1Q2PN1P/1P2BPP1/1KR2R2 w - - 3 11",
    "1r3rk1/1pqb1p2/pN1p1bp1/P1pPp3/2P1P2p/1P2QN1P/4RPP1/3R2K1 w - - 2 12",
    "1k1r1r2/pp3pp1/4qn1p/P1p1p3/2p1P3/3PPN1P/1PP3P1/2RQ1RK1 b - - 0 9",
    "1r1q1rk1/3np2p/1n1p2pb/pP1P4/4BP2/2p1BN1P/Q1P1N1P1/5RK1 b - - 0 11",
    "1k1r1n2/1pq1n1b1/2p1p1p1/p2p4/PP1P1PP1/2PB1N2/5B2/2Q1K2R w - - 0 12",
    "1kr1rq2/2p2pR1/p1n1pP2/n2pP3/3P3p/1P3N1P/2P1NQ2/1K1R4 w - - 4 13",
    "1b1rr1k1/1p4q1/1Qp1b1pp/p3p3/P7/2PBN3/1P1R1P2/3R2K1 b - - 1 9",
    "1kr2r2/1p1n1pp1/4p1p1/p2p2P1/3P3P/6P1/PPP3B1/1K1R1R2 b - - 0 10",
    "1Q2n1k1/4bp2/4p1p1/pB1pP2p/q2P1P1P/2P1KNP1/8/8 b - - 6 14",
    "1k2r3/1pp1bpKp/p7/8/2PNr3/1P2P1P1/P4P1P/3R3R b - - 2 9",
    "1B6/1p6/p1p2k2/P1P1p1p1/1P1nPp1p/5P1P/1K4P1/8 b - - 48 110",
    "1k1r4/1b4r1/p3P1n1/1p3pp1/2p5/2N5/1P2R1P1/4RBK1 b - - 4 11",
    "1Q4bk/3R2pp/p7/3p3P/1p6/1B6/P2q1PP1/6K1 w - - 2 17",
    "1R6/5ppk/2N1p2p/4P2P/P3P3/1P3KP1/1r2r3/8 b - - 1 25",
    "1B6/5p1k/3P1b2/p7/2r3Pp/2P4P/1P2R2K/8 b - - 0 9",
    "1R6/5pkp/4qp1b/3p4/7P/6P1/5Q1K/2r2B2 w - - 1 14",
    "1K6/1P1rkp2/B3p3/8/1R1Pb3/5p2/5P2/8 b - - 17 12",
    "1R6/5k2/3ppp2/4p3/P7/1P4P1/2P2r2/1K6 w - - 0 16",
    "1B6/8/1P1nk1p1/3b1p2/3K1P2/3B4/8/8 w - - 9 10",
    "1R6/3q1k2/6p1/7p/4p2P/6P1/5P2/6K1 b - - 3 37",
    "1Q4R1/5k2/4rpp1/3K4/8/7p/8/8 b - - 2 9",
    "1R6/8/4r3/6P1/1pk1b2P/8/3K4/8 b - - 0 11",
}};

// bench [depth] [repeats] [threads]
//   depth   : fixed search depth per position (default 13)
//   repeats : re-run the whole suite N times for a best-of-N NPS reading; the
//             deterministic fingerprint / EBF / concentration come from run 1
//             (default 1 -> per-position detail is printed)
//   threads : search workers (default 1; the fingerprint is only deterministic
//             single-threaded, as Lazy SMP helpers add non-deterministic work)
void run_bench(int depth, int repeats, int threads) {
    std::atomic_bool stop{false};
    TranspositionTable tt(16);
    threads = std::max(1, threads);
    repeats = std::max(1, repeats);

    SearchThreadPool search_pool(tt, stop);
    threads = search_pool.ensure_threads(threads);

    // Per-position detail is only printed for a single run; multi-run mode
    // (best-of-N NPS) prints one compact line per repeat instead.
    const bool detailed = (repeats == 1);

    // Fingerprint / EBF / concentration are deterministic across repeats, so
    // they are captured from run 1; one NPS sample is kept per run.
    int64_t fingerprint_nodes = 0;
    int64_t total_ms_first    = 0;
    double  geomean_ebf       = 0.0;
    int64_t median_nodes      = 0;
    int64_t max_nodes         = 0;
    std::vector<int64_t> nps_samples;
    nps_samples.reserve(static_cast<size_t>(repeats));

    uci_write_line("");
    for (int repeat = 0; repeat < repeats; ++repeat) {
        // Clean, identical starting state each repeat (TT + histories) so every
        // run is the same deterministic workload — the only NPS variation is
        // machine noise — and the fingerprint is independent of prior state.
        tt.clear();
        search_pool.clear();

        int64_t total_nodes = 0;
        int64_t total_ms    = 0;
        std::vector<int64_t> per_position_nodes;
        per_position_nodes.reserve(BENCH_FENS.size());
        double ln_ebf_sum = 0.0;
        int    ebf_count  = 0;

        for (size_t i = 0; i < BENCH_FENS.size(); ++i) {
            Board board;
            board.set_fen(std::string(BENCH_FENS[i]));

            SearchLimits limits;
            limits.depth = depth;

            stop.store(false, std::memory_order_release);
            SearchResult r = search_pool.search(board, limits, threads);

            total_nodes += r.nodes;
            total_ms    += r.elapsed_ms;
            per_position_nodes.push_back(r.nodes);

            // Per-position effective branching factor: nodes^(1/depth). Skip
            // positions solved before depth 1 (mates / trivial draws) so they
            // don't distort the geometric mean.
            double ebf = 0.0;
            if (r.depth >= 1 && r.nodes >= 1) {
                ebf = std::pow(static_cast<double>(r.nodes), 1.0 / r.depth);
                ln_ebf_sum += std::log(ebf);
                ++ebf_count;
            }

            if (detailed) {
                int64_t nps = r.elapsed_ms > 0
                    ? r.nodes * 1000 / r.elapsed_ms : r.nodes;
                std::ostringstream line;
                line << "bench " << (i + 1) << "/" << BENCH_FENS.size()
                     << "  depth " << r.depth
                     << "  score " << r.score
                     << "  nodes " << r.nodes
                     << "  ebf "   << std::fixed << std::setprecision(2) << ebf
                     << "  time "  << r.elapsed_ms << "ms"
                     << "  nps "   << nps;
                uci_write_line(line.str());
            }
        }

        int64_t run_nps = total_ms > 0
            ? total_nodes * 1000 / total_ms : total_nodes;
        nps_samples.push_back(run_nps);

        if (repeat == 0) {
            fingerprint_nodes = total_nodes;
            total_ms_first    = total_ms;
            geomean_ebf = ebf_count > 0
                ? std::exp(ln_ebf_sum / ebf_count) : 0.0;
            std::vector<int64_t> sorted = per_position_nodes;
            std::sort(sorted.begin(), sorted.end());
            median_nodes = sorted.empty() ? 0 : sorted[sorted.size() / 2];
            max_nodes = per_position_nodes.empty()
                ? 0 : *std::max_element(per_position_nodes.begin(), per_position_nodes.end());
        }
        if (!detailed) {
            std::ostringstream line;
            line << "run " << (repeat + 1) << "/" << repeats
                 << "  nodes " << total_nodes
                 << "  time "  << total_ms << "ms"
                 << "  nps "   << run_nps;
            uci_write_line(line.str());
        }
    }

    // Robust diagnostics so the node total is read as a fingerprint, not a
    // strength/speed proxy (it is hypersensitive and non-monotonic to tiny
    // threshold changes). Geomean EBF is the selectivity trend; median +
    // top-share expose how concentrated the total is.
    double top_share = fingerprint_nodes > 0
        ? static_cast<double>(max_nodes) * 100.0 / static_cast<double>(fingerprint_nodes) : 0.0;
    std::sort(nps_samples.begin(), nps_samples.end());
    int64_t best_nps   = nps_samples.empty() ? 0 : nps_samples.back();
    int64_t min_nps    = nps_samples.empty() ? 0 : nps_samples.front();
    int64_t median_nps = nps_samples.empty() ? 0 : nps_samples[nps_samples.size() / 2];

    std::ostringstream summary;
    summary << "\n=========================\n"
            << "Nodes searched  : " << fingerprint_nodes << '\n'
            << "Geomean EBF     : " << std::fixed << std::setprecision(3) << geomean_ebf << '\n'
            << "Median nodes    : " << median_nodes << '\n'
            << "Top-pos share   : " << std::fixed << std::setprecision(1) << top_share
            << "%  (" << max_nodes << " nodes)\n";
    // Keep a line beginning "Nodes/second" for the single-run case — the PGO
    // training harness waits for it as the completion marker.
    if (repeats == 1) {
        summary << "Total time (ms) : " << total_ms_first << '\n'
                << "Nodes/second    : " << best_nps << '\n';
    } else {
        summary << "Nodes/second    : " << best_nps
                << "   (best of " << repeats
                << "; median " << median_nps << ", min " << min_nps << ")\n";
    }
    uci_write(summary.str());
}
