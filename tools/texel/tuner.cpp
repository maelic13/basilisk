// Basilisk Texel eval tuner
// Phase: Step 2.2 / 2.4 - trace verification and staged Adam optimizer.
//
// Dataset format: one position per line, "FEN;target"
//   target: white-perspective score in [0,1]. Game results (1/0.5/0) and
//   Stockfish/WDL fractional targets are both accepted.
//
// Build: cmake -DTEXEL=ON -DUSE_PEXT=ON ... then cmake --build in Release.
// Run:
//   basilisk-texel --verify dataset.csv
//   basilisk-texel --tune material train.csv holdout.csv [out/eval_params.txt]

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <ratio>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../../src/eval.h"
#include "../../src/board.h"
#include "../../src/bitboard.h"
#include "../../src/attacks.h"
#include "../../src/zobrist.h"

// ---------------------------------------------------------------------------
// Position records
// ---------------------------------------------------------------------------
struct TexelPos {
    Board     board;
    float     result;  // white perspective, [0,1]
    int       score;   // evaluate() result from side-to-move perspective
    EvalTrace trace;   // collected after evaluate()
};

// The fit's rows, the coefficients held sparse: a row's nonzero coefficients
// only, as (position in the active list, value), row i's in
// [offset[i], offset[i + 1]). A position touches a few dozen of a thousand
// slots, and a dense matrix at a full corpus (3.5M rows by 1,025 slots) would
// take 14 GB.
struct TuneSet {
    int active_count = 0;
    std::vector<float> result;
    std::vector<float> base_score;
    std::vector<uint8_t> phase; // game phase 0..24, for bucketed holdout
    std::vector<size_t> offset{0};
    std::vector<uint16_t> index;
    std::vector<float> value;

    size_t size() const { return result.size(); }

    void add_coefficient(int active_position, float v) {
        if (v != 0.0f) {
            index.push_back(static_cast<uint16_t>(active_position));
            value.push_back(v);
        }
    }
    void end_row() { offset.push_back(index.size()); }
};

struct KSchedule {
    double k_eg = 0.0;
    double k_mg = 0.0;

    bool active() const { return k_eg > 0.0 && k_mg > 0.0; }
    bool is_phase() const { return k_eg != k_mg; }
    double at(int phase) const {
        return k_eg + (k_mg - k_eg) * (static_cast<double>(phase) / 24.0);
    }
    static KSchedule constant(double k) { return {k, k}; }
};

struct TuneOptions {
    std::string group;
    std::string train_path;
    std::string holdout_path;
    std::string out_path = "tools/texel/out/eval_params.txt";
    int max_positions = 5'000'000;
    int epochs = 200;
    double lr = 0.3;
    double l2 = 0.0;      // L2-to-prior (toward base/default weights); 0 = off
    double smooth = 0.0;  // second-difference penalty on the ordered tables; 0 = off
    double K = 0.0;       // the frozen K; 0 = fit it on the holdout, as before
    KSchedule K_phase;    // linear-in-phase K(phase) = K_eg + (K_mg - K_eg) * phase / 24
    int patience = 20;    // stop when the holdout has not improved for this many epochs; 0 = off
    double min_delta = 0.0;  // a gain counts for patience only above this; 0 = any gain
};

