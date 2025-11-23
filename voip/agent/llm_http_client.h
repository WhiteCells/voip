#pragma once

#include "../logger.h"
#include "../event/event.h"
#include "../global.h"
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

class LLMHttpClient
{
public:
    LLMHttpClient(EventBus &e)
        : m_event_bus(e)
    {
        start_worker();
        m_event_bus.subscribe<ASRTextMsg>([&](const ASRTextMsg &msg) {
            Json::Value resp = request(msg.text, msg.role, ASRWsClient::s_call_method,
                                       agent_session_remote_host,
                                       agent_session_remote_port,
                                       agent_session_remote_target,
                                       ASRWsClient::s_session_id,
                                       ASRWsClient::s_call_method);
            std::vector<std::string> text_vec;
            if (!resp.empty() && resp.isMember("data") && resp["data"].isObject()) {
                const auto &text_list = resp["data"];
                if (text_list.isMember("text") && text_list["text"].isArray()) {
                    for (const auto &t : text_list["text"]) {
                        text_vec.push_back(t.asString());
                    }
                }
            }
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                m_stop_customer.store(true);
                while (!m_asr_text_vec_que.empty()) {
                    m_asr_text_vec_que.pop();
                }
                m_asr_text_vec_que.push(text_vec);
            }
            m_cv.notify_one();
        });
        m_event_bus.subscribe<PrologTextMsg>([&](const PrologTextMsg &msg) {
            Json::Value resp = request(msg.text, "mediator", ASRWsClient::s_call_method,
                                       agent_session_remote_host,
                                       agent_session_remote_port,
                                       agent_session_remote_target,
                                       ASRWsClient::s_session_id,
                                       ASRWsClient::s_call_method);
            std::vector<std::string> text_vec;
            if (!resp.empty() && resp.isMember("data") && resp["data"].isObject()) {
                const auto &text_list = resp["data"];
                if (text_list.isMember("text") && text_list["text"].isArray()) {
                    for (const auto &t : text_list["text"]) {
                        text_vec.push_back(t.asString());
                    }
                }
            }
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                m_stop_customer.store(true);
                while (!m_asr_text_vec_que.empty()) {
                    m_asr_text_vec_que.pop();
                }
                m_asr_text_vec_que.push(text_vec);
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

    Json::Value request(const std::string &msg,
                        const std::string &role,
                        const std::string &call_method,
                        const std::string &host,
                        const std::string &port,
                        const std::string &target,
                        const std::string &session_id,
                        const std::string &access_token)
    {
        LOG_INFO("start llm requests");

        net::io_context ioc;
        ssl::context ctx(ssl::context::sslv23_client);
        ctx.set_verify_mode(ssl::verify_peer);
        ctx.load_verify_file(agent_session_verify_file);

        tcp::resolver resolver(ioc);
        auto const results = resolver.resolve(host, port);

        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

        if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
            beast::error_code ec {static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            throw beast::system_error {ec};
        }

        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(3));

        beast::get_lowest_layer(stream).connect(results);

        stream.handshake(ssl::stream_base::client);

        Json::Value root;
        root["call_method"] = call_method;
        root["role"] = role;
        root["text"] = msg;

        Json::StreamWriterBuilder writer;
        std::string body = Json::writeString(writer, root);

        std::string final_target = target + "/" + session_id;

        LOG_INFO("[LLMRequest] Sending request to {}:{} {} {} {}", host, port, final_target, call_method, role);

        http::request<http::string_body> req {http::verb::post, final_target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, "voip");
        req.set(http::field::content_type, "application/json; charset=utf-8");
        req.set(http::field::authorization, "Bearer " + access_token);
        req.set(http::field::accept_charset, "utf-8");
        req.body() = body;
        req.prepare_payload();

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;

        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.shutdown(ec);
        if (ec && ec != beast::errc::not_connected) {
            throw beast::system_error {ec};
        }

        Json::CharReaderBuilder builder;
        Json::Value resp;
        std::string errs;

        std::istringstream iss(res.body());
        if (Json::parseFromStream(builder, iss, &resp, &errs)) {
            return resp;
        }
        return {};
    }

private:
    bool is_end(const std::string &s)
    {
        if (!s.empty() && s.rfind("ENDENDEND:", 0) == 0) {
            return true;
        }
        return false;
    }

private:
    EventBus &m_event_bus;
    std::mutex m_mtx;
    std::condition_variable m_cv;
    std::queue<std::vector<std::string>> m_asr_text_vec_que;
    std::thread m_customer_thread;
    std::atomic<bool> m_stop_customer {false};
    std::atomic<bool> m_exit {false};
};
