// The evaluation's suite: parity with the
// classical reference, integer for integer, on every row of
// tests/fixtures/eval_reference.tsv that the reference's own specialised
// evaluations did not answer, and colour symmetry on the same rows.
//
// The fixture is the reference producer's output (tools/diag/c32_reference.py),
// one row per position: the FEN, its source, then the reference's
// intermediates in its own units and White's view. A field joins COMPARED
// once the part of the evaluation that computes it is built.

#include "attacks.h"
#include "bench.h"
#include "bitboard.h"
#include "board.h"
#include "eval.h"
#include "move.h"
#include "wac.h"
#include "zobrist.h"
#include "colour_mirror.h"
#include "test_harness.h"

#include <cstring>
#include <fstream>
#include <functional>
#include <memory>
#include <random>
#include <span>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#ifndef BASILISK_TEST_FIXTURE_DIR
#error "BASILISK_TEST_FIXTURE_DIR must name tests/fixtures"
#endif

namespace {

struct Row {
    std::string fen, source;
    std::unordered_map<std::string, int> value;
};

std::vector<Row> load_fixture() {
    const std::string path = std::string(BASILISK_TEST_FIXTURE_DIR) + "/eval_reference.tsv";
    std::ifstream in(path);
    if (!in) {
        std::fprintf(stderr, "cannot open %s\n", path.c_str());
        std::exit(2);
    }
    std::string line;
    // A checkout may end the fixture's lines with CR LF.
    auto get = [&](std::string& l) {
        if (!std::getline(in, l))
            return false;
        if (!l.empty() && l.back() == '\r')
            l.pop_back();
        return true;
    };
    get(line);
    std::vector<std::string> columns;
    {
        std::stringstream ss(line);
        for (std::string c; std::getline(ss, c, '\t');)
            columns.push_back(c);
    }
    std::vector<Row> rows;
    while (get(line)) {
        if (line.empty())
            continue;
        std::stringstream ss(line);
        Row row;
        std::string cell;
        for (size_t i = 0; std::getline(ss, cell, '\t'); i++) {
            if (i == 0)
                row.fen = cell;
            else if (i == 1)
                row.source = cell;
            else
                row.value[columns.at(i)] = std::stoi(cell);
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

// Where a field is compared, among the rows no specialised evaluation of the
// reference answered:
//   EVERY  on all of them;
//   WHOLE  where no scaling function is registered for the material: the
//          reference's imbalance is 0 there (Amendment 3), so nothing read
//          from its whole score (the imbalance itself, the lazy exit, the
//          adjustment, the strong side, the factors, the values) is compared;
//   STAGE1 where the reference passed its first lazy exit, STAGE2 its
//          second: the families it computed. The record holds every family
//          whichever exit the new arm takes.
//   SILENT on layer-silent rows: WHOLE rows where the reference's material
//          entry is normal and no scaling function answered, and Basilisk's
//          endgame layer neither overrode nor scaled (§9). A row the mate
//          drive fired on is compared too: its addition enters after the
//          value before conversion (Amendment 4).
enum Scope { EVERY, WHOLE, STAGE1, STAGE2, SILENT };

// A compared field: its fixture column, where the record holds it, whether
// it is a White-view score (negated in the colour mirror) or a colourless
// quantity, and where it is compared.
struct Field {
    const char* column;
    std::function<int(const EvalRebuildRecord&)> get;
    bool white_view;
    Scope scope;
};

bool layer_silent(const Row& row, const EvalRebuildRecord& rec) {
    return !row.value.at("specialised") && !row.value.at("registered") && row.value.at("entrysf") == 64
        && !row.value.at("scalefn") && rec.layer == 0;
}

bool in_scope(const Row& row, const EvalRebuildRecord& rec, Scope scope) {
    if (row.value.at("specialised"))
        return false;
    switch (scope) {
    case EVERY:  return true;
    case WHOLE:  return !row.value.at("registered");
    case STAGE1: return row.value.at("lazy") != 1;
    case STAGE2: return row.value.at("lazy") == 0;
    case SILENT: return layer_silent(row, rec);
    }
    return false;
}

const std::vector<Field>& compared() {
    static const std::vector<Field> fields = {
        {"psq_mg",       [](const EvalRebuildRecord& r) { return r.psq[0]; }, true, EVERY},
        {"psq_eg",       [](const EvalRebuildRecord& r) { return r.psq[1]; }, true, EVERY},
        {"imbalance_mg", [](const EvalRebuildRecord& r) { return r.imbalance[0]; }, true, WHOLE},
        {"imbalance_eg", [](const EvalRebuildRecord& r) { return r.imbalance[1]; }, true, WHOLE},
        {"ph",           [](const EvalRebuildRecord& r) { return r.ph; }, false, EVERY},
        {"pawns_mg",     [](const EvalRebuildRecord& r) { return r.family[EVAL_ABLATE_PAWNS][0]; }, true, EVERY},
        {"pawns_eg",     [](const EvalRebuildRecord& r) { return r.family[EVAL_ABLATE_PAWNS][1]; }, true, EVERY},
        {"knights_mg", [](const EvalRebuildRecord& r) { return r.piece_type[0][0]; }, true, STAGE1},
        {"knights_eg", [](const EvalRebuildRecord& r) { return r.piece_type[0][1]; }, true, STAGE1},
        {"bishops_mg", [](const EvalRebuildRecord& r) { return r.piece_type[1][0]; }, true, STAGE1},
        {"bishops_eg", [](const EvalRebuildRecord& r) { return r.piece_type[1][1]; }, true, STAGE1},
        {"rooks_mg", [](const EvalRebuildRecord& r) { return r.piece_type[2][0]; }, true, STAGE1},
        {"rooks_eg", [](const EvalRebuildRecord& r) { return r.piece_type[2][1]; }, true, STAGE1},
        {"queens_mg", [](const EvalRebuildRecord& r) { return r.piece_type[3][0]; }, true, STAGE1},
        {"queens_eg", [](const EvalRebuildRecord& r) { return r.piece_type[3][1]; }, true, STAGE1},
        {"pieces_mg", [](const EvalRebuildRecord& r) { return r.family[EVAL_ABLATE_PIECES][0]; }, true, STAGE1},
        {"pieces_eg", [](const EvalRebuildRecord& r) { return r.family[EVAL_ABLATE_PIECES][1]; }, true, STAGE1},
        {"mobility_mg", [](const EvalRebuildRecord& r) { return r.family[EVAL_ABLATE_MOBILITY][0]; }, true, STAGE1},
        {"mobility_eg", [](const EvalRebuildRecord& r) { return r.family[EVAL_ABLATE_MOBILITY][1]; }, true, STAGE1},
        {"king_mg", [](const EvalRebuildRecord& r) { return r.family[EVAL_ABLATE_KING][0]; }, true, STAGE1},
        {"king_eg", [](const EvalRebuildRecord& r) { return r.family[EVAL_ABLATE_KING][1]; }, true, STAGE1},
        {"passed_mg", [](const EvalRebuildRecord& r) { return r.family[EVAL_ABLATE_PASSERS][0]; }, true, STAGE1},
        {"passed_eg", [](const EvalRebuildRecord& r) { return r.family[EVAL_ABLATE_PASSERS][1]; }, true, STAGE1},
        {"threats_mg", [](const EvalRebuildRecord& r) { return r.family[EVAL_ABLATE_THREATS][0]; }, true, STAGE2},
        {"threats_eg", [](const EvalRebuildRecord& r) { return r.family[EVAL_ABLATE_THREATS][1]; }, true, STAGE2},
        {"space_mg", [](const EvalRebuildRecord& r) { return r.family[EVAL_ABLATE_SPACE][0]; }, true, STAGE2},
        {"space_eg", [](const EvalRebuildRecord& r) { return r.family[EVAL_ABLATE_SPACE][1]; }, true, STAGE2},
        {"lazy",          [](const EvalRebuildRecord& r) { return r.lazy; }, false, WHOLE},
        {"complexity_mg", [](const EvalRebuildRecord& r) { return r.complexity[0]; }, true, WHOLE},
        {"complexity_eg", [](const EvalRebuildRecord& r) { return r.complexity[1]; }, true, WHOLE},
        {"adjusted_mg",   [](const EvalRebuildRecord& r) { return r.adjusted[0]; }, true, WHOLE},
        {"adjusted_eg",   [](const EvalRebuildRecord& r) { return r.adjusted[1]; }, true, WHOLE},
        {"generalsf",     [](const EvalRebuildRecord& r) { return r.general_sf; }, false, WHOLE},
        {"sf",            [](const EvalRebuildRecord& r) { return r.sf; }, false, SILENT},
        {"v",             [](const EvalRebuildRecord& r) { return r.v; }, true, SILENT},
    };
    return fields;
}

Board board_of(const std::string& fen) {
    Board b;
    b.set_fen(fen);
    return b;
}

void test_parity(const std::vector<Row>& rows, Evaluator& ev) {
    begin_section("parity with the reference, field by field");
    std::vector<EvalRebuildRecord> records(rows.size());
    for (size_t i = 0; i < rows.size(); i++)
        ev.rebuild_record(board_of(rows[i].fen), records[i]);
    int checked = 0;
    for (const Field& f : compared()) {
        int differ = 0;
        for (size_t i = 0; i < rows.size(); i++) {
            const Row& row = rows[i];
            if (!in_scope(row, records[i], f.scope))
                continue;
            const int want = row.value.at(f.column), got = f.get(records[i]);
            if (got != want && ++differ <= 3)
                std::fprintf(stderr, "\n    %s: %d, reference %d: %s", f.column, got, want, row.fen.c_str());
        }
        if (differ)
            std::fprintf(stderr, "\n    %s differs on %d rows\n", f.column, differ);
        EXPECT_EQ(differ, 0);
        checked++;
    }
    end_section();
    std::printf("    %d fields on %zu rows\n", checked, rows.size());
}

void test_layer_silent(const std::vector<Row>& rows, Evaluator& ev) {
    // The layer-silent rows must be at least 90% of the fixture (§9).
    begin_section("layer-silent rows are at least 90% of the fixture");
    std::vector<EvalRebuildRecord> records(rows.size());
    for (size_t i = 0; i < rows.size(); i++)
        ev.rebuild_record(board_of(rows[i].fen), records[i]);
    size_t silent = 0;
    for (size_t i = 0; i < rows.size(); i++)
        silent += layer_silent(rows[i], records[i]);
    std::unordered_map<std::string, std::pair<size_t, size_t>> by_source;
    for (size_t i = 0; i < rows.size(); i++) {
        const std::string source = rows[i].source.substr(0, rows[i].source.find(':'));
        auto& [n, s] = by_source[source];
        n++;
        s += layer_silent(rows[i], records[i]);
    }
    for (const auto& [source, ns] : by_source)
        std::printf("\n    %-18s %5zu of %5zu silent", source.c_str(), ns.second, ns.first);
    // Why the game rows that are not silent are not, by the first reason.
    size_t why[6] = {};
    const char* const reasons[6] = {"specialised", "registered", "entry factor", "scaling function",
                                    "layer override", "layer scale"};
    for (size_t i = 0; i < rows.size(); i++) {
        const Row& row = rows[i];
        const EvalRebuildRecord& rec = records[i];
        if (row.source != "game" || layer_silent(row, rec))
            continue;
        const int r = row.value.at("specialised") ? 0 : row.value.at("registered") ? 1
                    : row.value.at("entrysf") != 64 ? 2 : row.value.at("scalefn") ? 3
                    : rec.layer == 2 ? 4 : 5;
        why[r]++;
    }
    for (int r = 0; r < 6; r++)
        std::printf("\n    game rows not silent by %-17s %5zu", reasons[r], why[r]);
    EXPECT(silent * 10 >= rows.size() * 9);
    end_section();
    std::printf("    %zu of %zu rows (%.1f%%)\n", silent, rows.size(), 100.0 * double(silent) / double(rows.size()));
}

// Whether the mirror's field is the original's, negated for a White-view
// score. Where the adjusted endgame half is 0 the strong side is Black for
// both colours, as in the reference, so the factors may differ; they scale
// nothing there, and the value is compared regardless.
bool mirrors(const Field& f, const EvalRebuildRecord& a, const EvalRebuildRecord& m) {
    const std::string column = f.column;
    if (a.adjusted[1] == 0 && (column == "generalsf" || column == "sf"))
        return true;
    return f.get(m) == (f.white_view ? -f.get(a) : f.get(a));
}

void test_colour_symmetry(const std::vector<Row>& rows, Evaluator& ev) {
    begin_section("colour symmetry of every compared field and the value");
    int differ = 0;
    for (const Row& row : rows) {
        const std::string mirrored = mirror_fen(row.fen);
        EvalRebuildRecord a, m;
        const int va = ev.rebuild_record(board_of(row.fen), a);
        const int vm = ev.rebuild_record(board_of(mirrored), m);
        bool same = va == vm && a.verdict == m.verdict;
        for (const Field& f : compared())
            same &= mirrors(f, a, m);
        if (!same && ++differ <= 3) {
            std::fprintf(stderr, "\n    asymmetric: %s (value %d, mirror %d; adjusted eg %d):",
                         row.fen.c_str(), va, vm, a.adjusted[1]);
            for (const Field& f : compared())
                if (!mirrors(f, a, m))
                    std::fprintf(stderr, " %s", f.column);
        }
    }
    EXPECT_EQ(differ, 0);
    end_section();
}

// Coverage (`test_eval --coverage`): for every registry entry, the rows on
// which moving it by `delta` changes the rebuilt evaluation's record or its
// value. A slot or knob reached on fewer than 20 rows needs more fixture
// rows, or its reason for being unreached. Prints the entries under 20.
int coverage(const std::vector<Row>& rows, Evaluator& ev) {
    std::vector<const Row*> used;
    for (const Row& row : rows)
        if (!row.value.at("specialised"))
            used.push_back(&row);
    std::vector<Board> boards;
    for (const Row* row : used)
        boards.push_back(board_of(row->fen));

    auto snapshot = [&](std::vector<EvalRebuildRecord>& recs, std::vector<int>& values) {
        init_eval_tables(g_eval_params);
        ev.clear_pawn_table();
        recs.resize(boards.size());
        values.resize(boards.size());
        for (size_t i = 0; i < boards.size(); i++)
            values[i] = ev.rebuild_record(boards[i], recs[i]);
    };
    std::vector<EvalRebuildRecord> base, moved;
    std::vector<int> base_values, moved_values;
    snapshot(base, base_values);

    constexpr int delta = 16;
    int entries = 0, under = 0;
#define X(name, member, len) \
    { const auto slot = eval_param_span(g_eval_params.member, (len)); \
      for (size_t i = 0; i < slot.size(); i++) { \
          slot[i] += delta; \
          snapshot(moved, moved_values); \
          slot[i] -= delta; \
          int reached = 0; \
          for (size_t r = 0; r < boards.size(); r++) \
              reached += moved_values[r] != base_values[r] \
                      || std::memcmp(&moved[r], &base[r], sizeof(EvalRebuildRecord)) != 0; \
          entries++; \
          if (reached < 20) { \
              under++; \
              std::printf("  %-22s %3zu  %d rows\n", #name, i, reached); \
          } \
      } }
    EVAL_PARAM_LIST(X)
#undef X
    init_eval_tables(g_eval_params);
    ev.clear_pawn_table();
    std::printf("%d entries, %d reached on fewer than 20 of %zu rows\n", entries, under, boards.size());
    return 0;
}

// A draw verdict is a 0: the search asks about a verdict only for a served 0
// (corrected_eval), so a verdict row must serve exactly that.
void test_verdict_is_zero(Evaluator& ev) {
    begin_section("a level pawnless draw serves 0 with its verdict");
    for (const char* fen : {"8/8/3k4/8/2b5/8/4B3/4K3 w - - 0 1", "8/8/3k4/8/2b5/8/4B3/4K3 b - - 0 1",
                            "8/8/3k4/8/2n5/8/4N3/4K3 w - - 0 1", "8/8/3k4/8/2n5/8/4N3/4K3 b - - 0 1"}) {
        const Board b = board_of(fen);
        EXPECT_EQ(ev.evaluate_undamped(b), 0);
        EXPECT(ev.draw_verdict(b));
    }
    end_section();
}

// The bounds (§6.11): a value at or past the known-win band is clamped
// inside it, and counted. No fixture row reaches it (the largest is about
// 4,000), so a position is built for it: eight queens against two pawns, far
// past 16,500 units, with no layer rule to answer it. Its mirror too.
void test_clamp(Evaluator& ev) {
    begin_section("a value past the known-win band serves KNOWN_WIN - 1, counted");
    struct Case { const char* fen; int sign; };
    for (const Case c : {Case{"k7/pp6/8/8/8/8/QQQQQQQ1/K6Q w - - 0 1", +1},
                         Case{"k7/pp6/8/8/8/8/QQQQQQQ1/K6Q b - - 0 1", -1},
                         Case{"k6q/qqqqqqq1/8/8/8/8/PP6/K7 b - - 0 1", +1},
                         Case{"k6q/qqqqqqq1/8/8/8/8/PP6/K7 w - - 0 1", -1}}) {
        const Board b = board_of(c.fen);
        EvalRebuildRecord rec;
        const int64_t before = ev.rebuild_clamps;
        const int served = ev.rebuild_record(b, rec);
        // The layer is silent and the drive is off at this phase, so the
        // unclamped value is the converted taper, past the band.
        EXPECT_EQ(rec.layer, 0);
        EXPECT_EQ(rec.drove, 0);
        EXPECT_EQ(rec.verdict, 0);
        EXPECT(std::abs(rec.v * EVAL_REBUILD_CONVERSION_S / 1024) >= KNOWN_WIN);
        EXPECT_EQ(served, c.sign * (KNOWN_WIN - 1));
        EXPECT_EQ(ev.evaluate_undamped(b), c.sign * (KNOWN_WIN - 1));
        // Only the Tune and Diag flavours count (DIAG_COUNT); each of the
        // two evaluations above clamps once.
        EXPECT_EQ(ev.rebuild_clamps - before, kDiagBuild ? 2 : 0);
    }
    end_section();
}

// ---- Carried from the accepted evaluation's suite -------------------------------
// The caches' purity, the lazy exits' semantics and the matched ablation are
// rules, not values: they hold on the rebuilt arm unchanged.

struct Squares {
    Piece p[SQUARE_NB]{};
    Piece& operator[](int sq) { return p[sq]; }
    const Piece& operator[](int sq) const { return p[sq]; }
};

Squares squares_of(const Board& b) {
    Squares s{};
    for (int sq = 0; sq < SQUARE_NB; ++sq)
        s[sq] = b.piece_on(Square(sq));
    return s;
}

// A FEN without castling or en passant, so that every variant below is a
// position of its own placement only.
std::string fen_of(const Squares& s, Color stm) {
    static const char piece_chars[] = ".pnbrqk";
    std::string placement;
    for (int r = 7; r >= 0; --r) {
        int empty = 0;
        for (int f = 0; f < 8; ++f) {
            const Piece p = s[make_square(File(f), Rank(r))];
            if (p == NO_PIECE) { ++empty; continue; }
            if (empty) { placement += static_cast<char>('0' + empty); empty = 0; }
            char ch = piece_chars[type_of(p)];
            if (color_of(p) == WHITE) ch = static_cast<char>(ch - 'a' + 'A');
            placement += ch;
        }
        if (empty) placement += static_cast<char>('0' + empty);
        if (r > 0) placement += '/';
    }
    return placement + (stm == WHITE ? " w" : " b") + " - - 0 1";
}

// The bench and WAC positions, each also after four and eight seeded random
// plies, without castling or en passant.
std::vector<std::string> purity_sample() {
    std::vector<std::string> sources;
    for (std::string_view fen : bench_fens())
        sources.emplace_back(fen);
    for (const WacPosition& w : wac_positions())
        sources.push_back(w.fen);
    std::mt19937_64 rng(0xCAC4E5EEDull);
    std::vector<std::string> out;
    for (const std::string& start : sources) {
        Board b;
        b.set_fen(start);
        for (int ply = 0; ply <= 8; ++ply) {
            if (ply % 4 == 0)
                out.push_back(fen_of(squares_of(b), b.turn()));
            MoveList legal;
            b.gen_legal(legal);
            if (legal.size() == 0)
                break;
            b.make_move(legal[static_cast<int>(rng() % static_cast<uint64_t>(legal.size()))]);
        }
    }
    return out;
}

// Evaluates `first` and then `second` on one warm evaluator; true when the
// second reads as it does on an evaluator whose caches are empty.
bool warm_reads_fresh(Evaluator& warm, Evaluator& fresh, const std::string& first, const std::string& second) {
    Board a, b;
    a.set_fen(first);
    b.set_fen(second);
    warm.evaluate(a);
    const int w = warm.evaluate(b);
    fresh.clear_pawn_table();
    return w == fresh.evaluate(b);
}

void test_cache_purity() {
    const std::vector<std::string> sample = purity_sample();
    auto warm = std::make_unique<Evaluator>();
    auto fresh = std::make_unique<Evaluator>();

    // The pawn cache is a function of the pawns alone, and its shelter of the
    // king square and castling rights: the same pawns with the pieces removed,
    // or with the kings moved, must read the entry the full position left,
    // and the full position the entry they left.
    int pawn_pairs = 0, pawn_stale = 0;
    std::string pawn_first;
    for (const std::string& fen : sample) {
        Board b;
        b.set_fen(fen);
        const Squares full = squares_of(b);
        Squares bare{};
        Squares moved = full;
        for (int sq = 0; sq < SQUARE_NB; ++sq)
            if (type_of(full[sq]) == PAWN || type_of(full[sq]) == KING)
                bare[sq] = full[sq];
        for (Color c : {WHITE, BLACK}) {
            const Square k = b.king_square(c);
            for (int i = 0; i < SQUARE_NB; ++i) {
                const int sq = c == WHITE ? i : SQUARE_NB - 1 - i;
                if (moved[sq] == NO_PIECE) {
                    moved[sq] = moved[k];
                    moved[k] = NO_PIECE;
                    break;
                }
            }
        }
        for (const Squares* v : {&bare, &moved}) {
            const std::string variant = fen_of(*v, b.turn());
            Board check;
            if (!check.try_set_fen(variant, true) || check.is_in_check())
                continue;
            for (int order = 0; order < 2; ++order) {
                const bool ok = order == 0 ? warm_reads_fresh(*warm, *fresh, fen, variant)
                                           : warm_reads_fresh(*warm, *fresh, variant, fen);
                ++pawn_pairs;
                if (!ok && pawn_stale++ == 0)
                    pawn_first = fen + " / " + variant;
            }
        }
    }
    begin_section("pawn cache: a function of the pawns alone");
    if (pawn_stale)
        std::printf("    %d of %d pairs stale; first: %s\n", pawn_stale, pawn_pairs, pawn_first.c_str());
    EXPECT_EQ(pawn_stale, 0);
    EXPECT(pawn_pairs > 2000);
    end_section();

    // The imbalance cache is a function of the ten piece counts alone: the
    // file mirror keeps the counts and moves every piece; one piece fewer
    // changes a count and must miss.
    int imb_pairs = 0, imb_stale = 0;
    std::string imb_first;
    for (const std::string& fen : sample) {
        Board b;
        b.set_fen(fen);
        const Squares full = squares_of(b);
        Squares mirrored{};
        Squares fewer = full;
        for (int sq = 0; sq < SQUARE_NB; ++sq)
            mirrored[sq ^ 7] = full[sq];
        for (int sq = 0; sq < SQUARE_NB; ++sq)
            if (full[sq] != NO_PIECE && type_of(full[sq]) != KING) {
                fewer[sq] = NO_PIECE;
                break;
            }
        for (const Squares* v : {&mirrored, &fewer}) {
            const std::string variant = fen_of(*v, b.turn());
            for (int order = 0; order < 2; ++order) {
                const bool ok = order == 0 ? warm_reads_fresh(*warm, *fresh, fen, variant)
                                           : warm_reads_fresh(*warm, *fresh, variant, fen);
                ++imb_pairs;
                if (!ok && imb_stale++ == 0)
                    imb_first = fen + " / " + variant;
            }
        }
    }
    begin_section("imbalance cache: a function of the ten piece counts alone");
    if (imb_stale)
        std::printf("    %d of %d pairs stale; first: %s\n", imb_stale, imb_pairs, imb_first.c_str());
    EXPECT_EQ(imb_stale, 0);
    EXPECT(imb_pairs > 2000);
    end_section();
}

// The registry's groups by the stage that reads them: 0 before the first
// lazy exit (material, the tables, the imbalance, the pawn structure) and
// the tempo, which every path serves; 1 between the exits (pieces, mobility,
// king safety, passers); 2 after the second (threats and space).
// Basilisk's own forms' slot groups by bit (src/eval.h, OwnForm), with the
// lazy stage their family sits in: the pawn entry's before the first exit,
// the pieces' and the passers' between the exits, the threats' and space's
// after the second; the tempo enters after winnable(), past both. A group
// whose bit is not compiled in is unread, so the census leaves it out.
// The classical groups a bit replaces are listed with it (replaced = true):
// with the bit compiled in they are unread, which the census checks rather
// than assumes. The Outpost group is not among them: its bishop entry is
// still read under OUTPOST.
struct OwnGroup { const char* prefix; unsigned bit; int stage; bool replaced = false; };
constexpr OwnGroup OWN_GROUPS[] = {
    {"Tempo", OWN_TEMPO, 0, true},
    {"SpaceWeight", OWN_SPACE, 2, true},
    {"RookOnKingRing", OWN_KING_ZONE, 1, true}, {"BishopOnKingRing", OWN_KING_ZONE, 1, true},
    {"BishopPawns", OWN_BAD_BISHOP, 1, true},
    {"BadOutpost", OWN_OUTPOST, 1, true}, {"ReachableOutpost", OWN_OUTPOST, 1, true},
    {"MinorBehindPawn", OWN_MINOR_BEHIND_PAWN, 1, true},
    {"RookOnQueenFile", OWN_ROOK_QUEEN_FILE, 1, true},
    {"QueenInfiltration", OWN_QUEEN_INFILTRATION, 1, true},
    {"ThreatBySafePawn", OWN_PAWN_THREAT, 2, true},
    {"Doubled", OWN_PAWN_CLASS, 0, true}, {"Backward", OWN_PAWN_CLASS, 0, true},
    {"OwnTempo", OWN_TEMPO, 0},
    {"OwnSpace", OWN_SPACE, 2},
    {"OwnMinorKingRing", OWN_KING_ZONE, 1}, {"OwnRookKingRing", OWN_KING_ZONE, 1},
    {"OwnBadBishop", OWN_BAD_BISHOP, 1},
    {"OwnKnightOutpost", OWN_OUTPOST, 1}, {"OwnReachableOutpost", OWN_OUTPOST, 1},
    {"OwnMinorBehindPawn", OWN_MINOR_BEHIND_PAWN, 1},
    {"OwnRookQueenFile", OWN_ROOK_QUEEN_FILE, 1},
    {"OwnQueenInfiltration", OWN_QUEEN_INFILTRATION, 1},
    {"OwnThreat", OWN_PAWN_THREAT, 2},
    {"OwnDoubled", OWN_PAWN_CLASS, 0}, {"OwnBackward", OWN_PAWN_CLASS, 0},
    {"OwnPawnMajority", OWN_MAJORITY, 0}, {"OwnPawnIslands", OWN_ISLANDS, 0},
    {"OwnConnectedRooks", OWN_CONNECTED_ROOKS, 1}, {"OwnRookClosed", OWN_ROOK_CLOSED_FILE, 1},
    {"OwnTrappedBishop", OWN_TRAPPED_BISHOP, 1},
    {"OwnRookBehindPasser", OWN_ROOK_BEHIND_PASSER, 1}, {"OwnEnemyRookPasser", OWN_ROOK_BEHIND_PASSER, 1},
};

// The entry naming group g: an own form's group, or a classical group a bit
// replaces (matched as "<prefix>", "<prefix>Mg" or "<prefix>Eg").
const OwnGroup* own_group(const std::string& g) {
    for (const OwnGroup& o : OWN_GROUPS) {
        const std::string p = o.prefix;
        if (o.replaced ? (g == p || g == p + "Mg" || g == p + "Eg") : g.rfind(p, 0) == 0)
            return &o;
    }
    return nullptr;
}

int stage_of(const std::string& g) {
    if (const OwnGroup* o = own_group(g); o && !o->replaced)
        return o->stage;
    static const char* const before_first[] = {
        "MgVal", "EgVal", "PstMgPawn", "PstMgKnight", "PstMgBishop", "PstMgRook", "PstMgQueen",
        "PstMgKing", "PstEgPawn", "PstEgKnight", "PstEgBishop", "PstEgRook", "PstEgQueen", "PstEgKing",
        "ImbOurs", "ImbTheirs", "DoubledMg", "DoubledEg", "IsolatedMg", "IsolatedEg", "BackwardMg",
        "BackwardEg", "WeakLeverMg", "WeakLeverEg", "WeakUnopposedMg", "WeakUnopposedEg",
        "BlockedPawnMg", "BlockedPawnEg", "ConnectedMg", "ConnectedEg", "Tempo"};
    static const char* const after_second[] = {
        "ThreatByMinorMg", "ThreatByMinorEg", "ThreatByRookMg", "ThreatByRookEg", "ThreatByKingMg",
        "ThreatByKingEg", "HangingMg", "HangingEg", "WeakQueenProtectionMg", "WeakQueenProtectionEg",
        "RestrictedPieceMg", "RestrictedPieceEg", "ThreatBySafePawnMg", "ThreatBySafePawnEg",
        "ThreatByPawnPushMg", "ThreatByPawnPushEg", "KnightOnQueenMg", "KnightOnQueenEg",
        "SliderOnQueenMg", "SliderOnQueenEg", "SpaceWeight"};
    for (const char* name : before_first)
        if (g == name)
            return 0;
    for (const char* name : after_second)
        if (g == name)
            return 2;
    return 1;
}

// The lazy exits: on the fixture's rows the rebuilt arm serves from its first
// or its second exit, no group after the exit moves the served score, and
// the groups before it do.
void test_lazy_exits(const std::vector<Row>& rows) {
    auto ev = std::make_unique<Evaluator>();
    std::vector<Board> at_exit[3];
    for (const Row& row : rows) {
        const Board b = board_of(row.fen);
        EvalRebuildRecord rec;
        ev->rebuild_record(b, rec);
        if (rec.lazy && !rec.layer)  // the layer may override what any group gave
            at_exit[rec.lazy].push_back(b);
    }
    std::vector<int> base[3];
    for (int e = 1; e <= 2; ++e)
        for (const Board& b : at_exit[e])
            base[e].push_back(ev->evaluate_undamped(b));

    struct Group { std::string name; std::span<int> values; };
    std::vector<Group> groups;
#define X(name, member, len) groups.push_back({#name, eval_param_span(g_eval_params.member, (len))});
    EVAL_PARAM_LIST(X)
#undef X
    // An own form's group whose bit is not compiled in is unread and left
    // out. A classical group a compiled-in bit replaces is unread too: it is
    // left out of the census, and perturbing it must move no score.
    std::vector<Group> replaced;
    std::erase_if(groups, [&](const Group& g) {
        const OwnGroup* o = own_group(g.name);
        if (o && o->replaced && own_form(o->bit))
            replaced.push_back(g);
        return o && (o->replaced ? own_form(o->bit) : !own_form(o->bit));
    });

    // How many scores served at each exit move when every value of g moves.
    auto perturb = [&](const Group& g, int (&changed)[3]) {
        const std::vector<int> saved(g.values.begin(), g.values.end());
        for (size_t i = 0; i < g.values.size(); ++i)
            g.values[i] += 40 + 3 * static_cast<int>(i);
        init_eval_tables();
        ev->clear_pawn_table();
        changed[0] = changed[1] = changed[2] = 0;
        for (int e = 1; e <= 2; ++e)
            for (size_t i = 0; i < at_exit[e].size(); ++i)
                changed[e] += ev->evaluate_undamped(at_exit[e][i]) != base[e][i];
        std::copy(saved.begin(), saved.end(), g.values.begin());
        init_eval_tables();
        ev->clear_pawn_table();
    };

    if (!replaced.empty()) {
        begin_section("lazy exits: a group an own form replaces moves no score");
        std::string read;
        for (const Group& g : replaced) {
            int changed[3];
            perturb(g, changed);
            if (changed[1] || changed[2])
                read += " " + g.name + "(" + std::to_string(changed[1]) + "," + std::to_string(changed[2]) + ")";
        }
        if (!read.empty())
            std::printf("    replaced groups that moved a score:%s\n", read.c_str());
        EXPECT(read.empty());
        end_section();
    }

    std::string leaked, missing;
    int served[3] = {}, moved[3] = {};
    for (const Group& g : groups) {
        int changed[3];
        perturb(g, changed);

        const int stage = stage_of(g.name);
        for (int e = 1; e <= 2; ++e) {
            if (stage >= e) {
                if (changed[e])
                    leaked += " " + g.name + "@" + std::to_string(e) + "(" + std::to_string(changed[e]) + ")";
            } else {
                ++served[e];
                moved[e] += changed[e] != 0;
                if (!changed[e])
                    missing += " " + g.name + "@" + std::to_string(e);
            }
        }
    }

    begin_section("lazy exits: no group after an exit moves its served score");
    if (!leaked.empty())
        std::printf("    groups after an exit that moved one:%s\n", leaked.c_str());
    EXPECT(leaked.empty());
    EXPECT(at_exit[1].size() > 300);
    EXPECT(at_exit[2].size() > 300);
    end_section();

    begin_section("lazy exits: the groups before an exit move its score");
    std::printf("    exit 1: %d of %d groups moved %zu scores; exit 2: %d of %d moved %zu\n", moved[1],
                served[1], at_exit[1].size(), moved[2], served[2], at_exit[2].size());
    if (!missing.empty())
        std::printf("    groups that moved none:%s\n", missing.c_str());
    EXPECT(moved[1] * 10 >= served[1] * 9);
    EXPECT(moved[2] * 10 >= served[2] * 9);
    end_section();
}

#ifdef BASILISK_ABLATION
// The matched evaluation ablation: every family's bit moves some score, and a
// warm evaluator serves the stock score again at mask 0. The pawn cache holds
// scores the mask shaped, so a key without the mask would serve a stale one.
void test_eval_ablation_mask() {
    static const char* const kFens[] = {
        "r1bq1rk1/pp3ppp/2n1pn2/2bp4/2B5/2N1PN2/PP3PPP/R1BQ1RK1 w - - 0 9",
        "6k1/5pp1/4p2p/1P6/5P2/4K1P1/7P/2r2R2 w - - 0 30",
        "2kr3r/ppp2ppp/2n1bq2/3pp3/4P3/2PP1N2/PP1QBPPP/R4RK1 b - - 0 12",
    };
    constexpr int N = int(sizeof(kFens) / sizeof(kFens[0]));
    auto warm = std::make_unique<Evaluator>();
    int stock[N];
    for (int i = 0; i < N; ++i)
        stock[i] = warm->evaluate(board_of(kFens[i]));

    begin_section("every family's bit moves a score");
    for (int bit = 0; bit < EVAL_ABLATION_BITS; ++bit) {
        set_eval_ablation_mask(1u << bit);
        bool moved = false;
        for (int i = 0; i < N; ++i)
            moved = moved || warm->evaluate(board_of(kFens[i])) != stock[i];
        if (!moved)
            std::printf("    bit %d moved no score\n", bit);
        EXPECT(moved);
    }
    end_section();

    set_eval_ablation_mask(0);
    begin_section("mask 0 serves the stock score, warm or fresh");
    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(warm->evaluate(board_of(kFens[i])), stock[i]);
        auto fresh = std::make_unique<Evaluator>();
        EXPECT_EQ(fresh->evaluate(board_of(kFens[i])), stock[i]);
    }
    end_section();

    begin_section("the mask keeps its seven family bits");
    set_eval_ablation_mask(0xFFFFFFFFu);
    EXPECT_EQ(eval_ablation_mask(), (1u << EVAL_ABLATION_BITS) - 1);
    set_eval_ablation_mask(0);
    EXPECT_EQ(eval_ablation_mask(), 0u);
    end_section();
}
#endif

} // namespace

int main(int argc, char** argv) {
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables();

    std::printf("Rebuilt evaluator tests\n");
    std::printf("%s\n", std::string(62, '=').c_str());

    const std::vector<Row> rows = load_fixture();
    auto ev = std::make_unique<Evaluator>();
    if (argc > 1 && std::string(argv[1]) == "--coverage")
        return coverage(rows, *ev);
    // `test_eval --served`: every fixture row's FEN and the value the rebuilt
    // evaluation serves it, side to move, undamped: what an oracle built from
    // this arm must read as its Basilisk value.
    if (argc > 1 && std::string(argv[1]) == "--served") {
        for (const Row& row : rows)
            std::printf("%s\t%d\n", row.fen.c_str(), ev->evaluate_undamped(board_of(row.fen)));
        return 0;
    }
    std::printf("\nThe reference fixture: %zu rows\n", rows.size());
    // Donor-seed parity (35 fields against 9587eeeb in eval_reference.tsv) held at 7d68e25
    // and is opt-in (--parity) once C.3.4 baked Basilisk's own k-phase weights.
    if (argc > 1 && std::string(argv[1]) == "--parity") {
        test_parity(rows, *ev);
        return harness_summary();
    }
    test_layer_silent(rows, *ev);
    test_colour_symmetry(rows, *ev);
    test_verdict_is_zero(*ev);
    test_clamp(*ev);

    std::printf("\nCarried from the accepted evaluation's suite\n");
    test_cache_purity();
    test_lazy_exits(rows);
#ifdef BASILISK_ABLATION
    test_eval_ablation_mask();
#endif

    return harness_summary();
}
