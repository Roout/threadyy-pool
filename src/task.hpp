#pragma once
#include <future>
#include <functional>
#include <memory>

namespace klyaksa {

struct WrongFutureType: public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Task {
public:
    Task() = default;

    template<class Func, class ...Args, class R = std::invoke_result_t<Func, Args...>>
        requires std::is_invocable_v<Func, Args...>
    Task(Func &&f, Args&&... args) {
        std::packaged_task<R()> task {[func = std::forward<Func>(f)
            , ...params = std::forward<Args>(args)]() mutable
        {
            if constexpr (std::is_same_v<R, void>) {
                std::invoke(std::forward<Func>(func), std::forward<Args>(params)...);
            }
            else {
                return std::invoke(std::forward<Func>(func), std::forward<Args>(params)...);
            }
        }};
        erased_future_.reset(new FutureKeeper<R>(task.get_future()));
        task_ = std::move(task);
    }
    
    void operator()() {
        task_();
    }
    
    template<class R>
    std::future<R> GetFutureSafetly() {
        auto base = erased_future_.get();
        if (!base) {
            throw std::runtime_error("empty future");
        }
        auto future = dynamic_cast<FutureKeeper<R>*>(base);
        if (future == nullptr) {
            throw WrongFutureType("wrong type");
        }
        return future->Get();
    }

    template<class R>
    std::future<R> GetFuture() {
        auto base = erased_future_.get();
        if (!base) {
            throw std::runtime_error("empty future");
        }
        return static_cast<FutureKeeper<R>*>(base)->Get();
    }

private:
   // use concept - model type erasure approach
    struct Concept {
        virtual ~Concept() = default;
    };

    template<class R>
    class FutureKeeper: public Concept {
    public:
        FutureKeeper(std::future<R> &&fut)
            : future_ { std::move(fut) } {}

        std::future<R> Get() {
            return std::move(future_);
        }

    private:
        std::future<R> future_;
    };
 
    std::move_only_function<void()> task_;
    std::unique_ptr<Concept> erased_future_;
};

template<typename T>
concept is_not_task = !std::is_same_v<Task, std::remove_reference_t<T>>;

} // namespace klyaksa
