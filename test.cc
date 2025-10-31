#ifndef _AGENT_WS_CLIENT_H_
#define _AGENT_WS_CLIENT_H_

#include "global.h"
#include "logger.h"
#include "io_context_pool.h"
#include "ws_interface.h"
#include "llm_request.h"
#include "tts_request.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <json/json.h>

#include <deque>
#include <thread>
#include <atomic>
#include <optional>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

class AgentWsClient : public std::enable_shared_from_this<AgentWsClient> {
public:
    using WebWsMsgHandler = std::function<void(const std::string &)>;

    AgentWsClient(const std::string &host, const std::string &port)
        : m_host(host)
        , m_port(port)
        , m_target("/")
        , m_ssl_ctx(ssl::context::tls_client)
        , m_llm_client(std::make_shared<LLMRequest>(
              agent_session_remote_host, agent_session_remote_port, agent_session_remote_target)) {
        m_ssl_ctx.set_verify_mode(ssl::verify_peer);
        m_ssl_ctx.load_verify_file(asr_server_verify_file);
    }

    ~AgentWsClient() {
        stop();
    }

    void start() {
        auto &ioc = IOContextPool::getInstance()->getIOContext();
        m_strand.emplace(net::make_strand(ioc));

        m_resolver = std::make_unique<tcp::resolver>(*m_strand);
        m_ws = std::make_unique<websocket::stream<beast::ssl_stream<beast::tcp_stream>>>(*m_strand, m_ssl_ctx);

        LOG_INFO("Resolving {}:{}", m_host, m_port);
        m_resolver->async_resolve(
            m_host, m_port,
            beast::bind_front_handler(&AgentWsClient::on_resolve, shared_from_this()));
    }

    void stop() {
        net::dispatch(*m_strand, [self = shared_from_this()]() {
            if (self->m_ws && self->m_ws->is_open()) {
                beast::error_code ec;
                self->m_ws->close(websocket::close_code::normal, ec);
                if (ec) {
                    LOG_ERROR("Close failed: {}", ec.message());
                } else {
                    LOG_INFO("WebSocket closed cleanly");
                }
            }
            self->m_send_queue.clear();
            self->m_writing = false;
        });
    }

    void sendBinary(const std::string &data, const std::string &role) {
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

private:
    void do_write() {
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
                    } else {
                        self->m_writing = false;
                    }
                }));
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec) {
            LOG_ERROR("Resolve error: {}", ec.message());
            return;
        }

        beast::get_lowest_layer(*m_ws).async_connect(
            results,
            beast::bind_front_handler(&AgentWsClient::on_connect, shared_from_this()));
    }

    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type endpoint) {
        if (ec) {
            LOG_ERROR("Connect error: {}", ec.message());
            return;
        }

        LOG_INFO("Connected to {}", endpoint);
        m_ws->next_layer().async_handshake(
            ssl::stream_base::client,
            beast::bind_front_handler(&AgentWsClient::on_tls_handshake, shared_from_this()));
    }

    void on_tls_handshake(beast::error_code ec) {
        if (ec) {
            LOG_ERROR("TLS handshake error: {}", ec.message());
            return;
        }

        m_ws->set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
        m_ws->async_handshake(
            m_host, m_target,
            beast::bind_front_handler(&AgentWsClient::on_ws_handshake, shared_from_this()));
    }

    void on_ws_handshake(beast::error_code ec) {
        if (ec) {
            LOG_ERROR("WebSocket handshake failed: {}", ec.message());
            return;
        }

        LOG_INFO("WebSocket handshake successful.");
        do_read();
    }

    void do_read() {
        m_ws->async_read(
            m_buffer,
            net::bind_executor(
                *m_strand,
                beast::bind_front_handler(&AgentWsClient::on_read, shared_from_this())));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
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

        LOG_INFO("Received message: {}", msg);

        // TODO: handle message...

        do_read();  // continue reading
    }

private:
    std::unique_ptr<tcp::resolver> m_resolver;
    std::unique_ptr<websocket::stream<beast::ssl_stream<beast::tcp_stream>>> m_ws;
    ssl::context m_ssl_ctx;
    beast::flat_buffer m_buffer;

    std::string m_host, m_port, m_target;
    std::string m_role;

    std::optional<net::strand<net::io_context::executor_type>> m_strand;
    std::deque<std::string> m_send_queue;
    bool m_writing{false};

    std::shared_ptr<LLMRequest> m_llm_client;
};

#endif
