#pragma once

#include <atomic>
#include <condition_variable>
#include <string>

#include "Parameters.h"

class UciProtocol {
public:
    UciProtocol(std::atomic_bool &go, std::atomic_bool &quit,
                Parameters &parameters, std::condition_variable &conditionVariable);

    void UciLoop();

private:
    std::atomic_bool &go;
    std::atomic_bool &quit;
    Parameters &parameters;
    std::condition_variable &conditionVariable;

    bool debug_mode = false;

    void cmdDebug(const std::string &args);
    static void cmdUci();
    static void cmdIsReady();
    static void cmdRegister();

    void cmdGo(const std::string &args);
    void cmdStop();
    void cmdQuit();
    void cmdPonderHit();
    void cmdSetOption(const std::string &args);
    void cmdPosition(const std::string &args);
    void cmdNewGame();
};

