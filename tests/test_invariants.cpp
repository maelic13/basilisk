/// Invariant, property, differential and fuzz tests (Phase 8.8).
///
/// This is the class of testing that would have caught the 8.1/8.2 bugs while
/// 253/253 curated board tests still passed: it exercises millions of states
/// rather than a fixed handful.
///
///  1. Random-walk make/unmake with `Board::assert_ok()` after every move and
///     a full-unwind check that the root is bit-for-bit restored.
///  2. Differential perft: the legal generator (`gen_legal`) vs the
///     pseudo-legal generator filtered by `is_legal` must agree, and both must
///     match the published node counts.
///  3. SEE fuzz: reference-free threshold invariants of `see_ge()` — strict
///     monotonicity in the threshold plus material bounds — over hundreds of
///     thousands of moves (curated ground-truth SEE cases live in test_board).
///  4. FEN round-trip fuzz: get_fen -> try_set_fen reproduces the position.
///  5. Parser robustness fuzz: malformed FEN never crashes and leaves the
///     board unchanged.
///
/// All randomness is a fixed-seed mt19937_64, so a failure is reproducible.
///
/// Build:
///   cmake --build --preset release --target test_invariants
///   ./build/release/test_invariants

#include "Board.h"
#include "attacks.h"
#include "bitboard.h"
#include "move.h"
#include "zobrist.h"
#include "test_harness.h"

#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

// Optional seed offset (env BASILISK_FUZZ_SEED) so nightly/extended runs can
// explore different random states while per-commit runs stay reproducible at
// offset 0. Every RNG below adds this to its fixed base seed.
static uint64_t fuzz_seed_offset() {
    const char* s = std::getenv("BASILISK_FUZZ_SEED");
    return s ? std::strtoull(s, nullptr, 10) : 0ULL;
}

// ---------------------------------------------------------------------------
// Shared helpers
// ---------------------------------------------------------------------------

static const char* const SEED_FENS[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",           // startpos
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", // kiwipete
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",                          // ep/pins
    "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1",   // tactical
    "8/8/8/8/8/8/6k1/4K2R w K - 0 1",                                     // castling
    "4k3/8/8/8/8/8/8/4K3 w - - 0 1",                                      // bare kings
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",          // promotions
};

static bool boards_bit_equal(const Board& a, const Board& b) {
    for (int c = 0; c < NCOLORS; ++c) {
        if (a.occupancy[c] != b.occupancy[c]) return false;
        for (int pt = 0; pt < PIECE_TYPE_NB; ++pt)
            if (a.pieces[c][pt] != b.pieces[c][pt]) return false;
    }
    for (int s = 0; s < SQUARE_NB; ++s)
        if (a.board_sq[s] != b.board_sq[s]) return false;
    return a.all_occ == b.all_occ
        && a.side_to_move == b.side_to_move
        && a.ep_sq == b.ep_sq
        && a.castling_rights == b.castling_rights
        && a.halfmove_clock == b.halfmove_clock
        && a.hash == b.hash
        && a.pawn_key == b.pawn_key
        && a.minor_key == b.minor_key
        && a.nonpawn_key[WHITE] == b.nonpawn_key[WHITE]
        && a.nonpawn_key[BLACK] == b.nonpawn_key[BLACK]
        && a.king_sq[WHITE] == b.king_sq[WHITE]
        && a.king_sq[BLACK] == b.king_sq[BLACK]
        && a.checkers == b.checkers;
}

// ---------------------------------------------------------------------------
// 1. Random-walk make/unmake + assert_ok + full unwind
// ---------------------------------------------------------------------------

