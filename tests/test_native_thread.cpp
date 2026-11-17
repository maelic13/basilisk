/// Search threads have the stack they state (src/native_thread.h).
///
/// A search line to the ply cap needs about 0.2 MB of release frames, more in
/// a debug or sanitizer build, and the platforms' defaults for a second thread
/// differ: 512 KB on macOS, 1 MiB on Windows (a release executable's reserve),
/// 8 MB on Linux. Every search thread states 8 MB instead. This test uses 6 MiB
/// of stack on a NativeThread, which a 1 MiB or 512 KB default cannot hold.
///
///     test_native_thread [--std-thread]
///
/// --std-thread runs the same work on a std::thread: the negative control, which
/// ends the process with a stack overflow where the default is smaller.

#include "native_thread.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>

namespace {

constexpr int kFrameBytes = 64 * 1024;
constexpr int kFrames = 96;   // 6 MiB

// Each frame holds kFrameBytes and touches both ends, so its pages are used,
// and is written again after its call returns, so the frames cannot be merged
// into a loop: all of them are live at the deepest call.
std::uintptr_t g_top = 0, g_bottom = 0;   // the outermost and innermost frames

[[gnu::noinline]] int deep(int frames) {
    volatile char frame[kFrameBytes];
    frame[0] = static_cast<char>(frames);
    frame[kFrameBytes - 1] = static_cast<char>(frames >> 1);
    if (frames == kFrames)
        g_top = reinterpret_cast<std::uintptr_t>(&frame[0]);
    if (frames == 0) {
        g_bottom = reinterpret_cast<std::uintptr_t>(&frame[0]);
        return frame[0];
    }
    const int below = deep(frames - 1);
    frame[1] = static_cast<char>(below);
    return below + frame[kFrameBytes - 1] + (frame[1] - static_cast<char>(below));
}

}  // namespace

int main(int argc, char** argv) {
    const bool std_thread = argc > 1 && std::strcmp(argv[1], "--std-thread") == 0;
    int result = -1;
    if (std_thread) {
        std::thread thread([&] { result = deep(kFrames); });
        thread.join();
    } else {
        NativeThread thread([&] { result = deep(kFrames); });
        thread.join();
    }
    // deep() returns the sum of frames >> 1 over the recursion, a fixed value.
    int expected = 0;
    for (int f = 1; f <= kFrames; ++f)
        expected += static_cast<char>(f >> 1);
    const bool ok = result == expected;
    const double span = static_cast<double>(g_top > g_bottom ? g_top - g_bottom : g_bottom - g_top);
    std::printf("%s: %d frames of %d KiB on a %s, %.2f MiB of stack\n", ok ? "ok" : "FAILED",
                kFrames, kFrameBytes / 1024, std_thread ? "std::thread" : "NativeThread",
                span / (1024.0 * 1024.0));
    return ok ? 0 : 1;
}
