#include "thread_pool.hpp"

namespace klyaksa {

ThreadPool::ThreadPool(std::size_t threads)
    : worker_count_ { threads }
    , workers_ { threads }
    , pending_tasks_ { kTaskQueueSentinel }
{}

ThreadPool::~ThreadPool() {
    stopped_.store(true, std::memory_order_release);
}

bool ThreadPool::Post(Task task) {
    return pending_tasks_.TryPush(std::move(task));
}

void ThreadPool::Start() {
    for (std::size_t i = 0; i < worker_count_; i++) {
        workers_[i] = std::jthread{ [this](std::stop_token stop) {
            while (!stop.stop_requested()) {
                try {
                    auto top = pending_tasks_.TryPop();
                    if (!top) {
                        // TODO: log as fatal
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

void ThreadPool::Stop() noexcept {
    stopped_.store(true, std::memory_order_release);
    for (auto&& worker: workers_) {
        (void) worker.request_stop();
    }
}


} // namespace klyaksa
