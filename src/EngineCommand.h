#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <future>
#include <mutex>
#include <string>

enum class EngineCommandType {
    SetOption,
    Position,
    NewGame,
    Go,
    Stop,
    PonderHit,
    Bench,
    Ready,
    Quit
};

struct EngineCommand {
    EngineCommandType type = EngineCommandType::Ready;
    std::string args;
    std::shared_ptr<std::promise<void>> ack;
    uint64_t epoch = 0;
    // Wall-clock instant the command was parsed off the UCI input (set for Go).
    // Lets the search measure the dispatch latency the GUI already charges
    // (Step 5.3 diagnostic; Step 5.4 will start the clock from here).
    std::chrono::steady_clock::time_point recv_time{};
};

class EngineCommandQueue {
public:
    void push(EngineCommand command) {
        {
            std::lock_guard lock(mutex_);
            commands_.push_back(std::move(command));
        }
        cv_.notify_one();
    }

    void push_priority(EngineCommand command) {
        {
            std::lock_guard lock(mutex_);
            commands_.push_front(std::move(command));
        }
        cv_.notify_one();
    }

    EngineCommand wait_pop() {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [&] { return !commands_.empty(); });
        EngineCommand command = std::move(commands_.front());
        commands_.pop_front();
        return command;
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<EngineCommand> commands_;
};
