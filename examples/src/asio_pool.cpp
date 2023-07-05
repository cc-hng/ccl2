
#include <ccl2/asio_pool.h>
#include <fmt/core.h>

using namespace std::chrono_literals;

int main() {
    ccl2::AsioPool pool(1);
    auto& ctx = pool.get_io_context();

    pool.enqueue([] { fmt::print("run task1\n"); });

    boost::asio::steady_timer timer(ctx, 3s);
    timer.async_wait([&](boost::system::error_code) {
        pool.shutdown();
        fmt::print("after 3s\n");
    });

    pool.run();
    return 0;
}
