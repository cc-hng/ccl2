#pragma once

#ifndef CCL2_USE_COROUTINES
#    error "Please rebuild with CCL2_WITH_COROUTINES"
#endif

#include <map>
#include <string>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace ccl2 {

struct FetchOptions {
    using verb_type = boost::beast::http::verb;
    FetchOptions()  = default;

    verb_type method = verb_type::get;
    int timeout      = 30;  // seconds
    std::map<std::string, std::string> headers;
    std::string body;
};

boost::asio::awaitable<std::string>
http_client_fetch(std::string url, const FetchOptions = {});

boost::asio::awaitable<std::string>
http_client_fetch(std::string hostname, std::string port, std::string target,
                  const FetchOptions = {});

}  // namespace ccl2
