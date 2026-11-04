#ifndef TRIUMV_FROZEN_H
#define TRIUMV_FROZEN_H

// Decides ONCE whether this build freezes the tuning parameters.
//
// 🔴 Why a header of its own. The guard was inside defs.h, and consilium_bridge.cpp
// does NOT include defs.h: in a shipping build, where only
// -DTRIUMV_RELEASE is passed, TRIUMV_FROZEN ended up defined in threads.cpp and NOT in
// consilium_bridge.cpp. The ten constants of the evaluation blend would have
// stayed variables, with their seven divisions by an unknown divisor, and nobody would
// have noticed: the bench does not change, because freezing or not gives the same
// tree. A gain that disappears without leaving a trace is the worst way to
// lose it. Checked with #pragma message on both units before and after.
//
// TRIUMV_RELEASE already means "no tuning", so freezing is the same
// decision taken once. -DTRIUMV_FROZEN forces the freezing in a test
// build too (needed to measure it in isolation); -DTRIUMV_NO_FROZEN turns it off in
// a shipping build.
#if defined(TRIUMV_RELEASE) && !defined(TRIUMV_NO_FROZEN) && !defined(TRIUMV_FROZEN)
    #define TRIUMV_FROZEN 1
#endif

#endif  // TRIUMV_FROZEN_H
