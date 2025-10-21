#ifndef LLM_REQUEST_H
#define LLM_REQUEST_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <json/json.h>
#include <string>
#include <iostream>
#include <chrono>
#include "logger.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::chrono_literals;

class LLMRequest
{
public:
    explicit LLMRequest(std::string host, std::string port, std::string target)
        : m_host(std::move(host)), m_port(std::move(port)), m_target(std::move(target)) {}

    // 发送请求（带超时与异常处理）
    std::string sendRequest(const std::string &user_text, const std::string &session_id, const std::string &access_token, const std::string &status = "true", int timeout_seconds = 10)
    {

        try {
            net::io_context ioc;
            tcp::resolver resolver(ioc);
            beast::tcp_stream stream(ioc);

            // 设置超时时间
            stream.expires_after(std::chrono::seconds(timeout_seconds));

            // 连接服务器
            auto const results = resolver.resolve(m_host, m_port);
            stream.connect(results);

            // 构造 JSON 请求体
            Json::Value root;
            root["user_text"] = user_text;
            root["status"] = status;
            Json::StreamWriterBuilder writer;
            std::string body = Json::writeString(writer, root);

            // 构造 HTTP POST 请求
            std::string final_target = m_target + "/" + session_id;
            LOG_INFO("[LLMRequest] Sending request to {}:{} {}", m_host, m_port, final_target);

            http::request<http::string_body> req {http::verb::post, final_target, 11};
            req.set(http::field::host, m_host);
            req.set(http::field::user_agent, "Boost.Beast-LLMRequest");
            req.set(http::field::content_type, "application/json; charset=utf-8");
            req.set(http::field::authorization, "Bearer " + access_token);
            req.set(http::field::accept_charset, "utf-8");
            req.body() = body;
            req.prepare_payload();

            // 发送请求
            http::write(stream, req);

            // 读取响应
            beast::flat_buffer buffer;
            http::response<http::string_body> res;

            // 读取前重置超时
            stream.expires_after(std::chrono::seconds(timeout_seconds));
            http::read(stream, buffer, res);

            // 优雅关闭
            beast::error_code ec;
            stream.socket().shutdown(tcp::socket::shutdown_both, ec);
            if (ec && ec != beast::errc::not_connected)
                throw beast::system_error {ec};

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
        catch (const beast::system_error &se) {
            LOG_INFO("[LLMRequest] Beast system_error: {}", se.what());
            return "";
        }
        catch (const std::exception &e) {
            LOG_INFO("[LLMRequest] Exception: {}", e.what());
            return "";
        }
        catch (...) {
            LOG_INFO("[LLMRequest] Unknown exception caught");
            return "";
        }
    }

    void setTarget(const std::string &target)
    {
        m_target = target;
    }

private:
    std::string m_host;
    std::string m_port;
    std::string m_target = "/api/llm_request";
};

#endif // LLM_REQUEST_H
