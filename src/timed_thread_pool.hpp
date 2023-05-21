#pragma once
#include "thread_pool.hpp"
#include "scheduler.hpp"
#include "ccqueue.hpp"

namespace klyaksa {

class TimedThreadPool {
public:
    using Queue = ThreadPool::Queue;

    TimedThreadPool(size_t threads);

    TimedThreadPool(const TimedThreadPool&) = delete;
    TimedThreadPool& operator=(const TimedThreadPool&) = delete;

    TimedThreadPool& operator=(TimedThreadPool&&) = delete;
    TimedThreadPool(TimedThreadPool&&) = delete;

    ~TimedThreadPool() = default;

    void Start();

    void Stop();

    /**
     * Post already ready task
     * @return true if task was successfully added 
    */
    [[nodiscard]] bool Post(Task&& task)
        noexcept(noexcept(std::declval<Queue>().TryPush(std::declval<Task>())))
    {
        return pool_.Post(std::move(task));
    }
    
    void Post(Task&& task, Timeout delay) {
        scheduler_.ScheduleAfter(delay, std::move(task));
    }
    
    void Post(Task&& task, Timepoint when) {
        scheduler_.ScheduleAt(when, std::move(task));
    }

    template<traits::Bindable Func>
    [[nodiscard]] auto Post(Timeout delay, Func &&f) {
        using R = std::invoke_result_t<Func>;
        Task task{ std::forward<Func>(f) };
        auto fut = task.GetFuture<R>();
        Post(std::move(task), delay);
        return fut;
    }

    template<traits::Bindable Func, traits::Bindable ...Args, class R = std::invoke_result_t<Func, Args...>>
        requires traits::Taskable<Func, Args...>
    [[nodiscard]] auto Post(Timeout delay, Func &&f, Args&&... args) {
        auto task = Task{std::forward<Func>(f), std::forward<Args>(args)...};
        auto fut = task.GetFuture<R>();
        Post(std::move(task), delay);
        return fut;
    }

    template<traits::Bindable Func>
    [[nodiscard]] auto Post(Timepoint when, Func &&f) {
        using R = std::invoke_result_t<Func>;
        Task task{ std::forward<Func>(f) };
        auto fut = task.GetFuture<R>();
        Post(std::move(task), when);
        return fut;
    }

    template<traits::Bindable Func, traits::Bindable ...Args, class R = std::invoke_result_t<Func, Args...>>
        requires traits::Taskable<Func, Args...>
    [[nodiscard]] auto Post(Timepoint when, Func &&f, Args&&... args) {
        auto task = Task{std::forward<Func>(f), std::forward<Args>(args)...};
        auto fut = task.GetFuture<R>();
        Post(std::move(task), when);
        return fut;
    }

    size_t Size() const noexcept {
        return pool_.Size();
    }

    bool IsStopped() const noexcept {
        return pool_.IsStopped();
    }

private:
    ThreadPool pool_;
    Scheduler scheduler_;
};

} // namespace klyaksa
