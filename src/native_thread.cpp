#include "native_thread.h"

#include <cerrno>
#include <exception>
#include <system_error>
#include <utility>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#endif

namespace {

struct Start {
    std::function<void()> body;
};

void run(Start* start) {
    const std::unique_ptr<Start> owned(start);
    try {
        owned->body();
    } catch (...) {
        // Reported by the fatal-error handler (output_sink.h), which reads
        // the current exception, as an escape from a std::thread is.
        std::terminate();
    }
}

#ifdef _WIN32
unsigned __stdcall trampoline(void* arg) {
    run(static_cast<Start*>(arg));
    return 0;
}
#else
void* trampoline(void* arg) {
    run(static_cast<Start*>(arg));
    return nullptr;
}
#endif

}  // namespace

NativeThread::NativeThread(std::function<void()> body) {
    auto start = std::make_unique<Start>(Start{std::move(body)});
#ifdef _WIN32
    // The size is the stack's reservation; pages are committed as it grows.
    const uintptr_t handle = _beginthreadex(nullptr, static_cast<unsigned>(kStackBytes), trampoline,
                                            start.get(), STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr);
    if (handle == 0)
        throw std::system_error(errno, std::generic_category(), "thread creation");
    start.release();
    handle_ = reinterpret_cast<void*>(handle);
#else
    pthread_attr_t attr;
    int rc = pthread_attr_init(&attr);
    if (rc == 0)
        rc = pthread_attr_setstacksize(&attr, kStackBytes);
    auto thread = std::make_unique<pthread_t>();
    if (rc == 0)
        rc = pthread_create(thread.get(), &attr, trampoline, start.get());
    pthread_attr_destroy(&attr);
    if (rc != 0)
        throw std::system_error(rc, std::generic_category(), "thread creation");
    start.release();
    handle_ = thread.release();
#endif
}

NativeThread::~NativeThread() {
    if (joinable())
        join();
}

NativeThread::NativeThread(NativeThread&& other) noexcept
    : handle_(std::exchange(other.handle_, nullptr)) {}

NativeThread& NativeThread::operator=(NativeThread&& other) noexcept {
    if (this != &other) {
        if (joinable())
            join();
        handle_ = std::exchange(other.handle_, nullptr);
    }
    return *this;
}

void NativeThread::join() {
    if (!joinable())
        return;
#ifdef _WIN32
    const HANDLE handle = static_cast<HANDLE>(handle_);
    WaitForSingleObject(handle, INFINITE);
    CloseHandle(handle);
#else
    auto* thread = static_cast<pthread_t*>(handle_);
    pthread_join(*thread, nullptr);
    delete thread;
#endif
    handle_ = nullptr;
}
