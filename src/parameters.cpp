#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <limits>
#include <regex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "board.h"
#include "constants.h"
#include "eval.h"
#include "parameters.h"
#include "uci_output.h"

namespace {

bool parse_bool_option(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value == "true" || value == "1" || value == "yes" || value == "on";
}

// 8.6.1: case-insensitive match of an already-lowercased option name against
// the mixed-case spelling stringized from the SearchParams X-macro table.
// Cold path (setoption only) — per-call tolower is fine.
[[maybe_unused]] bool uci_option_name_is(const std::string& name_lower, const char* spelling) {
    size_t i = 0;
    for (; spelling[i] != '\0'; ++i) {
        if (i >= name_lower.size()
            || name_lower[i] != static_cast<char>(std::tolower(static_cast<unsigned char>(spelling[i]))))
            return false;
    }
    return i == name_lower.size();
}

bool parse_i64(const std::string& value, int64_t& out) {
    try {
        size_t pos = 0;
        long long parsed = std::stoll(value, &pos);
        if (pos != value.size())
            return false;
        out = static_cast<int64_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

bool parse_int(const std::string& value, int& out) {
    int64_t parsed = 0;
    if (!parse_i64(value, parsed)
        || parsed < std::numeric_limits<int>::min()
        || parsed > std::numeric_limits<int>::max()) {
        return false;
    }
    out = static_cast<int>(parsed);
    return true;
}

bool apply_uci_move(Board& board, const std::string& token) {
    MoveList legal;
    board.gen_legal(legal);
    for (Move move : legal) {
        if (move_to_uci(move) == token) {
            board.make_move(move);
            return true;
        }
    }
    return false;
}

bool parse_legal_move(const Board& board, const std::string& token, Move& out) {
    MoveList legal;
    board.gen_legal(legal);
    for (Move move : legal) {
        if (move_to_uci(move) == token) {
            out = move;
            return true;
        }
    }
    out = MOVE_NONE;
    return false;
}

bool is_go_parameter(const std::string& token) {
    return token == "searchmoves"
        || token == "ponder"
        || token == "wtime"
        || token == "btime"
        || token == "winc"
        || token == "binc"
        || token == "movestogo"
        || token == "depth"
        || token == "nodes"
        || token == "mate"
        || token == "perft"
        || token == "movetime"
        || token == "infinite";
}

} // namespace

Parameters::Parameters() {
    board.set_fen(std::string(startPosition));

    move_time       = 0;
    white_time      = 0;
    white_increment = 0;
    black_time      = 0;
    black_increment = 0;
    depth          = infiniteDepth;
    movestogo      = 0;
    nodes          = 0;
    mate           = 0;
    perft          = 0;
    ponder         = false;
    search_moves.clear();

    move_overhead = defaultMoveOverhead;
    hash_mb      = 64;
    threads      = 1;
    ponder_enabled = false;
    syzygy_path.clear();
    syzygy_probe_depth = 1;
    syzygy_probe_limit = 7;
    syzygy_50_move_rule = true;
    tm_debug          = false;
    diag              = false;
}

void Parameters::reset() {
    board.set_fen(std::string(startPosition));
    new_game = true;
    reset_temporary_parameters();
}

void Parameters::reset_temporary_parameters() {
    move_time       = 0;
    white_time      = 0;
    white_increment = 0;
    black_time      = 0;
    black_increment = 0;
    depth          = infiniteDepth;
    movestogo      = 0;
    nodes          = 0;
    mate           = 0;
    perft          = 0;
    ponder         = false;
    search_moves.clear();
}

std::vector<std::string> Parameters::search_parameters() {
    return {"depth", "movetime", "wtime", "btime", "winc", "binc",
            "movestogo", "nodes", "mate", "perft"};
}

std::string Parameters::uci_options() {
    std::string opts =
           "option name Threads type spin default 1 min 1 max "
           + std::to_string(max_threads()) + "\n"
           "option name Hash type spin default 64 min 1 max 33554432\n"
           "option name Clear Hash type button\n"
           "option name Ponder type check default false\n"
           "option name Move Overhead type spin default 10 min 0 max 5000\n"
           "option name SyzygyPath type string default <empty>\n"
           "option name SyzygyProbeDepth type spin default 1 min 1 max 100\n"
           "option name Syzygy50MoveRule type check default true\n"
           "option name SyzygyProbeLimit type spin default 7 min 0 max 7\n";
#ifdef BASILISK_TUNE
    opts +=
        // TM diagnostic (Step 5.3): advertised only in tune/dev builds so a
        // harness/GUI will actually send the setoption (fastchess/LB skip
        // unadvertised options); release builds keep a clean 9-option list.
        "option name TM_Debug type check default false\n"
        // Diagnostic counters + lazy dual-eval audit (8.6.6).
        "option name Diag type check default false\n"
        // Atomic string keeps an SPSA/sweep harness from briefly installing
        // an unsafe partial KBNK vector while separate options arrive.
        "option name KBNK Drive type string default 17000,1000,0,220,0\n";
    // 8.6.1: generated from the SearchParams X-macro table — the advertised
    // default IS the compiled default by construction (the hand-written list
    // this replaces had drifted: PostLmrHistScale said 104, engine ran 0;
    // TmInstability was missing entirely).
#define BASILISK_SEARCH_PARAM_OPT(field, uci, def, lo, hi)                    \
    opts += "option name " #uci " type spin default " + std::to_string(def) + \
            " min " + std::to_string(lo) + " max " + std::to_string(hi) + "\n";
    BASILISK_SEARCH_PARAMS(BASILISK_SEARCH_PARAM_OPT)
#undef BASILISK_SEARCH_PARAM_OPT
#endif
    return opts;
}

// The advertised maximum must be exactly what the pool will actually accept,
// so both read the single definition in search.cpp (9.3a). Advertising a
// maximum the pool then silently clamps is a protocol lie; advertising one it
// would honour is a 2 GB allocation waiting for a curious operator.
int Parameters::max_threads() {
    return max_search_threads();
}

void Parameters::set_search_parameters(const std::string& args) {
    reset_temporary_parameters();

    if (args.empty()) {
        depth = defaultSearchDepth;
        return;
    }

    std::vector<std::string> tokens;
    std::istringstream iss(args);
    std::string token;
    while (iss >> token)
        tokens.push_back(token);

    bool infinite_requested = false;
    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string& parameter = tokens[i];

        if (parameter == "ponder") {
            ponder = true;
            continue;
        }
        if (parameter == "infinite") {
            infinite_requested = true;
            depth = infiniteDepth;
            continue;
        }
        if (parameter == "searchmoves") {
            while (i + 1 < tokens.size() && !is_go_parameter(tokens[i + 1])) {
                const std::string& move_token = tokens[++i];
                Move move = MOVE_NONE;
                if (parse_legal_move(board, move_token, move)) {
                    search_moves.push_back(move);
                } else {
                    uci_write_line("info string Invalid searchmoves move: " + move_token);
                    break;
                }
            }
            continue;
        }

        const auto params = Parameters::search_parameters();
        if (std::find(params.begin(), params.end(), parameter) == params.end())
            continue;
        if (i + 1 >= tokens.size() || is_go_parameter(tokens[i + 1])) {
            uci_write_line("info string Invalid " + parameter + " value.");
            continue;
        }

        set_search_parameter(parameter, tokens[++i]);
    }

    const bool has_limit = depth != infiniteDepth
        || move_time > 0 || white_time > 0 || black_time > 0
        || white_increment > 0 || black_increment > 0
        || movestogo > 0 || nodes > 0 || mate > 0 || perft > 0;
    if (!has_limit && !infinite_requested && !ponder)
        depth = defaultSearchDepth;
}

void Parameters::set_search_parameter(const std::string& parameter, const std::string& value) {
    if (parameter == "nodes") {
        int64_t parsed = 0;
        if (parse_i64(value, parsed))
            nodes = std::max<int64_t>(0, parsed);
        return;
    }

    int parsed = 0;
    if (!parse_int(value, parsed))
        return;

    if (parameter == "depth")     { depth          = std::clamp(parsed, 1, infiniteDepth); return; }
    if (parameter == "movetime")  { move_time       = std::max(0, parsed); return; }
    if (parameter == "wtime")     { white_time      = std::max(0, parsed); return; }
    if (parameter == "winc")      { white_increment = std::max(0, parsed); return; }
    if (parameter == "btime")     { black_time      = std::max(0, parsed); return; }
    if (parameter == "binc")      { black_increment = std::max(0, parsed); return; }
    if (parameter == "movestogo") { movestogo      = std::max(0, parsed); return; }
    if (parameter == "mate") {
        mate = std::max(0, parsed);
        if (mate > 0) {
            const int capped_mate = std::min(mate, infiniteDepth / 2);
            depth = std::clamp(2 * capped_mate - 1, 1, infiniteDepth);
        }
        return;
    }
    if (parameter == "perft") {
        perft = std::max(0, parsed);
        return;
    }
}

void Parameters::set_option(const std::string& args) {
    std::smatch matches;

    // Extract option name (everything between "name " and either " value" or end-of-string)
    std::string name;
    // Constructed once: std::regex compiles its pattern in the constructor, and
    // these were rebuilt on every setoption. Cold for a GUI, but an SPSA run
    // sends thousands (8.6.2b). Function-local statics are thread-safe to
    // initialise, and setoption is single-threaded regardless.
    static const std::regex name_value_re(R"(name (.*?) value)");
    static const std::regex name_only_re(R"(name (.*))");
    static const std::regex value_re(R"(value (.*))");

    // The two branches deliberately share one extraction body — a regex
    // fallback chain, not a clone bug.
    if (std::regex_search(args, matches, name_value_re))
        name = matches[1].str();  // NOLINT(bugprone-branch-clone)
    else if (std::regex_search(args, matches, name_only_re))
        name = matches[1].str();
    else {
        uci_write_line("info string Incorrect setoption format.");
        return;
    }

    // Trim trailing whitespace from name
    while (!name.empty() && name.back() == ' ') name.pop_back();

    std::string name_lower = name;
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    // Button-type options (no value)
    if (name_lower == "clear hash") {
        clear_hash = true;
        return;
    }

    // Extract value
    if (!std::regex_search(args, matches, value_re)) {
        uci_write_line("info string Option '" + name + "' requires a value.");
        return;
    }
    const std::string value = matches[1].str();

    int parsed = 0;
    if (name_lower == "syzygypath") {
        syzygy_path = (value == "<empty>") ? std::string{} : value;
    } else if (name_lower == "ponder") {
        ponder_enabled = parse_bool_option(value);
    } else if (name_lower == "syzygy50moverule") {
        syzygy_50_move_rule = parse_bool_option(value);
    } else if (name_lower == "diag") {
        // Diagnostic counters (8.6.6): advertised in TUNE builds, always
        // parseable. Enables the end-of-search `info string diag` dump and
        // the lazy dual-eval audit (served scores unchanged either way).
        diag = parse_bool_option(value);
    } else if (name_lower == "tm_debug") {
        // Diagnostic (Step 5.3): advertised only in tune/dev builds (see
        // uci_options) but always parseable. When on, the search emits one
        // `info string tm ...` per move with the time budget, actual elapsed,
        // and the go-receipt->search-start dispatch delta.
        tm_debug = parse_bool_option(value);
#ifdef BASILISK_TUNE
    } else if (name_lower == "kbnk drive") {
        std::string error;
        if (!set_kbnk_drive_weights(value, error))
            uci_write_line("info string Invalid KBNK Drive: " + error);
#endif
    } else if (!parse_int(value, parsed)) {
        uci_write_line("info string Invalid value for option '" + name + "': " + value);
    } else if (name_lower == "move overhead") {
        move_overhead = std::clamp(parsed, 0, 5000);
    } else if (name_lower == "hash") {
        hash_mb = std::clamp(parsed, 1, 33554432);
    } else if (name_lower == "threads") {
        threads = std::clamp(parsed, 1, max_threads());
    } else if (name_lower == "syzygyprobedepth") {
        syzygy_probe_depth = std::clamp(parsed, 1, 100);
    } else if (name_lower == "syzygyprobelimit") {
        syzygy_probe_limit = std::clamp(parsed, 0, 7);
    }
#ifdef BASILISK_TUNE
    // 8.6.1: generated from the SearchParams X-macro table — same single
    // source as the struct defaults and the uci_options() advertisement.
#define BASILISK_SEARCH_PARAM_SET(field, uci, def, lo, hi) \
    else if (uci_option_name_is(name_lower, #uci)) { search_params.field = std::clamp(parsed, lo, hi); }
    BASILISK_SEARCH_PARAMS(BASILISK_SEARCH_PARAM_SET)
#undef BASILISK_SEARCH_PARAM_SET
#endif
}

void Parameters::set_position(const std::string& args) {
    Board new_board;
    std::istringstream iss(args);
    std::string token;

    if (!(iss >> token)) {
        uci_write_line("info string Incorrect position format.");
        return;
    }

    bool moves_section = false;
    if (token == "startpos") {
        if (iss >> token) {
            if (token != "moves") {
                uci_write_line("info string Incorrect position format.");
                return;
            }
            moves_section = true;
        }
    } else if (token == "fen") {
        std::string fen;
        while (iss >> token) {
            if (token == "moves") {
                moves_section = true;
                break;
            }
            if (!fen.empty()) fen += ' ';
            fen += token;
        }

        if (fen.empty()) {
            uci_write_line("info string Invalid FEN. Missing FEN fields.");
            return;
        }

        if (auto r = new_board.try_set_fen(fen, /*validate_legal_position=*/true); !r) {
            uci_write_line("info string " + r.error());
            return;
        }
    } else {
        uci_write_line("info string Incorrect position format.");
        return;
    }

    if (moves_section) {
        while (iss >> token) {
            if (!apply_uci_move(new_board, token)) {
                uci_write_line("info string Illegal move: " + token);
                return;
            }
        }
    }

    board = std::move(new_board);
}
