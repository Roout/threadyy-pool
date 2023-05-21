#pragma once

#include <vector>
#include <future>
#include <thread>
#include <atomic>
#include <optional>
#include <type_traits>
#include <concepts>
#include <cstdint>
#include <functional>

#include "ccqueue.hpp"
#include "task.hpp"

namespace klyaksa {

/// execution context
class ThreadPool final {
public:
    static constexpr std::size_t kTaskQueueSize { 255 };

    using Queue = CcQueue<Task, kTaskQueueSize>;

    ThreadPool(std::size_t threads);

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ThreadPool& operator=(ThreadPool&&) = delete;
    ThreadPool(ThreadPool&&) = delete;

    ~ThreadPool();
    
    /**
     * Post function for execution
     * @return nullopt of failure to add task to queue
     * otherwise return optional future
    */
    template<traits::Bindable Func, traits::Bindable ...Args, class R = std::invoke_result_t<Func, Args...>>
        requires traits::Taskable<Func, Args...>
    [[nodiscard]] std::optional<std::future<R>> Post(Func &&f, Args&&... args) {
        Task task{std::forward<Func>(f), std::forward<Args>(args)...};
        auto fut = task.GetFuture<R>();
        if (!pending_tasks_.TryPush(std::move(task))) {
            return std::nullopt;
        }
        return std::make_optional(std::move(fut));
    }
    
    template<traits::Bindable Func, class R = std::invoke_result_t<Func>>
    [[nodiscard]] std::optional<std::future<R>> Post(Func &&f) {
        Task task{std::forward<Func>(f)};
        auto fut = task.GetFuture<R>();
        if (!pending_tasks_.TryPush(std::move(task))) {
            return std::nullopt;
        }
        return std::make_optional(std::move(fut));
    }

    /**
     * Post already ready task
     * @return true if task was successfully added 
    */
    [[nodiscard]] bool Post(Task&& task)
        noexcept(noexcept(std::declval<Queue>().TryPush(std::declval<Task>())))
    {
        return pending_tasks_.TryPush(std::move(task));
    }

    void Start();
    void Stop();

    std::size_t Size() const noexcept {
        return worker_count_;
    }
    std::size_t GetActiveTasks() const noexcept {
        return active_tasks_.load(std::memory_order_acquire);
    }
    bool IsStopped() const noexcept {
        return stopped_.load(std::memory_order_acquire);
    }
private:
    const std::size_t worker_count_ { 0 };
    std::atomic<bool> stopped_ { true };
    // number of tasks currently running
    std::atomic<std::size_t> active_tasks_ { 0 };
    Queue pending_tasks_;
    std::vector<std::jthread> workers_;
};

} // namespace klyaksa
