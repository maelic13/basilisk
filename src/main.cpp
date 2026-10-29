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

#include "cpu_detect.h"

#if defined(BASILISK_UNIVERSAL)
// The tiers' entry points. Each tier is the whole engine built for its ISA,
// with every symbol it defines renamed, so this is all the tiers share.
namespace basilisk_base         { int run_engine(int argc, char* argv[]); }
namespace basilisk_sse41_popcnt { int run_engine(int argc, char* argv[]); }
namespace basilisk_avx2         { int run_engine(int argc, char* argv[]); }
namespace basilisk_pext         { int run_engine(int argc, char* argv[]); }
#else
#  include "engine_entry.h"
#endif

namespace {

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
            report(std::string("BASILISK_ISA=") + forced + " names no tier; use base, sse41-popcnt, avx2 or pext");
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
    case IsaTier::Sse41Popcnt: return basilisk_sse41_popcnt::run_engine(argc, argv);
    case IsaTier::Base: break;
    }
    return basilisk_base::run_engine(argc, argv);
}
#else
int run_tier(int argc, char* argv[]) {
#  if defined(USE_PEXT)
    constexpr IsaTier built = IsaTier::Pext;
#  elif defined(USE_AVX2)
    constexpr IsaTier built = IsaTier::Avx2;
#  elif defined(USE_SSE41_POPCNT)
    constexpr IsaTier built = IsaTier::Sse41Popcnt;
#  else
    constexpr IsaTier built = IsaTier::Base;
#  endif
    const CpuFeatures features = detect_cpu_features();
    if (!supports_tier(features, built)) {
        report("this Basilisk build needs a CPU with the " + std::string(tier_name(built))
               + " tier's instructions (pext: AVX2, POPCNT, BMI1, BMI2 and LZCNT;"
                 " avx2: AVX2, SSE4.1 and POPCNT; sse41-popcnt: SSE4.1 and POPCNT); use the "
               + std::string(tier_name(best_tier(features))) + " x86-64 build");
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
