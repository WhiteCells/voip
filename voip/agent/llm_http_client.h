#pragma once

#include "../logger.h"
#include "../event/event.h"
#include "asr_ws_client.h"
#include "msg.h"
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
    std::mutex m_mtx;
    std::condition_variable m_cv;

    std::queue<std::vector<std::string>> m_asr_text_vec_que;

    std::thread m_customer_thread;

    std::atomic<bool> m_stop_customer {false};
    std::atomic<bool> m_exit {false};

    LLMHttpClient(EventBus &e)
        : m_event_bus(e)
    {
        start_worker();
        m_event_bus.subscribe<ASRTextMsg>([&](const ASRTextMsg &msg) {
            std::string result = request(msg.text, msg.role);

            {
                std::lock_guard<std::mutex> lock(m_mtx);
                m_stop_customer.store(true);
                while (!m_asr_text_vec_que.empty()) {
                    m_asr_text_vec_que.pop();
                }
                // m_asr_text_vec_que.push(result);
            }
            m_cv.notify_one();
        });
        m_event_bus.subscribe<PrologTextMsg>([&](const PrologTextMsg &msg) {
            // std::vector<std::string> result = request(msg.text);
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                m_stop_customer.store(true);
                while (!m_asr_text_vec_que.empty()) {
                    m_asr_text_vec_que.pop();
                }
                // m_asr_text_vec_que.push(result);
            }
            m_cv.notify_one();
        });
    }

    ~LLMHttpClient()
    {
        m_exit.store(true);
        m_cv.notify_one();
        if (m_customer_thread.joinable()) {
            m_customer_thread.join();
        }
    }

private:
    void start_worker()
    {
        m_customer_thread = std::thread([this]() {
            while (!m_exit.load()) {
                std::vector<std::string> llm_text;
                {
                    std::unique_lock<std::mutex> lock(m_mtx);

                    m_cv.wait(lock, [&]() {
                        return m_exit.load() || !m_asr_text_vec_que.empty();
                    });

                    if (m_exit.load())
                        break;

                    llm_text = m_asr_text_vec_que.front();
                    m_asr_text_vec_que.pop();

                    m_stop_customer.store(false);
                }

                for (const auto &sentence : llm_text) {
                    if (m_stop_customer.load()) {
                        break;
                    }
                    if (is_end(sentence)) {
                        std::string new_sentence = sentence.substr(10);
                        m_event_bus.publish(LLMEndMsg {new_sentence});
                    }
                    else {
                        m_event_bus.publish(LLMTextMsg {sentence});
                    }
                }
            }
        });
    }

    std::string request(const std::string &msg, const std::string &role)
    {
        LOG_INFO("start llm requests");

        net::io_context ioc;
        ssl::context ctx(ssl::context::sslv23_client);
        ctx.set_verify_mode(ssl::verify_peer);
        ctx.load_verify_file(agent_session_verify_file);

        tcp::resolver resolver(ioc);
        auto const results = resolver.resolve(m_host, m_port);

        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

        if (!SSL_set_tlsext_host_name(stream.native_handle(), m_host.c_str())) {
            beast::error_code ec {static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            throw beast::system_error {ec};
        }

        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(3));

        beast::get_lowest_layer(stream).connect(results);

        stream.handshake(ssl::stream_base::client);

        // 设置超时时间
        // stream.expires_after(std::chrono::seconds(timeout_seconds));

        // 连接服务器
        // auto const results = resolver.resolve(m_host, m_port);
        // stream.connect(results);

        // 构造 JSON 请求体
        Json::Value root;
        root["call_method"] = ASRWsClient::s_call_method;
        root["role"] = role;
        root["text"] = msg;

        // root["session_id"] = session_id];
        Json::StreamWriterBuilder writer;
        std::string body = Json::writeString(writer, root);

        // 构造 HTTP POST 请求
        std::string final_target = m_target + "/" + ASRWsClient::s_session_id;

        LOG_INFO("[LLMRequest] Sending request to {}:{} {} {} {}", m_host, m_port, final_target, "agent", role);

        http::request<http::string_body> req {http::verb::post, final_target, 11};
        req.set(http::field::host, m_host);
        req.set(http::field::user_agent, "Boost.Beast-LLMRequest");
        req.set(http::field::content_type, "application/json; charset=utf-8");
        req.set(http::field::authorization, "Bearer " + ASRWsClient::s_access_token);
        req.set(http::field::accept_charset, "utf-8");
        req.body() = body;
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
            return "";
        }
        else {
            LOG_INFO("[LLMRequest] JSON parse failed: {}", errs.c_str());
            return res.body();
        }
    }

private:
    bool
    is_end(const std::string &s)
    {
        if (!s.empty() && s.rfind("ENDENDEND:", 0) == 0) {
            return true;
        }
        return false;
    }

private:
    EventBus &m_event_bus;
    std::string m_target;
    std::string m_host;
    std::string m_port;
};
