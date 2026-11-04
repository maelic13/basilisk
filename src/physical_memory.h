#pragma once

#include <cstdint>

// The UCI protocol maximum Basilisk has always advertised for Hash, in MiB
// (32 TiB), and the minimum every value below it is held to.
inline constexpr int kHashProtocolMaxMb = 33554432;
inline constexpr int kHashMinMb = 1;

// The machine's total physical memory in bytes, as the operating system
// reports it, or 0 where it reports none. Total, not available: available
// memory moves with the file cache and other processes, and would refuse
// tables that fit.
std::uint64_t query_physical_memory_bytes();

// The largest Hash, in MiB, a machine with this much physical memory can
// hold: its whole MiB, capped at the protocol maximum and held to the
// minimum. 0 bytes (not reported) gives the protocol maximum.
constexpr int hash_limit_mb(std::uint64_t physical_bytes) {
    if (physical_bytes == 0)
        return kHashProtocolMaxMb;
    const std::uint64_t mib = physical_bytes >> 20;
    if (mib >= static_cast<std::uint64_t>(kHashProtocolMaxMb))
        return kHashProtocolMaxMb;
    return mib < static_cast<std::uint64_t>(kHashMinMb) ? kHashMinMb : static_cast<int>(mib);
}

// This machine's Hash limit: the operating system is asked once, at the first
// call, and the answer kept for the process.
int machine_hash_limit_mb();
