#pragma once
#include <thread>
#include <atomic>
#include <future>
#include <mutex>
#include <condition_variable>

#include <chrono>
#include <functional>
#include <map>

namespace klyaksa {

class ThreadPool;

using Timeout = std::chrono::milliseconds;
using Timepoint = std::chrono::steady_clock::time_point;
using Callback = std::move_only_function<void()>;

class FullQueueException : public std::exception {
public:
    using std::exception::exception;
};

class Scheduler {
public:
    
    Scheduler(ThreadPool *executor);

    void ScheduleAt(Timepoint tp, Callback &&cb);

    void ScheduleAfter(Timeout delay, Callback &&cb);

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

    bool SubmitToExecutor(Callback &&cb);

    Timepoint Now() const noexcept {
        return std::chrono::steady_clock::now();
    }

private:
    ThreadPool *executor_;

    mutable std::mutex vault_mutex_;
    std::condition_variable vault_waiter_;
    std::multimap<Timepoint, Callback> vault_;
    std::jthread timer_;
};

} // namespace klyaksa
