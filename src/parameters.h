#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "board.h"
#include "search_params.h"

class Parameters {
public:
    Board board;

    int move_time;           // [ms]
    int white_time;          // [ms]
    int white_increment;     // [ms]
    int black_time;          // [ms]
    int black_increment;     // [ms]
    int depth;

    int     move_overhead;   // [ms]
    int     hash_mb;        // TT size in MB
    int     threads;        // search worker count
    int64_t nodes;          // node limit (0 = unlimited)
    int     movestogo;      // moves until next time control (0 = sudden death)
    int     mate;           // mate search target in moves (0 = disabled)
    int     perft;          // perft depth requested via go perft (0 = disabled)
    bool    ponder;         // go ponder mode
    bool    ponder_enabled;  // UCI Ponder option advertised to the GUI
    std::vector<Move> search_moves; // root move restriction from go searchmoves
    std::string syzygy_path; // semicolon-separated Syzygy tablebase paths
    int     syzygy_probe_depth;
    int     syzygy_probe_limit;
    bool    syzygy_50_move_rule;
    bool    tm_debug;        // hidden TM_Debug check: log per-move time accounting
    bool    diag;            // hidden Diag check: end-of-search diagnostic counters (8.6.6)

    bool new_game    = false;  // set by "ucinewgame", cleared after engine processes it
    bool clear_hash  = false;  // set by "setoption name Clear Hash", cleared after engine clears TT

    SearchParams search_params; // tunable constants (exposed as UCI options under BASILISK_TUNE)

    Parameters();

    void reset();

    void reset_temporary_parameters();

    void set_option(const std::string &args);

    void set_position(const std::string &args);

    void set_search_parameters(const std::string &args);

    static std::string uci_options();
    static int max_threads();

private:
    void set_search_parameter(const std::string &parameter, const std::string &value);

    static std::vector<std::string> search_parameters();
};

