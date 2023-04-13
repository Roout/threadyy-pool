#include "thread_pool.hpp"

namespace klyaksa {

ThreadPool::ThreadPool(std::size_t threads)
    : worker_count_ { threads }
    , workers_ { threads }
    , pending_tasks_ { kTaskQueueSentinel }
{}

ThreadPool::~ThreadPool() {
    if (kTaskQueueSentinel) pending_tasks_.DisableSentinel();
    stopped_.store(true, std::memory_order_release);
}

void ThreadPool::Start() {
    for (std::size_t i = 0; i < worker_count_; i++) {
        workers_[i] = std::jthread{ [this](std::stop_token stop) {
            while (!stop.stop_requested()) {
                try {
                    auto top = pending_tasks_.TryPop();
                    if (!top) {
                        // TODO: maybe queue sentinel is gone
                        continue;
                    }
                    active_tasks_.fetch_add(1, std::memory_order_relaxed);
                    std::invoke(*top);
                    active_tasks_.fetch_sub(1, std::memory_order_release);
                }
                catch (...) {
                    // TODO: log error
                }
            }
        }};
    }
    stopped_.store(false, std::memory_order_release);
}

void ThreadPool::Stop() {
    if (kTaskQueueSentinel) pending_tasks_.DisableSentinel();
    stopped_.store(true, std::memory_order_release);
    for (auto&& worker: workers_) {
        (void) worker.request_stop();
    }
    for (auto&& worker: workers_) {
        worker.join();
    }
    workers_.clear();
}


} // namespace klyaksa
