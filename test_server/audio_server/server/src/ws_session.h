#ifndef _WS_SESSION_H_
#define _WS_SESSION_H_

#include "types.h"
#include "logger.h"
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <json/json.h>
#include <queue>
#include <memory>
#include <functional>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class WsSession :
    public std::enable_shared_from_this<WsSession>
{
public:
    // enum Type {
    //     kVoip,
    //     kRobot,
    // };
    // enum Status {
    //     kUsed,
    //     kFree,
    //     kNone,
    // };
    using Sptr = std::shared_ptr<WsSession>;
    // using Type = Type;
    // using Status = Status;

private:
    websocket::stream<beast::tcp_stream> m_stream;
    beast::flat_buffer m_buffer;
    http::request<http::string_body> m_req;
    std::queue<std::string> m_write_que;
    std::function<void(WsSessionType, const std::string &)> m_on_ready;
    WsSessionType m_type;
    std::string m_id;

public:
    explicit WsSession(tcp::socket &&socket) :
        m_stream(std::move(socket))
    {
    }
    ~WsSession()
    {
    }

    void run()
    {
        beast::get_lowest_layer(m_stream).expires_never();
        http::async_read(m_stream.next_layer(),
                         m_buffer, m_req,
                         beast::bind_front_handler(&WsSession::on_read_http,
                                                   shared_from_this()));
    }

    void send(const std::string &msg)
    {
        net::post(m_stream.get_executor(),
                  [self = shared_from_this(), msg]() {
                      bool write_processing = !self->m_write_que.empty();
                      self->m_write_que.push(std::move(msg));
                      if (!write_processing) {
                          self->do_write();
                      }
                  });
    }

    WsSessionType getType() const
    {
        return m_type;
    }

    std::string getId() const
    {
        return m_id;
    }

    void set_on_ready(std::function<void(WsSessionType, const WsSessionId &)> &&func)
    {
        m_on_ready = func;
    }

private:
    void on_read_http(beast::error_code ec, std::size_t)
    {
        if (ec) {
            LOG_ERROR("on_read_http: {}", ec.what());
            return;
        }

        // /<type>?id=
        std::string target = m_req.target();
        if (target.rfind("/voip", 0) == 0) {
            m_type = kVoip;
        }
        else if (target.rfind("/robot", 0) == 0) {
            m_type = kRobot;
        }
        else {
            LOG_ERROR("rfind");
            m_stream.next_layer().socket().shutdown(tcp::socket::shutdown_both);
            return;
        }

        std::string id;
        auto pos = target.find("id=");
        if (pos != std::string::npos) {
            id = target.substr(pos + 3);
        }
        m_id = id;

        m_stream.async_accept(m_req,
                              beast::bind_front_handler(&WsSession::on_accept,
                                                        shared_from_this()));
    }

    void on_accept(beast::error_code ec)
    {
        if (ec) {
            LOG_ERROR("on_accept");
            return;
        }
        if (m_on_ready) {
            m_on_ready(m_type, m_id);
        }
        do_read();
    }

    void do_read()
    {
        m_stream.async_read(m_buffer,
                            beast::bind_front_handler(&WsSession::on_read_ws,
                                                      shared_from_this()));
    }

    void on_read_ws(beast::error_code ec, std::size_t bytes);

    // void on_read_ws(beast::error_code ec, std::size_t bytes)
    // {
    //     if (ec == websocket::error::closed) {
    //         LOG_ERROR("on_read_ws: {} {}", (int)m_type, m_id);
    //         WsSessionMgr::leave_robot_session();
    //         return;
    //     }
    //     if (ec) {
    //         return;
    //     }
    //     std::string msg = beast::buffers_to_string(m_buffer.data());
    //     m_buffer.consume(bytes);
    //     // msg
    //     do_read();
    // }

    void do_write()
    {
        m_stream.text(true);
        m_stream.async_write(net::buffer(m_write_que.front()),
                             [self = shared_from_this()](beast::error_code ec, std::size_t) {
                                 if (ec) {
                                     return;
                                 }
                                 self->m_write_que.pop();
                                 if (!self->m_write_que.empty()) {
                                     self->do_write();
                                 }
                             });
    }
};

#endif // _WS_SESSION_H_