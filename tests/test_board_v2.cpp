/// Board contract corpus: the board checked against an oracle that shares no
/// code with it.
///
/// tests/data/board_v2_oracle.tsv is written by python-chess
/// (tools/diag/board_v2_oracle.py): canonical FEN, checkers, legal moves,
/// captures, checking moves, perft and divide for every profile position.
/// tests/data/see_v1.tsv is python-chess's legal same-square exchange tree
/// (tools/diag/see_oracle.py). This test compares the board with both, then
/// walks every position to its perft depth and checks, at every node:
///   - make leaves a consistent board (assert_ok recomputes every key);
///   - unmake, and a null move and its unmake, restore every field;
///   - gives_check agrees with playing the move;
///   - a position's FEN reads back to the same keys (one position, one key).
/// Clones, long unwinds, the staged generators' partition, the Zobrist key
/// set's rank, FEN robustness, the clock's absence from the position
/// identity and the history's capacity contract follow. Negative controls
/// show each comparison can fail.

#include "board.h"
#include "attacks.h"
#include "bitboard.h"
#include "constants.h"
#include "zobrist.h"
#include "test_harness.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::vector<std::string> split(const std::string& text, char sep) {
    std::vector<std::string> out;
    std::string field;
    std::istringstream in(text);
    while (std::getline(in, field, sep))
        out.push_back(field);
    if (!text.empty() && text.back() == sep)
        out.emplace_back();
    return out;
}

std::vector<std::string> words(const std::string& field) {
    std::vector<std::string> out;
    if (field == "-")
        return out;
    std::istringstream in(field);
    std::string w;
    while (in >> w)
        out.push_back(w);
    return out;
}

// Data lines of a '|'-separated file; '#' lines and CR line endings ignored.
std::vector<std::vector<std::string>> read_rows(const std::string& path, size_t fields) {
    std::vector<std::vector<std::string>> rows;
    std::ifstream in(path);
    if (!in) {
        std::fprintf(stderr, "cannot open %s\n", path.c_str());
        std::exit(2);
    }
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty() || line[0] == '#')
            continue;
        auto f = split(line, '|');
        if (f.size() != fields) {
            std::fprintf(stderr, "%s: expected %zu fields: %s\n", path.c_str(), fields, line.c_str());
            std::exit(2);
        }
        rows.push_back(std::move(f));
    }
    return rows;
}

struct OracleRow {
    std::string name, tags, input_fen, fen;
    int depth = 0;
    std::vector<std::string> checkers, legal, captures, checks;
    long long perft = 0;
    std::vector<std::pair<std::string, long long>> divide;
};

std::vector<OracleRow> load_oracle(const std::string& path) {
    std::vector<OracleRow> rows;
    for (const auto& f : read_rows(path, 11)) {
        OracleRow r;
        r.name = f[0];
        r.tags = f[1];
        r.input_fen = f[2];
        r.fen = f[3];
        r.depth = std::stoi(f[4]);
        r.checkers = words(f[5]);
        r.legal = words(f[6]);
        r.captures = words(f[7]);
        r.checks = words(f[8]);
        r.perft = std::stoll(f[9]);
        for (const auto& d : words(f[10])) {
            const auto colon = d.find(':');
            r.divide.emplace_back(d.substr(0, colon), std::stoll(d.substr(colon + 1)));
        }
        rows.push_back(std::move(r));
    }
    return rows;
}

std::string square_name(int sq) {
    return {char('a' + (sq & 7)), char('1' + (sq >> 3))};
}

std::vector<std::string> sorted_uci(const MoveList& ml) {
    std::vector<std::string> out;
    for (Move m : ml)
        out.push_back(move_to_uci(m));
    std::sort(out.begin(), out.end());
    return out;
}

bool is_capture(const Board& b, Move m) {
    return move_type(m) == EN_PASSANT || b.piece_on(to_sq(m)) != NO_PIECE;
}

