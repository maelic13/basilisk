#include <exception>
#include <iostream>

#if defined(USE_PEXT) || defined(USE_AVX2)
#  if defined(_MSC_VER) && defined(_M_X64)
#    include <intrin.h>
#  elif defined(__GNUC__) && defined(__x86_64__)
#    include <cpuid.h>
#  endif
#endif

#include "cpu_features.h"
#include "engine_entry.h"

namespace {

#if defined(USE_PEXT) || defined(USE_AVX2)
CpuFeatures detect_cpu_features() {
    CpuFeatures features;

#  if defined(_MSC_VER) && defined(_M_X64)
    int regs[4] = {};
    __cpuid(regs, 0);
    const int max_basic_leaf = regs[0];

    if (max_basic_leaf >= 1) {
        __cpuid(regs, 1);
        features.sse41  = (regs[2] & (1 << 19)) != 0;
        features.popcnt = (regs[2] & (1 << 23)) != 0;

        const bool osxsave = (regs[2] & (1 << 27)) != 0;
        const bool avx     = (regs[2] & (1 << 28)) != 0;
        if (osxsave && avx && (_xgetbv(0) & 0x6) == 0x6 && max_basic_leaf >= 7) {
            __cpuidex(regs, 7, 0);
            features.avx2 = (regs[1] & (1 << 5)) != 0;
        }
    }

    if (max_basic_leaf >= 7) {
        __cpuidex(regs, 7, 0);
        features.bmi1 = (regs[1] & (1 << 3)) != 0;
        features.bmi2 = (regs[1] & (1 << 8)) != 0;
    }

    __cpuid(regs, static_cast<int>(0x80000000U));
    if (static_cast<unsigned int>(regs[0]) >= 0x80000001U) {
        __cpuid(regs, static_cast<int>(0x80000001U));
        features.lzcnt = (regs[2] & (1 << 5)) != 0;
    }
#  elif defined(__GNUC__) && defined(__x86_64__)
    __builtin_cpu_init();
    features.sse41  = __builtin_cpu_supports("sse4.1");
    features.popcnt = __builtin_cpu_supports("popcnt");
    features.avx2   = __builtin_cpu_supports("avx2");
    features.bmi1   = __builtin_cpu_supports("bmi");
    features.bmi2   = __builtin_cpu_supports("bmi2");
    features.lzcnt  = __builtin_cpu_supports("lzcnt");
#  endif

    return features;
}
#endif

bool cpu_is_compatible() {
#if defined(USE_PEXT)
    if (!supports_pext_tier(detect_cpu_features())) {
        std::cerr << "Basilisk PEXT build requires AVX2, SSE4.1, POPCNT, BMI1, "
                     "BMI2/PEXT, and LZCNT support.\n"
                  << "Use the AVX2 or portable x86_64 build on this machine.\n";
        return false;
    }
#elif defined(USE_AVX2)
    if (!supports_avx2_tier(detect_cpu_features())) {
        std::cerr << "Basilisk AVX2 build requires AVX2, SSE4.1, and POPCNT support.\n"
                  << "Use the portable x86_64 build on this machine.\n";
        return false;
    }
#endif
    return true;
}

} // namespace

int main() {
    if (!cpu_is_compatible()) return 1;

    // Last-resort diagnostic: anything reaching here is a bug or resource
    // exhaustion, so report it instead of terminating silently under a GUI.
    try {
        return run_engine();
    } catch (const std::exception& e) {
        std::cerr << "FATAL: unhandled exception: " << e.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "FATAL: unhandled non-standard exception\n";
        return 1;
    }
}
