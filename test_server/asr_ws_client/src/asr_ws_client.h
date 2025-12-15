#pragma once

#include "logger.h"
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
        , m_is_ssl(is_ssl)
        , m_ssl_cert(ssl_cert)
    {
        LOG_INFO(">>> construct {}", __func__);
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
        m_strand.emplace(net::make_strand(m_ioc));
        m_resolver = std::make_unique<tcp::resolver>(*m_strand);
        ssl::context ssl_ctx(ssl::context::tls_client);
        ssl_ctx.set_verify_mode(ssl::verify_none); // todo
        ssl_ctx.load_verify_file(m_ssl_cert);
        m_ws = std::make_unique<websocket::stream<beast::ssl_stream<beast::tcp_stream>>>(*m_strand, ssl_ctx);
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
        m_ws->async_handshake(m_host + m_port,
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
        doRead();
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
        LOG_INFO("FunASR WebSocket Client Read: {}", text);

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
    // bool m_vad_flag {false};
    // bool m_first_flag {true};
    const std::string m_host;
    const std::string m_port;
    const std::string m_path;
    const bool m_is_ssl;
    const std::string m_ssl_cert;

public:
    static std::string s_call_method;
    static std::string s_session_id;
    static std::string s_access_token;
};