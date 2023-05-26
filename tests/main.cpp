#include "gtest/gtest.h"
#include "scheduler_test.hpp"
#include "thread_pool_test.hpp"
#include "timed_thread_pool_test.hpp"

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}