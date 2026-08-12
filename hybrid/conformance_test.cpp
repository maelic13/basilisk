/*
  Basilisk/Stockfish HCE oracle — adapter conformance test (PLAN 5.1).

  The oracle is only worth running if the evaluation it feeds to Stockfish's
  search is exactly the evaluation released Basilisk computes. The bridge
  rebuilds a Board from twelve bitboards and deliberately leaves ep_sq,
  checkers, hash, minor_key, nonpawn_key, ply and the move history untouched,
  on the claim that no evaluation term reads them.

  That claim is checked here rather than trusted. For each position the test
  compares:

    fast path  — the bridge's bitboard reconstruction, i.e. what the oracle
                 actually evaluates during a game;
    reference  — the same position parsed by Basilisk's own try_set_fen(),
                 producing a fully derived, fully consistent board.

  Any disagreement means the oracle measures something that is not Basilisk's
  evaluator, and the experiment must not run.

  Positions come from a random walk over legal moves, which reaches checks,
  promotions, en-passant states, castling-rights loss and lopsided endgames
  far more thoroughly than a curated list. En passant matters most: it is the
  one piece of state the bridge intentionally drops.

  This file is distributed under the GNU General Public License version 3
  or (at your option) any later version.
*/

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

#include "basilisk_bridge.h"

#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "move.h"
#include "types.h"

namespace {

struct Mismatch {
    std::string fen;
    std::int32_t fast;
    std::int32_t reference;
};

// Extracts the bridge's twelve-bitboard input from a fully derived board, so
// the fast path receives exactly what HybridEval::evaluate() would send it
// from a Stockfish Position.
void pack(const Board& b, std::uint64_t pieces[12]) {
    for (int c = 0; c < NCOLORS; ++c)
        for (int pt = PAWN; pt <= KING; ++pt)
            pieces[c * 6 + pt - 1] = b.pieces[c][pt];
}

bool has_ep(const Board& b) { return b.ep_sq != SQ_NONE; }

} // namespace

int main(int argc, char** argv) {
    const int walks = argc > 1 ? std::atoi(argv[1]) : 4000;
    const int plies = argc > 2 ? std::atoi(argv[2]) : 120;

    std::string error;
    if (!BasiliskBridge::initialize(error)) {
        std::printf("bridge init failed: %s\n", error.c_str());
        return 1;
    }
    init_bitboards();
    init_attacks();

    std::mt19937_64 rng(0x5115C0DEULL); // fixed seed: reproducible failures

    std::vector<Mismatch> mismatches;
    long long checked = 0, ep_checked = 0, check_positions = 0;

    for (int w = 0; w < walks; ++w) {
        Board b;
        b.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

        for (int ply = 0; ply < plies; ++ply) {
            MoveList ml;
            b.gen_legal(ml);
            if (ml.size() == 0)
                break;

            b.make_move(ml[rng() % ml.size()]);

            // Kings can never leave the board, but a malformed walk would be
            // a test bug rather than an adapter bug — skip rather than report.
            if (!b.pieces[WHITE][KING] || !b.pieces[BLACK][KING])
                break;

            const std::string fen = b.get_fen();

            std::uint64_t pieces[12];
            pack(b, pieces);

            const auto fast = BasiliskBridge::evaluate(
                pieces,
                static_cast<std::uint8_t>(b.side_to_move),
                static_cast<std::uint8_t>(b.castling_rights),
                static_cast<std::uint8_t>(b.halfmove_clock > 100 ? 100 : b.halfmove_clock));

            const auto reference = BasiliskBridge::evaluate_fen(fen);

            ++checked;
            if (has_ep(b))       ++ep_checked;
            if (b.is_in_check()) ++check_positions;

            if (fast != reference && mismatches.size() < 20)
                mismatches.push_back({fen, fast, reference});
        }
    }

    std::printf("positions compared : %lld\n", checked);
    std::printf("  with en passant  : %lld\n", ep_checked);
    std::printf("  side in check    : %lld\n", check_positions);
    std::printf("mismatches         : %zu\n", mismatches.size());

    if (!mismatches.empty()) {
        std::printf("\nFIRST MISMATCHES (fast vs reference):\n");
        for (const auto& m : mismatches)
            std::printf("  %+6d %+6d  %s\n", m.fast, m.reference, m.fen.c_str());
        std::printf("\nFAIL: the oracle does not compute Basilisk's evaluation.\n");
        return 1;
    }

    if (ep_checked == 0 || check_positions == 0) {
        std::printf("\nFAIL: the walk never reached en-passant or in-check positions;\n"
                    "      this run proves nothing about the state the bridge drops.\n");
        return 1;
    }

    std::printf("\nPASS: bitboard reconstruction is evaluation-identical to a parsed board.\n");
    return 0;
}