// ---------------------------------------------------------------------------
// Parsing helpers
// ---------------------------------------------------------------------------
static bool parse_target(const std::string& text, float& out) {
    if (text == "1-0") {
        out = 1.0f;
        return true;
    }
    if (text == "0-1") {
        out = 0.0f;
        return true;
    }
    if (text == "1/2-1/2") {
        out = 0.5f;
        return true;
    }

    char* end = nullptr;
    float parsed = std::strtof(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0' || parsed < 0.0f || parsed > 1.0f)
        return false;

    // Targets must be GAME RESULTS. Basilisk's Texel fits have always used
    // self-play WDL, so the only legal values are 0, 0.5 and 1.
    //
    // This is a hard abort rather than a skipped row because the failure it
    // prevents is silent and expensive. On 2026-08-26 a 348-parameter fit was
    // run against an old discarded corpus carrying Stockfish expected-score
    // targets; nothing objected, because a continuous target in [0,1] is
    // perfectly well-formed input to a least-squares fit -- it is simply the
    // wrong question. The candidate lost 77.92 Elo and the cause took a day and
    // three SPRTs to isolate. A corpus that is the wrong KIND of data must fail
    // loudly on contact, not quietly produce a worse engine.
    if (parsed != 0.0f && parsed != 0.5f && parsed != 1.0f) {
        std::cerr << "\nFATAL: target '" << text << "' is not a game result.\n"
                  << "  Texel targets must be self-play WDL: exactly 0, 0.5 or 1.\n"
                  << "  A continuous target means this corpus holds ENGINE EVALUATIONS,\n"
                  << "  which fits the evaluation to imitate another engine instead of\n"
                  << "  to win. beast_sf_*.csv is an old discarded distillation set;\n"
                  << "  the project corpora (train_beast_seed, train_phase911, ...) are\n"
                  << "  game-result labelled.\n";
        std::exit(2);
    }

    out = parsed;
    return true;
}

// ---------------------------------------------------------------------------
// Dataset rows
// ---------------------------------------------------------------------------
// Two row formats are read. The extractor's contract, basilisk-texel-rows-v1,
// opens with a `# basilisk-texel-rows-v1 <split>` line and a header naming the
// columns `fen,result,game_id,start_id,ply,phase`; a malformed row of such a
// file is fatal, since a published corpus is never partly readable. The legacy
// `FEN;target` lines that older corpora carry are still read, rows that do not
// parse being skipped and counted as before.
static constexpr const char* ROWS_V1_SCHEMA = "basilisk-texel-rows-v1";
static constexpr const char* ROWS_V1_HEADER = "fen,result,game_id,start_id,ply,phase";

class RowReader {
public:
    explicit RowReader(const std::string& path) : path_(path), in_(path) {
        if (!in_) {
            std::cerr << "Cannot open dataset: " << path << "\n";
            std::exit(1);
        }
    }

    // The next row's FEN and target text. False at the end of the file, or for
    // a legacy line with no ';' (counted in skipped()).
    bool next(std::string& fen, std::string& target) {
        std::string line;
        while (std::getline(in_, line)) {
            ++lineno_;
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (line.empty())
                continue;
            if (line[0] == '#') {
                if (lineno_ == 1 && line.rfind(std::string("# ") + ROWS_V1_SCHEMA, 0) == 0)
                    v1_ = true;
                continue;
            }
            if (v1_ && !header_seen_) {
                if (line != ROWS_V1_HEADER)
                    fatal("the header is not '" + std::string(ROWS_V1_HEADER) + "'");
                header_seen_ = true;
                continue;
            }
            if (v1_)
                return split_v1(line, fen, target);
            auto sep = line.rfind(';');
            if (sep == std::string::npos) {
                if (++skipped_ <= 5)
                    std::cerr << "Bad line " << lineno_ << " (no ';'): " << line << "\n";
                continue;
            }
            fen = line.substr(0, sep);
            target = line.substr(sep + 1);
            return true;
        }
        if (v1_ && !header_seen_)
            fatal("no header row");
        return false;
    }

    bool v1() const { return v1_; }
    int lineno() const { return lineno_; }
    int skipped() const { return skipped_; }

    // A row that the caller found unusable: fatal in a v1 file, counted in a
    // legacy one.
    void reject(const std::string& why) {
        if (v1_)
            fatal(why);
        if (++skipped_ <= 5)
            std::cerr << why << " at line " << lineno_ << "\n";
    }

private:
    bool split_v1(const std::string& line, std::string& fen, std::string& target) {
        std::vector<std::string> cells;
        size_t start = 0;
        while (true) {
            const size_t comma = line.find(',', start);
            cells.push_back(line.substr(start, comma - start));
            if (comma == std::string::npos)
                break;
            start = comma + 1;
        }
        if (cells.size() != 6)
            fatal("a row has " + std::to_string(cells.size()) + " columns, not 6");
        for (const std::string& cell : cells)
            if (cell.empty())
                fatal("a row has an empty column");
        fen = cells[0];
        target = cells[1];
        return true;
    }

    [[noreturn]] void fatal(const std::string& why) const {
        std::cerr << "FATAL: " << path_ << " line " << lineno_ << ": " << why
                  << " (" << ROWS_V1_SCHEMA << ").\n";
        std::exit(2);
    }

    std::string path_;
    std::ifstream in_;
    int lineno_ = 0;
    int skipped_ = 0;
    bool v1_ = false;
    bool header_seen_ = false;
};

static bool is_option(const char* s) {
    return s != nullptr && s[0] == '-' && s[1] == '-';
}

static int parse_int_arg(const char* value, const char* name) {
    char* end = nullptr;
    long parsed = std::strtol(value, &end, 10);
    if (end == value || *end != '\0' || parsed < 0 || parsed > 100'000'000L) {
        std::cerr << "Bad value for " << name << ": " << value << "\n";
        std::exit(1);
    }
    return static_cast<int>(parsed);
}

static double parse_double_arg(const char* value, const char* name) {
    char* end = nullptr;
    double parsed = std::strtod(value, &end);
    if (end == value || *end != '\0' || !std::isfinite(parsed) || parsed <= 0.0) {
        std::cerr << "Bad value for " << name << ": " << value << "\n";
        std::exit(1);
    }
    return parsed;
}

// A strength that may be zero (off).
static double parse_nonneg_double_arg(const char* value, const char* name) {
    char* end = nullptr;
    double parsed = std::strtod(value, &end);
    if (end == value || *end != '\0' || !std::isfinite(parsed) || parsed < 0.0) {
        std::cerr << "Bad value for " << name << ": " << value << "\n";
        std::exit(1);
    }
    return parsed;
}

static KSchedule parse_k_phase_arg(const char* value, const char* name) {
    std::string text = value ? value : "";
    const size_t comma = text.find(',');
    if (comma == std::string::npos || text.find(',', comma + 1) != std::string::npos) {
        std::cerr << "Bad value for " << name << ": " << text << " (expected K_eg,K_mg)\n";
        std::exit(1);
    }
    const std::string eg_s = text.substr(0, comma);
    const std::string mg_s = text.substr(comma + 1);
    if (eg_s.empty() || mg_s.empty()) {
        std::cerr << "Bad value for " << name << ": " << text << " (expected K_eg,K_mg)\n";
        std::exit(1);
    }
    return {parse_double_arg(eg_s.c_str(), name), parse_double_arg(mg_s.c_str(), name)};
}

static void usage(const char* exe) {
    std::cerr
        << "Usage:\n"
        << "  " << exe << " --verify <dataset.csv> [--max-positions N]\n"
        << "  " << exe << " --played-hash <dataset.csv>\n"
        << "  " << exe << " --loss <dataset.csv> [--K K | --K-phase K_eg,K_mg | --fit-k-phase] [--skip N] [--max-positions N]\n"
        << "  " << exe << " --dump-played <dataset.csv> <out.txt>\n"
        << "  " << exe << " --list-params\n"
        << "  " << exe << " --dump-coeffs <dataset.csv> <out.f64> [--max-positions N]\n"
        << "  " << exe << " --count-support <dataset.csv> <out.txt> [--max-positions N]\n"
        << "  " << exe << " --dump-row-lines <dataset.csv> <out.txt> [--max-positions N]\n"
        << "  Every mode also takes --init FILE (a complete `name index value` vector to\n"
        << "  start from) and --surface FILE (the surface manifest; --tune surface fits its\n"
        << "  free linear slots, --tune-kingsafety its king-safety knobs).\n"
        << "  " << exe << " --feature-support <dataset.csv> [--max-positions N]\n"
        << "  " << exe << " --dump-eval <dataset.csv> [limit]\n"
        << "  " << exe << " --audit-coverage\n"
        << "  " << exe << " --tune-kingsafety <train.csv> <holdout.csv> [out.txt] [--epochs N] [--max-positions N] [--step S] [--K K | --K-phase K_eg,K_mg]\n"
        << "  " << exe << " --tune <group> <train.csv> <holdout.csv> [out/eval_params.txt] [options]\n"
        << "\n"
        << "Groups:\n"
        << "  material    mg/eg material values for pawn..queen (10 params)\n"
        << "  scalars     non-PST, non-king-safety scalar/table terms\n"
        << "  pawnstruct  doubled/isolated/connected/backward pawn terms\n"
        << "  passers     passed/candidate/supported/free passer terms\n"
        << "  rooks       open/semi/7th/behind-passer rook terms\n"
        << "  minors      bishop pair, knight outpost, trapped bishop terms\n"
        << "  mobility    per-piece safe mobility terms\n"
        << "  threats     pawn threats to minor/rook/queen\n"
        << "  misc        passed-king proximity, space (+3.6 refinement), tempo\n"
        << "  kingsafety  king attack, shelter, and storm terms\n"
        << "  pst         PSTs plus material refit\n"
        << "  texel       PSTs + scalars, material pinned\n"
        << "  surface     the --surface manifest's free linear slots (the contract's form)\n"
        << "  all         all meaningful eval params\n"
        << "\n"
        << "Options:\n"
        << "  --epochs N            optimizer epochs (default 200)\n"
        << "  --lr X                Adam learning rate (default 0.3)\n"
        << "  --l2 X                L2-to-prior strength (pull toward default weights; default 0 = off)\n"
        << "  --smooth X            second-difference penalty on the ordered tables (default 0 = off)\n"
        << "  --K K                 the frozen K (also --tune-kingsafety); default: fitted on the holdout\n"
        << "  --K-phase K_eg,K_mg   frozen linear K(phase) = K_eg + (K_mg - K_eg) * phase / 24\n"
        << "  --patience N          stop when the holdout has not improved for N epochs (default 20; 0 = off)\n"
        << "  --min-delta X         count a holdout gain for --patience only when it exceeds X (default 0);\n"
        << "                        a run stopped by patience has converged, and the best vector is kept\n"
        << "  --max-positions N     cap positions loaded from each file (default 5000000; 0 = all)\n"
        << "  A fit whose holdout still improved at its last epoch, or whose largest move\n"
        << "  reached 0.9 x lr x epochs, exits 3: its schedule, not the data, stopped it.\n";
}

static TuneOptions parse_tune_options(int argc, char* argv[]) {
    if (argc < 5) {
        usage(argv[0]);
        std::exit(1);
    }

    TuneOptions opts;
    opts.group = argv[2];
    opts.train_path = argv[3];
    opts.holdout_path = argv[4];

    int argi = 5;
    if (argi < argc && !is_option(argv[argi])) {
        opts.out_path = argv[argi++];
    }

    while (argi < argc) {
        std::string opt = argv[argi++];
        if (opt == "--epochs" && argi < argc) {
            opts.epochs = parse_int_arg(argv[argi++], "--epochs");
        } else if (opt == "--lr" && argi < argc) {
            opts.lr = parse_double_arg(argv[argi++], "--lr");
        } else if (opt == "--max-positions" && argi < argc) {
            opts.max_positions = parse_int_arg(argv[argi++], "--max-positions");
        } else if (opt == "--l2" && argi < argc) {
            opts.l2 = parse_nonneg_double_arg(argv[argi++], "--l2");
        } else if (opt == "--smooth" && argi < argc) {
            opts.smooth = parse_nonneg_double_arg(argv[argi++], "--smooth");
        } else if (opt == "--K" && argi < argc) {
            opts.K = parse_double_arg(argv[argi++], "--K");
        } else if (opt == "--K-phase" && argi < argc) {
            opts.K_phase = parse_k_phase_arg(argv[argi++], "--K-phase");
        } else if (opt == "--patience" && argi < argc) {
            opts.patience = parse_int_arg(argv[argi++], "--patience");
        } else if (opt == "--min-delta" && argi < argc) {
            opts.min_delta = parse_nonneg_double_arg(argv[argi++], "--min-delta");
        } else {
            std::cerr << "Unknown or incomplete option: " << opt << "\n";
            usage(argv[0]);
            std::exit(1);
        }
    }

    if (opts.K > 0.0 && opts.K_phase.active()) {
        std::cerr << "--K and --K-phase cannot both be set.\n";
        std::exit(1);
    }
    if (opts.epochs <= 0) {
        std::cerr << "--epochs must be positive.\n";
        std::exit(1);
    }

    return opts;
}

// ---------------------------------------------------------------------------
// Flat parameter helpers (matches EVAL_PARAM_LIST order)
// ---------------------------------------------------------------------------
static void read_weights(const EvalParams& p, double* w) {
    int idx = 0;
#define X(name, member, len) \
    { const auto values = eval_param_cspan(p.member, (len)); \
      for (int v : values) w[idx++] = static_cast<double>(v); }
    EVAL_PARAM_LIST(X)
#undef X
}

static void write_weights(EvalParams& p, const double* w) {
    int idx = 0;
#define X(name, member, len) \
    { auto values = eval_param_span(p.member, (len)); \
      for (int& v : values) v = static_cast<int>(std::round(w[idx++])); }
    EVAL_PARAM_LIST(X)
#undef X
}

// ---------------------------------------------------------------------------
// The surface: every slot, the initial vector and the manifest
// ---------------------------------------------------------------------------
// One entry per flat slot, in EVAL_PARAM_LIST order: its group's name, its
// index within the group, and where its value lives in g_eval_params.
struct Slot {
    const char* group;
    int sub;
    int* value;
};

static std::vector<Slot> slots() {
    std::vector<Slot> out;
    out.reserve(EVAL_PARAM_FLAT_SIZE);
#define X(name, member, len) \
    { auto values = eval_param_span(g_eval_params.member, (len)); \
      for (size_t i = 0; i < values.size(); ++i) out.push_back({#name, static_cast<int>(i), &values[i]}); }
    EVAL_PARAM_LIST(X)
#undef X
    return out;
}

// A vector's identity: FNV-1a 64 over its canonical text, one `name index
// value` line per slot in slot order (tools/texel/bake.py computes the same
// over a header's initializers). A written vector carries the hash of the
// vector its fit started from (`seed`) and of the first vector of its chain of
// stages (`root`), so that bake can refuse a vector that was not fitted from
// the header it would overwrite.
static std::string vector_hash() {
    uint64_t h = 0xcbf29ce484222325ull;
    for (const Slot& s : slots()) {
        const std::string line = std::string(s.group) + " " + std::to_string(s.sub) + " "
                               + std::to_string(*s.value) + "\n";
        for (char c : line) {
            h ^= static_cast<unsigned char>(c);
            h *= 0x100000001b3ull;
        }
    }
    char text[17];
    std::snprintf(text, sizeof text, "%016llx", static_cast<unsigned long long>(h));
    return text;
}

static std::string g_root_hash, g_seed_hash;

// The initial vector, in the `name index value` form the tuner writes: every
// slot exactly once, and nothing else. A partial, duplicated or unknown entry
// is fatal, so a stage can only ever start from a vector that says every value.
static void load_initial_vector(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "Cannot open initial vector: " << path << "\n";
        std::exit(1);
    }
    std::vector<Slot> all = slots();
    std::map<std::pair<std::string, int>, size_t> where;
    for (size_t i = 0; i < all.size(); ++i)
        where[{all[i].group, all[i].sub}] = i;
    std::vector<char> seen(all.size(), 0);
    std::string line;
    int lineno = 0;
    auto fatal = [&](const std::string& why) {
        std::cerr << "FATAL: initial vector " << path << " line " << lineno << ": " << why << "\n";
        std::exit(2);
    };
    while (std::getline(in, line)) {
        ++lineno;
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.rfind("# root ", 0) == 0)
            g_root_hash = line.substr(7);
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream fields(line);
        std::string name, extra;
        int index = 0, value = 0;
        if (!(fields >> name >> index >> value) || (fields >> extra))
            fatal("not `name index value`");
        auto it = where.find({name, index});
        if (it == where.end())
            fatal("no slot " + name + "[" + std::to_string(index) + "]");
        if (seen[it->second]++)
            fatal(name + "[" + std::to_string(index) + "] given twice");
        *all[it->second].value = value;
    }
    long missing = 0;
    for (size_t i = 0; i < all.size(); ++i) {
        if (!seen[i] && ++missing <= 5)
            std::cerr << "  missing " << all[i].group << "[" << all[i].sub << "]\n";
    }
    if (missing) {
        std::cerr << "FATAL: initial vector " << path << " leaves " << missing << " of "
                  << all.size() << " slots unsaid.\n";
        std::exit(2);
    }
    init_eval_tables(g_eval_params);
    std::cout << "Initial vector: " << path << " (all " << all.size() << " slots).\n";
}

// The surface manifest, basilisk-texel-surface-v1 (tools/texel/surface.py
// writes and checks it): one row per slot, in slot order, saying whether the
// slot is free, fixed or excluded, which stage fits a free slot, its anchor,
// and a king-safety knob's bounds and first step.
static constexpr const char* SURFACE_SCHEMA = "basilisk-texel-surface-v1";
static constexpr const char* SURFACE_HEADER =
    "index\tgroup\tsub\tseed\tclass\tstage\tanchor\tlo\thi\tstep\treason";

struct SurfaceRow {
    std::string klass, stage, anchor;
    int lo = 0, hi = 0, step = 0;
};

static std::vector<SurfaceRow> load_surface(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "Cannot open surface manifest: " << path << "\n";
        std::exit(1);
    }
    const std::vector<Slot> all = slots();
    std::vector<SurfaceRow> out;
    std::string line;
    int lineno = 0;
    bool schema = false, header = false;
    auto fatal = [&](const std::string& why) {
        std::cerr << "FATAL: surface " << path << " line " << lineno << ": " << why << "\n";
        std::exit(2);
    };
    while (std::getline(in, line)) {
        ++lineno;
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (lineno == 1) {
            if (line.rfind(std::string("# ") + SURFACE_SCHEMA, 0) != 0)
                fatal(std::string("not a ") + SURFACE_SCHEMA + " file");
            schema = true;
            continue;
        }
        if (line.empty() || line[0] == '#')
            continue;
        if (!header) {
            if (line != SURFACE_HEADER)
                fatal("the header is not the schema's");
            header = true;
            continue;
        }
        std::vector<std::string> cells;
        size_t start = 0;
        while (true) {
            const size_t tab = line.find('\t', start);
            cells.push_back(line.substr(start, tab - start));
            if (tab == std::string::npos)
                break;
            start = tab + 1;
        }
        if (cells.size() != 11)
            fatal("a row has " + std::to_string(cells.size()) + " columns, not 11");
        const size_t i = out.size();
        if (i >= all.size() || std::stoi(cells[0]) != static_cast<int>(i)
            || cells[1] != all[i].group || std::stoi(cells[2]) != all[i].sub)
            fatal("row " + cells[0] + " (" + cells[1] + "[" + cells[2] + "]) is not slot " + std::to_string(i));
        SurfaceRow row;
        row.klass = cells[4];
        row.stage = cells[5];
        row.anchor = cells[6];
        if (row.klass != "free" && row.klass != "fixed" && row.klass != "excluded")
            fatal("class '" + row.klass + "'");
        if ((row.klass == "free") != (row.stage == "linear" || row.stage == "kingsafety"))
            fatal("a free slot needs a stage, and only a free slot has one");
        if (row.stage == "kingsafety") {
            row.lo = std::stoi(cells[7]);
            row.hi = std::stoi(cells[8]);
            row.step = std::stoi(cells[9]);
            if (row.lo > row.hi || row.step < 1)
                fatal("a king-safety knob needs lo <= hi and a step of at least 1");
        }
        out.push_back(row);
    }
    if (!schema || !header || out.size() != all.size()) {
        std::cerr << "FATAL: surface " << path << " has " << out.size() << " rows for "
                  << all.size() << " slots.\n";
        std::exit(2);
    }
    return out;
}

// The global options every mode takes, removed from argv before the mode
// reads its own: the initial vector, then the surface it is fitted on.
static std::string g_surface_path;

static void take_global_options(int& argc, char* argv[]) {
    std::string init_path;
    int out = 1;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if ((a == "--init" || a == "--surface") && i + 1 < argc) {
            (a == "--init" ? init_path : g_surface_path) = argv[++i];
            continue;
        }
        argv[out++] = argv[i];
    }
    argc = out;
    if (!init_path.empty())
        load_initial_vector(init_path);
    g_seed_hash = vector_hash();
    if (g_root_hash.empty())
        g_root_hash = g_seed_hash;  // the chain starts here
}

