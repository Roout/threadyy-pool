#include "thread_pool.hpp"

namespace klyaksa {

ThreadPool::ThreadPool(std::size_t threads)
    : worker_count_ { threads }
    , workers_ { threads }
{}

ThreadPool::~ThreadPool() {
    stopped_.store(true, std::memory_order_release);
    pending_tasks_.Halt();
}

void ThreadPool::Start() {
    pending_tasks_.Resume();
    for (std::size_t i = 0; i < worker_count_; i++) {
        auto worker = [this](std::stop_token stop) {
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
        };
        workers_[i] = std::jthread{std::move(worker)};
    }
    stopped_.store(false, std::memory_order_release);
}

void ThreadPool::Stop() {
    if (stopped_.load(std::memory_order_acquire)) {
        return;
    }
    stopped_.store(true, std::memory_order_release);

    pending_tasks_.Halt();
    for (auto&& worker: workers_) {
        if (worker.joinable()) {
            worker.request_stop();
            worker.join();
        }
    }
}


} // namespace klyaksa
