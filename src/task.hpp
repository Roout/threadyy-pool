#pragma once
#include <functional>
#include <future>
#include <memory>
#include <type_traits>

namespace klyaksa {

struct WrongFutureType : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

namespace traits {

// requirements like the std::bind has cuz we need to capture them
template <class T>
concept Bindable = std::is_constructible_v<std::decay_t<T>, T>;

template <class Func, class... Args>
concept Taskable = requires(Func&&, Args&&...) {
  {std::is_invocable_v<Func, std::unwrap_reference_t<Args>...>};
};

}  // namespace traits

class Task {
 public:
  Task() = default;

  template <traits::Bindable Func>
  Task(Func&& f) {
    using R = std::invoke_result_t<Func>;
#ifdef _MSC_VER
    std::promise<R> promise;
    auto fut = promise.get_future();
    auto wrapper = [state = std::move(promise),
                    func = std::forward<Func>(f)]() mutable {
      try {
        if constexpr (std::is_same_v<R, void>) {
          std::invoke(func);
          state.set_value();
        } else {
          state.set_value(std::invoke(func));
        }
      } catch (...) {
        // note, set_exception() may throw too
        state.set_exception(std::current_exception());
      }
    };
    erased_future_.reset(new FutureKeeper<R>(std::move(fut)));
    task_ = std::move(wrapper);
#else
    std::packaged_task<R()> task{[func = std::forward<Func>(f)]() mutable {
      if constexpr (std::is_same_v<R, void>) {
        std::invoke(func);
      } else {
        return std::invoke(func);
      }
    }};
    erased_future_.reset(new FutureKeeper<R>(task.get_future()));
    task_ = std::move(task);
#endif  // _MSC_VER
  }

  template <traits::Bindable Func, traits::Bindable... Args>
    requires traits::Taskable<Func, Args...>
  Task(Func&& f, Args&&... args) {
    using R = std::invoke_result_t<Func, Args...>;
#ifdef _MSC_VER
    std::promise<R> promise;
    auto fut = promise.get_future();
    auto wrapper = [state = std::move(promise), func = std::forward<Func>(f),
                    ... params = std::forward<Args>(args)]() mutable {
      try {
        if constexpr (std::is_same_v<R, void>) {
          std::invoke(func, std::unwrap_reference_t<Args>(params)...);
          state.set_value();
        } else {
          state.set_value(
              std::invoke(func, std::unwrap_reference_t<Args>(params)...));
        }
      } catch (...) {
        // note, set_exception() may throw too
        state.set_exception(std::current_exception());
      }
    };
    task_ = std::move(wrapper);
    erased_future_.reset(new FutureKeeper<R>(std::move(fut)));
#else
    std::packaged_task<R()> task{
        [func = std::forward<Func>(f),
         ... params = std::forward<Args>(args)]() mutable {
          if constexpr (std::is_same_v<R, void>) {
            std::invoke(func, std::unwrap_reference_t<Args>(params)...);
          } else {
            return std::invoke(func, std::unwrap_reference_t<Args>(params)...);
          }
        }};
    erased_future_.reset(new FutureKeeper<R>(task.get_future()));
    task_ = std::move(task);
#endif  // _MSC_VER
  }

  void operator()() { task_(); }

  template <class R>
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

  template <class R>
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

  template <class R>
  class FutureKeeper : public Concept {
   public:
    FutureKeeper(std::future<R>&& fut) : future_{std::move(fut)} {}

    std::future<R> Get() { return std::move(future_); }

   private:
    std::future<R> future_;
  };

  std::move_only_function<void()> task_;
  std::unique_ptr<Concept> erased_future_;
};

}  // namespace klyaksa
