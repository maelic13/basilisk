#include <iostream>
#include <limits>
#include <string>

#include "Board.h"
#include "Engine.h"

Engine::Engine(std::atomic_bool &go, std::atomic_bool &quit,
               Parameters &parameters, std::mutex &mutex, std::condition_variable &cv)
    : go(go), quit(quit), parameters(parameters), mutex(mutex), conditionVariable(cv) {
    timeForMove = std::numeric_limits<int>::max();
}

void Engine::start() {
    while (true) {
        std::unique_lock lock(mutex);
        conditionVariable.wait(lock, [&] { return go.load() || quit.load(); });

        if (quit.load()) {
            break;
        }
        if (!go.load()) {
            continue;
        }

        startTimer();
        search();
        go = false;
    }
}

bool Engine::checkStop() const {
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - startTime)
                             .count();
    return !go.load() || quit.load() || elapsed >= timeForMove;
}

void Engine::search() {
    chess::Movelist moves;
    chess::movegen::legalmoves(moves, parameters.board);

    if (moves.empty()) {
        // Checkmate or stalemate — no legal move to report
        return;
    }

    int depth = 0;
    while (depth < parameters.depth) {
        if (checkStop()) break;
        depth++;
    }

    std::cout << "bestmove " << bestMove() << "\n";
}

std::string Engine::bestMove() const {
    chess::Movelist moves;
    chess::movegen::legalmoves(moves, parameters.board);

    if (moves.empty()) return "0000";
    return chess::uci::moveToUci(moves[0]);
}

void Engine::startTimer() {
    startTime   = std::chrono::steady_clock::now();
    timeForMove = std::numeric_limits<int>::max();

    const Color side = parameters.board.sideToMove();

    if (parameters.moveTime > 0) {
        timeForMove = std::max(1, parameters.moveTime - parameters.moveOverhead);
        return;
    }

    const int remaining = (side == Color::WHITE) ? parameters.whiteTime : parameters.blackTime;
    const int increment = (side == Color::WHITE) ? parameters.whiteIncrement : parameters.blackIncrement;

    if (remaining > 0) {
        // Allocate ~1/30 of remaining time plus 75% of increment
        const int alloc = remaining / 30 + (increment * 3) / 4;
        timeForMove = std::max(10, alloc - parameters.moveOverhead);
    }
}

