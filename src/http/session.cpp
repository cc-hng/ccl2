#ifdef CCL2_USE_COROUTINES

#    include "ccl2/http/session.h"
#    include "ccl2/http/mime_types.h"
#    include "ccl2/http/router.h"
#    include <algorithm>
#    include <iostream>
#    include <vector>
#    include <boost/asio.hpp>
#    include <boost/beast.hpp>

namespace asio  = boost::asio;
namespace beast = boost::beast;
namespace http  = beast::http;
using tcp       = boost::asio::ip::tcp;

using tcp_stream = typename beast::tcp_stream::rebind_executor<
    asio::use_awaitable_t<>::executor_with_default<asio::any_io_executor>>::other;

namespace ccl2 {

namespace {

// Return a reasonable mime type based on the extension of a file.
std::string_view mime_type(std::string_view path) {
    using beast::iequals;
    auto const ext = [&path] {
        auto const pos = path.rfind(".");
        if (pos == std::string_view::npos) return std::string_view{};
        return path.substr(pos);
    }();

    return ccl2::get_mime_type(ext);
}

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
std::string path_cat(beast::string_view base, beast::string_view path) {
    if (base.empty()) return std::string(path);
    std::string result(base);
#    ifdef BOOST_MSVC
    char constexpr path_separator = '\\';
    if (result.back() == path_separator) result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for (auto& c : result)
        if (c == '/') c = path_separator;
#    else
    char constexpr path_separator = '/';
    if (result.back() == path_separator) result.resize(result.size() - 1);
    result.append(path.data(), path.size());
#    endif
    return result;
}

}  // namespace

class HttpServer::Impl {
public:
    using static_file_routes_t = std::map<std::string, std::string>;

    Impl(HttpServer::options_t options) : options_(std::move(options)) {}

    Router& router() { return router_; }

    void serve_static(std::string_view path, std::string_view doc_root) {
        static_file_routes_.emplace(std::string(path), std::string(doc_root));
    }

    // Accepts incoming connections and launches the sessions
    asio::task<void> do_listen(tcp::endpoint endpoint) {
        // Open the acceptor
        auto acceptor = asio::use_awaitable.as_default_on(
            tcp::acceptor(co_await asio::this_coro::executor));
        acceptor.open(endpoint.protocol());

        // Allow address reuse
        acceptor.set_option(asio::socket_base::reuse_address(true));

        // Bind to the server address
        acceptor.bind(endpoint);

        // Start listening for connections
        acceptor.listen(asio::socket_base::max_listen_connections);

        for (;;) {
            asio::co_spawn(acceptor.get_executor(),
                           do_session(tcp_stream(co_await acceptor.async_accept())),
                           [](std::exception_ptr e) {
                               if (e) try {
                                       std::rethrow_exception(e);
                                   } catch (std::exception& e) {
                                       std::cerr << "Error in session: " << e.what()
                                                 << "\n";
                                   }
                           });
        }
    }

private:
    static bool req_is_a_upload(auto& header) {
        auto method                   = header.method();
        std::string_view content_type = header[http::field::content_type];
        return method == http::verb::post
               && content_type.starts_with("multipart/form-data");
    }

    static Router::response_type static_file_handler(Router::request_type&& req,
                                                     std::string path,
                                                     std::string doc_root) {
        if (!path.empty() && (path[0] != '/' || path.find("..") != std::string::npos)) {
            return Router::bad_request(req, "Illegal request-target");
        }

        path = path_cat(doc_root, path);
        if (path.back() == '/') {
            path.append("index.html");
        }

        beast::error_code ec;
        http::file_body::value_type body;
        body.open(path.c_str(), beast::file_mode::scan, ec);

        // Handle the case where the file doesn't exist
        if (ec == beast::errc::no_such_file_or_directory) {
            return Router::not_found(req);
        }

        if (ec) {
            return Router::server_error(req, ec.message());
        }

        // Cache the size since we need it after the move
        auto const size = body.size();

        // Respond to HEAD request
        if (req.method() == http::verb::head) {
            http::response<http::empty_body> res{http::status::ok, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, mime_type(path));
            res.content_length(size);
            res.keep_alive(req.keep_alive());
            return res;
        }
        // Respond to GET request
        else {
            http::response<http::file_body> res{std::piecewise_construct,
                                                std::make_tuple(std::move(body)),
                                                std::make_tuple(http::status::ok,
                                                                req.version())};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, mime_type(path));
            res.content_length(size);
            res.keep_alive(req.keep_alive());
            return res;
        }
    }

