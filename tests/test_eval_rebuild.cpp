// The rebuilt evaluation's suite (EVAL_REBUILD builds): parity with the
// classical reference, integer for integer, on every row of
// tests/fixtures/eval_reference.tsv that the reference's own specialised
// evaluations did not answer, and colour symmetry on the same rows.
//
// The fixture is the reference producer's output (tools/diag/c32_reference.py),
// one row per position: the FEN, its source, then the reference's
// intermediates in its own units and White's view. A field joins COMPARED
// once the part of the evaluation that computes it is built.

#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "eval.h"
#include "zobrist.h"
#include "colour_mirror.h"
#include "test_harness.h"

#include <cstring>
#include <fstream>
#include <functional>
#include <memory>
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

    // The layer-silent rows must be at least 90% of the fixture (§9).
    begin_section("layer-silent rows are at least 90% of the fixture");
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
        bool same = va == vm;
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
    std::printf("\nThe reference fixture: %zu rows\n", rows.size());
    test_parity(rows, *ev);
    test_colour_symmetry(rows, *ev);

    return harness_summary();
}
