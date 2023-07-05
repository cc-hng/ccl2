#pragma once

#ifndef CCL2_USE_COROUTINES
#    error "Please rebuild with CCL2_WITH_COROUTINES"
#endif

#include <memory>
#include <string>
#include <string_view>
#include <ccl2/http/router.h>

namespace boost {
namespace asio {
class io_context;
}
}  // namespace boost

namespace ccl2 {

class HttpServer {
public:
    struct options_t {
        int timeout;
        int body_limit;
    };

public:
    HttpServer(boost::asio::io_context& ioc, std::string_view address,
               unsigned short port, options_t options = {30, -1});
    ~HttpServer();

    /// static file server
    void serve_static(std::string_view path, std::string_view doc_root);

    /// TODO:
    /// upload (POST)
    void serve_multipart(std::string_view path);

    /// run
    void do_launch();

    /// helper of router
    void set_route_prefix(std::string_view prefix) { router().set_prefix(prefix); }

    template <class Handler>
    void get(std::string_view path, Handler&& handler) {
        router().add_route(Router::method_type::get, path, std::move(handler));
    }

    template <class Handler>
    void post(std::string_view path, Handler&& handler) {
        router().add_route(Router::method_type::post, path, std::move(handler));
    }

    template <class Handler>
    void put(std::string_view path, Handler&& handler) {
        router().add_route(Router::method_type::put, path, std::move(handler));
    }

    template <class Handler>
    void patch(std::string_view path, Handler&& handler) {
        router().add_route(Router::method_type::patch, path, std::move(handler));
    }

    template <class Handler>
    void delete_(std::string_view path, Handler&& handler) {
        router().add_route(Router::method_type::delete_, path, std::move(handler));
    }

    template <class Handler>
    void all(std::string_view path, Handler&& handler) {
        router().add_route(Router::method_type::unknown, path, std::move(handler));
    }

private:
    Router& router();

private:
    boost::asio::io_context& ioc_;
    const std::string address_;
    const unsigned short port_;
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace ccl2
