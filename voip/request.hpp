#ifndef _REQUEST_H_
#define _REQUEST_H_

#include "io_context_pool.h"
#include "global.h"

#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <json/json.h>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace voip {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = Json;
using tcp = asio::ip::tcp;

// http 1.1
inline json::Value httpRequest(
    const std::string &host,
    const std::string &port,
    const std::string &target,
    http::verb method,
    const std::map<std::string, std::string> &params = {},
    const std::string &body = "")
{
    auto &ioc = IOContextPool::getInstance()->getIOContext();
    tcp::resolver resolver(ioc);
    const auto endpoint = resolver.resolve(host, port);

    // Connect endpoint
    beast::tcp_stream stream(ioc);
    stream.connect(endpoint);

    // Params
    std::string query_string;
    for (const auto &[key, value] : params) {
        query_string += (query_string.empty() ? "?" : "&") + key + "=" + value;
    }

    std::string full_target = target + query_string;

    // Request
    http::request<http::string_body> req {method, full_target, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, "voip");

    // Body
    if (!body.empty() && (method == http::verb::post || method == http::verb::put)) {
        req.body() = body;
        req.set(http::field::content_type, "application/json");
        req.content_length(body.size());
    }

    http::write(stream, req);

    // Accept return package
    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    // Parse `res` to json::Value
    json::Value resp;
    std::string errs;
    std::istringstream iss(res.body());
    json::CharReaderBuilder reader;
    if (!json::parseFromStream(reader, iss, &resp, &errs)) {
        return json::Value {};
    }

    // Close `stream`
    beast::error_code ec;
    ec = stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    if (ec && ec != beast::errc::not_connected) {
        throw beast::system_error {ec};
    }

    return resp;
}

inline void pushFile(const std::string file_path, const std::string &client_id)
{
    auto &ioc = IOContextPool::getInstance()->getIOContext();
    tcp::resolver resolver(ioc);
    beast::tcp_stream stream(ioc);

    auto const results = resolver.resolve(backend_host, backend_port);
    stream.connect(results);

    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return;
    }

    std::size_t file_size = file.tellg();
    if (file_size == 0) {
        std::cerr << "File is empty: " << file_path << std::endl;
        return;
    }

    file.seekg(0);

    std::string target_url = "/dial_wav/" + client_id;
    http::request<http::dynamic_body> req {
        http::verb::post, target_url, 11};
    req.set(http::field::host, backend_host);
    req.set("filename", std::filesystem::path(file_path).filename().string());

    beast::ostream(req.body()) << file.rdbuf();
    req.prepare_payload();

    try {
        http::write(stream, req);
        beast::flat_buffer buffer_res;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer_res, res);
        std::cout << "Response: " << res << std::endl;
    }
    catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    stream.socket().shutdown(tcp::socket::shutdown_both);
}

} // namespace voip

#endif // _REQUEST_H_