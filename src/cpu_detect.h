#pragma once

// Reading the CPU's features, for the launcher's dispatch and the build's
// configure-time PEXT check (cmake/probe_cpu.cpp), so both apply one rule.
// Compiled for the baseline wherever it is included.

#if defined(__x86_64__) || defined(_M_X64)
#  if defined(_MSC_VER)
#    include <intrin.h>
#  else
#    include <cpuid.h>
#  endif
#  define BASILISK_X86_64 1
#endif

#include "cpu_features.h"

#if defined(BASILISK_X86_64)
inline void cpuid(unsigned leaf, unsigned regs[4]) {
#  if defined(_MSC_VER)
    int r[4] = {};
    __cpuidex(r, static_cast<int>(leaf), 0);
    for (int i = 0; i < 4; ++i) regs[i] = static_cast<unsigned>(r[i]);
#  else
    __cpuid_count(leaf, 0, regs[0], regs[1], regs[2], regs[3]);
#  endif
}

inline unsigned long long xgetbv0() {
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
inline CpuFeatures detect_cpu_features() {
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
inline CpuFeatures detect_cpu_features() { return {}; }
#endif
