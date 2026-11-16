#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

// How the engine stops a run of many searches (`bench`, `wac`): its own stop
// flag, which `stop` and `quit` raise, and a test that says whether a control
// command has arrived since the run's own (the engine's control epoch moved).
// A default control never stops the run.
struct RunControl {
    const std::atomic_bool* stop = nullptr;
    std::function<bool()> cancelled;
};

// Watches a RunControl for the length of a run. The run's searches cannot
// watch the engine's flag themselves: the pool raises its flag at the end of
// every search to release its helpers, so each search has a flag of its own,
// and the watch raises that flag when the engine's is raised.
//
// The run clears its flag before each search and reads stopped() after the
// clear: the watch records the stop before it raises the flag, so a stop
// that lands between the two is seen either way.
class RunWatch {
public:
    RunWatch(const RunControl& control, std::atomic_bool& search_stop)
        : control_(control), search_stop_(search_stop) {
        if (control_.stop || control_.cancelled)
            thread_ = std::thread([this] { watch(); });
    }
    ~RunWatch() {
        done_.store(true, std::memory_order_release);
        if (thread_.joinable())
            thread_.join();
    }
    RunWatch(const RunWatch&) = delete;
    RunWatch& operator=(const RunWatch&) = delete;

    [[nodiscard]] bool stopped() const { return stopped_.load(std::memory_order_seq_cst); }

private:
    void watch() {
        while (!done_.load(std::memory_order_acquire)) {
            const bool flag = control_.stop && control_.stop->load(std::memory_order_acquire);
            if (flag || (control_.cancelled && control_.cancelled())) {
                stopped_.store(true, std::memory_order_seq_cst);
                search_stop_.store(true, std::memory_order_seq_cst);
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }

    const RunControl& control_;
    std::atomic_bool& search_stop_;
    std::atomic_bool stopped_{false};
    std::atomic_bool done_{false};
    std::thread thread_;
};
