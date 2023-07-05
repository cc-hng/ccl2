
// #include <ccl2/singleton_provider.h>
#include <tuple>
#include <type_traits>
#include <boost/callable_traits.hpp>
#include <ccl2/asio_helper.h>
#include <ccl2/asio_pool.h>
#include <ccl2/async_queue.h>
#include <ccl2/signal.h>
#include <stdio.h>

// ccl2::Signal::get();
namespace ct = boost::callable_traits;

template <class Signature>
class SignalStream {
public:
    using item_t  = ct::args_t<Signature, std::tuple>;
    using queue_t = ccl2::AsyncQueue<item_t>;
    using items_t = typename queue_t::queue_type;

    template <class Signal>
    SignalStream(Signal& sig, std::string_view topic)
      : queue_(std::make_shared<queue_t>(ccl2::asio_pool_get_io_context())) {
        std::weak_ptr<queue_t> queue_weak = queue_;
        std::function<Signature> f        = [queue_weak](auto&&... args) {
            if (auto queue = queue_weak.lock()) {
                queue->push(std::make_tuple(std::move(args)...));
            }
        };
        sig.register_handler(topic, f);
    }

    asio::awaitable<items_t> async_wait() { co_return co_await queue_->pop(); }

private:
    std::shared_ptr<queue_t> queue_;
};

template <class Signature>
decltype(auto) make_signal_stream(auto& sig, std::string_view topic) {
    return std::make_unique<SignalStream<Signature>>(sig, topic);
}

asio::awaitable<void> async_background_task() {
    auto& sig   = ccl2::SignalProvider::get();
    auto stream = make_signal_stream<void(int, int)>(sig, "/a");

    while (true) {
        auto items = co_await stream->async_wait();
        for (const auto& [x, y] : items) {
            printf("[X]: (%d, %d)\n", x, y);
        }
    }
}

class ModuleA {
public:
    static ModuleA& get() {
        static ModuleA m;
        return m;
    }

    void init() {
        asio::co_spawn(ccl2::asio_pool_get_io_context(), task(), asio::detached);
    }

private:
    asio::awaitable<void> task() {
        while (1) {
            co_await ccl2::async_sleep(1500);
            int x = rand() % 9 + 1;
            int y = rand() % 9 + 1;
            ccl2::SignalProvider::get().emit("/a", x, y);
        }
    }
};

class ModuleB {
public:
    static ModuleB& get() {
        static ModuleB m;
        return m;
    }

    void init() {
        ccl2::SignalProvider::get().register_handler("/a", &ModuleB::on_a, this);
    }

private:
    void on_a(int x, int y) { printf("[B]: (%d, %d)\n", x, y); }
};

class ModuleC {
public:
    static ModuleC& get() {
        static ModuleC m;
        return m;
    }

    void init() {
        ccl2::SignalProvider::get().register_handler("/a", &ModuleC::on_a, this);
        asio::co_spawn(
            ccl2::asio_pool_get_io_context(), background_task(), asio::detached);
    }

private:
    ModuleC() : queue_(ccl2::asio_pool_get_io_context()) {}

    void on_a(int x, int y) { queue_.push(std::make_tuple(x, y)); }

    asio::awaitable<void> background_task() {
        while (1) {
            auto queue = co_await queue_.pop();
            co_await ccl2::async_sleep(500);
            for (const auto& [x, y] : queue) {
                printf("[C]: (%d, %d) after 500ms\n", x, y);
            }
        }
    }

    ccl2::AsyncQueue<std::tuple<int, int>> queue_;
};

void on_a(int x, int y) {
    printf("[T]: (%d, %d)\n", x, y);
}

int main() {
    ccl2::asio_pool_init(1);

    ModuleA::get().init();
    ModuleB::get().init();
    ModuleC::get().init();

    asio::co_spawn(
        ccl2::asio_pool_get_io_context(), async_background_task(), asio::detached);

    ccl2::asio_pool_run();
    return 0;
}
