#pragma once

#include <atomic>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>
#include <boost/asio.hpp>
#include <boost/core/noncopyable.hpp>
#include <ccl2/singleton_provider.h>
#include <stddef.h>

namespace ccl2 {

class AsioPool final : boost::noncopyable {
public:
    /// @param num: the number of threads that io_context run on.
    explicit AsioPool(size_t num = std::thread::hardware_concurrency())
      : ctx_(num)
      , stopped_(false)
      , worker_count_(num)
      , work_guard_(boost::asio::make_work_guard(ctx_)) {
        if (num <= 0) {
            throw std::runtime_error("Expect: num >= 1");
        }
    }

    ~AsioPool() { shutdown(); }

    inline boost::asio::io_context& get_io_context() { return ctx_; }

    template <class CompletionToken>
    auto enqueue(CompletionToken&& token) {
        return boost::asio::dispatch(ctx_, std::forward<CompletionToken>(token));
    }

    void run() {
        if (stopped_.load(std::memory_order_relaxed)) {
            return;
        }

        std::vector<std::thread> threads_;
        threads_.reserve(worker_count_ - 1);
        for (size_t i = 0; i < worker_count_ - 1; i++) {
            threads_.emplace_back([&] { ctx_.run(); });
        }

        // run on current thread
        ctx_.run();

        for (auto& th : threads_) {
            if (th.joinable()) {
                th.join();
            }
        }
    }

    void shutdown() {
        if (!stopped_.exchange(true, std::memory_order_acquire)) {
            ctx_.stop();
            work_guard_.reset();
        }
    }

private:
    boost::asio::io_context ctx_;
    std::atomic<bool> stopped_;
    const size_t worker_count_;

    // prevent the run() method from return.
    typedef boost::asio::io_context::executor_type ExecutorType;
    boost::asio::executor_work_guard<ExecutorType> work_guard_;
};

/// AsioPool Singleton
using AsioPoolProvider = SingletonProvider<AsioPool>;

namespace detail {
static std::unique_ptr<AsioPool> kAsioPool = nullptr;
static std::once_flag kAsioPoolInitFlag;
}  // namespace detail

/// C-like interface
inline void asio_pool_init(size_t num = std::thread::hardware_concurrency()) {
    std::call_once(detail::kAsioPoolInitFlag,
                   [&, num] { detail::kAsioPool = std::make_unique<AsioPool>(num); });
}

inline void asio_pool_run() {
    asio_pool_init();
    detail::kAsioPool->run();
}

inline void asio_pool_shutdown() {
    detail::kAsioPool->shutdown();
}

inline boost::asio::io_context& asio_pool_get_io_context() {
    asio_pool_init();
    return detail::kAsioPool->get_io_context();
}

template <class CompletionToken>
auto asio_pool_enqueue(CompletionToken&& token) {
    asio_pool_init();
    return detail::kAsioPool->enqueue(std::forward<CompletionToken>(token));
}

}  // namespace ccl2
