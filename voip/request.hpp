#ifndef _REQUEST_H_
#define _REQUEST_H_

#include "io_context_pool.h"
#include "logger.h"
#include "global.h"
#include <boost/asio.hpp>
#include <json/value.h>
#ifdef VOIP_SSL
#include <boost/asio/ssl.hpp>
#include <boost/beast/ssl.hpp>
#endif
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <json/json.h>
#include <string>

namespace voip {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
#ifdef VOIP_SSL
namespace ssl = asio::ssl;
#endif
using tcp = asio::ip::tcp;

/**
 * @brief 封装 http 1.1 请求
 *
 * @param host 请求主机名
 * @param port 请求端口
 * @param target 请求路径
 * @param method 请求方法
 * @param params 可选请求路径查询参数
 * @param body 可选请求体内容
 * @return Json::Value 响应数据回包
 */
inline Json::Value httpRequest(const std::string &host,
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
        req.set(http::field::content_type, "application/Json");
        req.content_length(body.size());
    }

    http::write(stream, req);

    // Accept return package
    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    // Parse `res` to Json::Value
    Json::Value resp;
    std::string errs;
    std::istringstream iss(res.body());
    Json::CharReaderBuilder reader;
    if (!Json::parseFromStream(reader, iss, &resp, &errs)) {
        return Json::Value {};
    }

    // Close `stream`
    beast::error_code ec;
    ec = stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    if (ec && ec != beast::errc::not_connected) {
        throw beast::system_error {ec};
    }

    return resp;
}

#ifdef VOIP_SSL
inline Json::Value httpSSLRequest(const std::string &host,
                                  const std::string &port,
                                  const std::string &target,
                                  http::verb method,
                                  const std::map<std::string, std::string> &params = {},
                                  const std::string &body = "")
{
    auto &ioc = IOContextPool::getInstance()->getIOContext();

    // ssl
    ssl::context ctx(ssl::context::tls_client);
    ctx.set_verify_mode(ssl::verify_peer);     // 启用证书验证
    ctx.load_verify_file(backend_verify_file); // CA

    // resolve
    tcp::resolver resolver(ioc);
    auto const results = resolver.resolve(host, port);

    // ssl stream
    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

    // SNI
    if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
        beast::error_code ec {static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()};
        throw beast::system_error {ec};
    }

    // tcp connect
    beast::get_lowest_layer(stream).connect(results);

    // tls handshake
    stream.handshake(ssl::stream_base::client);

    // query
    std::string query_string;
    for (const auto &[key, value] : params) {
        query_string += (query_string.empty() ? "?" : "&") + key + "=" + value;
    }
    std::string full_target = target + query_string;

    // request
    http::request<http::string_body> req {method, full_target, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, "voip");

    if (!body.empty() && (method == http::verb::post || method == http::verb::put)) {
        req.body() = body;
        req.set(http::field::content_type, "application/Json");
        req.content_length(body.size());
    }

    // write
    http::write(stream, req);

    // read
    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    // parse Json
    Json::Value resp;
    std::string errs;
    std::istringstream iss(res.body());
    Json::CharReaderBuilder reader;
    if (!Json::parseFromStream(reader, iss, &resp, &errs)) {
        return Json::Value {};
    }

    // close
    beast::error_code ec;
    stream.shutdown(ec);
    if (ec == asio::error::eof) {
        ec.assign(0, ec.category()); // 忽略 EOF
    }
    if (ec && ec != beast::errc::not_connected) {
        throw beast::system_error {ec};
    }

    return resp;
}

inline Json::Value httpSSLRequest2(const std::string &host,
                                   const std::string &port,
                                   const std::string &target,
                                   http::verb method,
                                   const std::map<std::string, std::string> &params = {},
                                   const std::string &body = "")
{
    auto &ioc = IOContextPool::getInstance()->getIOContext();

    // ssl
    ssl::context ctx(ssl::context::tls_client);
    ctx.set_verify_mode(ssl::verify_peer);           // 启用证书验证
    ctx.load_verify_file(agent_session_verify_file); // CA

    // resolve
    tcp::resolver resolver(ioc);
    auto const results = resolver.resolve(host, port);

    // ssl stream
    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

    // SNI
    if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
        beast::error_code ec {static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()};
        throw beast::system_error {ec};
    }

    // tcp connect
    beast::get_lowest_layer(stream).connect(results);

    // tls handshake
    stream.handshake(ssl::stream_base::client);

    // query
    std::string query_string;
    for (const auto &[key, value] : params) {
        query_string += (query_string.empty() ? "?" : "&") + key + "=" + value;
    }
    std::string full_target = target + query_string;

    // request
    http::request<http::string_body> req {method, full_target, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, "voip");

    if (!body.empty() && (method == http::verb::post || method == http::verb::put)) {
        req.body() = body;
        req.set(http::field::content_type, "application/Json");
        req.content_length(body.size());
    }

    // write
    http::write(stream, req);

    // read
    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    // parse Json
    Json::Value resp;
    std::string errs;
    std::istringstream iss(res.body());
    Json::CharReaderBuilder reader;
    if (!Json::parseFromStream(reader, iss, &resp, &errs)) {
        return Json::Value {};
    }

    // close
    beast::error_code ec;
    stream.shutdown(ec);
    if (ec == asio::error::eof) {
        ec.assign(0, ec.category()); // 忽略 EOF
    }
    if (ec && ec != beast::errc::not_connected) {
        throw beast::system_error {ec};
    }

    return resp;
}

