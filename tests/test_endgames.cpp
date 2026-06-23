/// Endgame regression suite (Step 3.5).
///
/// Loads tests/endgames.epd and gates the scale-factor framework and the
/// known-endgame functions (KPK bitbase, KBNK corner mop-up, KNNK / insufficient
/// draws, KBP wrong-bishop draw). For static verdicts it checks the sign and
/// magnitude of the static eval; for `mate_*` verdicts it plays the position out
/// with a short fixed-depth search and asserts checkmate is delivered from the
/// board within a move budget.
///
/// Build:
///   cmake --build --preset release --target test_endgames
///   ./build/release/test_endgames tests/endgames.epd

#include "Board.h"
#include "attacks.h"
#include "bitboard.h"
#include "eval.h"
#include "move.h"
#include "search.h"
#include "tt.h"
#include "zobrist.h"
#include "test_harness.h"

#include <atomic>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Tunables for the static-eval verdicts and the mate playout
// ---------------------------------------------------------------------------

static constexpr int DRAW_TOL    = 75;    // |cp| <= this counts as a draw
static constexpr int WIN_MIN     = 150;   // |cp| >= this counts as a clear win
static constexpr int MATE_DEPTH  = 18;    // fixed search depth per playout move
static constexpr int MATE_BUDGET = 100;   // max plies to deliver mate

// White-perspective static eval.
static int eval_white(const std::string& fen) {
    Board b;
    b.set_fen(fen);
    Evaluator ev;
    int s = ev.evaluate(b);
    return (b.side_to_move == WHITE) ? s : -s;
}

static Move best_move(Board& b, int depth) {
    TranspositionTable tt(8);
    std::atomic_bool stop{false};
    SearchLimits lim;
    lim.depth = depth;
    auto searcher = std::make_unique<Searcher>(tt, stop);
    SearchResult sr = searcher->search(b, lim);
    return sr.bestmove;
}

// Plays the position out, searching each move to `MATE_DEPTH`. Returns true if
// checkmate is delivered by `winner` within MATE_BUDGET plies.
static bool delivers_mate(const std::string& fen, Color winner) {
    Board b;
    b.set_fen(fen);
    for (int ply = 0; ply < MATE_BUDGET; ply++) {
        MoveList ml;
        b.gen_legal(ml);
        if (ml.empty()) {
            // Terminal: checkmate only if the side to move is in check, and the
            // mated side must be the loser (opponent of the winner).
            return b.is_in_check() && b.side_to_move == ~winner;
        }
        Move m = best_move(b, MATE_DEPTH);
        if (m == MOVE_NONE)
            return false;
        b.make_move(m);
    }
    return false;
}

// ---------------------------------------------------------------------------
// EPD loading
// ---------------------------------------------------------------------------

struct EpdEntry {
    std::string fen;
    std::string verdict;
};

static std::vector<EpdEntry> load_epd(const std::string& path) {
    std::vector<EpdEntry> out;
    std::ifstream in(path);
    if (!in) {
        std::fprintf(stderr, "FATAL: cannot open EPD file '%s'\n", path.c_str());
        return out;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        auto sep = line.find(';');
        if (sep == std::string::npos)
            continue;
        std::string fen = line.substr(0, sep);
        std::string verdict = line.substr(sep + 1);
        // trim
        auto trim = [](std::string& s) {
            size_t a = s.find_first_not_of(" \t\r\n");
            size_t b = s.find_last_not_of(" \t\r\n");
            if (a == std::string::npos) { s.clear(); return; }
            s = s.substr(a, b - a + 1);
        };
        trim(fen);
        trim(verdict);
        if (!fen.empty() && !verdict.empty())
            out.push_back({fen, verdict});
    }
    return out;
}

// ---------------------------------------------------------------------------
// Targeted KBNK orientation check: the strong side must prefer driving the
// bare king toward the bishop-coloured corner.
// ---------------------------------------------------------------------------

static void test_kbnk_corner_preference() {
    // Same dark-squared bishop (d2) and knight (f3) in both positions; only the
    // bare king moves. Its bishop-coloured corners are a1 / h8, so the defender
    // near a1 must score better for White than the mirror near the wrong corner.
    begin_section("KBNK drives toward the bishop-coloured corner");
    int right = eval_white("8/8/8/8/4K3/5N2/3B4/k7 b - - 0 1");  // black king a1 (dark, right)
    int wrong = eval_white("k7/8/8/8/4K3/5N2/3B4/8 b - - 0 1");  // black king a8 (light, wrong)
    EXPECT(right > wrong);
    end_section();
}

// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables();

    std::string epd_path = (argc > 1) ? argv[1] : "tests/endgames.epd";
    std::vector<EpdEntry> entries = load_epd(epd_path);
    if (entries.empty()) {
        std::fprintf(stderr, "FATAL: no EPD entries loaded from '%s'\n", epd_path.c_str());
        return 1;
    }

    std::printf("Endgame regression suite (%zu positions)\n", entries.size());

    for (const auto& e : entries) {
        std::string label = e.verdict + ": " + e.fen;
        begin_section(label.c_str());
        if (e.verdict == "draw") {
            int cp = eval_white(e.fen);
            EXPECT(std::abs(cp) <= DRAW_TOL);
        } else if (e.verdict == "win_w") {
            EXPECT(eval_white(e.fen) >= WIN_MIN);
        } else if (e.verdict == "win_b") {
            EXPECT(eval_white(e.fen) <= -WIN_MIN);
        } else if (e.verdict == "mate_w") {
            EXPECT(delivers_mate(e.fen, WHITE));
        } else if (e.verdict == "mate_b") {
            EXPECT(delivers_mate(e.fen, BLACK));
        } else {
            std::fprintf(stderr, "  FAIL: unknown verdict '%s'\n", e.verdict.c_str());
            EXPECT(false);
        }
        end_section();
    }

    test_kbnk_corner_preference();

    return harness_summary();
}
