#pragma once

#ifndef CCL2_USE_COROUTINES
#    error "Please rebuild with CCL2_WITH_COROUTINES"
#endif

#include <list>
#include <mutex>
#include <boost/asio.hpp>
#include <boost/core/noncopyable.hpp>
#include <ccl2/asio_pool.h>
#include <ccl2/singleton_provider.h>

namespace ccl2 {

template <class T>
class AsyncQueue : boost::noncopyable {
public:
    using value_type = T;
    using queue_type = std::list<T>;
    using time_point = boost::asio::steady_timer::clock_type::time_point;

    AsyncQueue() : AsyncQueue(asio_pool_get_io_context()) {}

    explicit AsyncQueue(boost::asio::io_context& ioc) : strand_(ioc), timer_(ioc) {}

    void push(value_type&& v) {
        {
            std::lock_guard _lock{queue_mtx_};
            queue_.emplace_back(std::move(v));
        }
        wake();
    }

    boost::asio::awaitable<queue_type> pop() {
        queue_type r = try_pop();
        if (r.size() > 0) {
            co_return r;
        }

        // wait for event
        co_await boost::asio::dispatch(strand_, boost::asio::use_awaitable);
        boost::system::error_code ec;
        timer_.expires_at(time_point::max(), ec);
        if (!ec) {
            std::ignore = co_await timer_.async_wait(
                boost::asio::as_tuple(boost::asio::use_awaitable));
        }

        std::lock_guard _lock{queue_mtx_};
        r = std::move(queue_);
        co_return r;
    }

    queue_type try_pop() {
        queue_type r;
        if (queue_mtx_.try_lock()) {
            r = std::move(queue_);
            queue_mtx_.unlock();
        }
        return r;
    }

    int size() const {
        std::unique_lock _lock{queue_mtx_};
        return queue_.size();
    }

    bool empty() const { return size() == 0; }

    void wake() {
        boost::asio::dispatch(strand_, [this] {
            boost::system::error_code ec;
            timer_.cancel(ec);
        });
    }

private:
    // mutable std::mutex queue_mtx_;
    mutable std::recursive_mutex queue_mtx_;
    queue_type queue_;

    boost::asio::io_context::strand strand_;
    boost::asio::steady_timer timer_;
};

template <class T>
using AsyncQueueProvider = SingletonProvider<AsyncQueue<T>>;

}  // namespace ccl2
