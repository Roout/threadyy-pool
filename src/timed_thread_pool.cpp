#include "timed_thread_pool.hpp"

namespace klyaksa {

TimedThreadPool::TimedThreadPool(size_t threads)
    : ThreadPool { threads }
    , scheduler_ { this }
{}

void TimedThreadPool::Start() {
    ThreadPool::Start();
}

void TimedThreadPool::Stop() {
    scheduler_.Stop();
    ThreadPool::Stop();
}

} // namespace klyaksa
