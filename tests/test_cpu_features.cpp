#include "cpu_features.h"
#include "test_harness.h"

int main() {
    begin_section("AVX2 tier requires every advertised feature");
    constexpr CpuFeatures avx2_ok{true, true, true, false, false, false};
    EXPECT(supports_avx2_tier(avx2_ok));
    EXPECT(!supports_avx2_tier(CpuFeatures{false, true, true, false, false, false}));
    EXPECT(!supports_avx2_tier(CpuFeatures{true, false, true, false, false, false}));
    EXPECT(!supports_avx2_tier(CpuFeatures{true, true, false, false, false, false}));
    end_section();

    begin_section("PEXT tier requires the complete feature contract");
    constexpr CpuFeatures pext_ok{true, true, true, true, true, true};
    EXPECT(supports_pext_tier(pext_ok));
    EXPECT(!supports_pext_tier(CpuFeatures{false, true, true, true, true, true}));
    EXPECT(!supports_pext_tier(CpuFeatures{true, false, true, true, true, true}));
    EXPECT(!supports_pext_tier(CpuFeatures{true, true, false, true, true, true}));
    EXPECT(!supports_pext_tier(CpuFeatures{true, true, true, false, true, true}));
    EXPECT(!supports_pext_tier(CpuFeatures{true, true, true, true, false, true}));
    EXPECT(!supports_pext_tier(CpuFeatures{true, true, true, true, true, false}));
    end_section();

    begin_section("Dispatch picks the highest supported tier");
    constexpr CpuFeatures baseline{};
    constexpr CpuFeatures nehalem{true, true, false, false, false, false};  // SSE4.1 and POPCNT
    EXPECT(best_tier(baseline) == IsaTier::Portable);
    EXPECT(best_tier(nehalem) == IsaTier::Portable);
    EXPECT(best_tier(avx2_ok) == IsaTier::Avx2);
    EXPECT(best_tier(pext_ok) == IsaTier::Pext);
    // AVX2 without LZCNT (or BMI1, or BMI2) cannot run the pext tier.
    EXPECT(best_tier(CpuFeatures{true, true, true, true, true, false}) == IsaTier::Avx2);
    for (const IsaTier tier : {IsaTier::Portable, IsaTier::Avx2, IsaTier::Pext}) {
        EXPECT(supports_tier(pext_ok, tier));
        EXPECT(supports_tier(baseline, tier) == (tier == IsaTier::Portable));
    }
    EXPECT(!supports_tier(avx2_ok, IsaTier::Pext));
    end_section();

    begin_section("Slow pext on AMD family 15h and 17h selects avx2");
    // Leaf 1 EAX: base family 0xF plus the extended family in bits 20-27.
    EXPECT(cpuid_family(0x00600F20U) == 0x15);  // Excavator
    EXPECT(cpuid_family(0x00800F11U) == 0x17);  // Zen
    EXPECT(cpuid_family(0x00870F10U) == 0x17);  // Zen 2
    EXPECT(cpuid_family(0x00A20F10U) == 0x19);  // Zen 3
    EXPECT(cpuid_family(0x00B40F40U) == 0x1A);  // Zen 5
    EXPECT(cpuid_family(0x000906EAU) == 0x6);   // Intel Coffee Lake: no extended family
    EXPECT(slow_pext_cpu(true, 0x15));
    EXPECT(slow_pext_cpu(true, 0x17));
    EXPECT(!slow_pext_cpu(true, 0x19));
    EXPECT(!slow_pext_cpu(true, 0x1A));
    EXPECT(!slow_pext_cpu(false, 0x17));
    CpuFeatures zen2 = pext_ok;
    zen2.slow_pext = true;
    EXPECT(supports_tier(zen2, IsaTier::Pext));
    EXPECT(best_tier(zen2) == IsaTier::Avx2);
    end_section();

    begin_section("BASILISK_ISA names a tier");
    EXPECT(parse_tier("portable") == IsaTier::Portable);
    EXPECT(parse_tier("avx2") == IsaTier::Avx2);
    EXPECT(parse_tier("pext") == IsaTier::Pext);
    EXPECT(!parse_tier("PEXT").has_value());
    EXPECT(!parse_tier("bmi2").has_value());
    EXPECT(!parse_tier("").has_value());
    end_section();

    return harness_summary();
}
