//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ctime>
#include <iostream>
#include <string>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <ccl2/asio_pool.h>

namespace asio = boost::asio;

namespace boost {
namespace asio {
template <class T, class... Args>
using task = awaitable<T, Args...>;
}
}  // namespace boost

using boost::asio::ip::udp;

std::string make_daytime_string() {
    using namespace std;  // For time_t, time and ctime;
    time_t now = time(0);
    return ctime(&now);
}

class udp_server {
public:
    udp_server(boost::asio::io_context& io_context)
      : socket_(io_context, udp::endpoint(udp::v4(), 13)) {
        asio::co_spawn(io_context, async_receive(), asio::detached);
    }

private:
    asio::task<void> async_receive() {
        while (true) {
            [[maybe_unused]] auto sz = co_await socket_.async_receive_from(
                asio::buffer(recv_buffer_), remote_endpoint_, asio::use_awaitable);

            auto msg = make_daytime_string();
            sz       = co_await socket_.async_send_to(
                asio::buffer(msg), remote_endpoint_, asio::use_awaitable);
        }
    }

private:
    udp::socket socket_;
    udp::endpoint remote_endpoint_;
    boost::array<char, 1> recv_buffer_;
};

int main() {
    try {
        auto& io_ctx = ccl2::asio_pool_get_io_context();
        udp_server server(io_ctx);
        ccl2::asio_pool_run();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
