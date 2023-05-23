#include "scheduler.hpp"
#include "thread_pool.hpp"

#include <cassert>

namespace klyaksa {

Scheduler::Scheduler(ThreadPool *executor)
    : executor_ { executor }
    , timer_ {} // default constructible
{}

void Scheduler::ScheduleAt(Timepoint tp, Task &&cb) {
    if (tp <= Now()) {
        SubmitToExecutor(std::move(cb));
        return;
    }
    std::unique_lock lock{vault_mutex_};
    vault_.emplace(tp, std::move(cb));
    lock.unlock();
    vault_waiter_.notify_one();
}

std::size_t Scheduler::CallbackCount() const noexcept {
    std::unique_lock lock{vault_mutex_};
    return vault_.size();
}

void Scheduler::ScheduleAfter(Timeout delay, Task &&cb) {
    ScheduleAt(Now() + delay, std::move(cb));
}

void Scheduler::Stop() {
    if (timer_.joinable()) {
        if (timer_.request_stop()) {
            vault_waiter_.notify_one();
        }
        timer_.join();
    }
    // otherwise assumed that it was default constructible
}

void Scheduler::Start() {
    assert(!timer_.joinable());
    timer_ = std::jthread{[this](std::stop_token token) { TimerWorker(token); }};
}

void Scheduler::TimerWorker(std::stop_token stop_token) {
    static constexpr Timeout kSleepTimeout { 100 };
    auto need_wakeup = [this, stop_token]() noexcept {
        return !vault_.empty() || stop_token.stop_requested();
    };
    while (!stop_token.stop_requested()) {
        const auto now = Now();
        std::unique_lock lock{vault_mutex_};
        if (!vault_.empty()) { // need to wait for the next scheduled callback
            auto next_wakeup = vault_.cbegin()->first;
            if (next_wakeup <= now) {
                // avoid possible signal + timeout report on cv
                SubmitExpiredBefore(now);
                continue;
            }
            next_wakeup = std::min(next_wakeup, now + kSleepTimeout);
            bool wakeup_result = vault_waiter_.wait_until(lock, next_wakeup, need_wakeup);
            if (wakeup_result && !stop_token.stop_requested()) {
                assert(!vault_.empty() && "Cannot be empty otherwise "
                    "it meens CV woke up due to stop request. See predicate.");
                // cv predecate return true; don't submit if stopped
                SubmitExpiredBefore(Now());
            }
            // otherwise still need to sleep
        }
        else { // no callback scheduled so wait
            (void) vault_waiter_.wait_for(lock, kSleepTimeout, need_wakeup);
            // just wake up due to either timeout either stop predicate
        }
    }
};

void Scheduler::SubmitExpiredBefore(Timepoint tp) {
    // time to execute callbacks
    auto right = vault_.upper_bound(tp);
    auto left = vault_.begin();
    try {
        while (left != right) {
            if (!SubmitToExecutor(std::move(left->second))) {
                // TODO: handle this cases:
                // - [ ] queue is full
                // - [ ] excecutor is stopped
                break;
            }
            left++;
        }
    }
    catch (...) {
        // handle possible exception
    }
    // remove only submitted callbacks
    (void) vault_.erase(vault_.begin(), left);
}

bool Scheduler::SubmitToExecutor(Task &&cb) {
    assert(executor_);
    if (executor_->IsStopped()) {
        return false;
    }
    return executor_->Post(std::move(cb));
}

} // namespace klyaksa
