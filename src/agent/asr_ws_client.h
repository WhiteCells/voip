#pragma once

#include "../logger.h"
#include "../core/session_core.h"
#include "../event/event.h"
#include "../event/msg.h"
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <json/json.h>
#include <memory>
#include <deque>
#include <atomic>
#include <string>

namespace beast = boost::beast;
namespace net = boost::asio;
namespace websocket = beast::websocket;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

// FunASR WebSocket Client
// 提供给 AgentRobotAudioMediaPort 类使用
class ASRWsClient : public std::enable_shared_from_this<ASRWsClient>
{
public:
    // 每一个类需要向上提供参数
    ASRWsClient(net::io_context &ioc,
                const std::string &host,
                const std::string &port,
                const std::string &path,
                bool is_ssl = true,
                const std::string &ssl_cert = "")
        : m_ioc(ioc)
        , m_host(host)
        , m_port(port)
        , m_path(path)
        , m_ssl_ctx(ssl::context::tls_client)
        , m_is_ssl(is_ssl)
        , m_ssl_cert(ssl_cert)
    {
        LOG_INFO(">>> construct {}", __func__);
        m_ssl_ctx.set_verify_mode(ssl::verify_none); // todo
        m_ssl_ctx.load_verify_file(m_ssl_cert);
        LOG_INFO("<<< construct {}", __func__);
    }

    ~ASRWsClient()
    {
        LOG_INFO(">>> destruct {}", __func__);
        LOG_INFO("<<< destruct {}", __func__);
    }

    void start()
    {
        LOG_INFO("FunASR WebSocket Client Start");
        m_strand.emplace(m_ioc.get_executor());
        m_resolver = std::make_unique<tcp::resolver>(m_ioc);
        m_ws = std::make_unique<websocket::stream<beast::ssl_stream<beast::tcp_stream>>>(*m_strand, m_ssl_ctx);
        m_resolver->async_resolve(m_host, m_port,
                                  beast::bind_front_handler(&ASRWsClient::onResolve, shared_from_this()));
    }

    void stop()
    {
        LOG_INFO("FunASR WebSocket Client Stop");
        net::dispatch(*m_strand, [self = shared_from_this()]() {
            if (self->m_ws && self->m_ws->is_open()) {
                beast::error_code ec;
                self->m_ws->close(websocket::close_code::normal, ec);
                if (ec) {
                    // logger
                }
            }
            self->m_send_queue.clear();
            self->m_writing = false;
        });
    }

    // 发送消息(文本、二进制)
    // 消息由上层调用，加入队列
    void send(const std::string &msg, bool is_binary = false)
    {
        if (!m_ws || !m_ws->is_open()) {
            LOG_ERROR("FunASR WebSocket Client Send: WebSocket is not open");
            return;
        }
        // LOG_INFO("FunASR WebSocket Client Send: {}", msg);
        net::dispatch(*m_strand, [self = shared_from_this(), msg, is_binary]() {
            auto prefix = is_binary ? "B:" : "T:";
            self->m_send_queue.push_back(prefix + msg);
            if (!self->m_writing) {
                self->m_writing = true;
                self->doWrite();
            }
        });
    }

private:
    // using PlainWs = websocket::stream<beast::tcp_stream>;
    // using SslWs = websocket::stream<beast::ssl_stream<beast::tcp_stream>>;

    void onResolve(beast::error_code ec, tcp::resolver::results_type results)
    {
        if (ec) {
            LOG_ERROR("FunASR WebSocket Client Resolve: {}", ec.message());
            return;
        }
        beast::get_lowest_layer(*m_ws).async_connect(results,
                                                     beast::bind_front_handler(&ASRWsClient::onConnect,
                                                                               shared_from_this()));
    }