static void append_group(std::vector<int>& out, int group) {
    int base = eval_param_offset(group);
    int len = EVAL_PARAM_LENS[group];
    for (int i = 0; i < len; ++i)
        out.push_back(base + i);
}

[[maybe_unused]] static void append_groups(std::vector<int>& out, int first, int last) {
    for (int g = first; g <= last; ++g)
        append_group(out, g);
}

[[maybe_unused]] static void append_material(std::vector<int>& out) {
    int mg = eval_param_offset(EPG_MgVal);
    int eg = eval_param_offset(EPG_EgVal);
    for (int pt = PAWN; pt <= QUEEN; ++pt) {
        out.push_back(mg + pt);
        out.push_back(eg + pt);
    }
}

static void unique_sort(std::vector<int>& xs) {
    std::sort(xs.begin(), xs.end());
    xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
}

static std::vector<int> active_indices_for_group(const std::string& group) {
    std::vector<int> active;

    if (group == "surface") {
        // The manifest's free linear slots: the contract's way to say what a
        // fit moves. The named groups below are the older form.
        if (g_surface_path.empty()) {
            std::cerr << "--tune surface needs --surface FILE.\n";
            std::exit(1);
        }
        const std::vector<SurfaceRow> rows = load_surface(g_surface_path);
        for (size_t i = 0; i < rows.size(); ++i)
            if (rows[i].stage == "linear")
                active.push_back(static_cast<int>(i));
    } else {
        std::cerr << "Unknown tune group '" << group << "'.\n";
        std::exit(1);
    }

    unique_sort(active);
    if (active.empty()) {
        std::cerr << "Tune group '" << group << "' has no active params.\n";
        std::exit(1);
    }
    return active;
}

[[maybe_unused]] static float trace_coeff(const EvalTrace& tr, int idx) {
    return static_cast<float>(
        tr.mg[idx] * tr.phase + tr.eg[idx] * (24 - tr.phase)) / 24.0f;
}

[[maybe_unused]] static int flat_index(int group, int idx = 0) {
    return eval_param_offset(group) + idx;
}

// ---------------------------------------------------------------------------
// The played function, row by row
// ---------------------------------------------------------------------------
// The fit models the score the engine serves, from the evaluator's own record
// of how it got there (g_rest), never from a re-derivation of its rules:
//   - the running sum: the traced terms times the weights, plus the frozen
//     untraced additions;
//   - the taper of that sum;
//   - what apply_endgame did to it: left it, scaled it by num / den, or
//     replaced it with a known result;
//   - the rule-50 damping;
//   - and, where a release build would stop at the lazy checkpoint, the same
//     chain on the cheap block alone, with the trace written up to there.
// Every stage is recomputed here in the engine's integer arithmetic and must
// equal the evaluator's value at the seed weights, or the row is a mismatch:
// a model that disagrees with the engine is never fitted.
//
// A row whose served score is an override (a known win, a tier, a drawn KPK)
// or a draw verdict takes no part in K or in any loss: its score is not a
// function of the weights.
//
// The gradient is linearised at the seed: the scale apply_endgame applied there
// (whose strong side, for the pawnless scaler, the seed score's sign chose)
// and the damping's slope, times the trace's tapered count.
struct RowModel {
    enum Status { USED, OVERRIDE, VERDICT };
    Status status = USED;
    bool lazy = false;
    int played_white = 0;          // the score the engine serves, White's view
    double scale = 1.0;            // d played / d tapered sum, at the seed
    const EvalTrace* trace = nullptr;
    // The gradient: d played / d (traced middlegame, endgame
    // sum), and d played / d tempo, at the seed.
    double scale_mg = 0.0, scale_eg = 0.0, tempo = 0.0;
};

static const std::vector<long>& seed_weights() {
    static const std::vector<long> w = [] {
        std::vector<double> d(EVAL_PARAM_FLAT_SIZE);
        read_weights(g_eval_params, d.data());
        return std::vector<long>(d.begin(), d.end());
    }();
    return w;
}

[[maybe_unused]] static int taper_int(long mg, long eg, int phase) {
    return static_cast<int>((mg * phase + eg * (24 - phase)) / 24);
}

[[maybe_unused]] static int apply_step(const EvalEndgameStep& step, int tapered) {
    return step.kind == EG_SCALED ? tapered * step.num / step.den : tapered;
}

static int damp_white(int score, int clock) {
    return clock > 0 ? damp_rule50(score, clock) : score;
}

// The rebuilt evaluation's played function (analysis/c31_rebuild_2026-11-25.md
// §6), recomputed from its record in the engine's integer arithmetic:
//   - the sum: the traced terms times the weights, plus the knobs' terms;
//   - winnable, from the recorded complexity, which no slot moves;
//   - the taper at a factor of 64, converted: the layer's input with the
//     recorded mate drive;
//   - the layer's step: scaled by num / den, or silent, when the taper at
//     the general factor, converted, plus the drive; an override or a draw
//     verdict is served as the layer set it;
//   - the side to move, the tempo, the clamp and the rule-50 damping.
// Every stage must equal the engine's at the seed. The gradient is
// linearised there: winnable passes a half or holds it at 0, the taper
// weighs each half by its phase and factor, then S / 1024, the layer's scale
// and the damping's slope; a clamped row has none.
static int rb_taper(long mg, long eg, int ph, int sf) {
    return static_cast<int>((mg * ph + eg * (128 - ph) * sf / 64) / 128);
}
static int rb_cp(long v) { return static_cast<int>(v * EVAL_REBUILD_CONVERSION_S / 1024); }

static bool model_row(const Board& b, int served_stm, RowModel& out, std::string* mismatch) {
    const std::vector<long>& w = seed_weights();
    const int served_white = (b.turn() == WHITE) ? served_stm : -served_stm;
    const int clock = b.rule50_count();
    auto fail = [&](const char* stage) {
        if (mismatch)
            *mismatch = stage;
        return false;
    };

    out = RowModel{};
    out.lazy = g_rest.lazy != 0;
    out.trace = out.lazy ? &g_lazy_trace : &g_trace;
    const int* rest_mg = out.lazy ? g_rest.lazy_mg : g_rest.mg;
    const int* rest_eg = out.lazy ? g_rest.lazy_eg : g_rest.eg;
    long mg = 0, eg = 0;
    for (int i = 0; i < EVAL_PARAM_FLAT_SIZE; ++i) {
        mg += out.trace->mg[i] * w[static_cast<size_t>(i)];
        eg += out.trace->eg[i] * w[static_cast<size_t>(i)];
    }
    for (int s = 0; s < REST_SOURCE_NB; ++s) {
        mg += rest_mg[s];
        eg += rest_eg[s];
    }
    if (mg != g_rest.sum_mg || eg != g_rest.sum_eg)
        return fail("the running sum");

    const long c = g_rest.complexity;
    auto sign = [](long x) { return (x > 0) - (x < 0); };
    const long amg = mg + sign(mg) * std::clamp(c + 50, -std::abs(mg), 0L);
    const long aeg = eg + sign(eg) * std::max(c, -std::abs(eg));
    // Whether a small change of a half moves it after winnable.
    const bool pass_mg = c + 50 >= 0 || (mg != 0 && c + 50 >= -std::abs(mg));
    const bool pass_eg = eg != 0 ? c >= -std::abs(eg) : c >= 0;

    const int ph = g_rest.ph;
    const int t = rb_cp(rb_taper(amg, aeg, ph, 64));
    if (t != g_rest.t)
        return fail("the taper at 64");

    const EvalEndgameStep& step = g_rest.step;
    const int input = t + g_rest.drive;
    double layer_scale = 1.0;
    int sf = 64;
    int white = g_rest.white;
    if (step.kind == EG_SCALED) {
        white = input * step.num / step.den;
        layer_scale = double(step.num) / double(step.den);
    } else if (step.kind == EG_UNCHANGED) {
        sf = g_rest.general_sf;
        white = rb_cp(rb_taper(amg, aeg, ph, sf)) + g_rest.drive;
    }
    if (white != g_rest.white)
        return fail("the endgame layer");

    const int stm_white = b.turn() == WHITE ? white : -white;
    const bool fixed = step.kind == EG_OVERRIDE || step.verdict;
    int served = stm_white;
    bool clamped = false;
    if (!fixed) {
        served = stm_white + w[static_cast<size_t>(eval_param_offset(EPG_Tempo))]
                                 * EVAL_REBUILD_CONVERSION_S / 1024;
        if (std::abs(served) >= KNOWN_WIN) {
            served = served > 0 ? KNOWN_WIN - 1 : -(KNOWN_WIN - 1);
            clamped = true;
        }
    }
    if (served != g_rest.served)
        return fail("the tempo and the bounds");
    const int undamped_white = b.turn() == WHITE ? served : -served;
    if (damp_white(undamped_white, clock) != served_white)
        return fail("the rule-50 damping");

    out.played_white = served_white;
    out.status = step.kind == EG_OVERRIDE ? RowModel::OVERRIDE
               : step.verdict             ? RowModel::VERDICT
                                          : RowModel::USED;
    if (!fixed && !clamped) {
        const double tail = double(EVAL_REBUILD_CONVERSION_S) / 1024.0 * layer_scale
                          * (clock > 0 ? double(damp_rule50_scale_num(clock)) / double(DAMP_RULE50_DEN) : 1.0);
        out.scale_mg = pass_mg ? tail * ph / 128.0 : 0.0;
        out.scale_eg = pass_eg ? tail * (128 - ph) * sf / (64.0 * 128.0) : 0.0;
        out.tempo = (b.turn() == WHITE ? 1.0 : -1.0) * double(EVAL_REBUILD_CONVERSION_S) / 1024.0
                  * (clock > 0 ? double(damp_rule50_scale_num(clock)) / double(DAMP_RULE50_DEN) : 1.0);
    }
    return true;
}

// A row's coefficient of slot idx: d played / d weight at the seed.
static double row_coeff(const RowModel& model, int idx) {
    return model.trace->mg[idx] * model.scale_mg + model.trace->eg[idx] * model.scale_eg
         + (idx == eval_param_offset(EPG_Tempo) ? model.tempo : 0.0);
}

// Rows by what the model made of them, for the load reports.
struct ModelCounts {
    long used = 0, lazy = 0, overrides = 0, verdicts = 0, mismatches = 0;
    std::map<std::string, long> by_stage;
    std::vector<std::string> examples;

    void add(bool ok, const RowModel& m, const std::string& stage, const Board& b) {
        if (!ok) {
            ++mismatches;
            ++by_stage[stage];
            if (examples.size() < 5)
                examples.push_back(stage + ": " + b.get_fen());
            return;
        }
        if (m.status == RowModel::OVERRIDE) { ++overrides; return; }
        if (m.status == RowModel::VERDICT)  { ++verdicts; return; }
        ++used;
        lazy += m.lazy;
    }

    void print(const std::string& what) const {
        std::cout << what << ": " << used << " rows used (" << lazy << " at the lazy checkpoint), "
                  << overrides << " overrides and " << verdicts
                  << " draw verdicts left out of K and the loss";
        if (mismatches)
            std::cout << ", " << mismatches << " MISMATCHES";
        std::cout << ".\n";
        for (const auto& [stage, n] : by_stage)
            std::cerr << "  mismatch in " << stage << ": " << n << "\n";
        for (const std::string& e : examples)
            std::cerr << "    " << e << "\n";
    }
};

