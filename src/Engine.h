#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>

#include "Parameters.h"
#include "tt.h"
#include "search.h"

class Engine {
public:
    explicit Engine(std::atomic_bool& go, std::atomic_bool& quit,
                    Parameters& parameters, std::mutex& m, std::condition_variable& cv);

    void start();

private:
    std::atomic_bool& go;
    std::atomic_bool& quit;

    Parameters& parameters;
    std::mutex&             mutex;
    std::condition_variable& conditionVariable;

    TranspositionTable tt;
};

