#ifndef _WS_CLIENT_H_
#define _WS_CLIENT_H_

#include "global.h"
#include "io_context_pool.h"
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class WsClient :
    public std::enable_shared_from_this<WsClient>
{
private:
    std::unique_ptr<tcp::resolver> m_resolver;
    std::unique_ptr<websocket::stream<beast::tcp_stream>> m_ws;
    beast::flat_buffer m_buffer;

public:
    WsClient();

    void start_ws_client()
    {
        auto &ioc = IOContextPool::getInstance()->getIOContext();
        m_resolver = std::make_unique<tcp::resolver>(net::make_strand(ioc));
        m_ws = std::make_unique<websocket::stream<beast::tcp_stream>>(net::make_strand(ioc));

        m_resolver->async_resolve(backend_host,
                                  backend_port,
                                  beast::bind_front_handler(&WsClient::on_resolver, shared_from_this()));
    }

    void on_resolver()
    {
    }
};

#endif // _WS_CLIENT_H_