// Same set, reporting the first difference when asked.
bool same_set(const char* what, const std::string& name,
              std::vector<std::string> got, std::vector<std::string> want, bool report = true) {
    std::sort(got.begin(), got.end());
    std::sort(want.begin(), want.end());
    if (got == want)
        return true;
    if (report) {
        std::vector<std::string> extra, missing;
        std::set_difference(got.begin(), got.end(), want.begin(), want.end(), std::back_inserter(extra));
        std::set_difference(want.begin(), want.end(), got.begin(), got.end(), std::back_inserter(missing));
        std::fprintf(stderr, "  %s %s: extra [", name.c_str(), what);
        for (const auto& s : extra) std::fprintf(stderr, " %s", s.c_str());
        std::fprintf(stderr, " ] missing [");
        for (const auto& s : missing) std::fprintf(stderr, " %s", s.c_str());
        std::fprintf(stderr, " ]\n");
    }
    return false;
}

// Every field of the board a move or null move may touch.
struct State {
    Bitboard pieces[NCOLORS][PIECE_TYPE_NB];
    Bitboard occ[NCOLORS];
    Bitboard all;
    Piece mailbox[SQUARE_NB];
    Key hash, pawn, minor, nonpawn[NCOLORS];
    Bitboard checkers;
    Square ep, kings[NCOLORS];
    int castling, rule50, from_null, fullmove, ply, turn;
    size_t history;

    bool operator==(const State& o) const {
        for (int c = 0; c < NCOLORS; ++c) {
            for (int pt = 0; pt < PIECE_TYPE_NB; ++pt)
                if (pieces[c][pt] != o.pieces[c][pt]) return false;
            if (occ[c] != o.occ[c] || nonpawn[c] != o.nonpawn[c] || kings[c] != o.kings[c])
                return false;
        }
        for (int s = 0; s < SQUARE_NB; ++s)
            if (mailbox[s] != o.mailbox[s]) return false;
        return all == o.all && hash == o.hash && pawn == o.pawn && minor == o.minor
            && checkers == o.checkers && ep == o.ep && castling == o.castling
            && rule50 == o.rule50 && from_null == o.from_null && fullmove == o.fullmove
            && ply == o.ply && turn == o.turn && history == o.history;
    }
};

State capture(const Board& b) {
    State s{};
    for (int c = 0; c < NCOLORS; ++c) {
        for (int pt = 0; pt < PIECE_TYPE_NB; ++pt)
            s.pieces[c][pt] = b.piece_bb(c, pt);
        s.occ[c] = b.occupancy_bb(c);
        s.nonpawn[c] = b.nonpawn_key_value(Color(c));
        s.kings[c] = b.king_square(c);
    }
    for (int sq = 0; sq < SQUARE_NB; ++sq)
        s.mailbox[sq] = b.piece_on(sq);
    s.all = b.all_pieces();
    s.hash = b.position_key();
    s.pawn = b.pawn_key_value();
    s.minor = b.minor_key_value();
    s.checkers = b.checking_pieces();
    s.ep = b.ep_square();
    s.castling = b.castling();
    s.rule50 = b.rule50_count();
    s.from_null = b.plies_since_null();
    s.fullmove = b.fullmove();
    s.ply = b.root_ply();
    s.turn = b.turn();
    s.history = b.history_size();
    return s;
}

