#include <iostream>
#include <string>

#include "Constants.h"
#include "Engine.h"

Engine::Engine(std::atomic_bool& go_, std::atomic_bool& quit_,
               Parameters& params_, std::mutex& mutex_, std::condition_variable& cv)
    : go(go_), quit(quit_), parameters(params_), mutex(mutex_), conditionVariable(cv)
    , tt(64)
{}

void Engine::start() {
    while (true) {
        std::unique_lock lock(mutex);
        conditionVariable.wait(lock, [&] { return go.load() || quit.load(); });

        if (quit.load()) break;
        if (!go.load())  continue;

        // Build search limits from parameters
        SearchLimits limits;
        limits.depth     = parameters.depth;
        limits.movetime  = parameters.moveTime;
        limits.wtime     = parameters.whiteTime;
        limits.btime     = parameters.blackTime;
        limits.winc      = parameters.whiteIncrement;
        limits.binc      = parameters.blackIncrement;
        limits.infinite  = (parameters.depth == infiniteDepth && parameters.moveTime == 0
                            && parameters.whiteTime == 0 && parameters.blackTime == 0);

        // Create searcher with engine's TT and the shared stop signal
        Searcher searcher(tt, go,
            [](const std::string& info) {
                std::cout << info << '\n';
                std::cout.flush();
            });

        Board board_copy = parameters.board;
        lock.unlock();

        SearchResult result = searcher.search(board_copy, limits);

        go = false;

        if (result.bestmove != MOVE_NONE) {
            std::string out = "bestmove " + move_to_uci(result.bestmove);
            if (result.pondermove != MOVE_NONE)
                out += " ponder " + move_to_uci(result.pondermove);
            std::cout << out << '\n';
        } else {
            std::cout << "bestmove 0000\n";
        }
        std::cout.flush();
    }
}

