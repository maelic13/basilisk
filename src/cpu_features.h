#pragma once

struct CpuFeatures {
    bool sse41  = false;
    bool popcnt = false;
    bool avx2   = false;
    bool bmi1   = false;
    bool bmi2   = false;
    bool lzcnt  = false;
};

constexpr bool supports_avx2_tier(const CpuFeatures& features) {
    return features.sse41 && features.popcnt && features.avx2;
}

constexpr bool supports_pext_tier(const CpuFeatures& features) {
    return supports_avx2_tier(features)
        && features.bmi1 && features.bmi2 && features.lzcnt;
}
