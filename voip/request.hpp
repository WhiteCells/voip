#ifndef _REQUEST_H_
#define _REQUEST_H_

#include "async_timer.h"

#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <json/json.h>
#include <string>
#include <chrono>
#include <functional>
#include <memory>

namespace voip {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = Json;
using tcp = asio::ip::tcp;

using HttpPollCallback = std::function<void(void)>;

// http 1.1
inline json::Value httpRequest(
    const std::string &host,
    const std::string &port,
    const std::string &target,
    http::verb method)
{
    asio::io_context ioc;
    tcp::resolver resolver(ioc);
    const auto endpoint = resolver.resolve(host, port);

    // Connect endpoint
    beast::tcp_stream stream(ioc);
    stream.connect(endpoint);

    // Request
    http::request<http::string_body> req {method, target, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, "voip");
    http::write(stream, req);

    // Accept return package
    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    // Parse `res` to json::Value
    json::CharReaderBuilder reader;
    json::Value resp;
    std::string errs;
    std::istringstream iss(res.body());
    if (!json::parseFromStream(reader, iss, &resp, &errs)) {
        return json::Value {};
    }

    // close `stream`
    beast::error_code ec;
    ec = stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    if (ec && ec != beast::errc::not_connected) {
        throw beast::system_error {ec};
    }

    return resp;
}

// inline void polling(
//     asio::io_context &ioc,
//     unsigned int interval,
//     std::function<void()> cb,

// )
// {
// }

inline void httpPolling(
    asio::io_context &ioc,
    const std::string &host,
    const std::string &port,
    const std::string &target,
    http::verb method)
{
    // AsyncTimer timer {ioc, std::ch};
}

inline void heartbeatHttpPoll(asio::io_context &ioc, const std::string &id)
{
    AsyncTimer timer {ioc, std::chrono::seconds {1}};
    timer.start([&]() {
        httpRequest("localhost", "5000", "/" + id, http::verb::post);
    });
}

} // namespace voip

#endif // _REQUEST_H_