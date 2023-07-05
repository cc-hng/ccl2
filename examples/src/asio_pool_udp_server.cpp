#include <array>
#include <ctime>
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <ccl2/asio_pool.h>

namespace asio = boost::asio;
using boost::asio::ip::udp;
constexpr asio::as_tuple_t<asio::use_awaitable_t<>> yield_token;

std::string make_daytime_string() {
    using namespace std;  // For time_t, time and ctime;
    time_t now = time(0);
    return ctime(&now);
}

asio::awaitable<void> udp_server() {
    auto executor = co_await asio::this_coro::executor;
    udp::socket socket(executor, udp::endpoint(udp::v4(), 13));

    std::array<char, 1> recv_buffer;
    udp::endpoint remote_endpoint;
    for (;;) {
        // asio::use_awaitable;
        auto [e1, nread] = co_await socket.async_receive_from(
            asio::buffer(recv_buffer), remote_endpoint, yield_token);
        if (e1) {
            std::cout << "udp recv with exception: " << e1.what();
            continue;
        }

        std::string msg     = make_daytime_string();
        auto [e2, nwritten] = co_await socket.async_send_to(
            asio::buffer(msg), remote_endpoint, yield_token);
        if (e2) {
            std::cout << "udp send with exception: " << e2.what();
        }
    }
}

int main() {
    ccl2::AsioPool pool(1);
    auto& ctx = pool.get_io_context();

    asio::signal_set signals(ctx, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto) { ctx.stop(); });

    asio::co_spawn(ctx, udp_server(), asio::detached);
    pool.run();
    return 0;
}
