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

    auto result = executor.Post([] {
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
    auto results = { executor.Post([] { std::this_thread::sleep_for(5ms);})
        , executor.Post([&count] { count++; std::this_thread::sleep_for(5ms);})
        , executor.Post([&count](int x) { count += x; std::this_thread::sleep_for(5ms);}, 1)
        , executor.Post([&count](int x) noexcept { count += x; std::this_thread::sleep_for(5ms);}, 1)
        , executor.Post([&count](int x, int y) { count += x + y; std::this_thread::sleep_for(5ms);}, 1, 2)
        , executor.Post([&count](int x, int y) mutable { count += x + y; std::this_thread::sleep_for(5ms);}, 1, 2)
    };
    auto res = executor.Post([&count] { count++; std::this_thread::sleep_for(5ms); return count.load(); });
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
    (void) executor.Post(EmptyTest);
    (void) executor.Post(&EmptyTest);
    (void) executor.Post(WithParam, std::ref(inc));
    (void) executor.Post(&WithParam, std::ref(inc));
    (void) executor.Post(WithReturn, noChange);
    (void) executor.Post(&WithConstParam, std::cref(noChange));
    (void) executor.Post(WithConstParam, std::cref(noChange));
    (void) executor.Post(&WithRvalueParam, 1);
    (void) executor.Post(WithRvalueParam, 2);
    
    auto optFuture = executor.Post(&WithReturn, noChange);
    std::this_thread::sleep_for(50ms);
    EXPECT_TRUE(optFuture);
    EXPECT_EQ(optFuture->get(), 1);
    EXPECT_EQ(inc, 2);
    executor.Stop();
}

TEST(thread_pool, task_throw_exception) {

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
    (void) executor.Post(Empty{});
    (void) executor.Post(EmptyConst{});
    (void) executor.Post(WithParam{}, x);
    (void) executor.Post(WithCapture{1});
    (void) executor.Post(WithReturn{1});
    (void) executor.Post(std::function<void()>{[x]{}});
    (void) executor.Post(std::move_only_function<void()>{[x]{}});
    
    // as lvalue
    Empty empty{};
    EmptyConst emptyConst{};
    WithParam withParams{};
    WithCapture withCapture{1};
    WithReturn withReturn{1};
    std::function<void()> func{[x]{}};
    std::move_only_function<void()> moveFunc{[x]{}};
    (void) executor.Post(empty);
    (void) executor.Post(emptyConst);
    (void) executor.Post(withParams, x);
    (void) executor.Post(withCapture);
    (void) executor.Post(withReturn);
    (void) executor.Post(func);
    (void) executor.Post(std::move(moveFunc));
    std::this_thread::sleep_for(50ms);
    executor.Stop();
}


TEST(thread_pool, access_future_from_post_result) {
    using namespace std::chrono_literals;

    static constexpr std::size_t kWorkers{5};
    klyaksa::ThreadPool executor{kWorkers};

    static constexpr auto kReturnValue{32};

    executor.Start();
    auto fut = executor.Post([] {
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
            fut = *executor.Post([] {
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