#endif

inline void pushAccountsRegState(const std::vector<AccountsRegState> &accounts_reg_state)
{
    const auto target_url = genUrl(URL_ACCOUNTS_REGSTATE, g_gui_cfg.gui_client_id);
    try {
        Json::Value accounts_array(Json::arrayValue);
        for (const auto &account : accounts_reg_state) {
            Json::Value body_json;
            body_json["account_id"] = account.account_id;
            body_json["status"] = account.status;
            accounts_array.append(body_json);
        }
        Json::Value root;
        root["accounts"] = accounts_array;
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        std::string body = Json::writeString(writer, root);
        LOG_INFO("push call state body: {}", body);
#ifdef VOIP_SSL
        auto resp = httpSSLRequest(backend_host, backend_port, target_url,
                                   http::verb::post, {}, body);
#else
        auto resp = httpRequest(backend_host, backend_port, target_url,
                                http::verb::post, {}, body);
#endif
    }
    catch (const std::exception &e) {
        LOG_WARN("Exception: {}", e.what());
    }
}

inline void pushCallState(const std::string &phone,
                          const std::string &status,
                          const std::string &call_type,
                          const std::string &hangup_direction,
                          const std::string &call_method,
                          const std::string &different)
{
    const auto target_url = genUrl(URL_CALL_STATE, g_gui_cfg.gui_client_id);
    try {
        Json::Value body_json;
        body_json["task_id"] = g_task_id;
        body_json["phone"] = phone;
        body_json["status"] = status;
        body_json["call_type"] = call_type;
        body_json["hangup_direction"] = hangup_direction;
        body_json["call_method"] = call_method;
        body_json["different"] = different;

        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        std::string body = Json::writeString(writer, body_json);
        LOG_INFO("push call state body: {}", body);
#ifdef VOIP_SSL
        auto resp = httpSSLRequest(backend_host, backend_port, target_url,
                                   http::verb::post, {}, body);
#else
        auto resp = httpRequest(backend_host, backend_port, target_url,
                                http::verb::post, {}, body);
#endif
    }
    catch (const boost::system::system_error &e) {
        if (e.code() == asio::error::eof || e.code() == asio::ssl::error::stream_truncated) {
            LOG_INFO("Server closed connection prematurely");
        }
        else {
            LOG_ERROR("SSL error: {}", e.what());
        }
    }
    catch (const std::exception &e) {
        LOG_WARN("Exception: {}", e.what());
    }
}

inline void pushGroupCallFinished()
{
    const auto target_url = genUrl(URL_GROUP_CALL_STATE, g_gui_cfg.gui_client_id);
    try {
        std::map<std::string, std::string> params;
        params["is_push"] = "true";
#ifdef VOIP_SSL
        auto resp = httpSSLRequest(backend_host, backend_port, target_url,
                                   http::verb::post, params);
#else
        auto resp = httpRequest(backend_host, backend_port, target_url,
                                http::verb::post, params);
#endif
        LOG_INFO("push group call finished");
    }
    catch (const std::exception &e) {
        LOG_WARN("Exception: {}", e.what());
    }
}

// todo
inline void pushTTSStart(std::string session_id,
                         std::string tts_text,
                         std::uint64_t start_time,
                         float play_cast,
                         std::uint64_t req_cast)
{
    const std::string target_url = "/session/tts/start/" + session_id;
    try {
        Json::Value body_json;
        body_json["tts_text"] = tts_text;
        body_json["start_time"] = start_time;
        body_json["play_cast"] = play_cast;
        body_json["req_cast"] = req_cast;
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        std::string body = Json::writeString(writer, body_json);
        LOG_INFO("push tts start body: {}", body);
        auto resp = httpSSLRequest2(agent_session_remote_host,
                                    agent_session_remote_port,
                                    target_url, http::verb::post,
                                    {}, body);
    }
    catch (const std::exception &e) {
        LOG_WARN("Exception: {}", e.what());
    }
}

// todo
inline void pushTTSStop(std::string session_id,
                        std::uint64_t stop_time)
{
    const std::string target_url = "/session/tts/stop/" + session_id;
    try {
        Json::Value body_json;
        body_json["stop_time"] = stop_time;
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        std::string body = Json::writeString(writer, body_json);
        LOG_INFO("push tts stop body: {}", body);
        auto resp = httpSSLRequest2(agent_session_remote_host,
                                    agent_session_remote_port,
                                    target_url, http::verb::post,
                                    {}, body);
    }
    catch (const std::exception &e) {
        LOG_WARN("Exception: {}", e.what());
    }
}

} // namespace voip

#endif // _REQUEST_H_