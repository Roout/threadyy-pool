#pragma once

#include <atomic>
#include <concepts>
#include <cstdint>
#include <functional>
#include <future>
#include <optional>
#include <thread>
#include <type_traits>
#include <vector>

#include "ccqueue.hpp"
#include "task.hpp"

namespace klyaksa {

/// execution context
class ThreadPool {
 public:
  static constexpr std::size_t kTaskQueueSize{255};

  using Queue = CcQueue<Task, kTaskQueueSize>;

  ThreadPool(std::size_t threads);

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  ThreadPool& operator=(ThreadPool&&) = delete;
  ThreadPool(ThreadPool&&) = delete;

  virtual ~ThreadPool();

  /**
   * Post already created task
   * @return true if task was successfully added
   */
  [[nodiscard]] bool Post(Task&& task) noexcept(
      noexcept(std::declval<Queue>().TryPush(std::declval<Task>()))) {
    return pending_tasks_.TryPush(std::move(task));
  }

  /**
   * Not atomic operations so:
   * If called after stop - must be invoked by the same thread who invoked
   * `Stop()`
   */
  virtual void Start();

  /**
   * Not atomic operations so:
   * Must be called by the same thread who invoked `Start()`
   * or be sure that threadpool is not stopped and nobody else is trying to
   * stop
   */
  virtual void Stop();

  virtual bool IsStopped() const noexcept {
    return stopped_.load(std::memory_order_acquire);
  }

  std::size_t WorkerCount() const noexcept { return worker_count_; }

  std::size_t GetActiveTasks() const noexcept {
    return active_tasks_.load(std::memory_order_acquire);
  }

 private:
  const std::size_t worker_count_{0};
  std::atomic<bool> stopped_{true};
  // number of tasks currently running
  std::atomic<std::size_t> active_tasks_{0};
  Queue pending_tasks_;
  std::vector<std::jthread> workers_;
};

/**
 * Post function for execution
 * @return nullopt of failure to add task to queue
 * otherwise return optional future
 */
template <traits::Bindable Func, traits::Bindable... Args,
          class R = std::invoke_result_t<Func, Args...>>
requires traits::Taskable<Func, Args...>
[[nodiscard]] std::optional<std::future<R>> Post(ThreadPool& executor, Func&& f,
                                                 Args&&... args) {
  Task task{std::forward<Func>(f), std::forward<Args>(args)...};
  auto fut = task.GetFuture<R>();
  if (!executor.Post(std::move(task))) {
    return std::nullopt;
  }
  return std::make_optional(std::move(fut));
}

template <traits::Bindable Func, class R = std::invoke_result_t<Func>>
[[nodiscard]] std::optional<std::future<R>> Post(ThreadPool& executor,
                                                 Func&& f) {
  Task task{std::forward<Func>(f)};
  auto fut = task.GetFuture<R>();
  if (!executor.Post(std::move(task))) {
    return std::nullopt;
  }
  return std::make_optional(std::move(fut));
}

}  // namespace klyaksa
