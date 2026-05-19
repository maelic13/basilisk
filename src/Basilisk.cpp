#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

#include "Constants.h"
#include "Engine.h"
#include "UciProtocol.h"
#include "bitboard.h"
#include "attacks.h"
#include "zobrist.h"
#include "eval.h"

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // Initialize all precomputed tables
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables();

    std::cout << engineName << " " << engineVersion << " by " << engineAuthor << "\n";
    std::cout.flush();

    std::mutex mutex;
    std::condition_variable conditionVariable;

    std::atomic_bool go   = false;
    std::atomic_bool quit = false;
    Parameters parameters;

    Engine      engine(go, quit, parameters, mutex, conditionVariable);
    UciProtocol uciProtocol(go, quit, parameters, conditionVariable);

    std::thread engineThread(&Engine::start, &engine);
    uciProtocol.UciLoop();
    engineThread.join();

    return 0;
}
