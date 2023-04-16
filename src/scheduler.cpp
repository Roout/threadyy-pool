#include "scheduler.hpp"
#include "thread_pool.hpp"

#include <cassert>

namespace klyaksa {

Scheduler::Scheduler(ThreadPool *executor)
    : executor_ { executor }
    , timer_ {[this](std::stop_token token) { TimerWorker(token); } }
    , vault_add_ { false }
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
    vault_add_ = true;
    vault_waiter_.notify_one();
}

void Scheduler::ScheduleAfter(Timeout delay, Callback &&cb) {
    ScheduleAt(Now() + delay, std::move(cb));
}

void Scheduler::TimerWorker(std::stop_token stop_token) {
    auto need_wakeup = [this, stop_token]() noexcept {
        return vault_add_ || stop_token.stop_requested();
    };
    while (!stop_token.stop_requested()) {
        const auto before_wait = Now();
        std::unique_lock lock{vault_mutex_};
        if (!vault_.empty()) { // need to wait for the next scheduled callback
            const auto next_wakeup = vault_.cbegin()->first;
            if (next_wakeup <= before_wait) {
                // avoid possible signal + timeout report
                SubmitExpiredBefore(before_wait);
                continue;
            }
            bool is_timeout = vault_waiter_.wait_until(lock, next_wakeup, need_wakeup);
            if (is_timeout) {
                // timeout occured so there is something to submit to executor
                SubmitExpiredBefore(Now());
            }
            // otherwise (vault_add_ == true) and we need to update waiting time 
        }
        else { // no callback scheduled so wait until any callback will be scheduled
            vault_waiter_.wait(lock, need_wakeup);
        }
    }
};

void Scheduler::SubmitExpiredBefore(Timepoint tp) {
    // time to execute callbacks
    auto end = vault_.upper_bound(tp);
    auto beg = vault_.begin();
    while (beg++ != end) {
        if (!SubmitToExecutor(std::move(beg->second))) {
            // TODO: queue is full: maybe throw
            break;
        }
    }
    // remove submitted callbacks
    (void) vault_.erase(vault_.begin(), beg);
}

bool Scheduler::SubmitToExecutor(Callback &&cb) {
    assert(executor_);
    assert(!executor_->IsStopped());
    return executor_->Post(std::move(cb));
}

} // namespace klyaksa
