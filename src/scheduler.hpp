#pragma once
#include <thread>
#include <atomic>
#include <future>
#include <mutex>
#include <condition_variable>

#include <chrono>
#include <functional>
#include <map>

#include "task.hpp"

namespace klyaksa {

class ThreadPool;

using Timeout = std::chrono::milliseconds;
using Timepoint = std::chrono::steady_clock::time_point;

class FullQueueException : public std::exception {
public:
    using std::exception::exception;
};

class Scheduler {
public:
    
    Scheduler(ThreadPool *executor);

    void ScheduleAt(Timepoint tp, Task &&cb);

    void ScheduleAfter(Timeout delay, Task &&cb);

    std::size_t CallbackCount() const noexcept;

    /**
     * Stop scheduler (background thread-worker).
     * Before stopping it may submit some expired tasks for execution
     * or be in the middle of submitting.
     * 
     * Blocks execution thread waiting for thread to join.
     * UB if called concurrently
     * 
     * Note, the executor is not stopped only scheduler
    */
    void Stop();

    /**
     * Start scheduler's worker background thread.
     */
    void Start();

    bool IsStopped() const noexcept {
        return !timer_.joinable();
    }

private:
    /**
     * Background worker: track time for callbacks
     **/
    void TimerWorker(std::stop_token stop_token);

    /**
     * Submit all callbacks for execution
     * 
     * @param tp expiration date - everything before this time point will be send to executor and removed
     * @note thread-safe
     **/
    void SubmitExpiredBefore(Timepoint tp);

    bool SubmitToExecutor(Task &&cb);

    Timepoint Now() const noexcept {
        return std::chrono::steady_clock::now();
    }

private:
    ThreadPool *executor_;

    mutable std::mutex vault_mutex_;
    // TODO: switch to `std::condition_variable_any` which can use `std::stop_token`
    std::condition_variable vault_waiter_;
    std::multimap<Timepoint, Task> vault_;
    std::jthread timer_;
};

} // namespace klyaksa
