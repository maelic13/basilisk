#ifndef ENGINE_UCIPROTOCOL_H
#define ENGINE_UCIPROTOCOL_H

#include <mutex>
#include <thread>

#include "Parameters.h"

class UciProtocol {
public:
    UciProtocol(std::atomic_bool &go, std::atomic_bool &quit,
                Parameters &parameters, std::mutex &m, std::condition_variable &cv);

    void UciLoop();

private:
    std::mutex &mutex;
    std::condition_variable &conditionVariable;

    std::atomic_bool &go;
    std::atomic_bool &quit;
    Parameters &parameters;

    static void isReady();

    static void uci();

    void uciGo(const std::string &args);

    void uciStop();

    void uciQuit();

    void uciSetOption(const std::string &args);

    void uciPosition(const std::string &args);

    void uciNewGame();
};

#endif //ENGINE_UCIPROTOCOL_H
