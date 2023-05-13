#include "timed_thread_pool.hpp"

namespace klyaksa {

TimedThreadPool::TimedThreadPool(size_t threads)
    : pool_ { threads }
    , scheduler_ { &pool_ }
{}

void TimedThreadPool::Start() {
    pool_.Start();
}

void TimedThreadPool::Stop() {
    pool_.Stop();
}

} // namespace klyaksa
