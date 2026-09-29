#include <atomic>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>

#include "attacks.h"
#include "bitboard.h"
#include "constants.h"
#include "engine.h"
#include "engine_command.h"
#include "engine_entry.h"
#include "eval.h"
#include "uci_output.h"
#include "uci_protocol.h"
#include "zobrist.h"

int run_engine() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

#ifndef _WIN32
    // A GUI closing our stdout pipe must not signal-kill the engine. Windows
    // has no SIGPIPE; on POSIX, writes to a closed pipe then simply fail and
    // the reader loop exits cleanly at cin EOF.
    std::signal(SIGPIPE, SIG_IGN);
#endif

    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables(g_eval_params);
#ifdef BASILISK_TUNE
    load_eval_file_if_set();
#endif

    uci_write_line(std::string(engineName) + " " + std::string(engineVersion)
                   + " by " + std::string(engineAuthor));

    EngineCommandQueue command_queue;
    std::atomic_bool stop_requested{false};
    std::atomic_bool ponderhit_requested{false};
    std::atomic_bool searching{false};
    std::atomic_uint64_t control_epoch{0};

    Engine      engine(command_queue, stop_requested, ponderhit_requested, searching, control_epoch);
    UciProtocol uciProtocol(command_queue, stop_requested, ponderhit_requested, searching, control_epoch);

    std::thread engineThread(&Engine::start, &engine);
    uciProtocol.UciLoop();
    engineThread.join();

    return 0;
}
