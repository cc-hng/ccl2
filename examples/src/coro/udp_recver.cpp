//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <ccl2/asio_pool.h>

namespace asio = boost::asio;

namespace boost {
namespace asio {
template <class T, class... Args>
using task = awaitable<T, Args...>;
}
}  // namespace boost

using boost::asio::ip::udp;

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: client <host>" << std::endl;
            return 1;
        }

        ccl2::asio_pool_init(1);
        auto& io_context = ccl2::asio_pool_get_io_context();

        udp::resolver resolver(io_context);
        udp::endpoint receiver_endpoint =
            *resolver.resolve(udp::v4(), argv[1], "daytime").begin();

        udp::socket socket(io_context);
        socket.open(udp::v4());

        boost::array<char, 1> send_buf = {{0}};
        socket.send_to(boost::asio::buffer(send_buf), receiver_endpoint);
        // socket.send(boost::asio::buffer(send_buf));

        boost::array<char, 128> recv_buf;
        udp::endpoint sender_endpoint;
        size_t len = socket.receive_from(boost::asio::buffer(recv_buf), sender_endpoint);
        // auto len = socket.receive(boost::asio::buffer(recv_buf));

        std::cout.write(recv_buf.data(), len);
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
