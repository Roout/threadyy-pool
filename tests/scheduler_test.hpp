#pragma once

#include "gtest/gtest.h"
#include "thread_pool.hpp"
#include "scheduler.hpp"

#include <vector>
#include <algorithm>
#include <ranges>

TEST(scheduler, create_scheduler) {
    using namespace std::chrono_literals;

    klyaksa::ThreadPool pool{5};
    klyaksa::Scheduler scheduler{&pool};

    scheduler.Start();
    pool.Start();
    ASSERT_EQ(scheduler.CallbackCount(), 0);
    
    std::atomic<int> counter{0};
    scheduler.ScheduleAfter(20ms, [&counter]() { counter++; });
    scheduler.ScheduleAt(std::chrono::steady_clock::now() + 25ms, [&counter]() { counter++; });

    ASSERT_EQ(counter, 0);
    ASSERT_EQ(scheduler.CallbackCount(), 2);
    std::this_thread::sleep_for(55ms);
    ASSERT_EQ(counter, 2);
    ASSERT_EQ(scheduler.CallbackCount(), 0);
}

namespace {
    bool TimeIsNear(klyaksa::Timepoint lhs, klyaksa::Timepoint rhs, klyaksa::Timeout error) noexcept {
        auto diff = std::chrono::duration_cast<klyaksa::Timeout>(lhs - rhs);
        return std::abs(diff.count()) <= error.count(); 
    }
}

TEST(scheduler, schedule_at_expired_time) {
    using namespace std::chrono_literals;

    klyaksa::ThreadPool pool{5};
    klyaksa::Scheduler scheduler{&pool};

    scheduler.Start();
    pool.Start();
    ASSERT_EQ(scheduler.CallbackCount(), 0);

    klyaksa::Timepoint exec_time;
    scheduler.ScheduleAt(std::chrono::steady_clock::now() - 100ms, [&exec_time]() {
        exec_time = std::chrono::steady_clock::now();
    });

    auto now = std::chrono::steady_clock::now();
    auto error = 5ms;
    std::this_thread::sleep_for(10ms);
    ASSERT_TRUE(TimeIsNear(exec_time, now, error))
        << "Executed at " << exec_time.time_since_epoch().count()
        << " expected exceution around " << now.time_since_epoch().count()
        << " so |exec_time -  now| <= error: |"
        << std::chrono::duration_cast<klyaksa::Timeout>(exec_time - now).count() << "| <= " << error.count();
    ASSERT_EQ(scheduler.CallbackCount(), 0);
}

TEST(scheduler, schedule_at_current_time) {
    using namespace std::chrono_literals;

    klyaksa::ThreadPool pool{5};
    klyaksa::Scheduler scheduler{&pool};

    scheduler.Start();
    pool.Start();
    ASSERT_EQ(scheduler.CallbackCount(), 0);

    klyaksa::Timepoint exec_time;
    scheduler.ScheduleAt(std::chrono::steady_clock::now(), [&exec_time]() {
        exec_time = std::chrono::steady_clock::now();
    });

    auto now = std::chrono::steady_clock::now();
    auto error = 5ms;
    std::this_thread::sleep_for(10ms);
    ASSERT_TRUE(TimeIsNear(exec_time, now, error))
        << "Executed at " << exec_time.time_since_epoch().count()
        << " expected exceution around " << now.time_since_epoch().count()
        << " so |exec_time -  now| <= error: |"
        << exec_time.time_since_epoch().count() << " - " 
        << now.time_since_epoch().count() << "| <= " << error.count();
    ASSERT_EQ(scheduler.CallbackCount(), 0);

}

TEST(scheduler, schedule_at_future_time) {
    using namespace std::chrono_literals;

    klyaksa::ThreadPool pool{5};
    klyaksa::Scheduler scheduler{&pool};

    scheduler.Start();
    pool.Start();
    ASSERT_EQ(scheduler.CallbackCount(), 0);

    std::optional<klyaksa::Timepoint> exec_time;
    std::mutex mutex;
    std::condition_variable finished;

    const klyaksa::Timeout maxWaitTime{ 100ms };
    const klyaksa::Timeout error{ 20ms };
    const auto expectedExecTime = std::chrono::steady_clock::now() + maxWaitTime;

    scheduler.ScheduleAt(expectedExecTime, [&]() {
        std::lock_guard lock{ mutex };
        exec_time.emplace(std::chrono::steady_clock::now());
        finished.notify_one();
    });
    
    std::unique_lock lock{ mutex };
    (void) finished.wait_for(lock, maxWaitTime * 2, [&]() {
        return exec_time.has_value();
    });
    lock.unlock();

    ASSERT_EQ(scheduler.CallbackCount(), 0) << "Haven't been executed yet";

    const auto diff = std::chrono::duration_cast<klyaksa::Timeout>(*exec_time - expectedExecTime);
    ASSERT_TRUE(TimeIsNear(*exec_time, expectedExecTime, error))
        << "Executed at " << exec_time->time_since_epoch().count()
        << " expected execution around " << expectedExecTime.time_since_epoch().count()
        << " so |exec_time -  now| <= error: |" << diff.count() << "| <= " << error.count();
}

