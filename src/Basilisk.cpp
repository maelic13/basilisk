#include <atomic>
#include <iostream>
#include <thread>

#if defined(USE_PEXT)
#  if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
#    include <intrin.h>
#  elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
#    include <cpuid.h>
#  endif
#endif

#include "Constants.h"
#include "Engine.h"
#include "EngineCommand.h"
#include "UciOutput.h"
#include "UciProtocol.h"
#include "bitboard.h"
#include "attacks.h"
#include "zobrist.h"
#include "eval.h"

namespace {

#if defined(USE_PEXT)
bool cpu_supports_bmi2() {
#  if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    int regs[4] = {};
    __cpuid(regs, 0);
    if (regs[0] < 7) return false;
    __cpuidex(regs, 7, 0);
    return (regs[1] & (1 << 8)) != 0;
#  elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    if (__get_cpuid_max(0, nullptr) < 7) return false;
    __cpuid_count(7, 0, eax, ebx, ecx, edx);
    return (ebx & (1u << 8)) != 0;
#  else
    return false;
#  endif
}
#endif

} // namespace

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

#if defined(USE_PEXT)
    if (!cpu_supports_bmi2()) {
        std::cerr << "Basilisk PEXT build requires a CPU with BMI2/PEXT support.\n"
                  << "Use the non-PEXT x86_64 build on this machine.\n";
        return 1;
    }
#endif

    // Initialize all precomputed tables
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables();

    uci_write_line(std::string(engineName) + " " + std::string(engineVersion)
                   + " by " + std::string(engineAuthor));

    EngineCommandQueue command_queue;
    std::atomic_bool stop_requested{false};
    std::atomic_bool ponderhit_requested{false};
    std::atomic_bool searching{false};
    std::atomic_uint64_t control_epoch{0};

    Engine      engine(command_queue, stop_requested, ponderhit_requested, searching, control_epoch);
    UciProtocol uciProtocol(command_queue, stop_requested, ponderhit_requested, searching, control_epoch);

    std::thread engineThread(&Engine::start, &engine);
    uciProtocol.UciLoop();
    engineThread.join();

    return 0;
}
