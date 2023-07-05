
#ifdef CCL2_USE_COROUTINES

#    include <ccl2/asio_pool.h>
#    include <ccl2/http/client.h>
#    include <gtest/gtest.h>

TEST(http_client, get) {
    auto& ioc = ccl2::asio_pool_get_io_context();
    boost::asio::co_spawn(ioc,
                          ccl2::http_client_fetch("http://www.baidu.com"),
                          [&](std::exception_ptr e, std::string s) {
                              EXPECT_TRUE(!e);
                              EXPECT_TRUE(!s.empty());
                              ccl2::asio_pool_shutdown();
                          });

    ccl2::asio_pool_run();
}

#endif
