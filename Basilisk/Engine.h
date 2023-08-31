#ifndef BASILISK_ENGINE_H
#define BASILISK_ENGINE_H

#include <chrono>

#include "Board.h"
#include "Parameters.h"

class Engine {
public:
    explicit Engine(std::atomic_bool &go, std::atomic_bool &quit,
                    Parameters &parameters, std::mutex &m, std::condition_variable &cv);

    [[noreturn]] void start();

private:
    std::mutex &mutex;
    std::condition_variable &conditionVariable;

    std::atomic_bool &go;
    std::atomic_bool &quit;

    Parameters &parameters;
    std::chrono::time_point<std::chrono::system_clock> startTime;
    int timeForMove;

    bool check_stop();
    void search(const Board& board, int& depth);
    void startTimer();
};

#endif //BASILISK_ENGINE_H
