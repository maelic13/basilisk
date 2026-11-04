#include "physical_memory.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <sys/types.h>
#elif defined(__unix__)
#include <unistd.h>
#endif

// One query per platform, each a single system call with no allocation:
// GlobalMemoryStatusEx on Windows, the `hw.memsize` sysctl on macOS, the
// physical page count times the page size elsewhere on POSIX. A failed or
// missing query reports 0, which keeps the protocol maximum.
std::uint64_t query_physical_memory_bytes() {
#if defined(_WIN32)
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status) == 0)
        return 0;
    return static_cast<std::uint64_t>(status.ullTotalPhys);
#elif defined(__APPLE__)
    std::uint64_t bytes = 0;
    size_t length = sizeof(bytes);
    if (sysctlbyname("hw.memsize", &bytes, &length, nullptr, 0) != 0 || length != sizeof(bytes))
        return 0;
    return bytes;
#elif defined(__unix__) && defined(_SC_PHYS_PAGES) && defined(_SC_PAGE_SIZE)
    const long pages = sysconf(_SC_PHYS_PAGES);
    const long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages <= 0 || page_size <= 0)
        return 0;
    const auto page_count = static_cast<std::uint64_t>(pages);
    const auto page_bytes = static_cast<std::uint64_t>(page_size);
    if (page_count > UINT64_MAX / page_bytes)
        return UINT64_MAX;
    return page_count * page_bytes;
#else
    return 0;
#endif
}

int machine_hash_limit_mb() {
    static const int limit = hash_limit_mb(query_physical_memory_bytes());
    return limit;
}