// The slots a clamp must not move during a fit: those outside the fitted set
// (held at the seed, or frozen for want of support). Null outside a fit.
static const std::vector<char>* g_clamp_held = nullptr;

static bool clamp_held(int idx) {
    return g_clamp_held && (*g_clamp_held)[static_cast<size_t>(idx)];
}

[[maybe_unused]] static void clamp_range(double* w, int idx, double lo, double hi) {
    if (!clamp_held(idx))
        w[idx] = std::clamp(w[idx], lo, hi);
}

[[maybe_unused]] static void clamp_group_range(double* w, int group, double lo, double hi) {
    int base = eval_param_offset(group);
    int len = EVAL_PARAM_LENS[group];
    for (int i = 0; i < len; ++i)
        clamp_range(w, base + i, lo, hi);
}

// Non-decreasing along `chain`. A held entry stays where it is, so a free one
// is bounded below by the entry before it and above by the smallest held entry
// after it: a free entry that the data pushed past a held neighbour stops at
// it, instead of the held neighbour being moved.
[[maybe_unused]] static void enforce_chain(double* w, const std::vector<int>& chain) {
    std::vector<double> upper(chain.size(), std::numeric_limits<double>::infinity());
    double bound = std::numeric_limits<double>::infinity();
    for (size_t k = chain.size(); k-- > 0;) {
        upper[k] = bound;
        if (clamp_held(chain[k]))
            bound = std::min(bound, w[chain[k]]);
    }
    double previous = -std::numeric_limits<double>::infinity();
    for (size_t k = 0; k < chain.size(); ++k) {
        double& x = w[chain[k]];
        if (!clamp_held(chain[k]))
            x = std::clamp(x, previous, std::max(previous, upper[k]));
        previous = x;
    }
}

[[maybe_unused]] static void enforce_non_decreasing(double* w, int group, int first, int last) {
    const int base = eval_param_offset(group);
    std::vector<int> chain;
    for (int i = first; i <= last; ++i)
        chain.push_back(base + i);
    enforce_chain(w, chain);
}

// The knobs' bounds are the surface manifest's; only the slots no
// position reads are held at 0.
static void clamp_weights_for_group(const std::string&, double* w) {
    for (int g : {EPG_MgVal, EPG_EgVal}) {
        w[eval_param_offset(g) + NO_PIECE_TYPE] = 0.0;
        w[eval_param_offset(g) + KING] = 0.0;
    }
}

// ---------------------------------------------------------------------------
// Dataset loading
// ---------------------------------------------------------------------------
static std::vector<TexelPos> load_verify_dataset(const std::string& path,
                                                 int max_positions = 0) {
    RowReader rows(path);
    std::vector<TexelPos> out;
    std::string fen, target;
    auto evaluator_ptr = std::make_unique<Evaluator>();
    Evaluator& evaluator = *evaluator_ptr;

    while (rows.next(fen, target)) {
        float result = 0.5f;
        if (!parse_target(target, result)) {
            rows.reject("Unknown result '" + target + "'");
            continue;
        }

        TexelPos tp;
        tp.result = result;
        if (auto r = tp.board.try_set_fen(fen); !r) {
            rows.reject("FEN error: " + std::string(r.error()));
            continue;
        }

        g_trace = {};
        g_rest = {};
        tp.score = evaluator.evaluate(tp.board);
        tp.trace = g_trace;

        out.push_back(std::move(tp));

        if (max_positions > 0 && static_cast<int>(out.size()) >= max_positions)
            break;
    }

    if (rows.skipped() > 0)
        std::cerr << "Skipped " << rows.skipped() << " malformed rows from " << path << ".\n";
    return out;
}

// The used rows of a file, after skipping its first `skip` used rows (so that
// the rows a fit did not read can be scored on their own: the residuals on
// unused train rows).
static TuneSet load_tune_dataset(const std::string& path,
                                 const std::vector<int>& active,
                                 int max_positions, long skip = 0) {
    RowReader rows(path);
    TuneSet out;
    out.active_count = static_cast<int>(active.size());
    if (active.size() > 65535) {
        std::cerr << "FATAL: " << active.size() << " active slots overflow the sparse index.\n";
        std::exit(2);
    }

    if (max_positions > 0) {
        out.result.reserve(static_cast<size_t>(max_positions));
        out.base_score.reserve(static_cast<size_t>(max_positions));
    }

    std::string fen, target;
    Board board;
    auto evaluator_ptr = std::make_unique<Evaluator>();
    Evaluator& evaluator = *evaluator_ptr;
    ModelCounts counts;

    while (rows.next(fen, target)) {
        float result = 0.5f;
        if (!parse_target(target, result)) {
            rows.reject("Unknown result '" + target + "'");
            continue;
        }

        if (auto r = board.try_set_fen(fen); !r) {
            rows.reject("FEN error: " + std::string(r.error()));
            continue;
        }

        g_trace = {};
        g_rest = {};
        const int score = evaluator.evaluate(board);
        RowModel model;
        std::string stage;
        const bool ok = model_row(board, score, model, &stage);
        counts.add(ok, model, stage, board);
        if (!ok || model.status != RowModel::USED)
            continue;
        if (skip > 0) {
            --skip;
            continue;
        }

        out.result.push_back(result);
        out.base_score.push_back(static_cast<float>(model.played_white));
        out.phase.push_back(static_cast<uint8_t>(g_trace.phase));
        for (size_t j = 0; j < active.size(); ++j)
            out.add_coefficient(static_cast<int>(j), static_cast<float>(row_coeff(model, active[j])));
        out.end_row();

        if (max_positions > 0 && static_cast<int>(out.size()) >= max_positions)
            break;
    }

    counts.print(path);
    if (counts.mismatches) {
        std::cerr << "FATAL: the tuner's model of the served score disagrees with the "
                     "engine on " << counts.mismatches << " rows; nothing is fitted.\n";
        std::exit(2);
    }
    if (out.size() == 0) {
        std::cerr << "No positions loaded from " << path << ".\n";
        std::exit(1);
    }

    if (rows.skipped() > 0) {
        std::cerr << "Skipped " << rows.skipped() << " malformed rows from " << path << ".\n";
    }

    return out;
}

// ---------------------------------------------------------------------------
// Reconstruction check
// ---------------------------------------------------------------------------
static void cmd_dump_eval(const std::string& path, int limit) {
    auto positions = load_verify_dataset(path, limit);
    if (positions.empty()) {
        std::cerr << "No positions loaded from " << path << "\n";
        std::exit(1);
    }
    // load_verify_dataset already evaluated every position, so nothing is
    // recomputed here. One line per position, IN INPUT ORDER, so the caller
    // can join it against the same CSV's FEN column.
    for (const auto& tp : positions) {
        const int white_pov = (tp.board.turn() == WHITE) ? tp.score : -tp.score;
        std::cout << white_pov << ' ' << tp.result << '\n';
    }
}

// The model against the engine, row by row (model_row): the traced terms times
// the weights, plus the frozen additions, must equal the running sum the
// evaluator accumulated itself, and each later stage must reproduce the
// evaluator's value, through to the score evaluate() served. The served score
// is never an input to the model, so the check can fail.
static void cmd_verify(const std::string& path, int limit) {
    std::cout << "Verifying the tuner's model on " << (limit > 0 ? "up to " + std::to_string(limit) : "all")
              << " positions from " << path << " ...\n";
    RowReader rows(path);
    auto ev_ptr = std::make_unique<Evaluator>();
    Evaluator& evaluator = *ev_ptr;
    Board board;
    std::string fen, target;
    ModelCounts counts;
    long read = 0;
    while (rows.next(fen, target)) {
        float result = 0.5f;
        if (!parse_target(target, result)) { rows.reject("Unknown result '" + target + "'"); continue; }
        if (auto r = board.try_set_fen(fen); !r) { rows.reject("FEN error: " + std::string(r.error())); continue; }
        g_trace = {};
        g_rest = {};
        const int score = evaluator.evaluate(board);
        RowModel model;
        std::string stage;
        const bool ok = model_row(board, score, model, &stage);
        counts.add(ok, model, stage, board);
        if (limit > 0 && ++read >= limit)
            break;
    }
    if (counts.used + counts.overrides + counts.verdicts + counts.mismatches == 0) {
        std::cerr << "No positions loaded.\n";
        std::exit(1);
    }
    counts.print(path);
    if (counts.mismatches) {
        std::cout << "FAIL: the model disagrees with the engine on " << counts.mismatches << " rows.\n";
        std::exit(1);
    }
    std::cout << "PASS: the model reproduces the engine on every row.\n";
}

// The played score, White's view, of each row a fit uses, one per line in
// file order (the rows --dump-coeffs writes, in the same order).
static void cmd_dump_played(const std::string& path, const std::string& out_path) {
    TuneSet set = load_tune_dataset(path, {}, 0);
    std::ofstream out(out_path);
    if (!out) {
        std::cerr << "Cannot write " << out_path << "\n";
        std::exit(1);
    }
    for (size_t i = 0; i < set.size(); ++i)
        out << static_cast<int>(set.base_score[i]) << "\n";
}

// Every slot, in order, with its group, index and value: the registry that
// tools/texel/surface.py builds the manifest from.
static void cmd_list_params() {
    const std::vector<Slot> all = slots();
    std::printf("index\tgroup\tsub\tseed\n");
    for (size_t i = 0; i < all.size(); ++i)
        std::printf("%zu\t%s\t%d\t%d\n", i, all[i].group, all[i].sub, *all[i].value);
}

// The rows a fit uses, in file order, and each one's coefficients: every
// slot's d(played score)/d(weight) at the seed. --dump-coeffs,
// --count-support and --dump-row-lines all walk them here, so the support a
// manifest freezes on is counted on exactly the rows and the definition the
// rank reads, and a row line names the coefficient row it belongs to. Stops
// after `limit` used rows when it is positive. `row_fn` receives the
// coefficients, the row's line in the file and the model.
template <class RowFn>
static void for_each_coeff_row(const std::string& path, int limit, RowFn&& row_fn) {
    RowReader rows(path);
    auto ev_ptr = std::make_unique<Evaluator>();
    Board board;
    std::string fen, target;
    ModelCounts counts;
    std::vector<double> coeff(EVAL_PARAM_FLAT_SIZE);
    long used = 0;
    while (rows.next(fen, target)) {
        if (auto r = board.try_set_fen(fen); !r) { rows.reject("FEN error: " + std::string(r.error())); continue; }
        g_trace = {};
        g_rest = {};
        const int score = ev_ptr->evaluate(board);
        RowModel model;
        std::string stage;
        const bool ok = model_row(board, score, model, &stage);
        counts.add(ok, model, stage, board);
        if (!ok || model.status != RowModel::USED)
            continue;
        for (int i = 0; i < EVAL_PARAM_FLAT_SIZE; ++i)
            coeff[static_cast<size_t>(i)] = row_coeff(model, i);
        row_fn(coeff, rows.lineno(), model);
        if (limit > 0 && ++used >= limit)
            break;
    }
    counts.print(path);
    if (counts.mismatches)
        std::exit(2);
}

// The fit's coefficient matrix on the rows a fit uses, float64 little-endian,
// one row of EVAL_PARAM_FLAT_SIZE after another. tools/texel/surface.py reads
// it for the rank of the free columns on a sample.
static void cmd_dump_coeffs(const std::string& path, const std::string& out_path, int limit) {
    std::ofstream out(out_path, std::ios::binary);
    if (!out) {
        std::cerr << "Cannot write " << out_path << "\n";
        std::exit(1);
    }
    for_each_coeff_row(path, limit, [&](const std::vector<double>& coeff, int, const RowModel&) {
        out.write(reinterpret_cast<const char*>(coeff.data()),
                  static_cast<std::streamsize>(coeff.size() * sizeof(double)));
    });
}

