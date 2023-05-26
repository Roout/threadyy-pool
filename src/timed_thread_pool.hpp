#pragma once
#include "ccqueue.hpp"
#include "scheduler.hpp"
#include "thread_pool.hpp"

namespace klyaksa {

class TimedThreadPool : public ThreadPool {
 public:
  TimedThreadPool(size_t threads);

  /**
   * Not atomic operation so:
   * If called after stop - must be invoked by the same thread who invoked
   * `Stop()`
   */
  void Start() override;

  /**
   * Not atomic operation so:
   * Must be called by the same thread who invoked `Start()`
   * or be sure that threadpool is not stopped and nobody else is trying to
   * stop
   */
  void Stop() override;

  bool IsStopped() const noexcept override {
    return scheduler_.IsStopped() && ThreadPool::IsStopped();
  }

  void Post(Task&& task, Timeout delay) {
    scheduler_.ScheduleAfter(delay, std::move(task));
  }

  void Post(Task&& task, Timepoint when) {
    scheduler_.ScheduleAt(when, std::move(task));
  }

 private:
  Scheduler scheduler_;
  // Actually it's more usefull for asserts than for smth else
  std::atomic<bool> stopped_;
};

template <traits::Bindable Func>
[[nodiscard]] auto Post(TimedThreadPool& timed_executor, Timeout delay,
                        Func&& f) {
  using R = std::invoke_result_t<Func>;
  Task task{std::forward<Func>(f)};
  auto fut = task.GetFuture<R>();
  timed_executor.Post(std::move(task), delay);
  return fut;
}

template <traits::Bindable Func, traits::Bindable... Args,
          class R = std::invoke_result_t<Func, Args...>>
requires traits::Taskable<Func, Args...>
[[nodiscard]] auto Post(TimedThreadPool& timed_executor, Timeout delay,
                        Func&& f, Args&&... args) {
  auto task = Task{std::forward<Func>(f), std::forward<Args>(args)...};
  auto fut = task.GetFuture<R>();
  timed_executor.Post(std::move(task), delay);
  return fut;
}

template <traits::Bindable Func>
[[nodiscard]] auto Post(TimedThreadPool& timed_executor, Timepoint when,
                        Func&& f) {
  using R = std::invoke_result_t<Func>;
  Task task{std::forward<Func>(f)};
  auto fut = task.GetFuture<R>();
  timed_executor.Post(std::move(task), when);
  return fut;
}

template <traits::Bindable Func, traits::Bindable... Args,
          class R = std::invoke_result_t<Func, Args...>>
requires traits::Taskable<Func, Args...>
[[nodiscard]] auto Post(TimedThreadPool& timed_executor, Timepoint when,
                        Func&& f, Args&&... args) {
  auto task = Task{std::forward<Func>(f), std::forward<Args>(args)...};
  auto fut = task.GetFuture<R>();
  timed_executor.Post(std::move(task), when);
  return fut;
}

}  // namespace klyaksa