// gives_check without its discovered-check clause: only the moved piece (or
// the castling rook) on its destination may check. The differential must
// catch it, which shows the walk reaches discovered checks.
bool direct_check_only(const Board& b, Move m) {
    const Square from = from_sq(m), to = to_sq(m);
    const Color us = b.turn();
    const Bitboard king = sq_bb(b.king_square(~us));
    if (move_type(m) == CASTLING) {
        const bool king_side = to > from;
        const Square rook_from = make_square(king_side ? FILE_H : FILE_A, rank_of(from));
        const Square rook_to = make_square(king_side ? FILE_F : FILE_D, rank_of(from));
        const Bitboard occ = b.all_pieces() ^ sq_bb(from) ^ sq_bb(to) ^ sq_bb(rook_from) ^ sq_bb(rook_to);
        return (rook_attacks(rook_to, occ) & king) != 0;
    }
    const PieceType pt = move_type(m) == PROMOTION ? promo_type(m) : type_of(b.piece_on(from));
    Bitboard occ = (b.all_pieces() ^ sq_bb(from)) | sq_bb(to);
    if (move_type(m) == EN_PASSANT)
        occ ^= sq_bb(make_square(file_of(to), rank_of(from)));
    switch (pt) {
        case PAWN:   return (PawnAttacks[us][to] & king) != 0;
        case KNIGHT: return (KnightAttacks[to] & king) != 0;
        case BISHOP: return (bishop_attacks(to, occ) & king) != 0;
        case ROOK:   return (rook_attacks(to, occ) & king) != 0;
        case QUEEN:  return (queen_attacks(to, occ) & king) != 0;
        default:     return false;
    }
}

struct WalkStats {
    long long nodes = 0;
    long long inconsistent = 0;       // assert_ok failed after make or null
    long long unmake_mismatch = 0;    // unmake did not restore every field
    long long null_mismatch = 0;      // unmake_null_move did not restore
    long long gives_check_mismatch = 0;
    long long mutant_mismatch = 0;    // the direct-only predicate disagreed
    long long fen_key_mismatch = 0;   // FEN round trip gave other keys
    long long fen_roundtrips = 0;
    long long checks_seen = 0;
};

bool fen_keys_match(const Board& b) {
    Board fresh;
    if (!fresh.try_set_fen(b.get_fen()))
        return false;
    return fresh.position_key() == b.position_key()
        && fresh.pawn_key_value() == b.pawn_key_value()
        && fresh.minor_key_value() == b.minor_key_value()
        && fresh.nonpawn_key_value(WHITE) == b.nonpawn_key_value(WHITE)
        && fresh.nonpawn_key_value(BLACK) == b.nonpawn_key_value(BLACK)
        && fresh.checking_pieces() == b.checking_pieces()
        && fresh.ep_square() == b.ep_square();
}

long long walk(Board& b, int depth, WalkStats& st) {
    if (depth == 0)
        return 1;
    ++st.nodes;
    // Interior nodes: the FEN round trip and the null move.
    ++st.fen_roundtrips;
    if (!fen_keys_match(b))
        ++st.fen_key_mismatch;
    if (!b.is_in_check()) {
        const State before = capture(b);
        b.make_null_move();
        if (!b.assert_ok()) ++st.inconsistent;
        b.unmake_null_move();
        if (!(capture(b) == before)) ++st.null_mismatch;
    }
    MoveList ml;
    b.gen_legal(ml);
    long long total = 0;
    for (Move m : ml) {
        const State before = capture(b);
        const bool predicted = b.gives_check(m);
        const bool direct = direct_check_only(b, m);
        b.make_move(m);
        if (!b.assert_ok()) ++st.inconsistent;
        const bool actual = b.is_in_check();
        st.checks_seen += actual;
        st.gives_check_mismatch += predicted != actual;
        st.mutant_mismatch += direct != actual;
        total += walk(b, depth - 1, st);
        b.unmake_move(m);
        if (!(capture(b) == before)) ++st.unmake_mismatch;
    }
    return total;
}

long long perft(Board& b, int depth) {
    if (depth == 0)
        return 1;
    MoveList ml;
    b.gen_legal(ml);
    if (depth == 1)
        return ml.size();
    long long n = 0;
    for (Move m : ml) {
        b.make_move(m);
        n += perft(b, depth - 1);
        b.unmake_move(m);
    }
    return n;
}

std::vector<std::pair<std::string, long long>> divide(Board& b, int depth) {
    std::vector<std::pair<std::string, long long>> out;
    MoveList ml;
    b.gen_legal(ml);
    for (Move m : ml) {
        b.make_move(m);
        out.emplace_back(move_to_uci(m), perft(b, depth - 1));
        b.unmake_move(m);
    }
    std::sort(out.begin(), out.end());
    return out;
}