// Recursively walk to `depth` plies, choosing random legal moves (and an
// occasional null move when not in check). After each make the board must be
// internally consistent; after each unmake it must be bit-identical to the
// pre-move snapshot. `bad` accumulates failure descriptions.
static void random_walk(Board& b, std::mt19937_64& rng, int depth,
                        int& states, const char*& bad) {
    if (bad || depth == 0) return;

    if (!b.assert_ok()) { bad = "assert_ok failed mid-walk"; return; }

    MoveList legal;
    b.gen_legal(legal);
    if (legal.empty()) return;  // mate/stalemate leaf

    // Occasional null move (never while in check — that is illegal).
    if (!b.checkers && (rng() & 7) == 0) {
        Board snap = b;
        b.make_null_move();
        ++states;
        if (!b.assert_ok()) { bad = "assert_ok failed after null move"; return; }
        random_walk(b, rng, depth - 1, states, bad);
        b.unmake_null_move();
        if (!boards_bit_equal(b, snap)) { bad = "null-move unwind mismatch"; return; }
        if (bad) return;
    }

    // Try a couple of random legal moves at this node so the walk fans out.
    const int tries = 2;
    for (int t = 0; t < tries && !bad; ++t) {
        const Move m = legal[static_cast<int>(rng() % legal.size())];
        Board snap = b;
        b.make_move(m);
        ++states;
        if (!b.assert_ok()) {
            static char buf[96];
            std::snprintf(buf, sizeof buf, "assert_ok failed after %s",
                          move_to_uci(m).c_str());
            bad = buf;
            return;
        }
        random_walk(b, rng, depth - 1, states, bad);
        b.unmake_move(m);
        if (!boards_bit_equal(b, snap)) {
            static char buf[96];
            std::snprintf(buf, sizeof buf, "unwind mismatch after %s",
                          move_to_uci(m).c_str());
            bad = buf;
            return;
        }
    }
}

static void test_random_walk_make_unmake() {
    std::mt19937_64 rng(0xB0A12026ULL + fuzz_seed_offset());
    int total_states = 0;
    const char* bad = nullptr;

    // Many independent walks per seed so coverage reaches into the millions of
    // states (each walk fans out ~2^depth), exercising deep make/unmake stacks.
    const int walks_per_seed = 400;
    for (const char* fen : SEED_FENS) {
        for (int w = 0; w < walks_per_seed && !bad; ++w) {
            Board b;
            b.set_fen(fen);
            random_walk(b, rng, 8, total_states, bad);
        }
        if (bad) break;
    }

    std::printf("  walked %d states across %zu seeds\n",
                total_states, sizeof(SEED_FENS) / sizeof(SEED_FENS[0]));
    begin_section("random-walk make/unmake keeps the board consistent");
    if (bad) std::fprintf(stderr, "  first failure: %s\n", bad);
    EXPECT(bad == nullptr);
    EXPECT(total_states > 100000);  // the walk actually did substantial work
    end_section();
}

// ---------------------------------------------------------------------------
// 2. Differential perft (two independent generators) + published counts
// ---------------------------------------------------------------------------

// Perft via the legal generator.
static uint64_t perft_legal(Board& b, int depth) {
    if (depth == 0) return 1;
    MoveList ml;
    b.gen_legal(ml);
    if (depth == 1) return static_cast<uint64_t>(ml.size());
    uint64_t nodes = 0;
    for (int i = 0; i < ml.size(); ++i) {
        b.make_move(ml[i]);
        nodes += perft_legal(b, depth - 1);
        b.unmake_move(ml[i]);
    }
    return nodes;
}

// Perft via pseudo-legal generation filtered by is_legal — an independent
// path. Divergence localizes a generator/legality bug.
static uint64_t perft_pseudo(Board& b, int depth) {
    if (depth == 0) return 1;
    std::vector<Move> pseudo;
    b.gen_pseudo_legal(pseudo);
    uint64_t nodes = 0;
    for (Move m : pseudo) {
        if (!b.is_legal(m)) continue;
        if (depth == 1) { ++nodes; continue; }
        b.make_move(m);
        nodes += perft_pseudo(b, depth - 1);
        b.unmake_move(m);
    }
    return nodes;
}

