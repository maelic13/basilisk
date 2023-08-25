#include <chrono>
#include <iostream>

#include "Board.h"
#include "Engine.h"

using namespace std::chrono_literals;

Engine::Engine(std::atomic_bool &go, std::atomic_bool &quit,
               Parameters &parameters, std::mutex &mutex, std::condition_variable &cv)
        : go(go), quit(quit), parameters(parameters), mutex(mutex), conditionVariable(cv) {}

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

        search(parameters.board, parameters.depth);
    }
}

bool Engine::check_stop() {
    return !go || quit;
}

void Engine::search(const Board& board, int max_depth) {
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
