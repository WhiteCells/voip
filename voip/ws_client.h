#ifndef _WS_CLIENT_H_
#define _WS_CLIENT_H_

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <functional>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;

using tcp = net::ip::tcp;

class WSClient :
    public std::enable_shared_from_this<WSClient>
{
private:
    tcp::resolver m_resolver;
    websocket::stream<beast::tcp_stream> m_ws;
    beast::flat_buffer m_buffer;
    std::string m_host;
    std::string m_port;
    std::string m_target;
    std::string m_text;
    std::function<void(const std::string &)> m_on_read_handler;

public:
    explicit WSClient(net::io_context &ioc) :
        m_resolver(net::make_strand(ioc)),
        m_ws(net::make_strand(ioc))
    {
    }

    void set_read_handler(std::function<void(const std::string &)> read_handler)
    {
        m_on_read_handler = read_handler;
    }

    void run(const char *host,
             const char *port,
             const char *target,
             const char *text)
    {
        m_host = host;
        m_port = port;
        m_target = target;
        m_text = text;

        m_resolver.async_resolve(host,
                                 port,
                                 beast::bind_front_handler(&WSClient::on_resolve,
                                                           shared_from_this()));
    }

private:
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results)
    {
        if (ec) {
            // err
            return;
        }
        beast::get_lowest_layer(m_ws)
            .async_connect(results,
                           beast::bind_front_handler(&WSClient::on_connect,
                                                     shared_from_this()));
    }

    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type endpoint)
    {
        if (ec) {
            // err
            return;
        }

        beast::get_lowest_layer(m_ws).expires_never();
        m_ws.set_option(websocket::stream_base::decorator([](websocket::request_type &req) {
            req.set(http::field::user_agent, "<ws>");
        }));
        m_host += ":" + std::to_string(endpoint.port());
        m_ws.async_handshake(m_host,
                             m_target,
                             beast::bind_front_handler(&WSClient::on_handshake,
                                                       shared_from_this()));
    }

    void on_handshake(beast::error_code ec)
    {
        if (ec) {
            // err
            return;
        }
        // read
        do_read();
    }

    void do_read()
    {
        m_ws.async_read(m_buffer,
                        beast::bind_front_handler(&WSClient::on_read,
                                                  shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t len)
    {
        if (ec) {
            // err
            return;
        }

        std::string msg = beast::buffers_to_string(m_buffer.data());
        m_buffer.consume(m_buffer.size());

        if (m_on_read_handler) {
            m_on_read_handler(msg);
        }

        do_read(); // continue
    }
};

#endif // _WS_CLIENT_H_