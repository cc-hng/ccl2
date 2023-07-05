#include <iostream>
#include <thread>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <ccl2/asio_pool.h>
#include <ccl2/stopwatch.h>

namespace asio = boost::asio;
using namespace asio::experimental::awaitable_operators;
using namespace std::chrono_literals;

namespace boost {
namespace asio {
template <class T, class... Args>
using task = awaitable<T, Args...>;
}
}  // namespace boost

asio::task<void> async_sleep(int ms) {
    asio::steady_timer timer(co_await asio::this_coro::executor);
    timer.expires_after(std::chrono::milliseconds(ms));
    co_await timer.async_wait(asio::use_awaitable);
}

asio::task<int> task_1() {
    co_await async_sleep(300);
    std::cout << "[" << std::this_thread::get_id() << "] " << __FUNCTION__ << std::endl;
    co_return 3;
}

asio::task<int> task_2() {
    co_await async_sleep(600);
    std::cout << "[" << std::this_thread::get_id() << "] " << __FUNCTION__ << std::endl;
    co_return 6;
}

asio::task<int> task_3() {
    co_await async_sleep(900);
    std::cout << "[" << std::this_thread::get_id() << "] " << __FUNCTION__ << std::endl;
    co_return 9;
}

asio::task<void> run_all() {
    ccl2::StopWatch sw;
    auto [one, two, three] = co_await (task_1() && task_2() && task_3());
    std::cout << "[" << std::this_thread::get_id() << "] " << __FUNCTION__ << ": "
              << "cost: " << sw.elapsed() << "s: result: " << one << " " << two << " "
              << three << std::endl;
}

asio::task<void> run_any() {
    ccl2::StopWatch sw;
    auto v = co_await (task_1() || task_2() || task_3());  // NOLINT
    std::cout << "[" << std::this_thread::get_id() << "] " << __FUNCTION__ << ": "
              << "cost: " << sw.elapsed() << "s: result: " << v.index() << std::endl;
}

asio::task<void> tick() {
    ccl2::StopWatch stopwatch;
    for (int i = 0; i < 60; i++) {
        co_await async_sleep(20);
        // std::cout << "["  "] tick " << i + 1
        //           << ", cost: " << stopwatch.elapsed() << "s" << std::endl;
    }

    std::cout << "[" << std::this_thread::get_id() << "] shutdown" << std::endl;
    ccl2::asio_pool_shutdown();
}

asio::task<void> tick2() {
    for (;;) {
        co_await async_sleep(500);
        co_await ccl2::asio_pool_enqueue(asio::use_awaitable);
        // auto ctx = co_await asio::this_coro::executor;
        // co_await asio::post(ctx, asio::use_awaitable);
        std::cout << "tick2" << std::endl;
    }
}

int main() {
    ccl2::StopWatch stopwatch;
    ccl2::asio_pool_init();
    auto& ctx = ccl2::asio_pool_get_io_context();

    asio::co_spawn(ctx, run_all(), asio::detached);
    asio::co_spawn(ctx, run_any(), asio::detached);
    asio::co_spawn(ctx, tick(), asio::detached);
    asio::co_spawn(ctx, tick2(), asio::detached);

    ccl2::asio_pool_run();
    std::cout << "[" << std::this_thread::get_id() << "] "
              << "main cost: " << stopwatch.elapsed() << "s" << std::endl;
    return 0;
}
