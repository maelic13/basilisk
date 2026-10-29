// Prints the tier the building machine runs fastest, by the universal binary's
// rule (src/cpu_detect.h); CMakeLists.txt turns PEXT on for a native build
// when it is "pext".
#include <cstdio>

#include "cpu_detect.h"

int main() {
    const auto name = tier_name(best_tier(detect_cpu_features()));
    std::fwrite(name.data(), 1, name.size(), stdout);
    return 0;
}
