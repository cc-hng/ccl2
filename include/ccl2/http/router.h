#pragma once

#ifndef CCL2_USE_COROUTINES
#    error "Please rebuild with CCL2_WITH_COROUTINES"
#endif

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/core/noncopyable.hpp>

namespace boost {
namespace asio {
template <class T, class... Args>
using task = awaitable<T, Args...>;
}
}  // namespace boost

namespace ccl2 {

/// @brief: A simple router
class Router final : boost::noncopyable {
public:
    using method_type      = boost::beast::http::verb;
    using extra_param_type = std::map<std::string, std::string>;
    using request_type     = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type    = boost::beast::http::message_generator;
    using handler_type     = std::function<boost::asio::task<response_type>(
        request_type&&, extra_param_type&&)>;
    using sync_handler_type =
        std::function<response_type(request_type&&, extra_param_type&&)>;

    Router() = default;

    void set_prefix(std::string_view prefix);

    boost::asio::task<response_type> handle_request(request_type req);

    void add_route(method_type method, std::string_view path, handler_type&& handler);
    void add_route(method_type method, std::string_view path,
                   sync_handler_type&& handler_type);

    static response_type not_found(const request_type& r);
    static response_type server_error(const request_type& r, std::string_view what);
    static response_type bad_request(const request_type& r, std::string_view why);

private:
    struct api_route_t;
    using api_route_ptr_t = std::shared_ptr<api_route_t>;

private:
    std::string prefix_;
    handler_type not_found_handler_;
    std::vector<api_route_ptr_t> api_routes_;
};

}  // namespace ccl2
