
// #include <ccl2/singleton_provider.h>
#include <tuple>
#include <type_traits>
#include <boost/callable_traits.hpp>
#include <ccl2/asio_helper.h>
#include <ccl2/asio_pool.h>
#include <ccl2/signal.h>
#include <stdio.h>

namespace ct = boost::callable_traits;
/// A
/// -> /a  (int, int)
///
/// B
///

template <class CompletionToken, class Sig = void(int, int)>
auto signal_async_wait(const std::string& topic, CompletionToken&& token) {
    (void)topic;
    (void)token;
    auto init = [](BOOST_ASIO_COMPLETION_HANDLER_FOR(Sig) auto handler, auto&&... args1) {
        auto work = boost::asio::make_work_guard(handler);
        ccl2::SignalProvider::get().template register_handler2<Sig>(
            std::forward<decltype(args1)>(args1)...,
            [handler = std::move(handler),
             work    = std::move(work)](auto&&... args2) mutable {
                auto alloc = boost::asio::get_associated_allocator(
                    handler, boost::asio::recycling_allocator<void>());

                boost::asio::dispatch(
                    work.get_executor(),
                    boost::asio::bind_allocator(
                        alloc,
                        [handler = std::move(handler),
                         args3   = std::make_tuple(
                             std::forward<decltype(args2)>(args2)...)]() mutable {
                            std::apply(std::move(handler), std::move(args3));
                        }));
            });
    };
    return asio::async_initiate<CompletionToken, Sig>(init, token, topic);
}

asio::awaitable<void> background_task() {
    // Need Stream???
    // failed
    // while (true) {
    auto [x, y] = co_await signal_async_wait("/a", asio::as_tuple(asio::use_awaitable));
    printf("[X]: (%d, %d)\n", x, y);
    // }
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
    }

private:
    void on_a(int x, int y) { printf("[C]: (%d, %d)\n", x, y); }
};

void on_a(int x, int y) {
    printf("[T]: (%d, %d)\n", x, y);
}

int main() {
    ccl2::asio_pool_init(1);

    ModuleA::get().init();
    // ModuleB::get().init();
    // ModuleC::get().init();
    signal_async_wait("/a", [](int a, int b) { printf("[T]: (%d, %d)\n", a, b); });
    asio::co_spawn(ccl2::asio_pool_get_io_context(), background_task(), asio::detached);

    std::function<void(int, int)> f = [](auto&&... args) {
        on_a(std::forward<decltype(args)>(args)...);
    };

    f(1, 2);

    using Sig = void(int, int);
    // ct::return_type_t<Sig>
    printf("return is same: %d\n", std::is_same_v<void, ct::return_type_t<Sig>>);
    printf("args is same: %d\n",
           std::is_same_v<std::tuple<int, int>, ct::args_t<Sig, std::tuple>>);

    ccl2::asio_pool_run();
    return 0;
}
