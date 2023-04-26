#include "gtest/gtest.h"
#include "thread_pool_test.hpp"
#include "scheduler_test.hpp"

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}