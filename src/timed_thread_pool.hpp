#pragma once
#include "thread_pool.hpp"
#include "scheduler.hpp"
#include "ccqueue.hpp"

namespace klyaksa {

class TimedThreadPool : public ThreadPool {
public:
    TimedThreadPool(size_t threads);

    void Start() override;

    void Stop() override;

    void Post(Task&& task, Timeout delay) {
        scheduler_.ScheduleAfter(delay, std::move(task));
    }
    
    void Post(Task&& task, Timepoint when) {
        scheduler_.ScheduleAt(when, std::move(task));
    }

private:
    Scheduler scheduler_;
    std::atomic<bool> stopped_;
};

template<traits::Bindable Func>
[[nodiscard]] auto Post(TimedThreadPool& timed_executor, Timeout delay, Func &&f) {
    using R = std::invoke_result_t<Func>;
    Task task{ std::forward<Func>(f) };
    auto fut = task.GetFuture<R>();
    timed_executor.Post(std::move(task), delay);
    return fut;
}

template<traits::Bindable Func, traits::Bindable ...Args, class R = std::invoke_result_t<Func, Args...>>
    requires traits::Taskable<Func, Args...>
[[nodiscard]] auto Post(TimedThreadPool& timed_executor, Timeout delay, Func &&f, Args&&... args) {
    auto task = Task{std::forward<Func>(f), std::forward<Args>(args)...};
    auto fut = task.GetFuture<R>();
    timed_executor.Post(std::move(task), delay);
    return fut;
}

template<traits::Bindable Func>
[[nodiscard]] auto Post(TimedThreadPool& timed_executor, Timepoint when, Func &&f) {
    using R = std::invoke_result_t<Func>;
    Task task{ std::forward<Func>(f) };
    auto fut = task.GetFuture<R>();
    timed_executor.Post(std::move(task), when);
    return fut;
}

template<traits::Bindable Func, traits::Bindable ...Args, class R = std::invoke_result_t<Func, Args...>>
    requires traits::Taskable<Func, Args...>
[[nodiscard]] auto Post(TimedThreadPool& timed_executor, Timepoint when, Func &&f, Args&&... args) {
    auto task = Task{std::forward<Func>(f), std::forward<Args>(args)...};
    auto fut = task.GetFuture<R>();
    timed_executor.Post(std::move(task), when);
    return fut;
}

} // namespace klyaksa
