#pragma once

#include "gtest/gtest.h"
#include "thread_pool.hpp"

TEST(thread_pool, executor_lifetime_cycle) {
    using namespace std::chrono_literals;

    static constexpr std::size_t kWorkers{5};
    klyaksa::ThreadPool executor{kWorkers};

    ASSERT_TRUE(executor.IsStopped());
    ASSERT_EQ(executor.Size(), kWorkers);
    ASSERT_EQ(executor.GetActiveTasks(), 0);

    executor.Start();
    
    ASSERT_TRUE(!executor.IsStopped());
    ASSERT_EQ(executor.Size(), kWorkers);
    ASSERT_EQ(executor.GetActiveTasks(), 0);

    auto result = Post(executor, [] {
        // small delay for asserts
        std::this_thread::sleep_for(200ms);
    });
    // delay to check number of active tasks
    std::this_thread::sleep_for(100ms);

    ASSERT_TRUE(result) << "failed to post task";
    ASSERT_TRUE(!executor.IsStopped());
    ASSERT_EQ(executor.Size(), kWorkers);
    ASSERT_EQ(executor.GetActiveTasks(), 1);

    executor.Stop();

    ASSERT_TRUE(executor.IsStopped());
    ASSERT_EQ(executor.Size(), kWorkers);
    ASSERT_EQ(executor.GetActiveTasks(), 0);
}

TEST(thread_pool, post_lambdas) {
    using namespace std::chrono_literals;

    static constexpr std::size_t kWorkers{5};
    klyaksa::ThreadPool executor{kWorkers};
    executor.Start();
    std::atomic<int> count{0};
    auto results = { 
          Post(executor, [] { std::this_thread::sleep_for(5ms);})
        , Post(executor, [&count] { count++; std::this_thread::sleep_for(5ms);})
        , Post(executor, [&count](int x) { count += x; std::this_thread::sleep_for(5ms);}, 1)
        , Post(executor, [&count](int x) noexcept { count += x; std::this_thread::sleep_for(5ms);}, 1)
        , Post(executor, [&count](int x, int y) { count += x + y; std::this_thread::sleep_for(5ms);}, 1, 2)
        , Post(executor, [&count](int x, int y) mutable { count += x + y; std::this_thread::sleep_for(5ms);}, 1, 2)
    };
    auto res = Post(executor, [&count] { count++; std::this_thread::sleep_for(5ms); return count.load(); });
    std::this_thread::sleep_for(50ms);
    executor.Stop();
}

namespace {
    void EmptyTest() {}
    void WithParam(std::atomic<int> &x) { x++; }
    int WithReturn(int x) { return x + 1; }
    int WithConstParam(const int& x) { return x * x; }
    int WithRvalueParam(int&&x) { x++; auto y = std::move(x); return y + 1; }
} // namespace {

TEST(thread_pool, post_raw_functions) {
    using namespace std::chrono_literals;

    static constexpr std::size_t kWorkers{5};
    klyaksa::ThreadPool executor{kWorkers};
    executor.Start();
    std::atomic<int> inc { 0 };
    int noChange { 0 };
    (void) Post(executor, EmptyTest);
    (void) Post(executor, &EmptyTest);
    (void) Post(executor, WithParam, std::ref(inc));
    (void) Post(executor, &WithParam, std::ref(inc));
    (void) Post(executor, WithReturn, noChange);
    (void) Post(executor, &WithConstParam, std::cref(noChange));
    (void) Post(executor, WithConstParam, std::cref(noChange));
    (void) Post(executor, &WithRvalueParam, 1);
    (void) Post(executor, WithRvalueParam, 2);
    
    auto optFuture = Post(executor, &WithReturn, noChange);
    std::this_thread::sleep_for(50ms);
    EXPECT_TRUE(optFuture);
    EXPECT_EQ(optFuture->get(), 1);
    EXPECT_EQ(inc, 2);
    executor.Stop();
}

TEST(thread_pool, task_throw_exception) {
    using namespace std::chrono_literals;
    static constexpr std::size_t kWorkers{5};

    klyaksa::ThreadPool executor{kWorkers};
    executor.Start();
    auto fut = Post(executor, []() {
        throw std::runtime_error("test error");
    });
    fut->wait();
    EXPECT_THROW(fut->get(), std::runtime_error);
    executor.Stop();
}