// The rows --dump-coeffs writes, one per line in the same order: the row's
// line in the file, the score the engine serves (White's view) and 1 when a
// release build serves it at the lazy checkpoint. A tool joins coefficient
// rows to its own CSV rows by the line, and checks the join by the score on
// the rows that --dump-eval's full evaluation also serves.
static void cmd_dump_row_lines(const std::string& path, const std::string& out_path, int limit) {
    std::ofstream out(out_path);
    if (!out) {
        std::cerr << "Cannot write " << out_path << "\n";
        std::exit(1);
    }
    for_each_coeff_row(path, limit, [&](const std::vector<double>&, int line, const RowModel& model) {
        out << line << ' ' << model.played_white << ' ' << (model.lazy ? 1 : 0) << '\n';
    });
}

// Each slot's support on the rows a fit uses: the number of rows whose
// coefficient (--dump-coeffs's) is nonzero, streamed, so a whole train split
// needs no dense matrix. Writes `rows N`, then `index count` for every slot.
// It reads the evaluation and changes no weight and no loss.
static void cmd_count_support(const std::string& path, const std::string& out_path, int limit) {
    std::vector<long> support(EVAL_PARAM_FLAT_SIZE, 0);
    long used = 0;
    for_each_coeff_row(path, limit, [&](const std::vector<double>& coeff, int, const RowModel&) {
        for (int i = 0; i < EVAL_PARAM_FLAT_SIZE; ++i)
            if (coeff[static_cast<size_t>(i)] != 0.0)
                ++support[static_cast<size_t>(i)];
        ++used;
    });
    std::ofstream out(out_path);
    if (!out) {
        std::cerr << "Cannot write " << out_path << "\n";
        std::exit(1);
    }
    out << "rows " << used << "\n";
    for (int i = 0; i < EVAL_PARAM_FLAT_SIZE; ++i)
        out << i << ' ' << support[static_cast<size_t>(i)] << "\n";
}

// The played score of every row, side to move, hashed in file order exactly as
// tests/eval_hash.cpp hashes what a release build serves (FNV-1a over each
// score's eight bytes). Equal hashes on one file are the check that the
// model's lazy path and endgame steps are the release engine's, row for row,
// overrides included.
static void cmd_played_hash(const std::string& path) {
    RowReader rows(path);
    auto ev_ptr = std::make_unique<Evaluator>();
    Board board;
    std::string fen, target;
    uint64_t h = 0xcbf29ce484222325ull;
    long n = 0;
    while (rows.next(fen, target)) {
        if (auto r = board.try_set_fen(fen); !r) { rows.reject("FEN error: " + std::string(r.error())); continue; }
        g_trace = {};
        g_rest = {};
        const int score = ev_ptr->evaluate(board);
        RowModel model;
        std::string stage;
        if (!model_row(board, score, model, &stage)) {
            std::cerr << "FATAL: the model disagrees with the engine (" << stage << ") at "
                      << board.get_fen() << "\n";
            std::exit(2);
        }
        const int64_t played = (board.turn() == WHITE) ? model.played_white : -model.played_white;
        for (int i = 0; i < 8; ++i) {
            h ^= static_cast<uint64_t>(played >> (8 * i)) & 0xffu;
            h *= 0x100000001b3ull;
        }
        ++n;
    }
    std::printf("positions: %ld\nplayed_hash: %016llx\n", n, static_cast<unsigned long long>(h));
}

// ---------------------------------------------------------------------------
// Feature-support diagnostic (Step 4.0). Counts, per flat parameter, how many
// loaded positions give it a nonzero linear trace (the positions that can
// supply gradient signal).
// Flags any *linearly-traced* param active in fewer than max(200, 0.05% N)
// positions. The king-safety index knobs are tuned by finite difference, so
// they carry no linear trace by design --
// they are reported separately as expected-zero, not flagged. Also closes the
// Phase-3 trace-regression item (d): every traced term must fire nonzero.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Sigmoid / loss helpers
// ---------------------------------------------------------------------------
static double sigmoid(double score, double K) {
    return 1.0 / (1.0 + std::exp(-K * score / 400.0));
}

static double default_loss(const TuneSet& set, const KSchedule& ks) {
    double loss = 0.0;
    const bool by_phase = ks.is_phase() && !set.phase.empty();
    for (size_t i = 0; i < set.size(); ++i) {
        const double k = by_phase ? ks.at(set.phase[i]) : ks.k_eg;
        double sig = sigmoid(static_cast<double>(set.base_score[i]), k);
        double diff = static_cast<double>(set.result[i]) - sig;
        loss += diff * diff;
    }
    return loss / static_cast<double>(set.size());
}

static double default_loss(const TuneSet& set, double K) {
    return default_loss(set, KSchedule::constant(K));
}

static double score_from_weights(const TuneSet& set,
                                 size_t pos,
                                 const std::vector<int>& active,
                                 const double* base_w,
                                 const double* w) {
    double score = static_cast<double>(set.base_score[pos]);
    for (size_t k = set.offset[pos]; k < set.offset[pos + 1]; ++k) {
        const int idx = active[set.index[k]];
        score += static_cast<double>(set.value[k]) * (w[idx] - base_w[idx]);
    }
    return score;
}

static double traced_loss(const TuneSet& set,
                          const std::vector<int>& active,
                          const double* base_w,
                          const double* w,
                          const KSchedule& ks) {
    double loss = 0.0;
    const bool by_phase = ks.is_phase() && !set.phase.empty();
    for (size_t i = 0; i < set.size(); ++i) {
        const double k = by_phase ? ks.at(set.phase[i]) : ks.k_eg;
        double sig = sigmoid(score_from_weights(set, i, active, base_w, w), k);
        double diff = static_cast<double>(set.result[i]) - sig;
        loss += diff * diff;
    }
    return loss / static_cast<double>(set.size());
}

// Bucketed holdout: use the same five material-phase boundaries as the Phase
// 9.11 corpus builder so a global drop cannot hide a domain regression.
static void report_phase_buckets(const TuneSet& set,
                                 const std::vector<int>& active,
                                 const double* base_w,
                                 const double* w,
                                 const KSchedule& ks) {
    double initial_loss[5] = {0, 0, 0, 0, 0};
    double tuned_loss[5] = {0, 0, 0, 0, 0};
    long   cnt[5]  = {0, 0, 0, 0, 0};
    for (size_t i = 0; i < set.size(); ++i) {
        int ph = set.phase.empty() ? 12 : set.phase[i];
        int b = (ph >= 20) ? 0
              : (ph >= 14) ? 1
              : (ph >= 8)  ? 2
              : (ph >= 3)  ? 3
                           : 4;
        const double ki = ks.at(ph);
        double initial_sig = sigmoid(score_from_weights(set, i, active, base_w, base_w), ki);
        double tuned_sig = sigmoid(score_from_weights(set, i, active, base_w, w), ki);
        double initial_diff = static_cast<double>(set.result[i]) - initial_sig;
        double tuned_diff = static_cast<double>(set.result[i]) - tuned_sig;
        initial_loss[b] += initial_diff * initial_diff;
        tuned_loss[b] += tuned_diff * tuned_diff;
        cnt[b]++;
    }
    static const char* names[5] = {
        "opening      ", "early_mid    ", "middlegame   ",
        "endgame     ", "deep_endgame"
    };
    std::cout << "Holdout loss by phase (initial -> tuned):\n";
    for (int b = 0; b < 5; ++b)
        std::printf("  %s  %.8f -> %.8f  delta=%+.8f  (n=%ld)\n",
                    names[b],
                    cnt[b] ? initial_loss[b] / static_cast<double>(cnt[b]) : 0.0,
                    cnt[b] ? tuned_loss[b] / static_cast<double>(cnt[b]) : 0.0,
                    cnt[b] ? (tuned_loss[b] - initial_loss[b]) / static_cast<double>(cnt[b]) : 0.0,
                    cnt[b]);
}

// ---------------------------------------------------------------------------
// K calibration (golden-section search and phase-linear fit on validation)
// ---------------------------------------------------------------------------
// A K at the edge of its search interval is not a fit: the loss kept falling
// past the bound, so the scores and the labels disagree in scale.
static constexpr double K_LO = 0.3, K_HI = 5.0;
static void check_K_inside_bounds(double K) {
    if (K < K_LO + 1e-4 || K > K_HI - 1e-4) {
        std::cerr << "FATAL: K = " << K << " is at the bound of [" << K_LO << ", " << K_HI
                  << "]; the scores and the labels disagree in scale.\n";
        std::exit(2);
    }
}

static void check_K_schedule_inside_bounds(const KSchedule& ks) {
    check_K_inside_bounds(ks.k_eg);
    check_K_inside_bounds(ks.k_mg);
}

static double fit_K(const TuneSet& positions) {
    std::cout << "Fitting K on " << positions.size() << " positions... ";
    double lo = K_LO;
    double hi = K_HI;
    for (int i = 0; i < 50; i++) {
        double m1 = lo + (hi - lo) / 3.0;
        double m2 = hi - (hi - lo) / 3.0;
        if (default_loss(positions, m1) < default_loss(positions, m2))
            hi = m2;
        else
            lo = m1;
    }
    double K = (lo + hi) / 2.0;
    std::cout << "K = " << K << "\n";
    check_K_inside_bounds(K);
    return K;
}

static KSchedule fit_K_phase(const TuneSet& positions) {
    std::cout << "Fitting linear K(phase) on " << positions.size() << " positions... ";
    double best_loss = std::numeric_limits<double>::infinity();
    KSchedule best{2.0, 2.0};
    constexpr int GRID = 25;
    for (int i = 0; i < GRID; ++i) {
        const double keg = K_LO + (K_HI - K_LO) * static_cast<double>(i) / static_cast<double>(GRID - 1);
        for (int j = 0; j < GRID; ++j) {
            const double kmg = K_LO + (K_HI - K_LO) * static_cast<double>(j) / static_cast<double>(GRID - 1);
            const KSchedule cand{keg, kmg};
            const double loss = default_loss(positions, cand);
            if (loss < best_loss) {
                best_loss = loss;
                best = cand;
            }
        }
    }
    static constexpr double STEPS[] = {0.2, 0.05, 0.01, 0.002, 0.0005, 0.0001, 0.00002, 0.000005};
    static constexpr int DIRS[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}};
    for (double step : STEPS) {
        for (;;) {
            bool improved = false;
            for (const auto& dir : DIRS) {
                const KSchedule cand{
                    std::clamp(best.k_eg + dir[0] * step, K_LO, K_HI),
                    std::clamp(best.k_mg + dir[1] * step, K_LO, K_HI)
                };
                if (cand.k_eg == best.k_eg && cand.k_mg == best.k_mg)
                    continue;
                const double loss = default_loss(positions, cand);
                if (loss < best_loss - 1e-13) {
                    best_loss = loss;
                    best = cand;
                    improved = true;
                }
            }
            if (!improved)
                break;
        }
    }
    std::cout << "K_eg = " << best.k_eg << ", K_mg = " << best.k_mg << "\n";
    check_K_schedule_inside_bounds(best);
    return best;
}

static int phase_band(int phase) {
    return (phase >= 20) ? 0 : (phase >= 14) ? 1 : (phase >= 8) ? 2 : (phase >= 3) ? 3 : 4;
}