// The root comparisons, as one predicate so a negative control can run it.
bool root_matches(Board& b, const OracleRow& r, bool report) {
    bool ok = true;
    if (b.get_fen() != r.fen) {
        if (report)
            std::fprintf(stderr, "  %s FEN: %s != %s\n", r.name.c_str(), b.get_fen().c_str(), r.fen.c_str());
        ok = false;
    }
    std::vector<std::string> checkers;
    for (Bitboard c = b.checking_pieces(); c; )
        checkers.push_back(square_name(pop_lsb(c)));
    ok &= same_set("checkers", r.name, checkers, r.checkers, report);

    MoveList legal;
    b.gen_legal(legal);
    ok &= same_set("legal", r.name, sorted_uci(legal), r.legal, report);
    std::vector<std::string> captures, checks;
    for (Move m : legal) {
        if (is_capture(b, m)) captures.push_back(move_to_uci(m));
        if (b.gives_check(m)) checks.push_back(move_to_uci(m));
    }
    ok &= same_set("captures", r.name, captures, r.captures, report);
    ok &= same_set("checks", r.name, checks, r.checks, report);

    const auto got = divide(b, r.depth);
    long long total = 0;
    for (const auto& d : got) total += d.second;
    if (got != r.divide || total != r.perft) {
        if (report)
            std::fprintf(stderr, "  %s perft %d: %lld != %lld\n", r.name.c_str(), r.depth, total, r.perft);
        ok = false;
    }
    return ok;
}

// The staged generators partition the legal moves: captures plus every
// promotion (the tactical set) and the rest, with quiet checks a subset.
bool staged_partition_holds(const Board& b) {
    MoveList legal, tactical, quiet, quiet_checks;
    b.gen_legal(legal);
    b.gen_legal_captures(tactical);
    b.gen_legal_quiets(quiet);
    b.gen_quiet_checks(quiet_checks);
    std::vector<std::string> want_tactical, want_quiet, want_quiet_checks;
    for (Move m : legal) {
        const bool tact = is_capture(b, m) || move_type(m) == PROMOTION;
        (tact ? want_tactical : want_quiet).push_back(move_to_uci(m));
        if (!tact && b.gives_check(m))
            want_quiet_checks.push_back(move_to_uci(m));
    }
    const std::string n = b.get_fen();
    return same_set("tactical", n, sorted_uci(tactical), want_tactical)
        && same_set("quiet", n, sorted_uci(quiet), want_quiet)
        && same_set("quiet checks", n, sorted_uci(quiet_checks), want_quiet_checks)
        && tactical.size() + quiet.size() == legal.size();
}

