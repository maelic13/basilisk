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

    return harness_summary();
}
