#include "timed_thread_pool.hpp"

namespace klyaksa {

TimedThreadPool::TimedThreadPool(size_t threads)
    : ThreadPool{threads}, scheduler_{this}, stopped_{true} {}

void TimedThreadPool::Start() {
  assert(stopped_.load(std::memory_order_acquire));
  scheduler_.Start();
  ThreadPool::Start();
  stopped_.store(false, std::memory_order_release);
}

void TimedThreadPool::Stop() {
  assert(!stopped_.load(std::memory_order_acquire));
  scheduler_.Stop();
  ThreadPool::Stop();
  stopped_.store(true, std::memory_order_release);
}

}  // namespace klyaksa
