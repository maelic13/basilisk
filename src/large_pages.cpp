#include "large_pages.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <unordered_map>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#if defined(__linux__)
#include <fstream>
#include <string>
#endif
#endif

namespace large_pages {
namespace {

std::atomic<bool> g_enabled{BASILISK_LARGE_PAGES_DEFAULT != 0};

struct Record {
    std::size_t size = 0;  // the mapping's own size, for release
    bool asked = false;    // the option was on when it was allocated
    bool large = false;    // large pages back it
};

std::mutex& registry_mutex() {
    static std::mutex mutex;
    return mutex;
}
std::unordered_map<void*, Record>& registry() {
    static std::unordered_map<void*, Record> map;
    return map;
}

std::size_t round_up(std::size_t bytes, std::size_t unit) {
    return (bytes + unit - 1) / unit * unit;
}

#if defined(_WIN32)

// Large pages under the account's "Lock pages in memory" right, enabled for
// this call only; nullptr when the right, the size or the free pages are
// missing.
void* try_large(std::size_t bytes, std::size_t& size) {
    const SIZE_T page = GetLargePageMinimum();
    if (page == 0)
        return nullptr;
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
        return nullptr;
    void* memory = nullptr;
    LUID luid{};
    if (LookupPrivilegeValueA(nullptr, "SeLockMemoryPrivilege", &luid)) {
        TOKEN_PRIVILEGES want{};
        TOKEN_PRIVILEGES previous{};
        DWORD previous_length = 0;
        want.PrivilegeCount = 1;
        want.Privileges[0].Luid = luid;
        want.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        // AdjustTokenPrivileges succeeds without the right and says so only
        // through GetLastError.
        if (AdjustTokenPrivileges(token, FALSE, &want, sizeof(previous), &previous, &previous_length)
            && GetLastError() == ERROR_SUCCESS) {
            size = round_up(bytes, page);
            memory = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES,
                                  PAGE_READWRITE);
            AdjustTokenPrivileges(token, FALSE, &previous, 0, nullptr, nullptr);
        }
    }
    CloseHandle(token);
    return memory;
}

void* small_pages(std::size_t bytes, std::size_t& size) {
    size = bytes;
    return VirtualAlloc(nullptr, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void unmap(void* p, std::size_t) { VirtualFree(p, 0, MEM_RELEASE); }

#else

void* map_anonymous(std::size_t size, int extra_flags) {
    void* p = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | extra_flags,
                   -1, 0);
    return p == MAP_FAILED ? nullptr : p;
}

void unmap(void* p, std::size_t size) { munmap(p, size); }

#if defined(__linux__)

constexpr std::size_t kHugePage = std::size_t{2} << 20;

// Transparent huge pages in "always" or "madvise" mode: the kernel honours
// madvise(MADV_HUGEPAGE). Read once.
bool transparent_huge_pages() {
    static const bool available = [] {
        std::ifstream in("/sys/kernel/mm/transparent_hugepage/enabled");
        std::string mode;
        std::getline(in, mode);
        return mode.find("[always]") != std::string::npos
            || mode.find("[madvise]") != std::string::npos;
    }();
    return available;
}

// A 2 MiB-aligned mapping: map a page more than asked, then trim both ends.
void* map_aligned(std::size_t size) {
    void* raw = map_anonymous(size + kHugePage, 0);
    if (!raw)
        return nullptr;
    const auto begin = reinterpret_cast<std::uintptr_t>(raw);
    const std::uintptr_t aligned = (begin + kHugePage - 1) & ~std::uintptr_t(kHugePage - 1);
    if (aligned > begin)
        munmap(raw, aligned - begin);
    const std::size_t tail = (begin + size + kHugePage) - (aligned + size);
    if (tail)
        munmap(reinterpret_cast<void*>(aligned + size), tail);
    return reinterpret_cast<void*>(aligned);
}

void* try_large(std::size_t bytes, std::size_t& size, bool& large) {
#if defined(MAP_HUGETLB) && defined(MAP_HUGE_SHIFT) && defined(__x86_64__)
    constexpr std::size_t kGiantPage = std::size_t{1} << 30;
    if (bytes >= kGiantPage) {
        size = round_up(bytes, kGiantPage);
        if (void* p = map_anonymous(size, MAP_HUGETLB | (30 << MAP_HUGE_SHIFT))) {
            large = true;
            return p;
        }
    }
#endif
    size = round_up(bytes, kHugePage);
    void* p = map_aligned(size);
#if defined(MADV_HUGEPAGE)
    if (p) {
        madvise(p, size, MADV_HUGEPAGE);
        large = transparent_huge_pages();
    }
#endif
    return p;
}

#endif  // __linux__

void* small_pages(std::size_t bytes, std::size_t& size) {
    size = round_up(bytes, 4096);
    return map_anonymous(size, 0);
}

#endif  // _WIN32

}  // namespace

void set_enabled(bool on) noexcept { g_enabled.store(on, std::memory_order_relaxed); }
bool enabled() noexcept { return g_enabled.load(std::memory_order_relaxed); }
bool default_enabled() noexcept { return BASILISK_LARGE_PAGES_DEFAULT != 0; }

void* allocate(std::size_t bytes) noexcept {
    if (bytes == 0)
        bytes = 1;
    Record record;
    record.asked = enabled();
    void* p = nullptr;
    if (record.asked) {
#if defined(_WIN32)
        p = try_large(bytes, record.size);
        record.large = p != nullptr;
#elif defined(__linux__)
        p = try_large(bytes, record.size, record.large);
#endif
    }
    if (!p)
        p = small_pages(bytes, record.size);
    if (!p)
        return nullptr;
    try {
        std::scoped_lock lock(registry_mutex());
        registry().emplace(p, record);
    } catch (...) {
        unmap(p, record.size);
        return nullptr;
    }
    return p;
}

void release(void* p) noexcept {
    if (!p)
        return;
    std::size_t size = 0;
    {
        std::scoped_lock lock(registry_mutex());
        auto it = registry().find(p);
        if (it == registry().end())
            return;
        size = it->second.size;
        registry().erase(it);
    }
    unmap(p, size);
}

Status status() noexcept {
    if (!enabled())
        return Status::Off;
    int asked = 0;
    int large = 0;
    {
        std::scoped_lock lock(registry_mutex());
        for (const auto& [p, record] : registry()) {
            asked += record.asked;
            large += record.asked && record.large;
        }
    }
    if (asked == 0 || large == 0)
        return Status::NotAvailable;
    return large == asked ? Status::On : Status::Partial;
}

const char* status_name(Status status) noexcept {
    switch (status) {
        case Status::On:           return "on";
        case Status::Partial:      return "partial";
        case Status::Off:          return "off";
        case Status::NotAvailable: return "not available";
    }
    return "unknown";
}

}  // namespace large_pages
