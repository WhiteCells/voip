#ifndef _WS_SERVER_H_
#define _WS_SERVER_H_

#include "logger.h"
#include "io_context_pool.h"
#include "global.h"
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <json/json.h>
#include <queue>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class WebSocketSession :
    public std::enable_shared_from_this<WebSocketSession>
{
public:
    explicit WebSocketSession(tcp::socket &socket) :
        m_stream(std::move(socket))
    {
    }
    ~WebSocketSession() = default;

    void run()
    {
        beast::get_lowest_layer(m_stream).expires_after(std::chrono::seconds(3));
        http::async_read(m_stream.next_layer(),
                         m_buffer, m_req,
                         beast::bind_front_handler(&WebSocketSession::on_read_http,
                                                   shared_from_this()));
    }

    void send(std::string message)
    {
        boost::asio::post(m_stream.get_executor(),
                          [self = shared_from_this(), msg = std::move(message)]() {
                              bool write_processing = !self->m_write_que.empty();
                              self->m_write_que.push(std::move(msg));
                              if (!write_processing) {
                                  self->do_write();
                              }
                          });
    }

private:
    void on_read_http(beast::error_code ec, std::size_t)
    {
        if (ec) {
            return;
        }
        if (m_req.target() != "/ws") {
            return;
        }
        m_stream.async_accept(m_req,
                              beast::bind_front_handler(&WebSocketSession::on_accept,
                                                        shared_from_this()));
    }

    void on_accept(beast::error_code ec)
    {
        if (ec) {
            return;
        }
        do_read();
    }

    void do_read()
    {
        m_stream.async_read(m_buffer,
                            beast::bind_front_handler(&WebSocketSession::on_read_ws,
                                                      shared_from_this()));
    }

    void on_read_ws(beast::error_code ec, std::size_t bytes)
    {
        if (ec == websocket::error::closed) {
            return;
        }
        if (ec) {
            return;
        }

        std::string msg = beast::buffers_to_string(m_buffer.data());
        // on_read_ws_handler

        // config
        //  host
        //  port
        //  url
        //  client_id
        // command
        //  hangup
        endpoint.hangupAllCalls();
    }

    void do_write()
    {
        m_stream.text(true);
        m_stream.async_write(boost::asio::buffer(m_write_que.front()),
                             [self = shared_from_this()](beast::error_code ec, std::size_t) {
                                 if (ec) {
                                     return;
                                 }
                                 self->m_write_que.pop();
                                 if (!self->m_write_que.empty()) {
                                     self->do_write(); // continue
                                 }
                             });
    }

private:
    websocket::stream<beast::tcp_stream> m_stream;
    beast::flat_buffer m_buffer;
    http::request<http::string_body> m_req;
    std::queue<std::string> m_write_que;
};

class WSServer
{
public:
    WSServer(boost::asio::io_context &ioc, tcp::endpoint endpoint)
    {
    }

private:
    std::shared_ptr<WebSocketSession> m_seesion;
};

#endif // _WS_SERVER_H_