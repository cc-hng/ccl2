
#ifdef CCL2_USE_COROUTINES

#    include "ccl2/http/client.h"
#    include "ccl2/url.h"
#    include <chrono>
#    include <boost/asio.hpp>
#    include <boost/beast.hpp>
#    include <boost/beast/version.hpp>

namespace beast = boost::beast;
namespace http  = beast::http;
namespace asio  = boost::asio;
using tcp       = boost::asio::ip::tcp;

namespace ccl2 {

boost::asio::awaitable<std::string>
http_client_fetch(std::string hostname, std::string port, std::string target,
                  const FetchOptions options) {
    std::string result;
    target = target.empty() ? "/" : target;
    // These objects perform our I/O
    // They use an executor with a default completion token of use_awaitable
    // This makes our code easy, but will use exceptions as the default error handling,
    // i.e. if the connection drops, we might see an exception.
    auto resolver = asio::use_awaitable.as_default_on(
        tcp::resolver(co_await asio::this_coro::executor));
    auto stream = asio::use_awaitable.as_default_on(
        beast::tcp_stream(co_await asio::this_coro::executor));

    // Look up the domain name
    auto const results = co_await resolver.async_resolve(hostname, port);

    // Set the timeout.
    stream.expires_after(std::chrono::seconds(options.timeout));

    // Make the connection on the IP address we get from a lookup
    std::ignore = co_await stream.async_connect(results);

    // Set up an HTTP GET request message
    // 10/11 <HTTP version: 1.0 or 1.1(default)>
    http::request<http::string_body> req;
    req.method(options.method);
    req.target(target);
    req.version(11);
    req.set(http::field::host, hostname);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    for (const auto& kv : options.headers) {
        req.set(kv.first, kv.second);
    }

    // Set the timeout.
    stream.expires_after(std::chrono::seconds(options.timeout));

    // Send the HTTP request to the remote host
    std::ignore = co_await http::async_write(stream, req);

    // This buffer is used for reading and must be persisted
    beast::flat_buffer b;

    // Declare a container to hold the response
    http::response<http::dynamic_body> res;

    // Receive the HTTP response
    std::ignore = co_await http::async_read(stream, b, res);

    result = beast::buffers_to_string(res.body().data());

    // Gracefully close the socket
    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);

    // not_connected happens sometimes
    // so don't bother reporting it.
    //
    if (ec && ec != beast::errc::not_connected)
        throw boost::system::system_error(ec, "shutdown");

    co_return result;
}

asio::awaitable<std::string>
http_client_fetch(std::string url, const FetchOptions options) {
    url_t u;
    if (auto ok = url_parse(url, u); !ok || u.port == u.nport) {
        throw std::runtime_error("http_client_fetch parse url error: " + url);
    }

    co_return co_await http_client_fetch(
        u.hostname, std::to_string(u.port), u.requri, std::move(options));
}

}  // namespace ccl2
#endif
