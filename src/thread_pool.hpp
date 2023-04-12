#pragma once

#include <vector>
#include <future>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdint>

#include "ccqueue.hpp"

namespace klyaksa {

// TODO: add timer: execute after some time

/// execution context
class ThreadPool final {
public:
    static constexpr std::size_t kTaskQueueSize { 255 };
    static constexpr bool kTaskQueueSentinel { true };

    using Task = std::packaged_task<void()>;

    ThreadPool(std::size_t threads);

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;

    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    ~ThreadPool();
    
    bool Post(Task task);

    void Start();
    void Stop() noexcept;

    std::size_t Size() const noexcept {
        return worker_count_;
    }
    std::size_t GetActiveTasks() const noexcept {
        return active_tasks_.load(std::memory_order_acquire);
    }
    bool IsStopped() const noexcept {
        return stopped_.load(std::memory_order_acquire);
    }
private:
    using Queue = CcQueue<Task, kTaskQueueSize>;

    const std::size_t worker_count_ { 0 };
    std::atomic<bool> stopped_ { true };
    // number of tasks currently running
    std::atomic<std::size_t> active_tasks_ { 0 };
    std::vector<std::jthread> workers_;
    Queue pending_tasks_;
};

} // namespace klyaksa
