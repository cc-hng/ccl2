
#include <iostream>
#include <boost/beast/version.hpp>
#include <ccl2/asio_pool.h>
#include <ccl2/http.h>

namespace asio  = boost::asio;
namespace beast = boost::beast;
namespace http  = beast::http;

// debug
template <bool isRequest, class Body, class Fileds>
void http_message_print(const http::message<isRequest, Body, Fileds>& msg) {
    std::cout << ">>>>>>>>>>>>>>>>>>>>>>>> headers:" << std::endl;
    for (auto it = msg.begin(); it != msg.end(); ++it) {
        std::cout << it->name() << ": " << it->value() << std::endl;
    }
    std::cout << "\r\n";
}

static asio::awaitable<void> async_sleep(int ms) {
    asio::steady_timer timer(co_await asio::this_coro::executor);
    timer.expires_after(std::chrono::milliseconds(ms));
    co_await timer.async_wait(asio::use_awaitable);
}

static asio::awaitable<ccl2::Router::response_type>
api_b(ccl2::Router::request_type&& req, ccl2::Router::extra_param_type&&) {
    co_await async_sleep(3000);

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "The resource '" + std::string(req.target()) + "' was found.";
    res.prepare_payload();
    co_return res;
}

static ccl2::Router::response_type
api_a(ccl2::Router::request_type&& req, ccl2::Router::extra_param_type&&) {
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "The resource '" + std::string(req.target()) + "' was found.";
    res.prepare_payload();
    return res;
}

static ccl2::Router::response_type
api_upload(ccl2::Router::request_type&& req, ccl2::Router::extra_param_type&&) {
    http_message_print(req);
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.prepare_payload();
    std::cout << ">>>>>>>>>>>>>>>>>>>>>> body" << std::endl;
    std::cout << req.body() << std::endl;
    return res;
}

int main() {
    ccl2::asio_pool_init();

    auto& ioc = ccl2::asio_pool_get_io_context();
    ccl2::HttpServer server(ioc, "0.0.0.0", 8080, {});

    server.serve_static("/public", ".");
    server.set_route_prefix("/api/v1");
    server.get("/a", api_a);
    server.get("/b", api_b);
    server.post("/upload", api_upload);

    server.do_launch();
    std::cout << "http_server listen at: 0.0.0.0:8080" << std::endl;

    ccl2::asio_pool_run();
    return 0;
}
