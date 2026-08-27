// 5.9.1 deterministic evidence: every seeded-inert evaluation term must be shown
// to COMPUTE, because a zero weight makes bench blind to whether it runs at all.
// Build:
//   clang++ -std=c++23 -O2 -DNDEBUG -DUSE_PEXT -DUSE_POPCNT -mbmi2 -mpopcnt -Isrc \
//     tools/diag/eval_term_firing.cpp src/board.cpp src/bitboard.cpp src/move.cpp \
//     src/attacks.cpp src/zobrist.cpp src/eval.cpp -o eval_term_firing.exe

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <memory>
#include "board.h"
#include "eval.h"
#include "attacks.h"
#include "bitboard.h"
#include "zobrist.h"

// 5.9.1 firing check: each new term is seeded 0, so bench cannot tell whether it
// computes at all. Perturb one weight at a time and confirm the evaluation moves
// on at least one position. A term that never fires is invisible to the 5.9.4
// fit and would be silently dead.
int main() {
    init_bitboards(); init_attacks(); Zobrist::init();
    // Scan the real fitting corpus rather than hand-picked positions: a term
    // that fires rarely is still live, and a handful of positions cannot tell
    // "rare" from "dead". Format is "FEN;result" per line.
    std::vector<std::string> fens;
    {
        std::FILE* f = std::fopen("tools/texel/data/armC_basilisk25k_holdout.csv", "r");
        if (!f) { std::printf("corpus not found\n"); return 2; }
        char line[512];
        while (fens.size() < 20000 && std::fgets(line, sizeof(line), f)) {
            std::string l(line);
            auto semi = l.find(';');
            if (semi != std::string::npos) fens.push_back(l.substr(0, semi));
        }
        std::fclose(f);
        std::printf("scanning %zu corpus positions\n\n", fens.size());
    }

    struct T { const char* name; int EvalParams::* f; };
    std::vector<T> terms = {
        {"BadOutpost",        &EvalParams::bad_outpost_mg},
        {"BishopXrayPawn",    &EvalParams::bishop_xray_pawn_mg},
        {"LongDiagonalBishop",&EvalParams::long_diagonal_bishop_mg},
        {"KnightOnQueen",     &EvalParams::knight_on_queen_mg},
        {"SliderOnQueen",     &EvalParams::slider_on_queen_mg},
        {"TrappedRook",       &EvalParams::trapped_rook_mg},
        {"ThreatSafePawn",    &EvalParams::threat_safe_pawn_mg},
        {"BishopOutpost",     &EvalParams::bishop_outpost_mg},
        {"KingProtectorN",    &EvalParams::king_protector_n_mg},
        {"KingProtectorB",    &EvalParams::king_protector_b_mg},
    };
    int bad = 0;
    for (auto& t : terms) {
        init_eval_tables(g_eval_params);
        // Evaluator owns a 16,384-entry pawn cache (~900 KB); two of them on
        // the stack overflow Windows' 1 MB default.
        auto base = std::make_unique<Evaluator>();
        std::vector<int> before;
        for (auto& f : fens) { Board b; b.set_fen(f); before.push_back(base->evaluate(b)); }

        g_eval_params.*(t.f) = 97;              // distinctive probe value
        init_eval_tables(g_eval_params);
        auto probe = std::make_unique<Evaluator>();
        int moved = 0;
        for (size_t i = 0; i < fens.size(); ++i) {
            Board b; b.set_fen(fens[i]);
            if (probe->evaluate(b) != before[i]) ++moved;
        }
        g_eval_params.*(t.f) = 0;               // restore inert
        init_eval_tables(g_eval_params);

        std::printf("  %-20s fires on %d of %zu positions %s\n",
                    t.name, moved, fens.size(), moved ? "" : "  <-- DEAD");
        if (!moved) ++bad;
    }
    std::printf("\n%s\n", bad ? "FAIL: a term never fired" : "PASS: every new term computes");
    return bad ? 1 : 0;
}