TEST(scheduler, correct_schedule_after_sequence) {
    using namespace std::chrono_literals;
    klyaksa::ThreadPool pool{5};
    klyaksa::Scheduler scheduler{&pool};
    scheduler.Start();
    pool.Start();
    ASSERT_EQ(scheduler.CallbackCount(), 0);
    
    static constexpr size_t kInsertElements { 10 };
    std::vector<klyaksa::Timeout> sequence;
    std::mutex sequece_quard;

    klyaksa::Timeout maxWaitTime = 0ms;
    std::condition_variable finished;
    for (size_t i = 0; i < kInsertElements; i++) {
        const auto timeout = i * 10ms;
        scheduler.ScheduleAfter(timeout, [&finished, &sequence, &sequece_quard, timeout]() {
            std::lock_guard lock{sequece_quard};
            sequence.push_back(timeout);
            finished.notify_one();
        });
        maxWaitTime = timeout;
    }

    std::unique_lock lock{sequece_quard};
    (void) finished.wait_for(lock, maxWaitTime * 2, [&]() {
        return sequence.size() == kInsertElements;
    });
    lock.unlock();

    EXPECT_EQ(sequence.size(), kInsertElements);
    EXPECT_TRUE(std::ranges::is_sorted(sequence));
}

TEST(scheduler, correct_schedule_at_sequence) {
    using namespace std::chrono_literals;
    klyaksa::ThreadPool pool{5};
    klyaksa::Scheduler scheduler{&pool};
    pool.Start();
    scheduler.Start();
    ASSERT_EQ(scheduler.CallbackCount(), 0);
    
    static constexpr size_t kInsertElements { 10 };
    std::vector<klyaksa::Timeout> sequence;
    std::mutex sequece_quard;

    klyaksa::Timeout maxWaitTime = 0ms;
    std::condition_variable finished;
    for (size_t i = 0; i < kInsertElements; i++) {
        const auto timeout = i * 10ms;
        scheduler.ScheduleAt(std::chrono::steady_clock::now() + timeout
            , [&finished, &sequence, &sequece_quard, timeout]() {
                std::lock_guard lock{sequece_quard};
                sequence.push_back(timeout);
                finished.notify_one();
            });
        maxWaitTime = timeout;
    }

    std::unique_lock lock{sequece_quard};
    (void) finished.wait_for(lock, maxWaitTime * 2, [&]() {
        return sequence.size() == kInsertElements;
    });
    lock.unlock();

    EXPECT_EQ(sequence.size(), kInsertElements);
    EXPECT_TRUE(std::ranges::is_sorted(sequence));    
}

TEST(scheduler, correct_schedule_mix_sequence) {
    using namespace std::chrono_literals;
    klyaksa::ThreadPool pool{5};
    klyaksa::Scheduler scheduler{&pool};
    pool.Start();
    scheduler.Start();
    ASSERT_EQ(scheduler.CallbackCount(), 0);
    
    static constexpr size_t kInsertElements { 10 };
    std::vector<klyaksa::Timeout> sequence;
    std::mutex sequece_quard;

    klyaksa::Timeout maxWaitTime = 0ms;
    std::condition_variable finished;
    for (size_t i = 0; i < kInsertElements; i++) {
        const auto timeout = i * 5ms;
        auto cb = [&finished, &sequence, &sequece_quard, timeout]() {
            std::lock_guard lock{sequece_quard};
            sequence.push_back(timeout);
            finished.notify_one();
        };
        if (i&1) {
            scheduler.ScheduleAt(std::chrono::steady_clock::now() + timeout, std::move(cb)); 
        }
        else {
            scheduler.ScheduleAfter(timeout, std::move(cb));
        }
        maxWaitTime = timeout;
    }

    std::unique_lock lock{sequece_quard};
    (void) finished.wait_for(lock, maxWaitTime * 2, [&]() {
        return sequence.size() == kInsertElements;
    });
    lock.unlock();

    EXPECT_EQ(sequence.size(), kInsertElements);
    EXPECT_TRUE(std::ranges::is_sorted(sequence)); 
}
