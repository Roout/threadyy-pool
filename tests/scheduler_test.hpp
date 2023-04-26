#pragma once

#include "gtest/gtest.h"
#include "thread_pool.hpp"
#include "scheduler.hpp"

#include <vector>
#include <algorithm>

// ctor
TEST(scheduler, create_scheduler) {
    using namespace std::chrono_literals;

    klyaksa::ThreadPool pool{5};
    klyaksa::Scheduler scheduler{&pool};
    ASSERT_EQ(scheduler.CallbackCount(), 0);

    pool.Start();
    std::atomic<int> counter{0};
    scheduler.ScheduleAfter(5ms, [&counter]() { counter++; });
    scheduler.ScheduleAt(std::chrono::steady_clock::now() + 10ms, [&counter]() { counter++; });

    ASSERT_EQ(scheduler.CallbackCount(), 2);
    std::this_thread::sleep_for(15ms);
    ASSERT_EQ(counter, 2);
    ASSERT_EQ(scheduler.CallbackCount(), 0);
}

// schedule_at with expired/current/future time
TEST(scheduler, schedule_after_expired_time) {

}

TEST(scheduler, schedule_after_current_time) {

}

TEST(scheduler, schedule_after_future_time) {

}

TEST(scheduler, schedule_at_expired_time) {

}

TEST(scheduler, schedule_at_current_time) {

}

TEST(scheduler, schedule_at_future_time) {

}

TEST(scheduler, correct_schedule_after_sequence) {
    using namespace std::chrono_literals;
    std::vector<klyaksa::Timeout> sequence;
    //
    EXPECT_EQ(sequence.size(), 10);
    EXPECT_TRUE(std::ranges::is_sorted(sequence));
}

TEST(scheduler, correct_schedule_at_sequence) {
    using namespace std::chrono_literals;
    std::vector<klyaksa::Timeout> sequence;
    //
    EXPECT_EQ(sequence.size(), 10);
    EXPECT_TRUE(std::ranges::is_sorted(sequence));
}

TEST(scheduler, correct_schedule_mix_sequence) {
    using namespace std::chrono_literals;
    std::vector<klyaksa::Timeout> sequence;
    //
    EXPECT_EQ(sequence.size(), 10);
    EXPECT_TRUE(std::ranges::is_sorted(sequence));
}

// schedule_at a lot of callbacks to catch exception!
TEST(scheduler, callbacks_overflow) {

}
