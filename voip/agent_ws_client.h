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

    explicit AgentWsClient()
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

        do_read();
    }

    std::unique_ptr<tcp::resolver> m_resolver;
    std::unique_ptr<websocket::stream<beast::ssl_stream<beast::tcp_stream>>> m_ws;
    beast::flat_buffer m_buffer;
    std::string m_host;
    std::string m_port;
    std::string m_target;
    std::shared_ptr<IWSSender> m_gui_server_sender;

    WebWsMsgHandler m_ws_msg_handler;
};

#endif // _AGENT_WS_CLIENT_H_