#include <mutex>
#include <sstream>
#include <string_view>
#include <ccl2/asio_helper.h>
#include <ccl2/asio_pool.h>
#include <ccl2/async_queue.h>
#include <ccl2/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

namespace asio = boost::asio;

#define PRODUCER_COUNT      1000
#define PRODUCER_ITEM_COUNT 1000

class Counter {
public:
    void addone() {
        std::lock_guard _lck{mtx_};
        cnt_++;
    }

    int value() const {
        std::lock_guard _lck{mtx_};
        return cnt_;
    }

private:
    mutable std::recursive_mutex mtx_;
    int cnt_ = 0;
};

// run before queue
CCL2_CALL_FUNC_OUTSIDE(ccl2::asio_pool_init(1));
Counter prod_cnter, cons_cnter;
// ccl2::AsyncQueue<int> kQueue(ccl2::asio_pool_get_io_context());
auto& kQueue = ccl2::AsyncQueueProvider<int>::get();

asio::awaitable<void> co_producer(std::string remark, int count = PRODUCER_ITEM_COUNT) {
    for (int i = 0; i < count; i++) {
        co_await ccl2::async_sleep(0);
        int r = rand() % 10;
        (void)remark;
        //printf("[%s] produce: %d\n", remark.c_str(), r);
        kQueue.push((int&&)r);
        prod_cnter.addone();
    }
    co_return;
}

asio::awaitable<void> co_consumer() {
    while (1) {
        auto q = co_await kQueue.pop();
        for (auto v : q) {
            (void)v;
            //printf("cons: %d\n", v);
            cons_cnter.addone();
        }
    }
}

asio::awaitable<void> co_moniter() {
    constexpr int total = PRODUCER_COUNT * PRODUCER_ITEM_COUNT;

    while (1) {
        co_await ccl2::async_sleep(1000);
        int prods = prod_cnter.value();
        int conss = cons_cnter.value();
        printf("prod: %d, cons: %d\n", prods, conss);

        if (prods == conss && prods == total) {
            break;
        }
    }

    ccl2::asio_pool_shutdown();
}

int main() {
    srand((unsigned)time(NULL));

    auto& ioc = ccl2::asio_pool_get_io_context();
    asio::co_spawn(ioc, co_consumer(), asio::detached);

    for (int i = 0; i < PRODUCER_COUNT; i++) {
        std::ostringstream oss;
        oss << "prod" << i;
        asio::co_spawn(ioc, co_producer(oss.str()), asio::detached);
    }

    asio::co_spawn(ioc, co_moniter(), asio::detached);

    ccl2::asio_pool_run();
    return 0;
}
