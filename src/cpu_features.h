#pragma once

#include <optional>
#include <string_view>

struct CpuFeatures {
    bool sse41  = false;
    bool popcnt = false;
    bool avx2   = false;
    bool bmi1   = false;
    bool bmi2   = false;
    bool lzcnt  = false;
    // AMD family 15h (Excavator) and 17h (Zen to Zen 2) run pext and pdep in
    // microcode, tens of cycles where Intel and Zen 3 take three, so the pext
    // tier is the slowest one there.
    bool slow_pext = false;
};

enum class IsaTier { Base, Avx2, Pext };

constexpr bool supports_avx2_tier(const CpuFeatures& features) {
    return features.sse41 && features.popcnt && features.avx2;
}

constexpr bool supports_pext_tier(const CpuFeatures& features) {
    return supports_avx2_tier(features)
        && features.bmi1 && features.bmi2 && features.lzcnt;
}

constexpr bool supports_tier(const CpuFeatures& features, IsaTier tier) {
    switch (tier) {
    case IsaTier::Base:     return true;
    case IsaTier::Avx2:     return supports_avx2_tier(features);
    case IsaTier::Pext:     return supports_pext_tier(features);
    }
    return false;
}

// The tier a universal binary runs: the highest the CPU supports, except pext
// where pext is slow.
constexpr IsaTier best_tier(const CpuFeatures& features) {
    if (supports_pext_tier(features) && !features.slow_pext) return IsaTier::Pext;
    if (supports_avx2_tier(features)) return IsaTier::Avx2;
    return IsaTier::Base;
}

// CPUID leaf 1's family: the base family, plus the extended family when the
// base is 0xF.
constexpr unsigned cpuid_family(unsigned leaf1_eax) {
    const unsigned base = (leaf1_eax >> 8) & 0xFU;
    return base == 0xFU ? base + ((leaf1_eax >> 20) & 0xFFU) : base;
}

constexpr bool slow_pext_cpu(bool amd, unsigned family) {
    return amd && (family == 0x15 || family == 0x17);
}

constexpr std::string_view tier_name(IsaTier tier) {
    switch (tier) {
    case IsaTier::Base:     return "base";
    case IsaTier::Avx2:     return "avx2";
    case IsaTier::Pext:     return "pext";
    }
    return "?";
}

constexpr std::optional<IsaTier> parse_tier(std::string_view name) {
    for (const IsaTier tier : {IsaTier::Base, IsaTier::Avx2, IsaTier::Pext})
        if (name == tier_name(tier)) return tier;
    return std::nullopt;
}