// The loss of the played score at the current vector (--init), on a file's
// used rows after its first --skip rows: K fitted on it unless --K or
// --K-phase freezes it (or --fit-k-phase fits K_eg, K_mg), then K fitted again
// in each phase band and the loss under the schedule, by band.
static void cmd_loss(const std::string& path, double frozen_K, const KSchedule& frozen_K_phase,
                     bool fit_phase_k, long skip, int max_positions) {
    TuneSet set = load_tune_dataset(path, {}, max_positions, skip);
    KSchedule ks;
    if (frozen_K_phase.active()) {
        ks = frozen_K_phase;
        check_K_schedule_inside_bounds(ks);
    } else if (fit_phase_k) {
        ks = fit_K_phase(set);
    } else if (frozen_K > 0.0) {
        check_K_inside_bounds(frozen_K);
        ks = KSchedule::constant(frozen_K);
    } else {
        ks = KSchedule::constant(fit_K(set));
    }
    if (frozen_K_phase.active() || fit_phase_k)
        std::printf("rows: %zu\nK_eg: %.6f\nK_mg: %.6f\nloss: %.8f\n",
                    set.size(), ks.k_eg, ks.k_mg, default_loss(set, ks));
    else
        std::printf("rows: %zu\nK: %.6f\nloss: %.8f\n",
                    set.size(), ks.k_eg, default_loss(set, ks));
    static const char* names[5] = {"opening", "early_mid", "middlegame", "endgame", "deep_endgame"};
    for (int band = 0; band < 5; ++band) {
        TuneSet part;
        for (size_t i = 0; i < set.size(); ++i) {
            if (phase_band(set.phase[i]) != band)
                continue;
            part.result.push_back(set.result[i]);
            part.base_score.push_back(set.base_score[i]);
            part.phase.push_back(set.phase[i]);
            part.end_row();
        }
        if (part.size() < 100) {
            std::printf("band %s: rows %zu (too few to fit K)\n", names[band], part.size());
            continue;
        }
        double lo = K_LO, hi = K_HI;
        for (int i = 0; i < 50; ++i) {
            const double m1 = lo + (hi - lo) / 3.0, m2 = hi - (hi - lo) / 3.0;
            if (default_loss(part, m1) < default_loss(part, m2)) hi = m2; else lo = m1;
        }
        std::printf("band %s: rows %zu  K %.6f  loss at K %.8f  loss at its own K %.8f\n",
                    names[band], part.size(), (lo + hi) / 2.0, default_loss(part, ks),
                    default_loss(part, (lo + hi) / 2.0));
    }
}

