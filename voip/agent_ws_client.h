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
    }

    void end_config_send(){
        Json::Value config;
        config["is_speaking"] = "false";

        Json::StreamWriterBuilder builder;
        std::string config_str = Json::writeString(builder, config);
        send(config_str);
    }

    void AgentWsClient::sendBinary(const std::string& data) {
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
            //
            Json::StreamWriterBuilder builder;
            std::string resp_str = Json::writeString(builder, resp);
            m_gui_server_sender->send(resp_str);
            return;
        }
        std::string msg = beast::buffers_to_string(m_buffer.data());
        m_buffer.consume(m_buffer.size());

        LOG_INFO("recv Agent Ws Client {}", msg);

        // handler msg
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        std::string errors;

        if (reader->parse(msg.c_str(), msg.c_str() + msg.size(), &root, &errors)) {
            std::string is_final = root.get("is_final", "").asString();
            std::string text = root.get("text", "").asString();
            std::string mode = root.get("mode", "").asString();

            if (is_final == "false") {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_last_final_time).count();

                if (elapsed >= TIMEOUT_SECONDS) {
                    if(m_llm_msg_text.size() > 0) {
                        std::string response = m_llm_client->sendRequest(m_llm_msg_text); // 请求 llm 服务端
                        LOG_INFO("LLM response: {}", response);

                        Json::Value response_json;
                        Json::CharReaderBuilder response_builder;
                        std::unique_ptr<Json::CharReader> response_reader(response_builder.newCharReader());
                        std::string response_errors;

                        if (response_reader->parse(response.c_str(), response.c_str() + response.size(), &response_json, &response_errors)) {
                            if (response_json.isMember("data") && response_json["data"].isArray()) {
                                // 将data数组中的每个元素作为字符串推入m_llm_msg_list
                                for (const auto& item : response_json["data"]) {
                                    m_llm_msg_list.push_back(item.asString());
                                    TTSPlayer::getInstance()->produceTTS(m_llm_msg_list);
                                }
                                LOG_INFO("LLM response data pushed to m_llm_msg_list {}", m_llm_msg_list.size());
                                m_llm_msg_list.clear();
                            }
                        } else {
                            LOG_ERROR("Failed to parse LLM response JSON: {}", errors);
                        }
                    }
                    m_llm_msg_text.clear();  // 超时则清空累积文本
                }

                LOG_INFO("Final recognition result: {}", text);
                m_llm_msg_text += text;
                std::string msg_text = m_llm_msg_text;
                LOG_INFO("LLM text {}", msg_text);

                m_last_final_time = std::chrono::steady_clock::now();
            } else {
                LOG_INFO("Intermediate recognition result: {}", text);
                // 处理中间识别结果
            }
        } else {
            LOG_ERROR("Failed to parse JSON: {}", errors);
        }
        do_read();
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
    static constexpr int TIMEOUT_SECONDS = 2;

    WebWsMsgHandler m_ws_msg_handler;
};

#endif // _AGENT_WS_CLIENT_H_