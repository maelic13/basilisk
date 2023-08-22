#ifndef BASILISK_ENGINE_H
#define BASILISK_ENGINE_H

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

    void search();
};

#endif //BASILISK_ENGINE_H
