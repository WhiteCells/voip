#pragma once

#include "../logger.h"
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/version.hpp>
#include <json/json.h>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;
using namespace std::chrono_literals;

// 在接收到 ENDEND 之后通知
// 输入: 客户对话文本
// 输出: 调解员对话文本
class LLMHttpClient
{
public:
    LLMHttpClient();
    ~LLMHttpClient();

    Json::Value request(const std::string &msg)
    {
        try {
            LOG_INFO("start llm requests");
            // m_session_id = session_id;

            net::io_context ioc;
            ssl::context ctx(ssl::context::sslv23_client);
            ctx.set_verify_mode(ssl::verify_peer);
            ctx.load_verify_file("agent_session_verify_file");

            tcp::resolver resolver(ioc);
            auto const results = resolver.resolve(m_host, m_port);

            beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

            if (!SSL_set_tlsext_host_name(stream.native_handle(), m_host.c_str())) {
                beast::error_code ec {static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
                throw beast::system_error {ec};
            }

            beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(2));

            beast::get_lowest_layer(stream).connect(results);

            stream.handshake(ssl::stream_base::client);

            // 设置超时时间
            // stream.expires_after(std::chrono::seconds(timeout_seconds));

            // 连接服务器
            // auto const results = resolver.resolve(m_host, m_port);
            // stream.connect(results);

            //            root["session_id"] = session_id];
            // 构造 HTTP POST 请求
            std::string final_target = m_target + "/" + m_session_id;

            LOG_INFO("[LLMRequest] Sending request to {}:{} {} {} {}", m_host, m_port, final_target, m_call_method, m_role);

            http::request<http::string_body> req {http::verb::post, final_target, 11};
            req.set(http::field::host, m_host);
            req.set(http::field::user_agent, "Boost.Beast-LLMRequest");
            req.set(http::field::content_type, "application/json; charset=utf-8");
            req.set(http::field::authorization, "Bearer " + m_access_token);
            req.set(http::field::accept_charset, "utf-8");
            req.body() = "body";
            req.prepare_payload();

            // 发送请求
            http::write(stream, req);

            // 读取响应
            beast::flat_buffer buffer;
            http::response<http::string_body> res;

            // 读取前重置超时
            // stream.expires_after(std::chrono::seconds(timeout_seconds));
            http::read(stream, buffer, res);

            // 优雅关闭
            beast::error_code ec;
            stream.shutdown(ec);
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error {ec};
            }

            // 尝试解析 JSON 响应
            Json::CharReaderBuilder readerBuilder;
            Json::Value jsonResponse;
            std::string errs;

            std::istringstream iss(res.body());
            if (Json::parseFromStream(readerBuilder, iss, &jsonResponse, &errs)) {
                Json::StreamWriterBuilder writer;
                writer["emitUTF8"] = true;
                return Json::writeString(writer, jsonResponse);
            }
            else {
                LOG_INFO("[LLMRequest] JSON parse failed: {}", errs.c_str());
                return res.body();
            }
        }
        catch (...) {
            LOG_INFO("[LLMRequest] Unknown exception caught");
            return "";
        }
    }

private:
    std::string m_host;
    std::string m_port;
    std::string m_target;
    std::string m_session_id;
    std::string m_access_token;
    std::string m_call_method;
    std::string m_role;
};
