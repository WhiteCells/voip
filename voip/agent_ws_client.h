#ifndef _AGENT_WS_CLIENT_H_
#define _AGENT_WS_CLIENT_H_

#include "global.h"
#include "logger.h"
#include "io_context_pool.h"
#include "ws_interface.h"
#include <json/json.h>
#include <boost/beast/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <memory>
#include <functional>
#include <vector>
#include "llm_request.h"
#include "tts_request.h"

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

    explicit AgentWsClient(const std::string& host, const std::string& port)
            : m_host(host), m_port(port), m_target("/"),
              m_llm_client(std::make_unique<LLMRequest>("127.0.0.1", "50000", "/api/llm_request")),
              m_last_final_time(std::chrono::steady_clock::now())
    {
    }
    explicit AgentWsClient() = default;
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
        m_gui_server_sender = sender;
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
        m_resolver = std::make_unique<tcp::resolver>(net::make_strand(ioc));
        ssl::context ssl_ctx(ssl::context::tls_client);
        ssl_ctx.set_verify_mode(ssl::verify_peer);
        ssl_ctx.load_verify_file(verify_file);
        // ssl_ctx.set_verify_mode(ssl::verify_none); // 禁用SSL证书验证
        m_ws = std::make_unique<websocket::stream<beast::ssl_stream<beast::tcp_stream>>>(net::make_strand(ioc), ssl_ctx);
        m_timer = std::make_unique<net::steady_timer>(ioc);
        m_resolver->async_resolve(m_host, m_port,
                                  beast::bind_front_handler(&AgentWsClient::on_resolver,
                                                            shared_from_this()));
    }

    void stop()
    {
        if (m_ws->is_open()) {
            beast::error_code ec;
            m_ws->close(websocket::close_code::normal, ec);
            if (ec) {
                LOG_ERROR("Agent Ws Client Close Failed: {}", ec.message());
            }
            else {
                LOG_INFO("Agent Ws Client Close Successfully");
            }
            m_ws.reset();
        }
        m_resolver->cancel();
    }

    void start_config_send(){
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

    void end_config_send(){
        Json::Value config;
        config["is_speaking"] = "false";

        Json::StreamWriterBuilder builder;
        std::string config_str = Json::writeString(builder, config);
        send(config_str);
        LOG_INFO("success send asr end config");
    }

    void sendBinary(const std::string& data) {
//        LOG_INFO("Sending {} bytes of PCM data", data.size());
        if (m_ws && m_ws->is_open()) {
            // 设置为二进制模式
            m_ws->binary(true);
            // 异步发送二进制数据
            m_ws->async_write(
                    asio::buffer(data),
                    [self = shared_from_this()](beast::error_code ec, std::size_t) {
                        if (ec) {
                            LOG_ERROR("Agent Ws Client Send Binary Failed {}", ec.message());
                        } else {
//                            LOG_INFO("Successfully sent PCM data");
                        }
                    });
        } else {
            LOG_WARN("WebSocket connection is not open, cannot send binary data");
        }
    }

    std::vector<std::string> getLLMMessageList() {
        LOG_INFO("Get LLM Message msg_List {}", m_llm_msg_list.size());
        std::vector<std::string> temp_list = m_llm_msg_list;
        m_llm_msg_list.clear();
        LOG_INFO("Get LLM Message List {}", temp_list.size());
        return temp_list;
    }

private:
    void on_resolver(beast::error_code ec, tcp::resolver::results_type results)
    {
        if (ec) {
            Json::Value resp;
            // resp[""];
            Json::StreamWriterBuilder builder;
            std::string resp_str = Json::writeString(builder, resp);
            m_gui_server_sender->send(resp_str);
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
            Json::Value resp;
            // resp[""];
            Json::StreamWriterBuilder builder;
            std::string resp_str = Json::writeString(builder, resp);
            m_gui_server_sender->send(resp_str);
            return;
        }
        beast::get_lowest_layer(*m_ws).expires_never();
        m_ws->set_option(websocket::stream_base::decorator([](websocket::request_type &req) {
            req.set(http::field::user_agent, "<ws>");
        }));
        m_host += ":" + std::to_string(endpoint.port());
        // m_target;
        m_ws->next_layer().async_handshake(ssl::stream_base::client,
                                           beast::bind_front_handler(&AgentWsClient::on_tls_handshake,
                                                                     shared_from_this()));
    }

    void on_tls_handshake(beast::error_code ec)
    {
        if (ec) {
            Json::Value resp;
            //
            Json::StreamWriterBuilder builder;
            std::string resp_str = Json::writeString(builder, resp);
            m_gui_server_sender->send(resp_str);
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
            //
            std::string resp_str = Json::writeString(builder, resp);
            m_gui_server_sender->send(resp_str);
            return;
        }
        //
        std::string resp_str = Json::writeString(builder, resp);
        m_gui_server_sender->send(resp_str);

        do_read();
    }

    void do_read()
    {
        m_ws->async_read(m_buffer,
                         beast::bind_front_handler(&AgentWsClient::on_read,
                                                   shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t)
    {
        if (ec) {
            Json::Value resp;
            Json::StreamWriterBuilder builder;
            std::string resp_str = Json::writeString(builder, resp);
            m_gui_server_sender->send(resp_str);
            return;
        }

        std::string msg = beast::buffers_to_string(m_buffer.data());
        m_buffer.consume(m_buffer.size());
        LOG_INFO("recv Agent Ws Client {}", msg);

        Json::Value root;
        Json::CharReaderBuilder builder;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        std::string errors;

        if (reader->parse(msg.c_str(), msg.c_str() + msg.size(), &root, &errors)) {
            std::string is_final = root.get("is_final", "").asString();
            std::string text = root.get("text", "").asString();

            if (is_final == "false") {
                m_llm_msg_text += text;
                LOG_INFO("LLM text accumulating: {}", m_llm_msg_text);

                reset_timer();

                m_last_final_time = std::chrono::steady_clock::now();

            } else {
                LOG_INFO("Intermediate recognition result: {}", text);
            }
        } else {
            LOG_ERROR("Failed to parse JSON: {}", errors);
        }

        do_read();
    }

    void reset_timer() {
        if (!m_timer) return;
        m_timer->expires_after(std::chrono::seconds(TIMEOUT_SECONDS));

        m_timer->async_wait([self = shared_from_this()](beast::error_code ec) {
            if (ec) return; // 被取消或关闭
            if (!self->m_llm_msg_text.empty()) {
                LOG_INFO("Timeout reached ({}s), sending LLM request...", TIMEOUT_SECONDS);
                std::string response = self->m_llm_client->sendRequest(self->m_llm_msg_text);

                Json::Value response_json;
                Json::CharReaderBuilder builder;
                std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                std::string errors;
                if (reader->parse(response.c_str(), response.c_str() + response.size(), &response_json, &errors)) {
                    if (response_json.isMember("data") && response_json["data"].isArray()) {
                        for (const auto& item : response_json["data"]) {
                            self->m_llm_msg_list.push_back(item.asString());
                            TTSPlayer::getInstance()->produceTTS(self->m_llm_msg_list);
                        }
                        self->m_llm_msg_list.clear();
                    }
                } else {
                    LOG_ERROR("Failed to parse LLM response JSON: {}", errors);
                }

                self->m_llm_msg_text.clear();
            }

            // 重新启动检测
            self->reset_timer();
        });
    }


    std::unique_ptr<tcp::resolver> m_resolver;
    std::unique_ptr<websocket::stream<beast::ssl_stream<beast::tcp_stream>>> m_ws;
    beast::flat_buffer m_buffer;
    std::string m_host;
    std::string m_port;
    std::string m_target;
    std::shared_ptr<IWSSender> m_gui_server_sender;
    std::shared_ptr<LLMRequest> m_llm_client;
    std::string m_llm_msg_text;
    std::chrono::steady_clock::time_point m_last_final_time;
    std::vector<std::string> m_llm_msg_list;
    std::unique_ptr<net::steady_timer> m_timer;
    static constexpr int TIMEOUT_SECONDS = 2;

    WebWsMsgHandler m_ws_msg_handler;
};

#endif // _AGENT_WS_CLIENT_H_