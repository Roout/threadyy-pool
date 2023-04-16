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
