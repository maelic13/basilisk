// Triumviratus, a UCI chess engine.
// Copyright (C) 2026 Francesco Torsello
// SPDX-License-Identifier: GPL-3.0-or-later (see COPYING)

// large_pages.cpp -- see large_pages.h (block 7 of 8.1, 18/10/2026).

#include "large_pages.h"

#if defined(_WIN32)
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0601   // GetLargePageMinimum
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>

namespace {

// The three token functions live in advapi32, loaded once at run time.
using OpenProcessTokenFn      = BOOL(WINAPI*)(HANDLE, DWORD, PHANDLE);
using LookupPrivilegeValueFn  = BOOL(WINAPI*)(LPCSTR, LPCSTR, PLUID);
using AdjustTokenPrivilegesFn = BOOL(WINAPI*)(HANDLE, BOOL, PTOKEN_PRIVILEGES, DWORD, PTOKEN_PRIVILEGES, PDWORD);

struct TokenApi {
    OpenProcessTokenFn      open   = nullptr;
    LookupPrivilegeValueFn  lookup = nullptr;
    AdjustTokenPrivilegesFn adjust = nullptr;
};

const TokenApi& token_api() {
    static const TokenApi api = [] {
        TokenApi a;
        if (HMODULE m = LoadLibraryA("advapi32.dll")) {
            a.open   = reinterpret_cast<OpenProcessTokenFn>(reinterpret_cast<void*>(GetProcAddress(m, "OpenProcessToken")));
            a.lookup = reinterpret_cast<LookupPrivilegeValueFn>(
              reinterpret_cast<void*>(GetProcAddress(m, "LookupPrivilegeValueA")));
            a.adjust = reinterpret_cast<AdjustTokenPrivilegesFn>(
              reinterpret_cast<void*>(GetProcAddress(m, "AdjustTokenPrivileges")));
        }
        return a;
    }();
    return api;
}

// What lock_memory_privilege_enable changed, to be put back.
struct SavedToken {
    HANDLE           token;
    TOKEN_PRIVILEGES previous;
};

}  // namespace

size_t large_page_minimum() { return GetLargePageMinimum(); }

bool lock_memory_privilege_enable(void** saved) {
    *saved = nullptr;
    const TokenApi& api = token_api();
    if (!api.open || !api.lookup || !api.adjust) return false;
    HANDLE token = nullptr;
    if (!api.open(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) return false;
    TOKEN_PRIVILEGES want{};
    want.PrivilegeCount           = 1;
    want.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!api.lookup(nullptr, "SeLockMemoryPrivilege", &want.Privileges[0].Luid)) {
        CloseHandle(token);
        return false;
    }
    SavedToken* s   = new SavedToken{token, {}};
    DWORD       len = 0;
    // The call also succeeds when the account does not hold the right: GetLastError() then says ERROR_NOT_ALL_ASSIGNED.
    if (!api.adjust(token, FALSE, &want, sizeof(s->previous), &s->previous, &len) || GetLastError() != ERROR_SUCCESS) {
        CloseHandle(token);
        delete s;
        return false;
    }
    *saved = s;
    return true;
}

void lock_memory_privilege_restore(void* saved) {
    if (!saved) return;
    SavedToken* s = static_cast<SavedToken*>(saved);
    if (s->previous.PrivilegeCount) token_api().adjust(s->token, FALSE, &s->previous, 0, nullptr, nullptr);
    CloseHandle(s->token);
    delete s;
}

void* large_pages_alloc(std::size_t size) {
    void* p = with_lock_memory_privilege(
      [size](std::size_t lp) -> void* {
          const std::size_t rounded = (size + lp - 1) / lp * lp;
          return VirtualAlloc(nullptr, rounded, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
      },
      []() -> void* { return nullptr; });
    if (!p) p = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    return p;
}

void large_pages_free(void* mem) {
    if (mem) VirtualFree(mem, 0, MEM_RELEASE);
}

bool large_pages_available() {
    static const bool yes = [] {
        void* p = with_lock_memory_privilege(
          [](std::size_t lp) -> void* {
              return VirtualAlloc(nullptr, lp, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
          },
          []() -> void* { return nullptr; });
        if (p) VirtualFree(p, 0, MEM_RELEASE);
        return p != nullptr;
    }();
    return yes;
}

#else   // POSIX

    #include <cstdio>
    #include <cstdlib>
    #include <cstring>
    #if defined(__linux__)
        #include <sys/mman.h>
    #endif

namespace {
#if defined(__linux__)
constexpr std::size_t PAGE = 2 * 1024 * 1024;   // a transparent huge page
#else
constexpr std::size_t PAGE = 4096;
#endif
}  // namespace

void* large_pages_alloc(std::size_t size) {
    const std::size_t rounded = (size + PAGE - 1) / PAGE * PAGE;
    void*             p       = nullptr;
    if (posix_memalign(&p, PAGE, rounded ? rounded : PAGE) != 0) return nullptr;
    #if defined(__linux__) && defined(MADV_HUGEPAGE)
    madvise(p, rounded, MADV_HUGEPAGE);
    #endif
    return p;
}

void large_pages_free(void* mem) { std::free(mem); }

bool large_pages_available() {
    #if defined(__linux__)
    // "always" or "madvise" between brackets: the kernel backs madvise'd memory with huge pages.
    static const bool yes = [] {
        char buf[128] = {0};
        if (std::FILE* f = std::fopen("/sys/kernel/mm/transparent_hugepage/enabled", "r")) {
            const std::size_t n = std::fread(buf, 1, sizeof(buf) - 1, f);
            std::fclose(f);
            buf[n] = 0;
        }
        return std::strstr(buf, "[always]") != nullptr || std::strstr(buf, "[madvise]") != nullptr;
    }();
    return yes;
    #else
    return false;
    #endif
}

#endif
