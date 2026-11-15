#pragma once

// Large pages for the engine's big allocations: the transposition table, each
// search thread's state and its heap history tables. A large page maps 2 MiB
// (or 1 GiB) with one TLB entry where a small page maps 4 KiB, so the table's
// random probes and the histories' scattered reads miss the TLB far less.
//
// Each platform asks the way Stockfish 19 does (src/memory.cpp at sf_19):
// - Windows x64 and ARM64: VirtualAlloc(MEM_LARGE_PAGES) under the account's
//   "Lock pages in memory" right (SeLockMemoryPrivilege), enabled for the call
//   and restored after it;
// - Linux: explicit 1 GiB pages (MAP_HUGETLB) for an allocation that size or
//   larger, where an administrator reserved them; then transparent huge pages,
//   madvise(MADV_HUGEPAGE) on a 2 MiB-aligned mapping;
// - macOS (Apple silicon) and every other platform: none exist to ask for
//   (Apple silicon has only 4 and 16 KiB pages and no userspace superpage), so
//   plain pages, reported as not available.
// Every request falls back silently to ordinary pages. Memory comes zeroed
// from the operating system, page aligned (2 MiB aligned where large pages
// were tried), and is released with release().

#include <cstddef>
#include <memory>
#include <new>
#include <utility>

namespace large_pages {

// Whether new allocations ask for large pages: the LargePages UCI option. An
// allocation keeps what it got; the engine reallocates when the option moves.
void set_enabled(bool on) noexcept;
[[nodiscard]] bool enabled() noexcept;
// The option's default: true unless built with -DLARGE_PAGES=OFF.
[[nodiscard]] bool default_enabled() noexcept;

// `bytes` of zeroed memory, or nullptr when the system has none to give.
[[nodiscard]] void* allocate(std::size_t bytes) noexcept;
void release(void* p) noexcept;

// What the live allocations got. On: every one asked for and got large
// pages. Partial: some did. Off: the option is off. NotAvailable: the option
// is on and none could be given (the platform, the account's right, or the
// system's free large pages).
enum class Status { On, Partial, Off, NotAvailable };
[[nodiscard]] Status status() noexcept;
[[nodiscard]] const char* status_name(Status status) noexcept;

}  // namespace large_pages

// An owning pointer to one object on large_pages memory.
template<class T>
struct LargePageDeleter {
    void operator()(T* p) const noexcept {
        if (!p)
            return;
        p->~T();
        large_pages::release(p);
    }
};
template<class T>
using LargePagePtr = std::unique_ptr<T, LargePageDeleter<T>>;

// Constructs a T on large_pages memory; throws std::bad_alloc when there is
// none. The memory is page aligned, which covers every alignment T needs.
template<class T, class... Args>
[[nodiscard]] LargePagePtr<T> make_large_page(Args&&... args) {
    static_assert(alignof(T) <= 4096, "large_pages memory is page aligned");
    void* memory = large_pages::allocate(sizeof(T));
    if (!memory)
        throw std::bad_alloc();
    try {
        return LargePagePtr<T>(::new (memory) T(std::forward<Args>(args)...));
    } catch (...) {
        large_pages::release(memory);
        throw;
    }
}