static void test_differential_perft() {
    struct Case { const char* fen; int depth; uint64_t nodes; };
    static const Case CASES[] = {
        { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609 },
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603 },
        { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5, 674624 },
        { "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 4, 422333 },
        { "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, 2103487 },
    };
    for (const Case& c : CASES) {
        Board b;
        b.set_fen(c.fen);
        const uint64_t legal  = perft_legal(b, c.depth);
        Board b2;
        b2.set_fen(c.fen);
        const uint64_t pseudo = perft_pseudo(b2, c.depth);
        char label[96];
        std::snprintf(label, sizeof label, "perft(%d)=%llu, legal==pseudo==published",
                      c.depth, static_cast<unsigned long long>(c.nodes));
        begin_section(label);
        EXPECT_EQ(legal, c.nodes);
        EXPECT_EQ(pseudo, c.nodes);
        end_section();
    }
}

// ---------------------------------------------------------------------------
// 3. SEE fuzz — see_ge() threshold invariants
//
// A hand-rolled make/unmake "oracle" is NOT a reliable SEE reference: a correct
// one must model promotion and en-passant *recaptures* mid-exchange and the
// exact LVA-with-stand-pat minimax, which is as hard as SEE itself. (The
// curated, hand-verified oracle cases live in test_board.cpp, where each value
// is checked by chess reasoning.) Here we fuzz the properties that must hold
// for ANY correct threshold SEE, needing no external reference:
//   * Monotonicity: see_ge(m, t) is non-increasing in t, so a single cut point
//     V exists with see_ge true for t <= V and false for t > V.
//   * Material bounds: no capture beats winning every enemy piece, and every
//     capture beats losing more than the whole board.
// This is what actually guards the pruning-critical function; a violation is a
// real, reference-free bug.
// ---------------------------------------------------------------------------

static constexpr int SEE_PIECE[PIECE_TYPE_NB] = {0, 100, 300, 300, 500, 900, 20000};

static void see_fuzz_walk(Board& b, std::mt19937_64& rng, int depth,
                          int& checked, const char*& bad) {
    if (bad || depth == 0) return;
    MoveList legal;
    b.gen_legal(legal);
    for (int i = 0; i < legal.size() && !bad; ++i) {
        const Move m = legal[i];
        // Monotonicity across a sweep straddling any plausible SEE value.
        bool prev = true;  // see_ge(m, -inf) must be true
        for (int t = -2100; t <= 2100 && !bad; t += 100) {
            const bool ge = b.see_ge(m, t);
            if (ge && !prev) { bad = "see_ge not monotonic in threshold"; break; }
            prev = ge;
        }
        // Material bounds.
        if (!b.see_ge(m, -30000)) bad = "see_ge false at -30000 (below any exchange)";
        if (b.see_ge(m, 30000))   bad = "see_ge true at +30000 (above any exchange)";
        // A capture of an undefended piece must clear (captured - own) — the
        // attacker can at worst be recaptured, so value >= captured - attacker.
        if (move_type(m) == NORMAL && b.board_sq[to_sq(m)] != NO_PIECE) {
            const int captured = SEE_PIECE[type_of(b.board_sq[to_sq(m)])];
            const int attacker = SEE_PIECE[type_of(b.board_sq[from_sq(m)])];
            if (!b.see_ge(m, captured - attacker))
                bad = "see_ge below the guaranteed capture floor";
        }
        ++checked;
    }
    if (bad || legal.empty()) return;
    const Move m = legal[static_cast<int>(rng() % legal.size())];
    b.make_move(m);
    see_fuzz_walk(b, rng, depth - 1, checked, bad);
    b.unmake_move(m);
}

static void test_see_ge_invariants() {
    std::mt19937_64 rng(0x5EE0F0FDULL + fuzz_seed_offset());
    int checked = 0;
    const char* bad = nullptr;
    for (const char* fen : SEED_FENS) {
        for (int w = 0; w < 200 && !bad; ++w) {
            Board b;
            b.set_fen(fen);
            see_fuzz_walk(b, rng, 6, checked, bad);
        }
        if (bad) break;
    }
    std::printf("  checked %d moves for see_ge threshold invariants\n", checked);
    begin_section("see_ge() is monotone and materially bounded on every move");
    if (bad) std::fprintf(stderr, "  failure: %s\n", bad);
    EXPECT(bad == nullptr);
    EXPECT(checked > 5000);
    end_section();
}

// ---------------------------------------------------------------------------
// 4. FEN round-trip fuzz
// ---------------------------------------------------------------------------

static void fen_walk(Board& b, std::mt19937_64& rng, int depth,
                     int& checked, const char*& bad) {
    if (bad || depth == 0) return;
    const std::string fen = b.get_fen();
    Board reparsed;
    std::string err;
    if (!reparsed.try_set_fen(fen, &err)) { bad = "own FEN rejected on reparse"; return; }
    if (!boards_bit_equal(b, reparsed)) { bad = "FEN round-trip changed the board"; return; }
    ++checked;

    MoveList legal;
    b.gen_legal(legal);
    if (legal.empty()) return;
    const Move m = legal[static_cast<int>(rng() % legal.size())];
    b.make_move(m);
    fen_walk(b, rng, depth - 1, checked, bad);
    b.unmake_move(m);
}

static void test_fen_roundtrip_fuzz() {
    std::mt19937_64 rng(0x0FE9C0DEULL + fuzz_seed_offset());
    int checked = 0;
    const char* bad = nullptr;
    for (const char* fen : SEED_FENS) {
        Board b;
        b.set_fen(fen);
        fen_walk(b, rng, 8, checked, bad);
        if (bad) break;
    }
    std::printf("  round-tripped %d positions\n", checked);
    begin_section("get_fen -> try_set_fen reproduces every walked position");
    if (bad) std::fprintf(stderr, "  failure: %s\n", bad);
    EXPECT(bad == nullptr);
    EXPECT(checked > 40);
    end_section();
}

// ---------------------------------------------------------------------------
// 5. Parser robustness fuzz — malformed FEN must never crash or corrupt
// ---------------------------------------------------------------------------

static void test_parser_robustness_fuzz() {
    const std::string good = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    const char alphabet[] = "0123456789pnbrqkPNBRQK/wb KQkq-abcdefgh ";
    std::mt19937_64 rng(0x0BADF00DULL + fuzz_seed_offset());

    Board b;
    b.set_fen(good);
    const std::string baseline = b.get_fen();

    int fed = 0, accepted = 0;
    const char* bad = nullptr;
    for (int i = 0; i < 20000 && !bad; ++i) {
        // Random junk, plus mutations of a valid FEN.
        std::string s;
        if (i & 1) {
            const int len = static_cast<int>(rng() % 40);
            for (int j = 0; j < len; ++j)
                s += alphabet[rng() % (sizeof(alphabet) - 1)];
        } else {
            s = good;
            const int muts = 1 + static_cast<int>(rng() % 4);
            for (int m = 0; m < muts && !s.empty(); ++m)
                s[rng() % s.size()] = alphabet[rng() % (sizeof(alphabet) - 1)];
        }
        std::string err;
        // Strict validation: an accepted FEN must be a *legal* position, so
        // assert_ok (which checks legality invariants) must then hold. Without
        // strict mode, try_set_fen intentionally accepts parseable-but-illegal
        // positions, which assert_ok would rightly reject.
        const bool ok = b.try_set_fen(s, &err, /*validate_legal_position=*/true);
        ++fed;
        if (ok) {
            ++accepted;
            // Anything accepted must itself be internally consistent.
            if (!b.assert_ok()) { bad = "accepted a malformed FEN into a bad state"; break; }
            b.set_fen(good);  // reset for the next mutation baseline
        } else {
            // A rejected FEN must leave the previous board untouched.
            if (b.get_fen() != baseline && b.get_fen() != good) {
                bad = "rejected FEN mutated the board";
                break;
            }
        }
    }
    std::printf("  fed %d malformed inputs, %d parsed to a valid board\n", fed, accepted);
    begin_section("malformed FEN never crashes and never yields a bad board");
    if (bad) std::fprintf(stderr, "  failure: %s\n", bad);
    EXPECT(bad == nullptr);
    EXPECT(fed == 20000);
    end_section();
}

int main() {
    init_bitboards();
    init_attacks();
    Zobrist::init();

    std::printf("Invariant / property / differential / fuzz tests\n");
    std::printf("================================================\n");

    std::printf("\nRandom-walk make/unmake + assert_ok\n");
    test_random_walk_make_unmake();

    std::printf("\nDifferential perft (legal vs pseudo-legal)\n");
    test_differential_perft();

    std::printf("\nSEE fuzz vs legal-exchange oracle\n");
    test_see_ge_invariants();

    std::printf("\nFEN round-trip fuzz\n");
    test_fen_roundtrip_fuzz();

    std::printf("\nParser robustness fuzz\n");
    test_parser_robustness_fuzz();

    return harness_summary();
}
