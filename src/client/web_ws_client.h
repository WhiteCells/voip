#pragma once

#include "../logger.h"
#include "../event/event.h"
#include "../event/msg.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/ssl.hpp>
#include <json/json.h>
#include <memory>
#include <string>

namespace beast = boost::beast;
// namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

class WebWsClient :
    public std::enable_shared_from_this<WebWsClient>
{
public:
    WebWsClient(net::io_context &ioc,
                const std::string &host,
                const std::string &port,
                const std::string &path,
                const std::string &ssl_cert_file)
        : m_ioc(ioc)
        , m_host(host)
        , m_port(port)
        , m_path(path)
    {
        m_ssl_ctx = std::make_shared<ssl::context>(ssl::context::tls_client);
        m_ssl_ctx->set_verify_mode(ssl::verify_peer);
        m_ssl_ctx->load_verify_file(ssl_cert_file);
    }

    ~WebWsClient() = default;

    void restart()
    {
        stop();
        start();
    }

    void start()
    {
        m_resolver = std::make_shared<tcp::resolver>(net::make_strand(m_ioc));
        m_ws = std::make_shared<websocket::stream<beast::ssl_stream<beast::tcp_stream>>>(net::make_strand(m_ioc), *m_ssl_ctx);
        m_resolver->async_resolve(m_host,
                                  m_port,
                                  beast::bind_front_handler(&WebWsClient::onResolve,
                                                            shared_from_this()));
    }

    void stop()
    {
        if (m_ws->is_open()) {
            beast::error_code ec;
            m_ws->close(websocket::close_code::normal, ec);
            if (ec) {
                LOG_ERROR("WebSocket Close Failed: {}", ec.message());
            }
            else {
                LOG_INFO("WebSocket Close Success");
            }
            m_ws.reset();
        }
        m_resolver->cancel();
    }

    void send(const std::string &msg)
    {
        m_ws->text(true);
        m_ws->async_write(net::buffer(msg),
                          [self = shared_from_this()](beast::error_code ec, std::size_t) {
                              if (ec) {
                                  LOG_ERROR("Web Ws Client Send Failed {}", ec.message());
                                  return;
                              }
                          });
    }

private:
    void onResolve(beast::error_code ec, tcp::resolver::results_type results)
    {
        if (ec) {
            LOG_ERROR("onResolve error: {}", ec.message());
            // 通知 GuiServer 连接失败
            // todo 错误类型通知
            Json::Value resp;
            resp["type"] = "backend_status";
            resp["status"] = "error";
            EventBus::getInstance()->publish(WebConnStateMsg {resp.toStyledString()});
            return;
        }
        LOG_INFO("onResolve success, endpoint: {}:{}",
                 results.begin()->endpoint().address().to_string(),
                 results.begin()->endpoint().port());
        beast::get_lowest_layer(*m_ws)
            .async_connect(results,
                           beast::bind_front_handler(&WebWsClient::onConnect,
                                                     shared_from_this()));
    }

    void onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type endpoint)
    {
        if (ec) {
            LOG_ERROR("onConnect error: {}", ec.message());
            // 通知 GuiServer 连接失败
            Json::Value resp;
            resp["type"] = "backend_status";
            resp["status"] = "error";
            EventBus::getInstance()->publish(WebConnStateMsg {resp.toStyledString()});
            return;
        }
        LOG_INFO("onConnect success, endpoint: {}:{}",
                 endpoint.address().to_string(),
                 endpoint.port());
        beast::get_lowest_layer(*m_ws).expires_never();
        m_ws->next_layer()
            .async_handshake(ssl::stream_base::client,
                             beast::bind_front_handler(&WebWsClient::onTLSHandshake,
                                                       shared_from_this()));
    }

    void onTLSHandshake(beast::error_code ec)
    {
        if (ec) {
            LOG_ERROR("onTLSHandshake error: {}", ec.message());
            // 通知 GuiServer 连接失败
            Json::Value resp;
            resp["type"] = "backend_status";
            resp["status"] = "error";
            EventBus::getInstance()->publish(WebConnStateMsg {resp.toStyledString()});
            return;
        }
        LOG_INFO("onTLSHandshake success");
        m_ws->async_handshake(m_host + ":" + m_port, m_path,
                              beast::bind_front_handler(&WebWsClient::onWsHandshake,
                                                        shared_from_this()));
    }

    void onWsHandshake(beast::error_code ec)
    {
        if (ec) {
            LOG_ERROR("onWsHandshake error: {}", ec.message());
            // 通知 GuiServer 连接失败
            Json::Value resp;
            resp["type"] = "backend_status";
            resp["status"] = "error";
            EventBus::getInstance()->publish(WebConnStateMsg {resp.toStyledString()});
            return;
        }
        LOG_INFO("onWsHandshake success");
        // 通知 GuiServer 连接成功
        Json::Value resp;
        resp["status"] = "connected";
        resp["type"] = "backend_status";
        EventBus::getInstance()->publish(WebConnStateMsg {resp.toStyledString()});
        doRead();
    }

    void doRead()
    {
        m_ws->async_read(m_buffer,
                         beast::bind_front_handler(&WebWsClient::onRead,
                                                   shared_from_this()));
    }

    void onRead(beast::error_code ec, std::size_t len)
    {
        if (ec) {
            LOG_ERROR("onRead error: {}", ec.message());
            // 通知 GuiServer 连接失败
            Json::Value resp;
            resp["type"] = "backend_status";
            resp["status"] = "error";
            EventBus::getInstance()->publish(WebConnStateMsg {resp.toStyledString()});
            return;
        }
        std::string msg = beast::buffers_to_string(m_buffer.data());
        m_buffer.consume(len);
        // LOG_INFO("onRead success, msg: {}", msg);
        // 通知 OutcomingCenter 收到消息
        EventBus::getInstance()->publish(OutcomingEvent {msg});
        doRead();
    }

private:
    net::io_context &m_ioc;
    std::shared_ptr<tcp::resolver> m_resolver;
    std::shared_ptr<websocket::stream<beast::ssl_stream<beast::tcp_stream>>> m_ws;
    std::shared_ptr<ssl::context> m_ssl_ctx;
    beast::flat_buffer m_buffer;
    std::string m_host;
    std::string m_port;
    std::string m_path;
};
