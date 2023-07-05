//
// callback_wrapper.cpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <ccl2/asio_helper.h>
#include <ccl2/asio_pool.h>

//------------------------------------------------------------------------------

// This is a mock implementation of an API that uses a move-only function object
// for exposing a callback. The callback has the signature void(std::string).

template <typename Callback>
void read_input(const std::string& prompt, Callback cb) {
    std::thread([prompt, cb = std::move(cb)]() mutable {
        std::cout << prompt << ": ";
        std::cout.flush();
        std::string line;
        std::getline(std::cin, line);
        std::move(cb)("hi", std::move(line));
    }).detach();
}

template <typename Callback>
void read_input2(Callback cb) {
    std::thread([cb = std::move(cb)]() mutable {
        std::cout << "Press any key to stop: ";
        std::cout.flush();
        std::string line;
        std::getline(std::cin, line);
        std::move(cb)();
    }).detach();
}

template <typename CompletionToken, class Sig = void(std::string, std::string)>
auto async_read_input(const std::string& prompt, CompletionToken&& token) {
    return boost::asio::async_initiate<CompletionToken, Sig>(
        CCL2_ASIO_DEFINE_ASYNC_INIT(read_input, Sig), token, prompt);
}

template <typename CompletionToken, class Sig = void()>
auto async_read_input2(CompletionToken&& token) {
    return boost::asio::async_initiate<CompletionToken, Sig>(
        CCL2_ASIO_DEFINE_ASYNC_INIT(read_input2, Sig), token);
}

//------------------------------------------------------------------------------

void test_callback() {
    boost::asio::io_context io_context;

    // Test our asynchronous operation using a lambda as a callback. We will use
    // an io_context to specify an associated executor.
    async_read_input("Enter your name",
                     boost::asio::bind_executor(
                         io_context,
                         [](const std::string& a1, const std::string& result) {
                             std::cout << "Hello " << a1 << ", " << result << "\n";
                         }));

    io_context.run();
}

asio::awaitable<void> test_awaitable() {
    auto [a1, a2] = co_await async_read_input("Enter your name", asio::use_awaitable);
    std::cout << "Hello " << a1 << ", " << a2 << "\n";

    co_await async_read_input2(asio::use_awaitable);

    ccl2::asio_pool_shutdown();
    co_return;
}

//------------------------------------------------------------------------------

int main() {
    // test_callback();

    auto& ioc = ccl2::asio_pool_get_io_context();

    asio::co_spawn(ioc, test_awaitable(), asio::detached);

    ccl2::asio_pool_run();
    return 0;
}
