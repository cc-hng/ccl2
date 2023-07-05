
#ifdef CCL2_USE_COROUTINES

#    include "ccl2/http/router.h"
#    include "ccl2/http/mime_types.h"
#    include "ccl2/url.h"
#    include <regex>
#    include <stdexcept>
#    include <boost/beast/version.hpp>

namespace asio  = boost::asio;
namespace beast = boost::beast;
namespace http  = beast::http;

namespace {

struct rule_t {
    std::vector<std::string> keys;
    std::regex re;
};

/// @brief url target
/// @in:
///   path: "/user/:id/:name"
/// @out:
///   keys: ["id", "ip"]
///   regex string: "^/user/(\\S+)/(\\S+)$"
void http_router_path_parse(std::string_view path, rule_t& rule) {
    std::string regex                     = "^" + std::string(path);
    std::string_view::size_type start_pos = 0;
    while (start_pos != std::string_view::npos) {
        start_pos = regex.find(':', start_pos);
        if (start_pos != std::string_view::npos) {
            const auto next_end_pos  = regex.find(':', start_pos + 1);
            const auto slash_end_pos = regex.find('/', start_pos);
            const auto end_pos       = std::min(slash_end_pos, next_end_pos);
            const auto key_name = regex.substr(start_pos + 1, end_pos - start_pos - 1);

            if (key_name.empty()) {
                throw std::runtime_error(
                    std::string("Empty parameter name found in path: ") + path.data());
            }

            regex.replace(start_pos, end_pos - start_pos, "(\\S+)");
            rule.keys.emplace_back(key_name);
        }
    }
    regex += "$";
    rule.re = std::regex(regex);
}

bool http_router_query_parse(std::string_view query,
                             ccl2::Router::extra_param_type& out) {
    const char* s = &*query.begin();
    int left      = query.size();
    int len;

    while (left > 0) {
        for (len = 0; len < left; len++) {
            if (s[len] == '&') {
                break;
            }
        }

        int j = 0;
        for (j = 0; j < len; j++) {
            if (s[j] == '=') {
                break;
            }
        }

        if (j + 1 >= len) {
            return false;
        }
        std::string key(s, j);
        std::string val(s + j + 1, len - j - 1);
        out.emplace(key, val);

        s += len + 1;
        left -= len + 1;
    }

    return true;
}

/// @brief parse target to a kv map according to rule_t
///        throw a exception if parse failed.
/// @param target:
///             "/user/:id/:name"
///             "/user/:id/:name?address=aa&phone=123456"
///         rule:
/// @return {"id": "1", "name": "J", "address": "aa", "phone": "123456"}
bool http_router_path_apply_rule(std::string_view path, const rule_t& rule,
                                 ccl2::Router::extra_param_type& out) {
    std::cmatch cm;
    if (!std::regex_match((const char*)path.data(), cm, rule.re)) {
        return false;
    }

    if (cm.size() - 1 != rule.keys.size()) {
        return false;
    }

    int i = 1;
    for (const auto& key : rule.keys) {
        out.emplace(key, cm[i++].str());
    }

    return true;
}

}  // namespace

namespace ccl2 {

struct Router::api_route_t {
    method_type method;
    rule_t rule;
    handler_type handler;
};

void Router::set_prefix(std::string_view prefix) {
    prefix_ = std::string(prefix);
}

void Router::add_route(method_type method, std::string_view path,
                       handler_type&& handler) {
    auto r     = std::make_shared<api_route_t>();
    r->method  = method;
    r->handler = std::move(handler);
    http_router_path_parse(path, r->rule);
    api_routes_.emplace_back(r);
}

void Router::add_route(method_type method, std::string_view path,
                       sync_handler_type&& handler_type) {
    add_route(method,
              path,
              [handler_type = std::move(handler_type)](
                  request_type&& req,
                  extra_param_type&& params) -> asio::task<response_type> {
                  co_return handler_type(std::move(req), std::move(params));
              });
}

asio::task<Router::response_type> Router::handle_request(Router::request_type req) {
    auto requri = req.target();
    ccl2::url_t u;
    if (!ccl2::url_parse_requri(requri, u)) {
        co_return bad_request(req, "Illegal request-target");
    }

    /// match api_route first
    /// prefix match
    if (u.path.starts_with(prefix_)) {
        extra_param_type extra_params;
        for (const auto& r : api_routes_) {
            // method match
            if (r->method != method_type::unknown && r->method != req.method()) {
                continue;
            }

            std::string path = u.path.substr(prefix_.length());
            // route match
            if (http_router_path_apply_rule(path, r->rule, extra_params)) {
                if (!u.query.empty() && http_router_query_parse(u.query, extra_params)) {
                    co_return bad_request(req, "Illegal request-target");
                }
                co_return co_await r->handler(std::move(req), std::move(extra_params));
            }
        }
    }

    co_return not_found(req);
}

Router::response_type Router::not_found(const Router::request_type& req) {
    http::response<http::string_body> res{http::status::not_found, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "The resource '" + std::string(req.target()) + "' was not found.";
    res.prepare_payload();
    return res;
}

Router::response_type
Router::server_error(const Router::request_type& req, std::string_view what) {
    http::response<http::string_body> res{http::status::internal_server_error,
                                          req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "An error occurred: '" + std::string(what) + "'";
    res.prepare_payload();
    return res;
}

Router::response_type
Router::bad_request(const Router::request_type& req, std::string_view why) {
    http::response<http::string_body> res{http::status::bad_request, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = std::string(why);
    res.prepare_payload();
    return res;
}

}  // namespace ccl2

#endif
