#pragma once

#include <vector>
#include <future>
#include <thread>
#include <atomic>
#include <optional>
#include <type_traits>
#include <cstdint>

#include "ccqueue.hpp"

namespace klyaksa {

// TODO: add timer: execute after some time

/// execution context
class ThreadPool final {
public:
    static constexpr std::size_t kTaskQueueSize { 255 };
    static constexpr bool kTaskQueueSentinel { true };

    using Task = std::move_only_function<void()>;

    ThreadPool(std::size_t threads);

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;

    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    ~ThreadPool();
    
    template<class Func, class ...Args>
    [[nodiscard]] auto Post(Func &&f, Args&&... args)
        -> std::optional<std::future<std::invoke_result_t<Func&&, Args&&...>>>
    {
        using R = std::invoke_result_t<Func&&, Args&&...>;

        std::packaged_task<R()> task { [func = std::forward<Func>(f)
            , ...params = std::forward<Args>(args)]() mutable
        {
            if constexpr (std::is_same_v<R, void>) {
                std::invoke(std::forward<Func>(func), std::forward<Args>(params)...);
            }
            else {
                return std::invoke(std::forward<Func>(func), std::forward<Args>(params)...);
            }    
        }};
        auto fut = task.get_future();
        if (!pending_tasks_.TryPush(std::move(task))) {
            return std::nullopt;
        }
        return std::make_optional(std::move(fut));
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
    using Queue = CcQueue<Task, kTaskQueueSize>;

    const std::size_t worker_count_ { 0 };
    std::atomic<bool> stopped_ { true };
    // number of tasks currently running
    std::atomic<std::size_t> active_tasks_ { 0 };
    std::vector<std::jthread> workers_;
    Queue pending_tasks_;
};

} // namespace klyaksa
