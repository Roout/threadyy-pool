#include "scheduler.hpp"
#include "thread_pool.hpp"

#include <cassert>

namespace klyaksa {

Scheduler::Scheduler(ThreadPool *executor)
    : executor_ { executor }
    , timer_ {[this](std::stop_token token) { TimerWorker(token); } }
{
}

void Scheduler::ScheduleAt(Timepoint tp, Callback &&cb) {
    auto point = Now();
    if (tp <= point) {
        SubmitToExecutor(std::move(cb));
        return;
    }
    std::unique_lock lock{vault_mutex_};
    vault_.emplace(tp, std::move(cb));
    lock.unlock();
    vault_waiter_.notify_one();
}

void Scheduler::ScheduleAfter(Timeout delay, Callback &&cb) {
    ScheduleAt(Now() + delay, std::move(cb));
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
            if (wakeup_result && !vault_.empty()) {
                // cv predecate return true; don't submit if stopped
                SubmitExpiredBefore(Now());
            }
            // otherwise still need to sleep
        }
        else { // no callback scheduled so wait until any callback will be scheduled
            (void) vault_waiter_.wait_for(lock, kSleepTimeout, need_wakeup);
            // just wake up due to either timeout either stop predicate
        }
    }
};

void Scheduler::SubmitExpiredBefore(Timepoint tp) {
    // time to execute callbacks
    auto right = vault_.upper_bound(tp);
    auto left = vault_.begin();
    while (left != right) {
        if (!SubmitToExecutor(std::move(left->second))) {
            // remove only submitted callbacks
            (void) vault_.erase(vault_.begin(), left);
            throw FullQueueException{"not enough space for callback"};
        }
        left++;
    }
    // remove submitted callbacks
    (void) vault_.erase(vault_.begin(), left);
}

bool Scheduler::SubmitToExecutor(Callback &&cb) {
    assert(executor_);
    assert(!executor_->IsStopped());
    return executor_->Post(std::move(cb));
}

} // namespace klyaksa
