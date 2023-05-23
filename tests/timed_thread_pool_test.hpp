#pragma once

#include "gtest/gtest.h"
#include "timed_thread_pool.hpp"

TEST(timed_thread_pool, executor_lifetime_cycle) {
    using namespace std::chrono_literals;

    static constexpr std::size_t kWorkers{5};
    klyaksa::TimedThreadPool executor{kWorkers};

    ASSERT_TRUE(executor.IsStopped());
    ASSERT_EQ(executor.WorkerCount(), kWorkers);
    ASSERT_EQ(executor.GetActiveTasks(), 0);

    executor.Start();
    
    ASSERT_TRUE(!executor.IsStopped());
    ASSERT_EQ(executor.WorkerCount(), kWorkers);
    ASSERT_EQ(executor.GetActiveTasks(), 0);

    auto result = Post(executor, [] {
        // small delay for asserts
        std::this_thread::sleep_for(100ms);
    });
    // delay to check number of active tasks
    std::this_thread::sleep_for(10ms);

    ASSERT_TRUE(result) << "failed to post task";
    ASSERT_TRUE(!executor.IsStopped());
    ASSERT_EQ(executor.WorkerCount(), kWorkers);
    ASSERT_EQ(executor.GetActiveTasks(), 1);

    executor.Stop();

    ASSERT_TRUE(executor.IsStopped());
    ASSERT_EQ(executor.WorkerCount(), kWorkers);
    ASSERT_EQ(executor.GetActiveTasks(), 0);
}

TEST(timed_thread_pool, start_after_stop) {
    static constexpr std::size_t kWorkers{5};
    klyaksa::TimedThreadPool executor{kWorkers};

    ASSERT_TRUE(executor.IsStopped());
    ASSERT_EQ(executor.WorkerCount(), kWorkers);
    ASSERT_EQ(executor.GetActiveTasks(), 0);

    for (std::size_t i = 0; i < 100; i++) {
        executor.Start();
        
        ASSERT_TRUE(!executor.IsStopped());
        ASSERT_EQ(executor.WorkerCount(), kWorkers);
        ASSERT_EQ(executor.GetActiveTasks(), 0);

        executor.Stop();

        ASSERT_TRUE(executor.IsStopped());
        ASSERT_EQ(executor.WorkerCount(), kWorkers);
        ASSERT_EQ(executor.GetActiveTasks(), 0);
    }
}

TEST(timed_thread_pool, dtor_after_stop) {
    static constexpr std::size_t kWorkers{5};
    for (std::size_t i = 0; i < 100; i++) {
        klyaksa::TimedThreadPool executor{kWorkers};

        ASSERT_TRUE(executor.IsStopped());
        ASSERT_EQ(executor.WorkerCount(), kWorkers);
        ASSERT_EQ(executor.GetActiveTasks(), 0);

        executor.Start();
        
        ASSERT_TRUE(!executor.IsStopped());
        ASSERT_EQ(executor.WorkerCount(), kWorkers);
        ASSERT_EQ(executor.GetActiveTasks(), 0);

        executor.Stop();

        ASSERT_TRUE(executor.IsStopped());
        ASSERT_EQ(executor.WorkerCount(), kWorkers);
        ASSERT_EQ(executor.GetActiveTasks(), 0);
    }
}

TEST(timed_thread_pool, dtor_after_start) {
    using namespace std::chrono_literals;
    static constexpr std::size_t kWorkers{5};
    for (std::size_t i = 0; i < 100; i++) {
        klyaksa::TimedThreadPool executor{kWorkers};

        ASSERT_TRUE(executor.IsStopped());
        ASSERT_EQ(executor.WorkerCount(), kWorkers);
        ASSERT_EQ(executor.GetActiveTasks(), 0);

        executor.Start();
        
        ASSERT_TRUE(!executor.IsStopped());
        ASSERT_EQ(executor.WorkerCount(), kWorkers);
        ASSERT_EQ(executor.GetActiveTasks(), 0);
    }
}

