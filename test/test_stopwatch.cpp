#include <chrono>
#include <thread>
#include <ccl2/stopwatch.h>
#include <gtest/gtest.h>

using namespace std::chrono_literals;

TEST(StopWatch, elapsed) {
    ccl2::StopWatch sw;
    std::this_thread::sleep_for(250ms);
    EXPECT_TRUE(sw.elapsed() > 0.25);
    EXPECT_TRUE(sw.elapsed() < 0.5);
}
