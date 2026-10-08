#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>

#include "engine_command.h"
#include "parameters.h"

class UciProtocol {
public:
    UciProtocol(EngineCommandQueue& commands,
                std::atomic_bool& stop_requested,
                std::atomic_bool& ponderhit_requested,
                std::atomic_bool& searching,
                std::atomic_uint64_t& control_epoch);

    void UciLoop();

    // Runs one command given on the command line, lets the engine finish it,
    // then quits. Returns the process exit status: 0, or 2 for an unknown
    // command.
    int RunArguments(const std::string& command_line);

private:
    enum class Dispatch { Handled, Quit, Unknown };
    Dispatch dispatch(const std::string& input);
    static void cmdHelp();
    EngineCommandQueue& commands_;
    std::atomic_bool& stop_requested_;
    std::atomic_bool& ponderhit_requested_;
    std::atomic_bool& searching_;
    std::atomic_uint64_t& control_epoch_;

    bool debug_mode = false;

    void cmdDebug(const std::string &args);
    static void cmdUci();
    void cmdIsReady();
    static void cmdRegister();

    uint64_t next_control_epoch();
    void enqueue(EngineCommandType type, const std::string& args = {}, uint64_t epoch = 0,
                 std::chrono::steady_clock::time_point recv_time = {});
    void cmdGo(const std::string &args);
    void cmdStop();
    void cmdQuit();
    void cmdPonderHit();
    void cmdSetOption(const std::string &args);
    void cmdPosition(const std::string &args);
    void cmdNewGame();
    void cmdBench(const std::string& args);
    void cmdWac(const std::string& args);
};

