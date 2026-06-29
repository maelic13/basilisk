#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <regex>
#include <sstream>
#include <string>
#include <thread>

#include "Board.h"
#include "Constants.h"
#include "Parameters.h"
#include "UciOutput.h"

namespace {

bool parse_bool_option(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value == "true" || value == "1" || value == "yes" || value == "on";
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

    moveTime       = 0;
    whiteTime      = 0;
    whiteIncrement = 0;
    blackTime      = 0;
    blackIncrement = 0;
    depth          = infiniteDepth;
    movestogo      = 0;
    nodes          = 0;
    mate           = 0;
    perft          = 0;
    ponder         = false;
    searchMoves.clear();

    moveOverhead = defaultMoveOverhead;
    hash_mb      = 64;
    threads      = 1;
    ponderEnabled = false;
    syzygyPath.clear();
    syzygyProbeDepth = 1;
    syzygyProbeLimit = 7;
    syzygy50MoveRule = true;
    tmDebug          = false;
}

void Parameters::reset() {
    board.set_fen(std::string(startPosition));
    new_game = true;
    resetTemporaryParameters();
}

void Parameters::resetTemporaryParameters() {
    moveTime       = 0;
    whiteTime      = 0;
    whiteIncrement = 0;
    blackTime      = 0;
    blackIncrement = 0;
    depth          = infiniteDepth;
    movestogo      = 0;
    nodes          = 0;
    mate           = 0;
    perft          = 0;
    ponder         = false;
    searchMoves.clear();
}

std::vector<std::string> Parameters::searchParameters() {
    return {"depth", "movetime", "wtime", "btime", "winc", "binc",
            "movestogo", "nodes", "mate", "perft"};
}

std::string Parameters::uciOptions() {
    std::string opts =
           "option name Threads type spin default 1 min 1 max "
           + std::to_string(maxThreads()) + "\n"
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
        "option name RfpCoeff type spin default 160 min 60 max 240\n"
        "option name RfpImproving type spin default 72 min 0 max 140\n"
        "option name RazorCoeff type spin default 243 min 120 max 500\n"
        "option name NullBase type spin default 3 min 2 max 6\n"
        "option name NullEvalDiv type spin default 192 min 80 max 400\n"
        "option name ProbCutMargin type spin default 189 min 80 max 360\n"
        "option name FutilityBase type spin default 180 min 40 max 280\n"
        "option name FutilityCoeff type spin default 128 min 40 max 200\n"
        "option name HistPruneCoeff type spin default 4210 min 1000 max 7000\n"
        "option name SeePruneCoeff type spin default 73 min 30 max 160\n"
        "option name SingularBetaMult type spin default 4 min 1 max 6\n"
        "option name SingularDoubleMargin type spin default 4 min 0 max 60\n"
        "option name AspirationDelta type spin default 19 min 10 max 60\n"
        "option name LmrBase type spin default 60 min 0 max 150\n"
        "option name LmrDivisor type spin default 209 min 150 max 350\n"
        "option name LmrHistDiv type spin default 7830 min 4096 max 16384\n"
        "option name LmrNonPvAdj type spin default 1 min 0 max 3\n"
        "option name LmrCutNodeAdj type spin default 0 min 0 max 3\n"
        "option name LmrTtPvAdj type spin default 0 min 0 max 3\n"
        "option name LmrNotImprovingAdj type spin default 0 min 0 max 3\n";
#endif
    return opts;
}

int Parameters::maxThreads() {
    const unsigned hw = std::thread::hardware_concurrency();
    if (hw == 0)
        return 1024;
    return static_cast<int>(std::max(1024u, 4u * hw));
}

void Parameters::setSearchParameters(const std::string& args) {
    resetTemporaryParameters();

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
                    searchMoves.push_back(move);
                } else {
                    uci_write_line("info string Invalid searchmoves move: " + move_token);
                    break;
                }
            }
            continue;
        }

        const auto params = Parameters::searchParameters();
        if (std::find(params.begin(), params.end(), parameter) == params.end())
            continue;
        if (i + 1 >= tokens.size() || is_go_parameter(tokens[i + 1])) {
            uci_write_line("info string Invalid " + parameter + " value.");
            continue;
        }

        setSearchParameter(parameter, tokens[++i]);
    }

    const bool has_limit = depth != infiniteDepth
        || moveTime > 0 || whiteTime > 0 || blackTime > 0
        || whiteIncrement > 0 || blackIncrement > 0
        || movestogo > 0 || nodes > 0 || mate > 0 || perft > 0;
    if (!has_limit && !infinite_requested && !ponder)
        depth = defaultSearchDepth;
}

