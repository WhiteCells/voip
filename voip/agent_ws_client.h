#ifndef _AGENT_WS_CLIENT_H_
#define _AGENT_WS_CLIENT_H_

#include "global.h"
#include "logger.h"
#include "io_context_pool.h"
#include "ws_interface.h"
#include "llm_request.h"
#include "tts_request.h"

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/beast/core/error.hpp>
#include <json/json.h>
#include <boost/beast/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <memory>
#include <functional>
#include <vector>
#include <chrono>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

class AgentWsClient :
    public std::enable_shared_from_this<AgentWsClient>
{
public:
    using WebWsMsgHandler = std::function<void(const std::string &)>;

    explicit AgentWsClient(const std::string &host, const std::string &port)
        : m_host(host)
        , m_port(port)
        , m_target("/")
        , m_llm_client(std::make_shared<LLMRequest>(agent_session_remote_host, agent_session_remote_port, agent_session_remote_target))
    {
    }

    ~AgentWsClient()
    {
        stop();
    }

    void set_web_ws_sender(WebWsMsgHandler handler)
    {
        m_ws_msg_handler = std::move(handler);
    }

    void set_server_sender(std::shared_ptr<IWSSender> sender)
    {
        // m_gui_server_sender = sender;
    }

    void send(const std::string &msg)
    {
        m_ws->text(true);
        m_ws->async_write(asio::buffer(msg),
                          [self = shared_from_this()](beast::error_code ec, std::size_t) {
                              if (ec) {
                                  LOG_ERROR("Agent Ws Client Send Failed {}", ec.message());
                                  return;
                              }
                          });
    }

    void restart()
    {
        stop();
        start();
    }

    void start()
    {
        auto &ioc = IOContextPool::getInstance()->getIOContext();
        m_strand.emplace(net::make_strand(ioc));
        m_resolver = std::make_unique<tcp::resolver>(*m_strand);

        ssl::context ssl_ctx(ssl::context::tls_client);
        ssl_ctx.set_verify_mode(ssl::verify_peer);
        ssl_ctx.load_verify_file(asr_server_verify_file);
        m_ws = std::make_unique<websocket::stream<beast::ssl_stream<beast::tcp_stream>>>(*m_strand, ssl_ctx);

        m_resolver->async_resolve(m_host, m_port,
                                  beast::bind_front_handler(&AgentWsClient::on_resolver,
                                                            shared_from_this()));
    }

    void stop()
    {
        net::dispatch(*m_strand, [self = shared_from_this()]() {
            if (self->m_ws && self->m_ws->is_open()) {
                beast::error_code ec;
                self->m_ws->close(websocket::close_code::normal, ec);
                if (ec) {
                    LOG_ERROR("Close failed: {}", ec.message());
                }
                else {
                    LOG_INFO("WebSocket closed cleanly");
                }
            }
            self->m_send_queue.clear();
            self->m_writing = false;
        });
    }

    void start_config_send()
    {
        Json::Value config;
        config["mode"] = "2pass";
        config["wav_name"] = "record";
        config["wav_format"] = "pcm";
        config["audio_fs"] = 16000.0;
        config["is_speaking"] = true;
        config["itn"] = true;
        config["svs_itn"] = true;

        Json::Value chunk_size(Json::arrayValue);
        chunk_size.append(5);
        chunk_size.append(10);
        chunk_size.append(5);
        config["chunk_size"] = chunk_size;

        Json::StreamWriterBuilder builder;
        std::string config_str = Json::writeString(builder, config);
        send(config_str);
        LOG_INFO("success send asr start config");
    }

    void end_config_send()
    {
        Json::Value config;
        config["is_speaking"] = "false";

        Json::StreamWriterBuilder builder;
        std::string config_str = Json::writeString(builder, config);
        send(config_str);
        LOG_INFO("success send asr end config");
    }

    void sendBinary(const std::string &data, const std::string &role)
    {
        if (!m_ws || !m_ws->is_open()) {
            LOG_WARN("WebSocket not open, cannot send binary data");
            return;
        }

        net::dispatch(*m_strand, [self = shared_from_this(), data, role]() {
            self->m_role = role;
            self->m_send_queue.push_back(data);
            if (!self->m_writing) {
                self->do_write();
            }
        });
    }

    void do_write()
    {
        if (m_send_queue.empty() || !m_ws || !m_ws->is_open()) {
            m_writing = false;
            return;
        }

        m_writing = true;
        std::string msg = std::move(m_send_queue.front());
        m_send_queue.pop_front();

        m_ws->binary(true);
        m_ws->async_write(
            net::buffer(msg),
            net::bind_executor(
                *m_strand,
                [self = shared_from_this()](beast::error_code ec, std::size_t) {
                    if (ec) {
                        LOG_ERROR("Send Binary Failed: {}", ec.message());
                        self->m_send_queue.clear();
                        self->m_writing = false;
                        return;
                    }
                    if (!self->m_send_queue.empty()) {
                        self->do_write();
                    }
                    else {
                        self->m_writing = false;
                    }
                }));
    }

    void start_llm_style()
    {
        if (m_call_method == "agent") {
            m_llm_start_time = std::chrono::steady_clock::now();
            end_timeout_check();
            start_timeout_check(m_llm_start_time);

            std::string response = m_llm_client->sendRequest("请用开场话术开始对话", "agent", "mediator", m_session_id, m_access_token);

            Json::Value llm_style_json;
            Json::CharReaderBuilder llm_style_builder;
            std::unique_ptr<Json::CharReader> reader(llm_style_builder.newCharReader());
            std::string llm_style_errors;

            if (reader->parse(response.c_str(), response.c_str() + response.size(), &llm_style_json, &llm_style_errors)) {
                if (llm_style_json.isMember("data") && llm_style_json["data"].isObject()) {
                    const auto &data = llm_style_json["data"];
                    if (data.isMember("text") && data["text"].isArray()) {
                        m_llm_msg_list.clear();
                        for (const auto &item : data["text"]) {
                            m_llm_msg_list.push_back(item.asString());
                        }
                        LOG_INFO("LLM response data pushed to m_llm_msg_list, size: {}", m_llm_msg_list.size());

                        if (!m_llm_msg_list.empty()) {
                            TTSPlayer::getInstance()->produceTTS(m_llm_msg_list);
                            m_llm_msg_list.clear();
                        }
                    }
                }
            }
            else {
                LOG_ERROR("Failed to parse LLM response JSON: {}", llm_style_errors);
            }
        }
    }

    void clear_llm_msg_list()
    {
        m_llm_msg_list.clear();
        m_llm_msg_text.clear();
        TTSPlayer::getInstance()->stop();
        TTSPlayer::getInstance()->resume();
    }

    void get_session_id(const std::string &call_method,
                        const std::string &session_id,
                        const std::string &access_token)
    {
        m_call_method = call_method;
        m_session_id = session_id;
        m_access_token = access_token;

        LOG_INFO("get call_method {} session_id {} access_token {}", m_call_method, m_session_id, m_access_token);

        start_llm_style();
    }

private:
    void on_resolver(beast::error_code ec, tcp::resolver::results_type results)
    {
        if (ec) {
            LOG_ERROR("resolver error: {}", ec.message());
            return;
        }
        beast::get_lowest_layer(*m_ws)
            .async_connect(results,
                           beast::bind_front_handler(&AgentWsClient::on_connect,
                                                     shared_from_this()));
    }

    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type endpoint)
    {
        if (ec) {
            LOG_ERROR("connect error: {}", ec.message());
            return;
        }
        beast::get_lowest_layer(*m_ws).expires_never();
        m_ws->set_option(websocket::stream_base::decorator([](websocket::request_type &req) {
            req.set(http::field::user_agent, "<ws>");
        }));
        m_host += ":" + std::to_string(endpoint.port());
        m_ws->next_layer().async_handshake(ssl::stream_base::client,
                                           beast::bind_front_handler(&AgentWsClient::on_tls_handshake,
                                                                     shared_from_this()));
    }

    void on_tls_handshake(beast::error_code ec)
    {
        if (ec) {
            LOG_ERROR("tls handshake error: {}", ec.message());
            return;
        }
        m_ws->set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
        m_ws->set_option(websocket::stream_base::decorator([](websocket::request_type &req) {
            req.set(http::field::user_agent, "voip-client");
        }));
        m_ws->async_handshake(m_host, m_target,
                              beast::bind_front_handler(&AgentWsClient::on_ws_handshake,
                                                        shared_from_this()));
    }

    void on_ws_handshake(beast::error_code ec)
    {
        Json::Value resp;
        Json::StreamWriterBuilder builder;
        if (ec) {
            LOG_ERROR("ws handshake error: {}", ec.message());
            return;
        }
        do_read();
    }

    void do_read()
    {
        m_ws->async_read(m_buffer,
                         net::bind_executor(
                             *m_strand,
                             beast::bind_front_handler(&AgentWsClient::on_read, shared_from_this())));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred)
    {
        if (ec == websocket::error::closed) {
            LOG_WARN("WebSocket closed by server");
            return;
        }
        if (ec) {
            LOG_ERROR("Read error: {}", ec.message());
            return;
        }

        std::string msg = beast::buffers_to_string(m_buffer.data());
        m_buffer.consume(bytes_transferred);
        LOG_INFO("recv Agent Ws Client {}", msg);

        Json::Value root;
        Json::CharReaderBuilder builder;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        std::string errors;

        if (reader->parse(msg.c_str(), msg.c_str() + msg.size(), &root, &errors)) {
            std::string is_final = root.get("is_final", "").asString();
            std::string text = root.get("text", "").asString();
            std::string mode = root.get("mode", "").asString();

            if (mode == "2pass-offline") {
                LOG_INFO("m_role: {}, mode:2pass-offline mode test: {}", m_role, text);
                process_asr_with_llm(text);
            }
        }
        else {
            LOG_ERROR("Failed to parse JSON: {}", errors);
        }

        do_read();
    }

    void process_asr_with_llm(const std::string &text)
    {

        m_llm_msg_text = text;
        LOG_INFO("m_call_method: {}, process_asr_with_llm: {}", m_call_method, text);
        if (m_call_method == "manual") {
            LOG_INFO("LLM manual start");
            if (m_role == "customer") {
                LOG_INFO("LLM manual_customer start");
                std::string response = m_llm_client->sendRequest(m_llm_msg_text, "manual", "customer", m_session_id, m_access_token);
                LOG_INFO("LLM manual_customer response: {}", response);
            }
            else if (m_role == "mediator") {
                LOG_INFO("LLM manual_mediator start");
                std::string response = m_llm_client->sendRequest(m_llm_msg_text, "manual", "mediator", m_session_id, m_access_token);
                LOG_INFO("LLM manual_mediator response: {}", response);
            }
        }
        else if (m_call_method == "agent") {
            TTSPlayer::getInstance()->stop(); // 停止播放
            m_llm_start_time = std::chrono::steady_clock::now();
            end_timeout_check();
            start_timeout_check(m_llm_start_time);

            std::string response = m_llm_client->sendRequest(m_llm_msg_text, m_call_method, m_role, m_session_id, m_access_token); // 发送ASR结果给LLM
            LOG_INFO("LLM agent response: {}", response);
            if (response.empty()) {
                do_read();
                return;
            }
            Json::Value response_json;
            Json::CharReaderBuilder response_builder;
            std::unique_ptr<Json::CharReader> response_reader(response_builder.newCharReader());
            std::string response_errors;

            if (response_reader->parse(response.c_str(), response.c_str() + response.size(), &response_json, &response_errors)) {
                if (response_json.isMember("data") && response_json["data"].isObject()) {
                    const auto &data = response_json["data"];
                    if (data.isMember("text") && data["text"].isArray()) {
                        m_llm_msg_list.clear();
                        for (const auto &item : data["text"]) {
                            m_llm_msg_list.push_back(item.asString());
                        }
                        LOG_INFO("LLM response data pushed to m_llm_msg_list, size: {}", m_llm_msg_list.size());

                        if (!m_llm_msg_list.empty()) {
                            TTSPlayer::getInstance()->resume();                   // 恢复播放
                            TTSPlayer::getInstance()->produceTTS(m_llm_msg_list); // 将LLM的文本转换为TTS的音频
                            m_llm_msg_list.clear();
                        }
                    }
                }
            }
        }
    }

    void start_timeout_check(std::chrono::steady_clock::time_point timeout_time)
    {
        m_llm_start_time = timeout_time;
        m_llm_timer_running = true;

        LOG_INFO("Starting new LLM timeout timer...");

        m_llm_timer_thread = std::thread([self = shared_from_this()]() {
            LOG_INFO("LLM timeout timer started ({}s)", LLM_TIMEOUT_SECONDS);

            // 等待超时时间或被手动中止
            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(LLM_TIMEOUT_SECONDS);
            while (self->m_llm_timer_running && std::chrono::steady_clock::now() < deadline) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            if (!self->m_llm_timer_running) {
                LOG_INFO("LLM timeout timer manually stopped.");
                return;
            }

            // 超时触发
            LOG_WARN("LLM timeout ({}s) reached, executing hangup_call()", LLM_TIMEOUT_SECONDS);
            self->clear_llm_msg_list();
            self->hangup_call();
        });
    }

    void end_timeout_check()
    {
        if (m_llm_timer_running) {
            LOG_INFO("Stopping LLM timeout timer...");
            m_llm_timer_running = false;
            if (m_llm_timer_thread.joinable()) {
                m_llm_timer_thread.join();
            }
            LOG_INFO("LLM timeout timer stopped.");
        }
    }

    static void hangup_call()
    {
        endpoint.libRegisterThread("Worker");
        endpoint.hangupAllCalls();
        LOG_INFO("Hangup all calls");
    }

    std::unique_ptr<tcp::resolver> m_resolver;
    std::unique_ptr<websocket::stream<beast::ssl_stream<beast::tcp_stream>>> m_ws;
    beast::flat_buffer m_buffer;
    std::string m_host;
    std::string m_port;
    std::string m_target;

    // std::shared_ptr<IWSSender> m_gui_server_sender;
    std::shared_ptr<LLMRequest> m_llm_client;

    std::string m_llm_msg_text;
    std::vector<std::string> m_llm_msg_list;
    std::chrono::steady_clock::time_point m_llm_start_time;
    static constexpr int LLM_TIMEOUT_SECONDS = 60;
    std::thread m_llm_timer_thread;
    std::atomic<bool> m_llm_timer_running {false};
    std::string m_session_id;
    std::string m_access_token;
    std::string m_role;
    std::string m_call_method;
    WebWsMsgHandler m_ws_msg_handler;
    std::optional<net::strand<net::io_context::executor_type>> m_strand;
    std::deque<std::string> m_send_queue;
    bool m_writing {false};
};

#endif // _AGENT_WS_CLIENT_H_