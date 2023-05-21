#include "timed_thread_pool.hpp"

namespace klyaksa {

TimedThreadPool::TimedThreadPool(size_t threads)
    : ThreadPool { threads }
    , scheduler_ { this }
    , stopped_ { true }
{
}

void TimedThreadPool::Start() {
    assert(stopped_.load());
    scheduler_.Start();
    ThreadPool::Start();
    stopped_.store(false);
}

void TimedThreadPool::Stop() {
    assert(!stopped_.load());
    scheduler_.Stop();
    ThreadPool::Stop();
    stopped_.store(true);
}

} // namespace klyaksa