TEST(thread_pool, post_functor) {
    using namespace std::chrono_literals;
    struct Empty {
        void operator()() {}
    };
    struct EmptyConst {
        void operator()() const noexcept {}
    };
    struct WithParam {
        void operator()(int &x) const noexcept { ++x; }
    };
    struct WithCapture {
        WithCapture(int x) : x_ { x } {}
        void operator()() noexcept { x_++; }
        int x_;
    };
    struct WithReturn {
        WithReturn(int x) : x_ { x } {}
        int operator()() noexcept { x_++; return x_; }
        int x_;
    };
    static constexpr std::size_t kWorkers{5};

    klyaksa::ThreadPool executor{kWorkers};
    executor.Start();
    int x = 0; 
    // as rvalue
    (void) Post(executor, Empty{});
    (void) Post(executor, EmptyConst{});
    (void) Post(executor, WithParam{}, x);
    (void) Post(executor, WithCapture{1});
    (void) Post(executor, WithReturn{1});
    (void) Post(executor, std::function<void()>{[x]{}});
    (void) Post(executor, std::move_only_function<void()>{[x]{}});
    
    // as lvalue
    Empty empty{};
    EmptyConst emptyConst{};
    WithParam withParams{};
    WithCapture withCapture{1};
    WithReturn withReturn{1};
    std::function<void()> func{[x]{}};
    std::move_only_function<void()> moveFunc{[x]{}};
    (void) Post(executor, empty);
    (void) Post(executor, emptyConst);
    (void) Post(executor, withParams, x);
    (void) Post(executor, withCapture);
    (void) Post(executor, withReturn);
    (void) Post(executor, func);
    (void) Post(executor, std::move(moveFunc));
    std::this_thread::sleep_for(50ms);
    executor.Stop();
}


TEST(thread_pool, access_future_from_post_result) {
    using namespace std::chrono_literals;

    static constexpr std::size_t kWorkers{5};
    klyaksa::ThreadPool executor{kWorkers};

    static constexpr auto kReturnValue{32};

    executor.Start();
    auto fut = Post(executor, [] {
        std::this_thread::sleep_for(200ms);
        return kReturnValue;
    });
    
    // delay to check number of active tasks
    std::this_thread::sleep_for(100ms);

    ASSERT_TRUE(!executor.IsStopped());
    ASSERT_EQ(executor.Size(), kWorkers);
    ASSERT_EQ(executor.GetActiveTasks(), 1);

    ASSERT_TRUE(fut.has_value());
    ASSERT_EQ(fut->get(), kReturnValue);

    executor.Stop();

    ASSERT_TRUE(executor.IsStopped());
    ASSERT_EQ(executor.Size(), kWorkers);
    ASSERT_EQ(executor.GetActiveTasks(), 0);
}

TEST(thread_pool, start_after_stop) {
    static constexpr std::size_t kWorkers{5};
    klyaksa::ThreadPool executor{kWorkers};

    ASSERT_TRUE(executor.IsStopped());
    ASSERT_EQ(executor.Size(), kWorkers);
    ASSERT_EQ(executor.GetActiveTasks(), 0);

    for (std::size_t i = 0; i < 100; i++) {
        executor.Start();
        
        ASSERT_TRUE(!executor.IsStopped());
        ASSERT_EQ(executor.Size(), kWorkers);
        ASSERT_EQ(executor.GetActiveTasks(), 0);

        executor.Stop();

        ASSERT_TRUE(executor.IsStopped());
        ASSERT_EQ(executor.Size(), kWorkers);
        ASSERT_EQ(executor.GetActiveTasks(), 0);
    }
}

TEST(thread_pool, dtor_after_stop) {
    static constexpr std::size_t kWorkers{5};
    for (std::size_t i = 0; i < 100; i++) {
        klyaksa::ThreadPool executor{kWorkers};

        ASSERT_TRUE(executor.IsStopped());
        ASSERT_EQ(executor.Size(), kWorkers);
        ASSERT_EQ(executor.GetActiveTasks(), 0);

        executor.Start();
        
        ASSERT_TRUE(!executor.IsStopped());
        ASSERT_EQ(executor.Size(), kWorkers);
        ASSERT_EQ(executor.GetActiveTasks(), 0);

        executor.Stop();

        ASSERT_TRUE(executor.IsStopped());
        ASSERT_EQ(executor.Size(), kWorkers);
        ASSERT_EQ(executor.GetActiveTasks(), 0);
    }
}

TEST(thread_pool, dtor_after_start) {
    using namespace std::chrono_literals;
    static constexpr std::size_t kWorkers{5};
    for (std::size_t i = 0; i < 100; i++) {
        klyaksa::ThreadPool executor{kWorkers};

        ASSERT_TRUE(executor.IsStopped());
        ASSERT_EQ(executor.Size(), kWorkers);
        ASSERT_EQ(executor.GetActiveTasks(), 0);

        executor.Start();
        
        ASSERT_TRUE(!executor.IsStopped());
        ASSERT_EQ(executor.Size(), kWorkers);
        ASSERT_EQ(executor.GetActiveTasks(), 0);
    }
}

TEST(thread_pool, access_future_after_executor_destruction) {
    using namespace std::chrono_literals;

    static constexpr std::size_t kWorkers{5};
    static constexpr auto kReturnValue{32};

    for (std::size_t i = 0; i < 100; i++) {
        std::future<int> fut;
        {
            klyaksa::ThreadPool executor{kWorkers};
            executor.Start();
            fut = *Post(executor, [] {
                std::this_thread::sleep_for(20ms);
                return kReturnValue;
            });
            // sleep required to have time to at least start execution of job
            // otherwise it's possible that packaged_task is not being called
            // which leads to `broken promise` exception
            std::this_thread::sleep_for(5ms);
        }
        ASSERT_TRUE(fut.valid());
        ASSERT_EQ(fut.get(), kReturnValue);
    }

}
