#pragma once

#include <boost/asio.hpp>

namespace asio = boost::asio;  // NOLINT

// alias
namespace boost {
namespace asio {
template <class T, class... Args>
using task = awaitable<T, Args...>;
}
}  // namespace boost

#define CCL2_ASIO_DEFINE_ASYNC_INIT(_fn, _signature)                                  \
    [](BOOST_ASIO_COMPLETION_HANDLER_FOR(_signature) auto handler, auto&&... args1) { \
        auto work = boost::asio::make_work_guard(handler);                            \
        (_fn)(std::forward<decltype(args1)>(args1)...,                                \
              [handler = std::move(handler),                                          \
               work    = std::move(work)](auto&&... args2) mutable {                     \
                  auto alloc = boost::asio::get_associated_allocator(                 \
                      handler, boost::asio::recycling_allocator<void>());             \
                                                                                      \
                  boost::asio::dispatch(                                              \
                      work.get_executor(),                                            \
                      boost::asio::bind_allocator(                                    \
                          alloc,                                                      \
                          [handler = std::move(handler),                              \
                           args3   = std::make_tuple(                                 \
                               std::forward<decltype(args2)>(args2)...)]() mutable {  \
                              std::apply(std::move(handler), std::move(args3));       \
                          }));                                                        \
              });                                                                     \
    }

namespace ccl2 {

inline asio::awaitable<void> async_sleep(int ms) {
    if (ms <= 0) {
        co_await boost::asio::post(co_await asio::this_coro::executor,
                                   asio::use_awaitable);
    } else {
        asio::steady_timer timer(co_await asio::this_coro::executor);
        timer.expires_after(std::chrono::milliseconds(ms));
        co_await timer.async_wait(asio::use_awaitable);
    }
}

}  // namespace ccl2