// Deterministic walk that records every state, then unwinds to the root.
bool long_unwind_restores(Board& b, uint64_t seed, int plies) {
    std::vector<State> states;
    std::vector<int> kinds;  // 0 move, 1 null
    std::vector<Move> moves;
    uint64_t x = seed;
    auto next = [&] { x ^= x << 13; x ^= x >> 7; x ^= x << 17; return x; };
    for (int i = 0; i < plies; ++i) {
        MoveList ml;
        b.gen_legal(ml);
        if (ml.empty())
            break;
        states.push_back(capture(b));
        if (i % 7 == 6 && !b.is_in_check()) {
            b.make_null_move();
            kinds.push_back(1);
            moves.push_back(MOVE_NONE);
        } else {
            const Move m = ml[int(next() % uint64_t(ml.size()))];
            b.make_move(m);
            kinds.push_back(0);
            moves.push_back(m);
        }
        if (!b.assert_ok())
            return false;
    }
    for (size_t i = moves.size(); i-- > 0; ) {
        if (kinds[i]) b.unmake_null_move();
        else          b.unmake_move(moves[i]);
        if (!(capture(b) == states[i]))
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------

void test_corpus(const std::vector<OracleRow>& rows) {
    WalkStats st;
    for (const auto& r : rows) {
        begin_section(r.name.c_str());
        Board b;
        // The input FEN reads as the canonical position, with the canonical
        // position's keys (a phantom en-passant token adds no key).
        EXPECT(b.try_set_fen(r.input_fen, true).has_value());
        Board canonical;
        EXPECT(canonical.try_set_fen(r.fen, true).has_value());
        EXPECT(b.position_key() == canonical.position_key());
        EXPECT(root_matches(b, r, true));
        EXPECT(staged_partition_holds(b));

        const State root = capture(b);
        EXPECT(walk(b, r.depth, st) == r.perft);
        EXPECT(capture(b) == root);

        // A clone searches as the original and leaves it untouched; a clone
        // taken mid-line carries the history its unwind needs.
        Board clone = b;
        EXPECT(perft(clone, r.depth) == r.perft);
        EXPECT(capture(b) == root);
        MoveList ml;
        b.gen_legal(ml);
        if (!ml.empty()) {
            b.make_move(ml[0]);
            Board mid = b;
            b.unmake_move(ml[0]);
            mid.unmake_move(ml[0]);
            EXPECT(capture(mid) == root);
        }
        EXPECT(long_unwind_restores(b, 0x9E3779B97F4A7C15ULL ^ uint64_t(r.perft), 160));
        EXPECT(capture(b) == root);
        end_section();
    }

    begin_section("walk totals: consistency, restoration, keys");
    std::printf("\n    %lld interior nodes, %lld checks, %lld FEN round trips\n  %-52s ",
                st.nodes, st.checks_seen, st.fen_roundtrips, "");
    EXPECT_EQ(st.inconsistent, 0);
    EXPECT_EQ(st.unmake_mismatch, 0);
    EXPECT_EQ(st.null_mismatch, 0);
    EXPECT_EQ(st.fen_key_mismatch, 0);
    end_section();

    begin_section("gives_check equals playing the move");
    EXPECT_EQ(st.gives_check_mismatch, 0);
    EXPECT(st.checks_seen > 1000);
    end_section();

    // Negative control: without the discovered-check clause the same walk
    // disagrees, so the corpus reaches discovered checks.
    begin_section("negative control: gives_check without discovered checks");
    std::printf("\n    direct-only predicate: %lld disagreements\n  %-52s ", st.mutant_mismatch, "");
    EXPECT(st.mutant_mismatch > 0);
    end_section();
}

void test_negative_controls(const std::vector<OracleRow>& rows) {
    const OracleRow* ep = nullptr;
    for (const auto& r : rows)
        if (r.name == "ep-legal") ep = &r;
    begin_section("negative controls: the comparisons can fail");
    EXPECT(ep != nullptr);
    if (ep) {
        Board b;
        b.set_fen(ep->input_fen);
        EXPECT(root_matches(b, *ep, false));

        OracleRow no_ep = *ep;  // the en-passant capture dropped
        std::erase_if(no_ep.legal, [](const std::string& u) { return u == "e5d6"; });
        EXPECT(no_ep.legal.size() + 1 == ep->legal.size());
        EXPECT(!root_matches(b, no_ep, false));

        OracleRow perft_off = *ep;
        perft_off.perft += 1;
        EXPECT(!root_matches(b, perft_off, false));

        OracleRow fen_off = *ep;  // the canonical FEN without its EP square
        fen_off.fen = "8/8/8/3pP3/8/8/8/4K2k w - - 0 1";
        EXPECT(!root_matches(b, fen_off, false));

        // A make that is never unmade is seen by the state comparison.
        const State before = capture(b);
        MoveList ml;
        b.gen_legal(ml);
        b.make_move(ml[0]);
        EXPECT(!(capture(b) == before));
        b.unmake_move(ml[0]);
        EXPECT(capture(b) == before);
        b.make_null_move();
        EXPECT(!(capture(b) == before));
        b.unmake_null_move();
        EXPECT(capture(b) == before);
    }
    end_section();
}

// Rank over GF(2) of a set of 64-bit keys.
int gf2_rank(const std::vector<Key>& keys) {
    Key basis[64] = {};
    int rank = 0;
    for (Key k : keys) {
        for (int bit = 63; bit >= 0 && k; --bit) {
            if (!((k >> bit) & 1)) continue;
            if (!basis[bit]) { basis[bit] = k; ++rank; k = 0; break; }
            k ^= basis[bit];
        }
    }
    return rank;
}

void test_zobrist_rank() {
    std::vector<Key> keys;
    for (int c = 0; c < NCOLORS; ++c)
        for (int pt = PAWN; pt <= KING; ++pt)
            for (int sq = 0; sq < SQUARE_NB; ++sq)
                keys.push_back(Zobrist::PieceKeys[c][pt][sq]);
    keys.push_back(Zobrist::SideKey);
    for (Key k : Zobrist::CastlingKeys) keys.push_back(k);
    for (Key k : Zobrist::EpKeys) keys.push_back(k);

    begin_section("Zobrist keys span all 64 bits over GF(2)");
    EXPECT_EQ(gf2_rank(keys), 64);
    std::vector<Key> sorted = keys;
    std::erase(sorted, Key(0));
    std::sort(sorted.begin(), sorted.end());
    EXPECT(std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end());
    end_section();

    // A 32-bit generator's keys (the defect that cost Triumviratus a game to a
    // false hash hit) read rank 32: the test sees it.
    begin_section("negative control: 32-bit keys read rank 32");
    std::vector<Key> narrow;
    for (Key k : keys) narrow.push_back(k & 0xFFFFFFFFULL);
    EXPECT_EQ(gf2_rank(narrow), 32);
    end_section();
}

void test_fen_robustness() {
    Board b;
    const std::string start = b.get_fen();
    auto rejected = [&](const char* fen) {
        const bool r = !b.try_set_fen(fen, true).has_value();
        return r && b.get_fen() == start;
    };
    auto key_of = [](const char* fen) {
        Board x;
        x.set_fen(fen);
        return x.position_key();
    };

    begin_section("EP square on the wrong side's rank is rejected");
    EXPECT(rejected("4k3/8/8/3pP3/8/8/8/4K3 b - d6 0 1"));
    EXPECT(rejected("4k3/8/8/8/3Pp3/8/8/4K3 w - d3 0 1"));
    EXPECT(rejected("4k3/8/8/4P3/8/8/8/4K3 w - d6 0 1"));   // no pushed pawn
    EXPECT(rejected("4k3/8/3p4/3pP3/8/8/8/4K3 w - d6 0 1")); // EP square occupied
    end_section();

    begin_section("castling rights the board cannot hold are dropped");
    struct Case { const char* fen; const char* canonical; const char* twin; };
    const Case cases[] = {
        {"4k3/8/8/8/8/8/8/R3K3 w KQkq - 0 1", "4k3/8/8/8/8/8/8/R3K3 w Q - 0 1",
         "4k3/8/8/8/8/8/8/R3K3 w Q - 0 1"},
        {"4k3/8/8/8/8/8/8/4K2R w KQkq - 0 1", "4k3/8/8/8/8/8/8/4K2R w K - 0 1",
         "4k3/8/8/8/8/8/8/4K2R w K - 0 1"},
        {"r3k3/8/8/8/8/8/8/4K3 b KQkq - 0 1", "r3k3/8/8/8/8/8/8/4K3 b q - 0 1",
         "r3k3/8/8/8/8/8/8/4K3 b q - 0 1"},
        {"4k2r/8/8/8/8/8/8/4K3 b KQkq - 0 1", "4k2r/8/8/8/8/8/8/4K3 b k - 0 1",
         "4k2r/8/8/8/8/8/8/4K3 b k - 0 1"},
        {"r3k2r/8/8/8/8/8/8/R2K3R w KQkq - 0 1", "r3k2r/8/8/8/8/8/8/R2K3R w kq - 0 1",
         "r3k2r/8/8/8/8/8/8/R2K3R w kq - 0 1"},
    };
    for (const auto& c : cases) {
        Board x;
        EXPECT(x.try_set_fen(c.fen, true).has_value());
        EXPECT_STR(x.get_fen(), c.canonical);
        EXPECT(x.position_key() == key_of(c.twin));
    }
    end_section();

    begin_section("side not to move in check is rejected");
    EXPECT(rejected("4k3/8/8/8/8/8/4R3/4K3 w - - 0 1"));
    EXPECT(rejected("4k3/4R3/8/8/8/8/8/4K3 w - - 0 1"));
    end_section();

    begin_section("one position, one key: EP tokens without a capture");
    // A double push with no legal capturer: the token adds no key.
    EXPECT(key_of("rnbqkbnr/ppp1pppp/8/3p4/8/8/PPPPPPPP/RNBQKBNR w KQkq d6 0 2")
           == key_of("rnbqkbnr/ppp1pppp/8/3p4/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 2"));
    // The same position reached by moves and read from FEN.
    Board played;
    played.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    for (const char* uci : {"e2e4", "d7d5"}) {
        MoveList ml;
        played.gen_legal(ml);
        for (Move m : ml)
            if (move_to_uci(m) == uci) { played.make_move(m); break; }
    }
    EXPECT(played.ep_square() == SQ_NONE);
    EXPECT(played.position_key()
           == key_of("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2"));
    EXPECT(played.position_key()
           == key_of("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2"));
    end_section();
}

void test_clock_outside_identity() {
    begin_section("the halfmove clock is not in the position key");
    Board a, b;
    a.set_fen("4k3/8/8/8/8/8/8/R3K3 w Q - 0 1");
    b.set_fen("4k3/8/8/8/8/8/8/R3K3 w Q - 87 40");
    EXPECT(a.position_key() == b.position_key());
    EXPECT(a.rule50_count() != b.rule50_count());
    // A repetition is found although the clock moved on.
    Board r;
    const Key start = r.position_key();
    for (const char* uci : {"g1f3", "g8f6", "f3g1", "f6g8"}) {
        MoveList ml;
        r.gen_legal(ml);
        for (Move m : ml)
            if (move_to_uci(m) == uci) { r.make_move(m); break; }
    }
    EXPECT(r.position_key() == start);
    EXPECT_EQ(r.rule50_count(), 4);
    EXPECT(r.is_draw());
    end_section();
}

void test_history_capacity() {
    // A game longer than the reserve: four knight moves repeated.
    Board game;
    const char* cycle[] = {"g1f3", "g8f6", "f3g1", "f6g8"};
    const int plies = int(Board::HISTORY_RESERVE) + 400;
    for (int i = 0; i < plies; ++i) {
        MoveList ml;
        game.gen_legal(ml);
        for (Move m : ml)
            if (move_to_uci(m) == cycle[i % 4]) { game.make_move(m); break; }
    }

    begin_section("game history of any length is kept");
    EXPECT_EQ(game.history_size(), size_t(plies));
    EXPECT(game.assert_ok());
    end_section();

    // Every copy a search starts from holds the game history plus a full
    // search line, so no node grows it.
    begin_section("copies reserve a search line past the game history");
    const Board copy = game;
    EXPECT(copy.history_capacity() >= copy.history_size() + Board::SEARCH_HISTORY_HEADROOM);
    Board assigned;
    assigned = game;
    EXPECT(assigned.history_capacity() >= assigned.history_size() + Board::SEARCH_HISTORY_HEADROOM);
    Board fresh;
    EXPECT(fresh.history_capacity() >= Board::HISTORY_RESERVE);
    EXPECT(Board::SEARCH_HISTORY_HEADROOM >= size_t(MAX_PLY));
    end_section();

    begin_section("a full search line on a copy does not reallocate");
    Board line = game;
    const size_t cap = line.history_capacity();
    for (int i = 0; i < MAX_PLY; ++i)
        line.make_null_move();
    EXPECT(line.history_capacity() == cap);
    for (int i = 0; i < MAX_PLY; ++i)
        line.unmake_null_move();
    EXPECT(line.history_size() == game.history_size());
    end_section();
}

struct SeeRow {
    std::string name, source, fen, uci;
    int tree = 0, lva = 0, gain = 0;
};

// see() keeps the gain-array prune (see its comment in board.cpp), so its
// value may differ from the exact exchange where an X-ray is revealed late;
// see_ge, which the search uses, is exact. The corpus's known cases, with the
// value see() returns: a queen stepping onto a pawn-attacked square whose
// origin opens a bishop's X-ray.
struct KnownSeeValue { const char* name; int see; };
constexpr KnownSeeValue KNOWN_SEE_PRUNE[] = {
    {"test_see_quiet_moves:3:f3f5", -800},
    {"test_see_quiet_moves:3:f3h5", -800},
};

void test_see_corpus(const std::string& path) {
    int compared = 0, disagree = 0, pruned = 0;
    begin_section("see_ge's boundary is the legal exchange tree's");
    for (const auto& f : read_rows(path, 8)) {
        SeeRow r{f[0], f[1], f[2], f[3], std::stoi(f[4]), std::stoi(f[5]), std::stoi(f[6])};
        Board b;
        if (!b.try_set_fen(r.fen, true)) {
            std::fprintf(stderr, "  %s: FEN refused\n", r.name.c_str());
            ++disagree;
            continue;
        }
        MoveList ml;
        b.gen_legal(ml);
        Move m = MOVE_NONE;
        for (Move x : ml)
            if (move_to_uci(x) == r.uci) m = x;
        if (m == MOVE_NONE) {
            std::fprintf(stderr, "  %s: %s not legal\n", r.name.c_str(), r.uci.c_str());
            ++disagree;
            continue;
        }
        if (move_type(m) == CASTLING)
            continue;  // outside the exchange model
        ++compared;
        int expected_see = r.tree;
        for (const auto& k : KNOWN_SEE_PRUNE)
            if (r.name == k.name) { expected_see = k.see; ++pruned; }
        const int v = b.see(m);
        const bool boundary = b.see_ge(m, r.tree) && !b.see_ge(m, r.tree + 1);
        if (v != expected_see || !boundary) {
            std::fprintf(stderr, "  %s [%s] %s %s: see %d see_ge boundary %s, tree %d\n",
                         r.name.c_str(), r.source.c_str(), r.fen.c_str(), r.uci.c_str(),
                         v, boundary ? "ok" : "off", r.tree);
            ++disagree;
        }
    }
    std::printf("\n    %d moves compared, %d see() prune cases\n  %-52s ", compared, pruned, "");
    EXPECT(compared > 150);
    EXPECT_EQ(pruned, int(std::size(KNOWN_SEE_PRUNE)));
    EXPECT_EQ(disagree, 0);
    end_section();
}

static_assert(sizeof(Move) == 2, "a move is 16 bits: the TT entry and killers assume it");
static_assert(MoveList::CAPACITY >= 218, "a move list holds the largest legal move count");

}  // namespace

int main(int argc, char** argv) {
    init_bitboards();
    init_attacks();
    Zobrist::init();
    const std::string data = argc > 1 ? argv[1] : "tests/data";

    std::printf("Board contract corpus\n%s\n", std::string(65, '=').c_str());
    const auto rows = load_oracle(data + "/board_v2_oracle.tsv");
    std::printf("\nCorpus (%zu positions, python-chess oracle)\n", rows.size());
    test_corpus(rows);
    std::printf("\nNegative controls\n");
    test_negative_controls(rows);
    std::printf("\nZobrist keys\n");
    test_zobrist_rank();
    std::printf("\nFEN robustness\n");
    test_fen_robustness();
    std::printf("\nPosition identity\n");
    test_clock_outside_identity();
    std::printf("\nHistory capacity\n");
    test_history_capacity();
    std::printf("\nSEE corpus\n");
    test_see_corpus(data + "/see_v1.tsv");
    return harness_summary();
}
