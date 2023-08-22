#include <chrono>
#include <iostream>

#include "Engine.h"

using namespace std::chrono_literals;

Engine::Engine(std::atomic_bool &go, std::atomic_bool &quit,
               Parameters &parameters, std::mutex &mutex, std::condition_variable &cv)
        : go(go), quit(quit), parameters(parameters), mutex(mutex), conditionVariable(cv) {}

[[noreturn]] void Engine::start() {
    while (true) {
        std::unique_lock lk(mutex);
        conditionVariable.wait(lk, [&] { return go || quit; });

        if (quit) {
            lk.unlock();
            break;
        }
        if (go) {
            search();
        }
    }
}

void Engine::search() {
    int depth = 0;
    while (go && !quit) {
        std::cout << "Calculating... Depth: " << depth << "\n";
        depth++;
        std::this_thread::sleep_for(2s);
    }
}
