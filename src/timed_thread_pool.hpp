#pragma once
#include "thread_pool.hpp"
#include "scheduler.hpp"
#include "ccqueue.hpp"

namespace klyaksa {

class TimedThreadPool {
public:
    using Task = ThreadPool::Task;
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