// ---------------------------------------------------------------------------
// Output helpers
// ---------------------------------------------------------------------------
static void write_eval_params_file(const std::string& out_path) {
    std::filesystem::path path(out_path);
    if (!path.parent_path().empty())
        std::filesystem::create_directories(path.parent_path());

    std::ofstream out(out_path);
    if (!out) {
        std::cerr << "Cannot write " << out_path << "\n";
        std::exit(1);
    }
    out << "# basilisk-texel-vector-v1\n"
        << "# root " << g_root_hash << "\n"
        << "# seed " << g_seed_hash << "\n";

#define X(name, member, len) \
    { const int* ptr = eval_param_cptr(g_eval_params.member); \
      for (int i = 0; i < (len); i++) out << #name << " " << i << " " << ptr[i] << "\n"; }
    EVAL_PARAM_LIST(X)
#undef X
}

static void print_material_delta(const double* base_w, const double* w) {
    static const char* names[] = {"-", "Pawn", "Knight", "Bishop", "Rook", "Queen", "King"};
    int mg = eval_param_offset(EPG_MgVal);
    int eg = eval_param_offset(EPG_EgVal);

    std::cout << "\nMaterial values (rounded output):\n";
    std::cout << "Piece     MG old -> new    EG old -> new\n";
    for (int pt = PAWN; pt <= QUEEN; ++pt) {
        int mg_old = static_cast<int>(std::round(base_w[mg + pt]));
        int mg_new = static_cast<int>(std::round(w[mg + pt]));
        int eg_old = static_cast<int>(std::round(base_w[eg + pt]));
        int eg_new = static_cast<int>(std::round(w[eg + pt]));
        std::printf("%-8s %5d -> %-5d  %5d -> %-5d\n",
                    names[pt], mg_old, mg_new, eg_old, eg_new);
    }
}

struct FlatParamInfo {
    const char* name;
    int index;
};

static std::vector<FlatParamInfo> flat_param_info() {
    std::vector<FlatParamInfo> info;
    info.reserve(EVAL_PARAM_FLAT_SIZE);
#define X(name, member, len) \
    for (int i = 0; i < (len); ++i) info.push_back({#name, i});
    EVAL_PARAM_LIST(X)
#undef X
    return info;
}

static void print_active_deltas(const std::vector<int>& active,
                                const double* base_w,
                                const double* w) {
    static const std::vector<FlatParamInfo> info = flat_param_info();

    int changed = 0;
    for (int idx : active) {
        int old_v = static_cast<int>(std::round(base_w[idx]));
        int new_v = static_cast<int>(std::round(w[idx]));
        if (old_v != new_v)
            ++changed;
    }

    std::cout << "\nActive parameter deltas (rounded output): "
              << changed << " changed / " << active.size() << " active\n";
    if (changed == 0)
        return;

    std::cout << "Param                  Idx  Old -> New   Delta\n";
    int printed = 0;
    for (int idx : active) {
        int old_v = static_cast<int>(std::round(base_w[idx]));
        int new_v = static_cast<int>(std::round(w[idx]));
        if (old_v == new_v)
            continue;
        const FlatParamInfo& param = info[static_cast<size_t>(idx)];
        std::printf("%-22s %3d  %4d -> %-4d  %+d\n",
                    param.name, param.index, old_v, new_v, new_v - old_v);
        if (++printed >= 120 && changed > printed) {
            std::cout << "... " << (changed - printed)
                      << " additional changed params omitted\n";
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Adam optimizer using active trace coefficients
// ---------------------------------------------------------------------------
static void cmd_tune(const TuneOptions& opts) {
    constexpr double BETA1 = 0.9;
    constexpr double BETA2 = 0.999;
    constexpr double EPS = 1e-8;

    std::vector<int> active = active_indices_for_group(opts.group);

    std::cout << "Tune group: " << opts.group
              << " (" << active.size() << " active params)\n";

    std::cout << "Loading train dataset from " << opts.train_path << " ...\n";
    TuneSet train = load_tune_dataset(opts.train_path, active, opts.max_positions);
    std::cout << "Loaded " << train.size() << " train positions.\n";

    std::cout << "Loading holdout dataset from " << opts.holdout_path << " ...\n";
    TuneSet holdout = load_tune_dataset(opts.holdout_path, active, opts.max_positions);
    std::cout << "Loaded " << holdout.size() << " holdout positions.\n";

    // K is fitted once, on validation at the seed, and frozen across every
    // stage and arm (--K or --K-phase); fitting it again per run moves the
    // target the stages are compared on.
    KSchedule ks;
    if (opts.K_phase.active()) {
        ks = opts.K_phase;
        check_K_schedule_inside_bounds(ks);
        std::cout << "K frozen at K(phase): K_eg=" << ks.k_eg << ", K_mg=" << ks.k_mg << ".\n";
    } else if (opts.K > 0.0) {
        check_K_inside_bounds(opts.K);
        ks = KSchedule::constant(opts.K);
        std::cout << "K frozen at " << ks.k_eg << ".\n";
    } else {
        ks = KSchedule::constant(fit_K(holdout));
    }

    double base_w[EVAL_PARAM_FLAT_SIZE];
    double w[EVAL_PARAM_FLAT_SIZE];
    read_weights(g_eval_params, base_w);
    std::copy(base_w, base_w + EVAL_PARAM_FLAT_SIZE, w);

    // The second-difference penalty (--smooth, off by default): for each run
    // of three consecutive active entries of an ordered table -- the mobility
    // tables by count, the passer and connected-pawn tables by rank -- the
    // square of w[i-1] - 2 w[i] + w[i+1], so a fit cannot leave a sawtooth in
    // a term that should be smooth.
    std::vector<std::array<int, 3>> triples;
    if (opts.smooth > 0.0) {
        std::vector<char> on(EVAL_PARAM_FLAT_SIZE, 0);
        for (int idx : active)
            on[static_cast<size_t>(idx)] = 1;
        // The tables ordered by one count: mobility and the
        // passer's rank.
        const int ordered[] = {EPG_MobNMg, EPG_MobNEg, EPG_MobBMg, EPG_MobBEg, EPG_MobRMg, EPG_MobREg,
                               EPG_MobQMg, EPG_MobQEg, EPG_PassedRankMg, EPG_PassedRankEg};
        for (int g : ordered) {
            const int base = eval_param_offset(g);
            for (int i = 1; i + 1 < EVAL_PARAM_LENS[g]; ++i) {
                const int a = base + i - 1, m = base + i, c = base + i + 1;
                if (on[static_cast<size_t>(a)] && on[static_cast<size_t>(m)] && on[static_cast<size_t>(c)])
                    triples.push_back({a, m, c});
            }
        }
        std::cout << "Smoothness penalty " << opts.smooth << " on " << triples.size() << " triples.\n";
    }
    auto smooth_penalty = [&](const double* x) {
        double sum = 0.0;
        for (const auto& t : triples) {
            const double d = x[t[0]] - 2.0 * x[t[1]] + x[t[2]];
            sum += d * d;
        }
        return opts.smooth * sum;
    };

    // The clamps are priors on the fitted coordinates. The seed must satisfy
    // them, or the first clamp would move it with no data asking; and a
    // coordinate outside the group stays at the seed whatever a clamp says.
    {
        double probe[EVAL_PARAM_FLAT_SIZE];
        std::copy(base_w, base_w + EVAL_PARAM_FLAT_SIZE, probe);
        clamp_weights_for_group(opts.group, probe);
        int moved = 0;
        const auto info = flat_param_info();
        for (int i = 0; i < EVAL_PARAM_FLAT_SIZE; ++i) {
            if (probe[i] != base_w[i] && ++moved <= 10)
                std::cerr << "  the clamps move the seed's " << info[static_cast<size_t>(i)].name
                          << " from " << base_w[i] << " to " << probe[i] << "\n";
        }
        if (moved) {
            std::cerr << "FATAL: the clamps move " << moved << " seed coordinates.\n";
            std::exit(2);
        }
    }
    std::vector<char> is_active(EVAL_PARAM_FLAT_SIZE, 0);
    for (int idx : active)
        is_active[static_cast<size_t>(idx)] = 1;
    std::vector<char> held(EVAL_PARAM_FLAT_SIZE, 1);
    for (int idx : active)
        held[static_cast<size_t>(idx)] = 0;
    // A `mean:<set>` anchor holds the mean of its set at the seed: a set whose
    // level the data cannot see (each king PST, each mobility table) moves
    // only in shape. Its members are all free linear slots.
    std::vector<std::vector<int>> mean_sets;
    if (opts.group == "surface") {
        const std::vector<SurfaceRow> rows = load_surface(g_surface_path);
        std::map<std::string, std::vector<int>> sets;
        for (size_t i = 0; i < rows.size(); ++i) {
            if (rows[i].anchor.rfind("mean:", 0) != 0)
                continue;
            if (rows[i].stage != "linear") {
                std::cerr << "FATAL: anchor " << rows[i].anchor << " names a slot the linear stage does not fit.\n";
                std::exit(2);
            }
            sets[rows[i].anchor].push_back(static_cast<int>(i));
        }
        for (auto& [name, members] : sets)
            mean_sets.push_back(members);
        std::cout << "Surface: " << active.size() << " free linear slots, " << mean_sets.size()
                  << " mean anchors.\n";
    }
    auto project_anchors = [&](double* x) {
        for (const std::vector<int>& set : mean_sets) {
            double shift = 0.0;
            for (int i : set)
                shift += x[i] - base_w[i];
            shift /= static_cast<double>(set.size());
            for (int i : set)
                x[i] -= shift;
        }
    };
    auto l2_penalty = [&](const double* x) {
        double sum = 0.0;
        for (int idx : active)
            sum += (x[idx] - base_w[idx]) * (x[idx] - base_w[idx]);
        return opts.l2 * sum;
    };

    double initial_train_loss = traced_loss(train, active, base_w, w, ks);
    double initial_holdout_loss = traced_loss(holdout, active, base_w, w, ks);
    std::cout << "Initial train loss  = " << initial_train_loss << "\n";
    std::cout << "Initial holdout loss= " << initial_holdout_loss << "\n";

    double best_w[EVAL_PARAM_FLAT_SIZE];
    std::copy(w, w + EVAL_PARAM_FLAT_SIZE, best_w);
    double best_holdout = initial_holdout_loss;
    int best_epoch = 0;
    int last_epoch = 0;
    // Patience reads gains above --min-delta: the loss and epoch of the last
    // such gain. With --min-delta 0 they are the best loss and its epoch.
    double gain_holdout = initial_holdout_loss;
    int gain_epoch = 0;
    bool stopped_by_patience = false;

    std::vector<double> grad(active.size(), 0.0);
    std::vector<double> m(active.size(), 0.0);
    std::vector<double> v(active.size(), 0.0);
    int t = 0;
    const double n = static_cast<double>(train.size());
    const bool by_phase = ks.is_phase() && !train.phase.empty();

    for (int epoch = 1; epoch <= opts.epochs; epoch++) {
        auto t0 = std::chrono::steady_clock::now();
        std::fill(grad.begin(), grad.end(), 0.0);

        for (size_t i = 0; i < train.size(); ++i) {
            const double ki = by_phase ? ks.at(train.phase[i]) : ks.k_eg;
            double score = score_from_weights(train, i, active, base_w, w);
            double sig = sigmoid(score, ki);
            double err = static_cast<double>(train.result[i]) - sig;
            double dsig = sig * (1.0 - sig);
            double coeff = -2.0 * err * dsig * (ki / 400.0);

            for (size_t k = train.offset[i]; k < train.offset[i + 1]; ++k)
                grad[train.index[k]] += coeff * static_cast<double>(train.value[k]);
        }

        ++t;
        double bc1 = 1.0 - std::pow(BETA1, static_cast<double>(t));
        double bc2 = 1.0 - std::pow(BETA2, static_cast<double>(t));

        std::vector<double> smooth_grad;
        if (!triples.empty()) {
            smooth_grad.assign(EVAL_PARAM_FLAT_SIZE, 0.0);
            for (const auto& tri : triples) {
                const double d = w[tri[0]] - 2.0 * w[tri[1]] + w[tri[2]];
                smooth_grad[static_cast<size_t>(tri[0])] += 2.0 * opts.smooth * d;
                smooth_grad[static_cast<size_t>(tri[1])] -= 4.0 * opts.smooth * d;
                smooth_grad[static_cast<size_t>(tri[2])] += 2.0 * opts.smooth * d;
            }
        }
        for (size_t j = 0; j < active.size(); ++j) {
            // Data gradient + optional L2-to-prior (pull toward the default/base
            // weights; a guard for sparse/suspect terms, off by default) + the
            // optional smoothness penalty.
            double g = grad[j] / n;
            if (opts.l2 > 0.0)
                g += 2.0 * opts.l2 * (w[active[j]] - base_w[active[j]]);
            if (!smooth_grad.empty())
                g += smooth_grad[static_cast<size_t>(active[j])];
            m[j] = BETA1 * m[j] + (1.0 - BETA1) * g;
            v[j] = BETA2 * v[j] + (1.0 - BETA2) * g * g;
            double m_hat = m[j] / bc1;
            double v_hat = v[j] / bc2;
            w[active[j]] -= opts.lr * m_hat / (std::sqrt(v_hat) + EPS);
        }

        // Slots outside the fitted set stay at the seed; the anchors hold
        // their sets' means; and the clamps, which never move a held slot,
        // have the last word, so every vector a stage writes satisfies them.
        for (int i = 0; i < EVAL_PARAM_FLAT_SIZE; ++i)
            if (!is_active[static_cast<size_t>(i)])
                w[i] = base_w[i];
        project_anchors(w);
        g_clamp_held = &held;
        clamp_weights_for_group(opts.group, w);
        g_clamp_held = nullptr;

        auto t1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        double holdout_loss = traced_loss(holdout, active, base_w, w, ks);
        if (holdout_loss < best_holdout) {
            best_holdout = holdout_loss;
            best_epoch = epoch;
            std::copy(w, w + EVAL_PARAM_FLAT_SIZE, best_w);
        }
        if (holdout_loss < gain_holdout - opts.min_delta) {
            gain_holdout = holdout_loss;
            gain_epoch = epoch;
        }
        last_epoch = epoch;

        if (epoch == 1 || epoch % 10 == 0 || epoch == opts.epochs) {
            // The data loss, and the objective the optimiser descends: the
            // data loss plus the penalties when they are on.
            double train_loss = traced_loss(train, active, base_w, w, ks);
            if (opts.l2 > 0.0 || opts.smooth > 0.0)
                std::printf("Epoch %4d  train=%.8f  objective=%.8f  holdout=%.8f  %.0f ms\n",
                            epoch, train_loss, train_loss + l2_penalty(w) + smooth_penalty(w),
                            holdout_loss, ms);
            else
                std::printf("Epoch %4d  train=%.8f  holdout=%.8f  %.0f ms\n",
                            epoch, train_loss, holdout_loss, ms);
        }
        // Converged: the holdout has not improved for `patience` epochs.
        if (opts.patience > 0 && epoch - gain_epoch >= opts.patience) {
            std::cout << "Stopped at epoch " << epoch << ": no holdout gain";
            if (opts.min_delta > 0.0)
                std::cout << " above " << opts.min_delta;
            std::cout << " for " << opts.patience << " epochs.\n";
            stopped_by_patience = true;
            break;
        }
    }

    // A run whose holdout was still improving when its epochs ran out, or in
    // which a slot travelled as far as the optimiser could carry it, stopped
    // on the schedule, not on the data: full-batch Adam moves a one-signed
    // coordinate about lr an epoch, so lr x epochs bounds every move.
    {
        double travel = 0.0;
        for (int idx : active)
            travel = std::max(travel, std::abs(best_w[idx] - base_w[idx]));
        const double bound = opts.lr * static_cast<double>(last_epoch);
        std::cout << "Largest move " << travel << " of the schedule's bound " << bound
                  << " (lr " << opts.lr << " x " << last_epoch << " epochs).\n";
        if (!stopped_by_patience && best_epoch == last_epoch && best_epoch > 0) {
            std::cerr << "FATAL: the holdout was still improving at the last epoch, " << last_epoch
                      << "; the run was stopped by its schedule, not converged.\n";
            std::exit(3);
        }
        if (travel >= 0.9 * bound) {
            std::cerr << "FATAL: a slot moved " << travel << ", 0.9 or more of the bound " << bound
                      << "; the schedule, not the data, set how far it went.\n";
            std::exit(3);
        }
    }

    if (best_epoch != last_epoch) {
        std::copy(best_w, best_w + EVAL_PARAM_FLAT_SIZE, w);
        std::cout << "Restored best holdout epoch " << best_epoch
                  << " (holdout=" << best_holdout << ").\n";
    } else {
        std::cout << "Best holdout was final epoch " << best_epoch
                  << " (holdout=" << best_holdout << ").\n";
    }

    report_phase_buckets(holdout, active, base_w, w, ks);

    for (int i = 0; i < EVAL_PARAM_FLAT_SIZE; ++i) {
        if (!is_active[static_cast<size_t>(i)] && w[i] != base_w[i]) {
            std::cerr << "FATAL: coordinate " << i << " is outside the fitted group and moved.\n";
            std::exit(2);
        }
    }

    write_weights(g_eval_params, w);
    print_material_delta(base_w, w);
    print_active_deltas(active, base_w, w);
    write_eval_params_file(opts.out_path);
    std::cout << "Tuned weights written to " << opts.out_path << "\n";
}

// ---------------------------------------------------------------------------
// King-safety finite-difference tuner (Step 4.0).
//
// The KS funnel knobs shape the integer danger index that looks up
// safety_table, so the linear trace is structurally blind to them
// (feature-support confirms 0 activation). This path re-evaluates the dataset
// with perturbed weights and does integer coordinate descent on the sigmoid
// MSE, keeping safety_table non-decreasing. evaluate() reads only the position
// fields snapshotted below (pawn_key is used only in the non-TEXEL cache path,
// which this TEXEL build bypasses), so we store a compact snapshot and restore
// it into one reused Board rather than keeping millions of heavy Board objects.
// ---------------------------------------------------------------------------
struct KsSnap {
    Board::PositionSnapshot position;
    Color    stm;
    uint8_t  phase;
    float    result;
};

static void ks_board_from_snap(Board& b, const KsSnap& s) {
    b.restore_position(s.position);
}

static std::vector<KsSnap> ks_load(const std::string& path, int max_positions) {
    RowReader rows(path);
    std::vector<KsSnap> out;
    Board tmp;
    std::string fen, target;
    auto ev_ptr = std::make_unique<Evaluator>();
    ModelCounts counts;
    while (rows.next(fen, target)) {
        float result;
        if (!parse_target(target, result)) { rows.reject("Unknown result '" + target + "'"); continue; }
        if (!tmp.try_set_fen(fen)) { rows.reject("FEN error"); continue; }
        // The same rows as the linear fit's: an override or a verdict depends
        // on the material alone, which no king-safety knob moves.
        g_trace = {};
        g_rest = {};
        const int score = ev_ptr->evaluate(tmp);
        RowModel model;
        std::string stage;
        const bool ok = model_row(tmp, score, model, &stage);
        counts.add(ok, model, stage, tmp);
        if (!ok || model.status != RowModel::USED) continue;
        KsSnap s{tmp.snapshot_position(), tmp.turn(), static_cast<uint8_t>(g_trace.phase), result};
        out.push_back(s);
        if (max_positions > 0 && static_cast<int>(out.size()) >= max_positions) break;
    }
    counts.print(path);
    if (counts.mismatches) {
        std::cerr << "FATAL: the tuner's model of the served score disagrees with the engine.\n";
        std::exit(2);
    }
    if (out.empty()) { std::cerr << "No positions from " << path << "\n"; std::exit(1); }
    if (rows.skipped()) std::cerr << "Skipped " << rows.skipped() << " rows.\n";
    return out;
}

static double ks_mse(const std::vector<KsSnap>& pos, const KSchedule& ks, Board& scratch, Evaluator& ev) {
    double loss = 0.0;
    const bool by_phase = ks.is_phase();
    for (const auto& s : pos) {
        ks_board_from_snap(scratch, s);
        g_rest = {};
        const int sc = ev.evaluate(scratch);
        // The traced build serves what a release build serves.
        const int white = (s.stm == WHITE) ? sc : -sc;
        const double ki = by_phase ? ks.at(s.phase) : ks.k_eg;
        double sig = 1.0 / (1.0 + std::exp(-ki * static_cast<double>(white) / 400.0));
        double d = static_cast<double>(s.result) - sig;
        loss += d * d;
    }
    return loss / static_cast<double>(pos.size());
}

static double ks_fit_K(const std::vector<KsSnap>& pos, Board& scratch, Evaluator& ev) {
    double lo = K_LO, hi = K_HI;
    for (int i = 0; i < 40; ++i) {
        double m1 = lo + (hi - lo) / 3.0, m2 = hi - (hi - lo) / 3.0;
        if (ks_mse(pos, KSchedule::constant(m1), scratch, ev)
            < ks_mse(pos, KSchedule::constant(m2), scratch, ev)) hi = m2; else lo = m1;
    }
    const double K = (lo + hi) / 2.0;
    check_K_inside_bounds(K);
    return K;
}

// ---------------------------------------------------------------------------
// Coverage audit: prove every fittable parameter has an instrument.
//
// A fit once reported how many slots its group freed without checking what the
// group covered; every PST was silently frozen, unnoticed until BAS-E16. This
// enumerates EVERY slot in the
// registry and reports which instrument reaches it, so that an exclusion has to
// be a decision someone made rather than an oversight nobody saw.
// ---------------------------------------------------------------------------

static void cmd_tune_kingsafety(int argc, char* argv[]) {
    if (argc < 4) { usage(argv[0]); std::exit(1); }
    std::string train_path = argv[2], holdout_path = argv[3];
    std::string out_path = "tools/texel/out/kingsafety.txt";
    int epochs = 0, max_positions = 0, init_step = 8;
    double frozen_K = 0.0;
    KSchedule frozen_K_phase;
    int argi = 4;
    if (argi < argc && !is_option(argv[argi])) out_path = argv[argi++];
    while (argi < argc) {
        std::string o = argv[argi++];
        if (o == "--epochs" && argi < argc) epochs = parse_int_arg(argv[argi++], "--epochs");
        else if (o == "--max-positions" && argi < argc) max_positions = parse_int_arg(argv[argi++], "--max-positions");
        else if (o == "--step" && argi < argc) init_step = parse_int_arg(argv[argi++], "--step");
        else if (o == "--K" && argi < argc) frozen_K = parse_double_arg(argv[argi++], "--K");
        else if (o == "--K-phase" && argi < argc) frozen_K_phase = parse_k_phase_arg(argv[argi++], "--K-phase");
        else { std::cerr << "Unknown option " << o << "\n"; usage(argv[0]); std::exit(1); }
    }
    if (frozen_K > 0.0 && frozen_K_phase.active()) {
        std::cerr << "--K and --K-phase cannot both be set.\n";
        std::exit(1);
    }

    std::cout << "Loading train from " << train_path << " ...\n";
    auto train = ks_load(train_path, max_positions);
    std::cout << "Loaded " << train.size() << " train positions.\n";
    std::cout << "Loading holdout from " << holdout_path << " ...\n";
    auto holdout = ks_load(holdout_path, max_positions);
    std::cout << "Loaded " << holdout.size() << " holdout positions.\n";

    auto ev_ptr = std::make_unique<Evaluator>();
    Evaluator& ev = *ev_ptr;
    Board scratch;

    KSchedule ks;
    if (frozen_K_phase.active()) {
        ks = frozen_K_phase;
        check_K_schedule_inside_bounds(ks);
        std::cout << "K frozen at K(phase): K_eg=" << ks.k_eg << ", K_mg=" << ks.k_mg << ".\n";
    } else if (frozen_K > 0.0) {
        check_K_inside_bounds(frozen_K);
        ks = KSchedule::constant(frozen_K);
        std::cout << "K frozen at " << ks.k_eg << ".\n";
    } else {
        ks = KSchedule::constant(ks_fit_K(holdout, scratch, ev));
        std::cout << "K = " << ks.k_eg << "\n";
    }

    EvalParams& P = g_eval_params;
    (void)P;
    if (g_surface_path.empty()) {
        std::cerr << "FATAL: the knobs are the surface manifest's; pass --surface.\n";
        std::exit(2);
    }
    struct Knob { int* p; const char* name; int sub; int lo; int hi; int table_idx; int step; };
    std::vector<Knob> knobs;
    auto add = [&](int* p, const char* n, int sub, int lo, int hi, int ti) {
        knobs.push_back({p, n, sub, lo, hi, ti, init_step});
    };
    // With a surface manifest the knobs are its king-safety slots, each with
    // its own bounds and first step; each step halves on its own. Without one,
    // the older fixed list and one global step.
    const bool per_knob = !g_surface_path.empty();
    if (per_knob) {
        const std::vector<SurfaceRow> rows = load_surface(g_surface_path);
        const std::vector<Slot> all = slots();
        for (size_t i = 0; i < rows.size(); ++i) {
            if (rows[i].stage != "kingsafety")
                continue;
            const bool table = false;
            if (table && all[i].sub == 0) {
                std::cerr << "FATAL: safety_table[0] is the table's anchor and cannot be a knob.\n";
                std::exit(2);
            }
            knobs.push_back({all[i].value, all[i].group, all[i].sub, rows[i].lo, rows[i].hi,
                             table ? all[i].sub : -1, rows[i].step});
        }
        std::cout << "Surface: " << knobs.size() << " king-safety knobs.\n";
    } else {
    (void)add;
    }
    for (const Knob& kn : knobs) {
        if (*kn.p < kn.lo || *kn.p > kn.hi) {
            std::cerr << "FATAL: " << kn.name << "[" << kn.sub << "] = " << *kn.p
                      << " starts outside its bounds [" << kn.lo << ", " << kn.hi << "].\n";
            std::exit(2);
        }
    }

    std::vector<int> seed;
    for (auto& kn : knobs) seed.push_back(*kn.p);

    double best = ks_mse(train, ks, scratch, ev);
    double init_holdout = ks_mse(holdout, ks, scratch, ev);
    double best_holdout = init_holdout;
    int best_holdout_pass = 0;
    std::vector<int> best_holdout_values = seed;
    std::cout << "Initial train MSE = " << best << ", holdout MSE = " << init_holdout << "\n";
    std::cout << "Tuning " << knobs.size() << " king-safety knobs (coordinate descent)...\n";

    int step = init_step, pass = 0;
    auto live = [&] {
        if (!per_knob)
            return step >= 1;
        for (const Knob& kn : knobs)
            if (kn.step >= 1)
                return true;
        return false;
    };
    while (live()) {
        ++pass;
        bool improved = false;
        for (auto& kn : knobs) {
            const int kstep = per_knob ? kn.step : step;
            if (kstep < 1)
                continue;
            bool knob_improved = false;
            for (int dir = 0; dir < 2; ++dir) {
                int delta = (dir == 0) ? kstep : -kstep;
                int old = *kn.p;
                int lo = kn.lo, hi = kn.hi;
                int trial = std::clamp(old + delta, lo, hi);
                if (trial == old) continue;
                *kn.p = trial;
                double m = ks_mse(train, ks, scratch, ev);
                if (m < best - 1e-12) { best = m; improved = true; knob_improved = true; }
                else *kn.p = old;
            }
            if (per_knob && !knob_improved)
                kn.step /= 2;
        }
        double hold = ks_mse(holdout, ks, scratch, ev);
        if (per_knob)
            std::printf("pass %2d  train=%.8f  holdout=%.8f\n", pass, best, hold);
        else
            std::printf("pass %2d  step %2d  train=%.8f  holdout=%.8f\n", pass, step, best, hold);
        if (hold < best_holdout - 1e-12) {
            best_holdout = hold;
            best_holdout_pass = pass;
            for (size_t i = 0; i < knobs.size(); ++i)
                best_holdout_values[i] = *knobs[i].p;
        }
        if (!per_knob && !improved) step /= 2;
        if (epochs > 0 && pass >= epochs) break;
    }

    for (size_t i = 0; i < knobs.size(); ++i)
        *knobs[i].p = best_holdout_values[i];
    double final_train = ks_mse(train, ks, scratch, ev);
    double final_holdout = ks_mse(holdout, ks, scratch, ev);
    std::cout << "Restored best holdout pass " << best_holdout_pass
              << " (holdout=" << best_holdout << ").\n";
    std::cout << "\nFinal train MSE = " << final_train << ", holdout MSE = " << final_holdout
              << " (was " << init_holdout << ", delta " << (final_holdout - init_holdout) << ")\n";

    std::cout << "\nChanged king-safety knobs (old -> new):\n";
    for (size_t i = 0; i < knobs.size(); ++i) {
        if (*knobs[i].p != seed[i]) {
            if (knobs[i].sub) std::printf("  %s[%d]  %d -> %d\n", knobs[i].name, knobs[i].sub, seed[i], *knobs[i].p);
            else              std::printf("  %s  %d -> %d\n", knobs[i].name, seed[i], *knobs[i].p);
        }
    }

    write_eval_params_file(out_path);
    std::cout << "\nWrote tuned params to " << out_path << "\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables(g_eval_params);
    take_global_options(argc, argv);

    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "--list-params") {
        cmd_list_params();
    } else if (mode == "--loss") {
        if (argc < 3) {
            usage(argv[0]);
            return 1;
        }
        double K = 0.0;
        KSchedule K_phase;
        bool fit_phase_k = false;
        long skip = 0;
        int maxpos = 0;
        for (int a = 3; a < argc;) {
            const std::string o = argv[a++];
            if (o == "--fit-k-phase") {
                fit_phase_k = true;
                continue;
            }
            if (a >= argc) { usage(argv[0]); return 1; }
            if (o == "--K") K = parse_double_arg(argv[a++], "--K");
            else if (o == "--K-phase") K_phase = parse_k_phase_arg(argv[a++], "--K-phase");
            else if (o == "--skip") skip = parse_int_arg(argv[a++], "--skip");
            else if (o == "--max-positions") maxpos = parse_int_arg(argv[a++], "--max-positions");
            else { usage(argv[0]); return 1; }
        }
        if ((K > 0.0) + K_phase.active() + fit_phase_k > 1) {
            std::cerr << "--K, --K-phase and --fit-k-phase are mutually exclusive.\n";
            return 1;
        }
        cmd_loss(argv[2], K, K_phase, fit_phase_k, skip, maxpos);
    } else if (mode == "--dump-played") {
        if (argc != 4) {
            usage(argv[0]);
            return 1;
        }
        cmd_dump_played(argv[2], argv[3]);
    } else if (mode == "--dump-coeffs") {
        if (argc != 4 && !(argc == 6 && std::string(argv[4]) == "--max-positions")) {
            usage(argv[0]);
            return 1;
        }
        cmd_dump_coeffs(argv[2], argv[3], argc == 6 ? parse_int_arg(argv[5], "--max-positions") : 0);
    } else if (mode == "--count-support") {
        if (argc != 4 && !(argc == 6 && std::string(argv[4]) == "--max-positions")) {
            usage(argv[0]);
            return 1;
        }
        cmd_count_support(argv[2], argv[3], argc == 6 ? parse_int_arg(argv[5], "--max-positions") : 0);
    } else if (mode == "--dump-row-lines") {
        if (argc != 4 && !(argc == 6 && std::string(argv[4]) == "--max-positions")) {
            usage(argv[0]);
            return 1;
        }
        cmd_dump_row_lines(argv[2], argv[3], argc == 6 ? parse_int_arg(argv[5], "--max-positions") : 0);
    } else if (mode == "--verify") {
        if (argc != 3 && !(argc == 5 && std::string(argv[3]) == "--max-positions")) {
            usage(argv[0]);
            return 1;
        }
        // 10,000 rows by default, as before; --max-positions 0 reads the file.
        cmd_verify(argv[2], argc == 5 ? parse_int_arg(argv[4], "--max-positions") : 10000);
    } else if (mode == "--played-hash") {
        if (argc != 3) {
            usage(argv[0]);
            return 1;
        }
        cmd_played_hash(argv[2]);
    } else if (mode == "--feature-support") {
        if (argc < 3) {
            usage(argv[0]);
            return 1;
        }
        int maxpos = 0;
        for (int a = 3; a + 1 < argc; ++a)
            if (std::string(argv[a]) == "--max-positions")
                maxpos = parse_int_arg(argv[a + 1], "--max-positions");
        (void)maxpos;
        std::cerr << "--feature-support was the retired evaluation's; use "
                     "test_eval --coverage.\n";
        return 1;
    } else if (mode == "--tune") {
        TuneOptions opts = parse_tune_options(argc, argv);
        cmd_tune(opts);
    } else if (mode == "--dump-eval") {
        if (argc < 3) { usage(argv[0]); std::exit(1); }
        cmd_dump_eval(argv[2], argc > 3 ? std::atoi(argv[3]) : 0);
    } else if (mode == "--audit-coverage") {
        std::cerr << "--audit-coverage was the retired evaluation's; the "
                     "surface manifest classes every slot.\n";
        return 1;
    } else if (mode == "--tune-kingsafety") {
        cmd_tune_kingsafety(argc, argv);
    } else {
        std::cerr << "Unknown mode '" << mode << "'.\n";
        usage(argv[0]);
        return 1;
    }

    return 0;
}
