#include <atomic>
#include <iostream>
#include <thread>

#if defined(USE_PEXT) || defined(USE_AVX2)
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

#if defined(USE_PEXT) || defined(USE_AVX2)
#if defined(USE_AVX2)
bool cpu_supports_sse41_popcnt() {
#  if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    int regs[4] = {};
    __cpuid(regs, 0);
    if (regs[0] < 1) return false;
    __cpuid(regs, 1);
    return (regs[2] & (1 << 19)) != 0 && (regs[2] & (1 << 23)) != 0;
#  elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    __builtin_cpu_init();
    return __builtin_cpu_supports("sse4.1") && __builtin_cpu_supports("popcnt");
#  else
    return false;
#  endif
}
#endif

#if defined(USE_AVX2)
bool cpu_supports_avx2() {
#  if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    int regs[4] = {};
    __cpuid(regs, 0);
    if (regs[0] < 7) return false;
    __cpuid(regs, 1);
    const bool osxsave = (regs[2] & (1 << 27)) != 0;
    const bool avx     = (regs[2] & (1 << 28)) != 0;
    if (!osxsave || !avx) return false;
    const unsigned long long xcr0 = _xgetbv(0);
    if ((xcr0 & 0x6) != 0x6) return false;
    __cpuidex(regs, 7, 0);
    return (regs[1] & (1 << 5)) != 0;
#  elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx2");
#  else
    return false;
#  endif
}
#endif

#if defined(USE_PEXT)
bool cpu_supports_bmi2() {
#  if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    int regs[4] = {};
    __cpuid(regs, 0);
    if (regs[0] < 7) return false;
    __cpuidex(regs, 7, 0);
    return (regs[1] & (1 << 8)) != 0;
#  elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    __builtin_cpu_init();
    return __builtin_cpu_supports("bmi2");
#  else
    return false;
#  endif
}
#endif
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

#if defined(USE_AVX2)
    if (!cpu_supports_avx2() || !cpu_supports_sse41_popcnt()) {
        std::cerr << "Basilisk AVX2 build requires a CPU with AVX2, SSE4.1, and POPCNT support.\n"
                  << "Use the generic x86_64 build on this machine.\n";
        return 1;
    }
#endif

    // Initialize all precomputed tables
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables(g_eval_params);
#ifdef BASILISK_TUNE
    load_eval_file_if_set();
#endif

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
