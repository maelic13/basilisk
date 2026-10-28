#if defined(BASILISK_UNIVERSAL_TIER)
// A universal binary's tier: the engine link that produces the tier's objects
// needs a main, and this one defines nothing else, so the engine can bind to
// nothing here that the universal binary's link would not have.
#include "engine_entry.h"

int main(int argc, char* argv[]) { return run_engine(argc, argv); }
#else
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

#if defined(__x86_64__) || defined(_M_X64)
#  if defined(_MSC_VER)
#    include <intrin.h>
#  else
#    include <cpuid.h>
#  endif
#  define BASILISK_X86_64 1
#endif

#include "cpu_features.h"

#if defined(BASILISK_UNIVERSAL)
// The tiers' entry points. Each tier is the whole engine built for its ISA,
// with every symbol it defines renamed, so this is all the tiers share.
namespace basilisk_portable { int run_engine(int argc, char* argv[]); }
namespace basilisk_avx2     { int run_engine(int argc, char* argv[]); }
namespace basilisk_pext     { int run_engine(int argc, char* argv[]); }
#else
#  include "engine_entry.h"
#endif

namespace {

#if defined(BASILISK_X86_64)
void cpuid(unsigned leaf, unsigned regs[4]) {
#  if defined(_MSC_VER)
    int r[4] = {};
    __cpuidex(r, static_cast<int>(leaf), 0);
    for (int i = 0; i < 4; ++i) regs[i] = static_cast<unsigned>(r[i]);
#  else
    __cpuid_count(leaf, 0, regs[0], regs[1], regs[2], regs[3]);
#  endif
}

unsigned long long xgetbv0() {
#  if defined(_MSC_VER)
    return _xgetbv(0);
#  else
    unsigned eax = 0, edx = 0;
    __asm__ volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
    return (static_cast<unsigned long long>(edx) << 32) | eax;
#  endif
}

// Read from CPUID directly rather than the compiler's feature cache, so both
// compilers see the same bits, and nothing here can be folded to true by a
// tier's flags: this file is compiled for the baseline.
CpuFeatures detect_cpu_features() {
    CpuFeatures features;
    unsigned regs[4] = {};
    cpuid(0, regs);
    const unsigned max_leaf = regs[0];
    const bool amd = regs[1] == 0x68747541U && regs[3] == 0x69746E65U && regs[2] == 0x444D4163U;

    if (max_leaf >= 1) {
        cpuid(1, regs);
        features.slow_pext = slow_pext_cpu(amd, cpuid_family(regs[0]));
        features.sse41  = (regs[2] & (1U << 19)) != 0;
        features.popcnt = (regs[2] & (1U << 23)) != 0;
        const bool osxsave = (regs[2] & (1U << 27)) != 0;
        const bool avx     = (regs[2] & (1U << 28)) != 0;
        const bool ymm_state = osxsave && avx && (xgetbv0() & 0x6) == 0x6;
        if (max_leaf >= 7) {
            cpuid(7, regs);
            features.avx2 = ymm_state && (regs[1] & (1U << 5)) != 0;
            features.bmi1 = (regs[1] & (1U << 3)) != 0;
            features.bmi2 = (regs[1] & (1U << 8)) != 0;
        }
    }
    cpuid(0x80000000U, regs);
    if (regs[0] >= 0x80000001U) {
        cpuid(0x80000001U, regs);
        features.lzcnt = (regs[2] & (1U << 5)) != 0;
    }
    return features;
}
#else
CpuFeatures detect_cpu_features() { return {}; }
#endif

// Before `uci`, on stdout as well: a GUI or tester records stdout, not stderr.
void report(const std::string& message) {
    std::cout << "info string " << message << std::endl;
    std::cerr << message << '\n';
}

// A tier slower than the one this CPU could run. Advice only: it runs.
void advise(const CpuFeatures& features, IsaTier running, std::string_view what) {
#if defined(BASILISK_X86_64)
    const IsaTier best = best_tier(features);
    if (running == IsaTier::Pext && features.slow_pext) {
        std::cout << "info string " << what << " runs the pext tier, which is slow on this AMD CPU;"
                  << " the avx2 tier is faster here" << std::endl;
    } else if (best > running) {
        std::cout << "info string " << what << " runs the " << tier_name(running)
                  << " tier; this CPU runs the " << tier_name(best) << " tier faster" << std::endl;
    }
#else
    (void)features; (void)running; (void)what;
#endif
}

#if defined(BASILISK_UNIVERSAL)
int run_tier(int argc, char* argv[]) {
    const CpuFeatures features = detect_cpu_features();
    IsaTier tier = best_tier(features);
    if (const char* forced = std::getenv("BASILISK_ISA"); forced && *forced) {
        const auto named = parse_tier(forced);
        if (!named) {
            report(std::string("BASILISK_ISA=") + forced + " names no tier; use portable, avx2 or pext");
            return 1;
        }
        if (!supports_tier(features, *named)) {
            report("BASILISK_ISA forces the " + std::string(tier_name(*named))
                   + " tier, which this CPU cannot run");
            return 1;
        }
        tier = *named;
        advise(features, tier, "BASILISK_ISA");
    }
    switch (tier) {
    case IsaTier::Pext: return basilisk_pext::run_engine(argc, argv);
    case IsaTier::Avx2: return basilisk_avx2::run_engine(argc, argv);
    case IsaTier::Portable: break;
    }
    return basilisk_portable::run_engine(argc, argv);
}
#else
int run_tier(int argc, char* argv[]) {
#  if defined(USE_PEXT)
    constexpr IsaTier built = IsaTier::Pext;
#  elif defined(USE_AVX2)
    constexpr IsaTier built = IsaTier::Avx2;
#  else
    constexpr IsaTier built = IsaTier::Portable;
#  endif
    const CpuFeatures features = detect_cpu_features();
    if (!supports_tier(features, built)) {
        report("this Basilisk build needs a CPU with the " + std::string(tier_name(built))
               + " tier's instructions (pext: AVX2, POPCNT, BMI1, BMI2 and LZCNT;"
                 " avx2: AVX2, SSE4.1 and POPCNT); use the universal x86-64 build");
        return 1;
    }
    advise(features, built, "this build");
    return run_engine(argc, argv);
}
#endif

} // namespace

int main(int argc, char* argv[]) {
    // Last-resort diagnostic: anything reaching here is a bug or resource
    // exhaustion, so report it instead of terminating silently under a GUI.
    try {
        return run_tier(argc, argv);
    } catch (const std::exception& e) {
        // stdout too: a GUI or harness records the engine's stdout, not stderr.
        std::cout << "info string FATAL: unhandled exception: " << e.what() << std::endl;
        std::cerr << "FATAL: unhandled exception: " << e.what() << '\n';
        return 1;
    } catch (...) {
        std::cout << "info string FATAL: unhandled non-standard exception" << std::endl;
        std::cerr << "FATAL: unhandled non-standard exception\n";
        return 1;
    }
}
#endif
