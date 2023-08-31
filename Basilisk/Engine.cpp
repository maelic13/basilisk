#include <iostream>

#include "Board.h"
#include "Engine.h"

using namespace std::chrono_literals;

Engine::Engine(std::atomic_bool &go, std::atomic_bool &quit,
               Parameters &parameters, std::mutex &mutex, std::condition_variable &cv)
        : go(go), quit(quit), parameters(parameters), mutex(mutex), conditionVariable(cv) {
    timeForMove = std::numeric_limits<int>::max();
}

[[noreturn]] void Engine::start() {
    while (true) {
        std::unique_lock lock(mutex);
        conditionVariable.wait(lock, [&] { return go || quit; });

        if (quit) {
            lock.unlock();
            break;
        }
        if (!go) {
            continue;
        }

        startTimer();
        search(parameters.board, parameters.depth);
        go = false;
    }
}

bool Engine::check_stop() {
    bool timeout = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - startTime).count() >= timeForMove;
    return !go || quit || timeout;
}

void Engine::search(const Board& board, int& max_depth) {
    int depth = 0;
    while (depth < max_depth) {
        if (check_stop()) {
            break;
        }

        depth++;
        std::cout << "Calculating... Depth: " << depth << "\n";
        std::this_thread::sleep_for(3s);
    }
    std::cout << "bestmove d2d4" << "\n";
}

void Engine::startTimer() {
    startTime = std::chrono::system_clock::now();
    timeForMove = std::numeric_limits<int>::max();

    if (parameters.moveTime != 0) {
        timeForMove = parameters.moveTime - parameters.moveOverhead;
    }
    if (parameters.board.sideToMove() && parameters.whiteTime != 0) {
        timeForMove = (2 * parameters.whiteTime / 10) - parameters.moveOverhead;
    }
    if (!parameters.board.sideToMove() && parameters.blackTime != 0) {
        timeForMove = (2 * parameters.blackTime / 10) - parameters.moveOverhead;
    }
}
