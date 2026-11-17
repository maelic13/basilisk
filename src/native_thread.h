#pragma once

// A thread with a stated stack size, for every thread that runs a search: the
// engine thread (the main searcher) and the pool's helpers. std::thread takes
// the platform's default, which differs: 512 KB for a second thread on macOS,
// the executable's reserve on Windows (1 MiB in a release build), 8 MB on
// Linux. A line to the ply cap needs about 0.2 MB of release frames, more in
// a debug or sanitizer build, so every search thread gets 8 MB everywhere, as
// Stockfish's threads do.

#include <cstddef>
#include <functional>
#include <memory>

class NativeThread {
public:
    static constexpr std::size_t kStackBytes = std::size_t{8} * 1024 * 1024;

    NativeThread() noexcept = default;
    // Starts `body` on a new thread; throws std::system_error if the system
    // will not create one. An exception that leaves `body` ends the process
    // through std::terminate, as on a std::thread.
    explicit NativeThread(std::function<void()> body);
    ~NativeThread();

    NativeThread(NativeThread&& other) noexcept;
    NativeThread& operator=(NativeThread&& other) noexcept;
    NativeThread(const NativeThread&) = delete;
    NativeThread& operator=(const NativeThread&) = delete;

    [[nodiscard]] bool joinable() const noexcept { return handle_ != nullptr; }
    void join();

private:
    void* handle_ = nullptr;   // a HANDLE on Windows, a heap pthread_t elsewhere
};
