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

TEST(thread_pool, start_after_stop) {
    static constexpr std::size_t kWorkers{5};
    klyaksa::ThreadPool executor{kWorkers};

    ASSERT_TRUE(executor.IsStopped());
    ASSERT_EQ(executor.Size(), kWorkers);
    ASSERT_EQ(executor.GetActiveTasks(), 0);

    for (std::size_t i = 0; i < 1000; i++) {
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
    for (std::size_t i = 0; i < 1000; i++) {
        {
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
        ASSERT_TRUE(true) << "unreachable";
    }
}

TEST(thread_pool, dtor_after_start) {
    using namespace std::chrono_literals;
    static constexpr std::size_t kWorkers{5};
    for (std::size_t i = 0; i < 10000; i++) {
        {
            klyaksa::ThreadPool executor{kWorkers};

            ASSERT_TRUE(executor.IsStopped());
            ASSERT_EQ(executor.Size(), kWorkers);
            ASSERT_EQ(executor.GetActiveTasks(), 0);

            executor.Start();
            
            ASSERT_TRUE(!executor.IsStopped());
            ASSERT_EQ(executor.Size(), kWorkers);
            ASSERT_EQ(executor.GetActiveTasks(), 0);
        }
        ASSERT_TRUE(true) << "unreachable";
    }
}

TEST(thread_pool, access_future_after_executor_destruction) {
    using namespace std::chrono_literals;

    static constexpr std::size_t kWorkers{5};
    static constexpr auto kReturnValue{32};

    for (std::size_t i = 0; i < 10000; i++) {
        std::future<int> fut;
        {
            klyaksa::ThreadPool executor{kWorkers};
            executor.Start();
            fut = *executor.Post([] {
                std::this_thread::sleep_for(200ms);
                return kReturnValue;
            });
        }
        ASSERT_TRUE(fut.valid());
        ASSERT_EQ(fut.get(), kReturnValue);
    }

}
