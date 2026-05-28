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
    ponder         = false;

    moveOverhead = defaultMoveOverhead;
    hash_mb      = 64;
    threads      = 1;
    ponderEnabled = false;
    syzygyPath.clear();
    syzygyProbeDepth = 1;
    syzygyProbeLimit = 7;
    syzygy50MoveRule = true;
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
    ponder         = false;
}

std::vector<std::string> Parameters::searchParameters() {
    return {"depth", "movetime", "wtime", "btime", "winc", "binc", "movestogo", "nodes"};
}

std::string Parameters::uciOptions() {
    return "option name Threads type spin default 1 min 1 max "
           + std::to_string(maxThreads()) + "\n"
           "option name Hash type spin default 64 min 1 max 33554432\n"
           "option name Clear Hash type button\n"
           "option name Ponder type check default false\n"
           "option name Move Overhead type spin default 10 min 0 max 5000\n"
           "option name SyzygyPath type string default <empty>\n"
           "option name SyzygyProbeDepth type spin default 1 min 1 max 100\n"
           "option name Syzygy50MoveRule type check default true\n"
           "option name SyzygyProbeLimit type spin default 7 min 0 max 7\n";
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

    const bool infinite_requested = args.find("infinite") != std::string::npos;
    if (infinite_requested) {
        depth = infiniteDepth;
    }

    ponder = (args.find("ponder") != std::string::npos);

    std::smatch matches;
    const auto params = Parameters::searchParameters();
    for (const std::string& parameter : params) {
        for (const std::regex& re : {std::regex(parameter + " (\\S+) "),
                                     std::regex(parameter + " (\\S+)$")}) {
            if (std::regex_search(args, matches, re)) {
                setSearchParameter(parameter, matches[1].str());
                break;
            }
        }
    }

    const bool has_limit = depth != infiniteDepth
        || moveTime > 0 || whiteTime > 0 || blackTime > 0
        || whiteIncrement > 0 || blackIncrement > 0
        || movestogo > 0 || nodes > 0;
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