    // Handles an HTTP server connection
    asio::task<void> do_session(tcp_stream stream) {
        static const std::vector<boost::system::error_code> errc_to_ignored = {
            boost::asio::error::connection_reset,
            boost::asio::error::timed_out,
        };

        // This buffer is required to persist across reads
        beast::flat_buffer buffer;

        // This lambda is used to send messages
        try {
            for (;;) {
                // Set the timeout.
                stream.expires_after(std::chrono::seconds(
                    options_.timeout > 0 ? options_.timeout : INT_MAX));

                // Read a request header
                http::request_parser<http::empty_body> header;
                co_await http::async_read_header(stream, buffer, header);

                // If this is not a form upload then use a string_body
                if (req_is_a_upload(header.get())) {
                    // TODO:
                } else {
                    // Read a request
                    http::request_parser<http::string_body> parser{std::move(header)};
                    if (options_.body_limit > 0) {
                        parser.body_limit(options_.body_limit);
                    }
                    co_await http::async_read(stream, buffer, parser);

                    // Handle the request
                    /// static file or router
                    std::string_view requri = parser.get().target();
                    auto& routes            = static_file_routes_;
                    auto iter               = std::find_if(
                        routes.begin(), routes.end(), [&requri](const auto& kv) {
                            std::string prefix = kv.first == "/" ? kv.first
                                                                               : (kv.first + "/");
                            return requri.starts_with(prefix);
                        });

                    std::shared_ptr<Router::response_type> msg = nullptr;
                    if (iter != routes.end()) {
                        auto path = requri.substr(iter->first.size());
                        msg = std::make_shared<Router::response_type>(static_file_handler(
                            parser.release(), std::string(path), iter->second));
                    } else {
                        msg = std::make_shared<Router::response_type>(
                            co_await router_.handle_request(parser.release()));
                    }

                    // Determine if we should close the connection
                    assert(msg != nullptr);
                    bool keep_alive = msg->keep_alive();

                    co_await beast::async_write(
                        stream, std::move(*msg), asio::use_awaitable);

                    if (!keep_alive) {
                        // This means we should close the connection, usually because
                        // the response indicated the "Connection: close" semantic.
                        break;
                    }
                }
            }
        } catch (boost::system::system_error& se) {
            auto errc = se.code();
            if (std::find(errc_to_ignored.begin(), errc_to_ignored.end(), errc)
                != errc_to_ignored.end()) {
                co_return;
            } else if (errc != http::error::end_of_stream) {
                throw;
            }
        }

        // Send a TCP shutdown
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
        // we ignore the error because the client might have
        // dropped the connection already.
    }

private:
    Router router_;
    const HttpServer::options_t options_;
    static_file_routes_t static_file_routes_;
};

HttpServer::HttpServer(boost::asio::io_context& ioc, std::string_view address,
                       unsigned short port, HttpServer::options_t options)
  : ioc_(ioc)
  , address_(address.data(), address.size())
  , port_(port)
  , impl_(std::make_unique<Impl>(std::move(options))) {
}

HttpServer::~HttpServer() {
}

void HttpServer::serve_static(std::string_view path, std::string_view doc_root) {
    impl_->serve_static(path, doc_root);
}

void HttpServer::do_launch() {
    auto addr = asio::ip::make_address(address_);
    asio::co_spawn(ioc_, impl_->do_listen(tcp::endpoint{addr, port_}), asio::detached);
}

Router& HttpServer::router() {
    return impl_->router();
}

}  // namespace ccl2

#endif
