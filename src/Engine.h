#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>

#include "Parameters.h"

class Engine {
public:
    explicit Engine(std::atomic_bool &go, std::atomic_bool &quit,
                    Parameters &parameters, std::mutex &m, std::condition_variable &cv);

    // Runs on a dedicated thread — blocks until quit is set.
    void start();

private:
    std::mutex &mutex;
    std::condition_variable &conditionVariable;

    std::atomic_bool &go;
    std::atomic_bool &quit;

    Parameters &parameters;
    std::chrono::steady_clock::time_point startTime;
    int timeForMove; // [ms], INT_MAX == unlimited

    [[nodiscard]] bool checkStop() const;

    void search();

    void startTimer();

    // Returns the best move in UCI notation (placeholder: first legal move).
    [[nodiscard]] std::string bestMove() const;
};

