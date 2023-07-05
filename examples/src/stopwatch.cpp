#include <chrono>
#include <thread>
#include <ccl2/stopwatch.h>
#include <fmt/core.h>

using namespace std::chrono_literals;

int main() {
    ccl2::StopWatch sw;
    std::this_thread::sleep_for(3s);
    fmt::print("elapsed: {}\n", sw.elapsed());
    return 0;
}