void Parameters::setSearchParameter(const std::string& parameter, const std::string& value) {
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
    if (parameter == "movetime")  { moveTime       = std::max(0, parsed); return; }
    if (parameter == "wtime")     { whiteTime      = std::max(0, parsed); return; }
    if (parameter == "winc")      { whiteIncrement = std::max(0, parsed); return; }
    if (parameter == "btime")     { blackTime      = std::max(0, parsed); return; }
    if (parameter == "binc")      { blackIncrement = std::max(0, parsed); return; }
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

void Parameters::setOption(const std::string& args) {
    std::smatch matches;

    // Extract option name (everything between "name " and either " value" or end-of-string)
    std::string name;
    if (std::regex_search(args, matches, std::regex(R"(name (.*?) value)")))
        name = matches[1].str();
    else if (std::regex_search(args, matches, std::regex(R"(name (.*))")))
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
    if (!std::regex_search(args, matches, std::regex(R"(value (.*))"))) {
        uci_write_line("info string Option '" + name + "' requires a value.");
        return;
    }
    const std::string value = matches[1].str();

    int parsed = 0;
    if (name_lower == "syzygypath") {
        syzygyPath = (value == "<empty>") ? std::string{} : value;
    } else if (name_lower == "ponder") {
        ponderEnabled = parse_bool_option(value);
    } else if (name_lower == "syzygy50moverule") {
        syzygy50MoveRule = parse_bool_option(value);
    } else if (name_lower == "tm_debug") {
        // Hidden diagnostic (Step 5.3): not advertised in `uci`; when on, the
        // search emits one `info string tm ...` per move with the time budget,
        // actual elapsed, and the go-receipt->search-start dispatch delta.
        tmDebug = parse_bool_option(value);
    } else if (!parse_int(value, parsed)) {
        uci_write_line("info string Invalid value for option '" + name + "': " + value);
    } else if (name_lower == "move overhead") {
        moveOverhead = std::clamp(parsed, 0, 5000);
    } else if (name_lower == "hash") {
        hash_mb = std::clamp(parsed, 1, 33554432);
    } else if (name_lower == "threads") {
        threads = std::clamp(parsed, 1, maxThreads());
    } else if (name_lower == "syzygyprobedepth") {
        syzygyProbeDepth = std::clamp(parsed, 1, 100);
    } else if (name_lower == "syzygyprobelimit") {
        syzygyProbeLimit = std::clamp(parsed, 0, 7);
    }
#ifdef BASILISK_TUNE
    else if (name_lower == "rfpcoeff")              { search_params.rfp_coeff              = std::clamp(parsed,    60,  240); }
    else if (name_lower == "rfpimproving")          { search_params.rfp_improving          = std::clamp(parsed,     0,  140); }
    else if (name_lower == "razorcoeff")            { search_params.razor_coeff            = std::clamp(parsed,   120,  500); }
    else if (name_lower == "nullbase")              { search_params.null_base              = std::clamp(parsed,     2,    6); }
    else if (name_lower == "nullevaldiv")           { search_params.null_eval_div          = std::clamp(parsed,    80,  400); }
    else if (name_lower == "probcutmargin")         { search_params.probcut_margin         = std::clamp(parsed,    80,  360); }
    else if (name_lower == "futilitybase")          { search_params.futility_base          = std::clamp(parsed,    40,  280); }
    else if (name_lower == "futilitycoeff")         { search_params.futility_coeff         = std::clamp(parsed,    40,  200); }
    else if (name_lower == "histprunecoeff")        { search_params.hist_prune_coeff       = std::clamp(parsed,  1000, 7000); }
    else if (name_lower == "seeprunecoeff")         { search_params.see_prune_coeff        = std::clamp(parsed,    30,  160); }
    else if (name_lower == "singularbetamult")      { search_params.singular_beta_mult     = std::clamp(parsed,     1,    6); }
    else if (name_lower == "singulardoublemargin")  { search_params.singular_double_margin = std::clamp(parsed,     0,   60); }
    else if (name_lower == "aspirationdelta")       { search_params.aspiration_delta       = std::clamp(parsed,    10,   60); }
    else if (name_lower == "lmrbase")               { search_params.lmr_base               = std::clamp(parsed,     0,  150); }
    else if (name_lower == "lmrdivisor")            { search_params.lmr_divisor            = std::clamp(parsed,   150,  350); }
    else if (name_lower == "lmrhistdiv")            { search_params.lmr_hist_div           = std::clamp(parsed,  4096, 16384); }
    else if (name_lower == "lmrnonpvadj")           { search_params.lmr_non_pv_adj         = std::clamp(parsed,     0,    3); }
    else if (name_lower == "lmrcutnodeadj")         { search_params.lmr_cut_node_adj       = std::clamp(parsed,     0,    3); }
    else if (name_lower == "lmrttpvadj")            { search_params.lmr_tt_pv_adj          = std::clamp(parsed,     0,    3); }
    else if (name_lower == "lmrnotimprovingadj")    { search_params.lmr_not_improving_adj  = std::clamp(parsed,     0,    3); }
#endif
}

void Parameters::setPosition(const std::string& args) {
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

        std::string error;
        if (!new_board.try_set_fen(fen, &error, true)) {
            uci_write_line("info string " + error);
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