    void onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type endpoint)
    {
        if (ec) {
            LOG_ERROR("FunASR WebSocket Client Connect: {}", ec.message());
            return;
        }
        LOG_INFO("FunASR WebSocket Client Connect: {}:{}", endpoint.address().to_string(), endpoint.port());
        beast::get_lowest_layer(*m_ws).expires_never();
        m_ws->next_layer().async_handshake(ssl::stream_base::client,
                                           beast::bind_front_handler(&ASRWsClient::onTLSHandshake,
                                                                     shared_from_this()));
    }

    void onTLSHandshake(beast::error_code ec)
    {
        if (ec) {
            LOG_ERROR("FunASR WebSocket Client TLS Handshake: {}", ec.message());
            return;
        }
        m_ws->async_handshake(m_host + ":" + m_port,
                              m_path,
                              beast::bind_front_handler(&ASRWsClient::onWsHandshake,
                                                        shared_from_this()));
    }

    void onWsHandshake(beast::error_code ec)
    {
        if (ec) {
            LOG_ERROR("FunASR WebSocket Client WebSocket Handshake: {}", ec.message());
            return;
        }
        sendStartConfig();
        doRead();
    }

    void sendStartConfig()
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
        chunk_size.append(0);
        chunk_size.append(6);
        chunk_size.append(3);
        config["chunk_size"] = chunk_size;
        send(config.toStyledString());
    }

    void doRead()
    {
        m_ws->async_read(m_buffer,
                         beast::bind_front_handler(&ASRWsClient::onRead, shared_from_this()));
    }

    void onRead(beast::error_code ec, std::size_t bytes_transferred)
    {
        if (ec == websocket::error::closed) {
            LOG_ERROR("FunASR WebSocket Client Read: {}", ec.message());
            return;
        }
        if (ec) {
            LOG_ERROR("FunASR WebSocket Client Read: {}", ec.message());
            return;
        }
        std::string data = beast::buffers_to_string(m_buffer.data());
        m_buffer.consume(bytes_transferred);

        Json::Value root;
        Json::CharReaderBuilder builder;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        std::string parse_error;

        if (!reader->parse(data.c_str(), data.c_str() + data.size(), &root, &parse_error)) {
            LOG_ERROR("FunASR WebSocket Client Read: JSON Parse Error: {}", parse_error);
            doRead();
            return;
        }

        std::string text = root["text"].asString();
        std::string mode = root["mode"].asString();
        LOG_INFO("FunASR WebSocket Client mode: {}, text: {}", mode, text);

        if (mode == m_offline_mode) {
            if (m_vad_flag) {
                auto call_method = SessionCore::getInstance()->getCallMethod();
                LOG_INFO("FunASR WebSocket Client Read: call_method: {}", call_method);
                if (call_method == "manual") {
                    // 通知 SessionHttpClient 话术提醒文本结果
                    // EventBus::getInstance()->publish(ASRTextMsg {call_method, text});
                }
                else {
                    // 通知 LLMHttpClient 智能客服文本结果
                    LOG_INFO("FunASR WebSocket Client Read: VAD True, text: {}", text);
                    EventBus::getInstance()->publish(ASRTextMsg {call_method, text});
                }
            }
            else {
                LOG_INFO("FunASR WebSocket Client Read: VAD False, text: {}", text);
            }
            m_vad_flag = false;
            m_first_flag = true;
        }
        else if (mode == m_online_mode) {
            if (m_first_flag) {
                m_first_flag = false;
            }
            else {
                m_vad_flag = true;
            }
        }
        else {
            LOG_ERROR("FunASR WebSocket Client Read: Unknown mode: {}", mode);
        }

        doRead();
    }

    void doWrite()
    {
        if (m_send_queue.empty()) {
            m_writing = false;
            return;
        }
        m_writing = true;
        auto msg = std::move(m_send_queue.front());
        m_send_queue.pop_front();
        bool is_binary = msg.starts_with("B:");
        msg.erase(0, 2); // remove prefix
        if (is_binary) {
            m_ws->binary(true);
        }
        else {
            m_ws->text(true);
        }
        m_ws->async_write(net::buffer(msg),
                          beast::bind_front_handler(&ASRWsClient::onWrite, shared_from_this()));
    }

    void onWrite(beast::error_code ec, std::size_t bytes_transferred)
    {
        if (ec) {
            LOG_ERROR("FunASR WebSocket Client Write: {}", ec.message());
            return;
        }
        // LOG_INFO("FunASR WebSocket Client Write: {} bytes", bytes_transferred);
        doWrite();
    }

private:
    net::io_context &m_ioc;
    std::unique_ptr<tcp::resolver> m_resolver;
    std::unique_ptr<websocket::stream<beast::ssl_stream<beast::tcp_stream>>> m_ws;
    std::optional<net::strand<net::io_context::executor_type>> m_strand;
    std::deque<std::string> m_send_queue;
    beast::flat_buffer m_buffer;
    std::atomic<bool> m_writing {false};
    bool m_vad_flag {false};
    bool m_first_flag {true};
    const std::string m_host;
    const std::string m_port;
    const std::string m_path;
    ssl::context m_ssl_ctx;
    const bool m_is_ssl;
    const std::string m_ssl_cert;
    const std::string m_online_mode = "2pass-online";
    const std::string m_offline_mode = "2pass-offline";
};