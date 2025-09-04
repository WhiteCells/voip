#ifndef _WS_SERVER_H_
#define _WS_SERVER_H_

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <json/json.h>
#include <string>
#include <mutex>

namespace net = boost::asio;
using tcp = net::ip::tcp;

class WsServer
{
private:
    tcp::acceptor m_acceptor;
    tcp::endpoint m_endpoint;
    std::mutex m_session_mtx;

public:
    WsServer(net::io_context &ioc,
             const std::string &addr,
             const unsigned short port);
    ~WsServer();

    void send(const std::string &id,
              const std::string &msg);

private:
    void do_accept();
};

#endif // _WS_SERVER_H_