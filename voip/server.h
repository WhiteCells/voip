#ifndef _SERVER_H_
#define _SERVER_H_

#include <memory>
#include <boost/asio.hpp>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class Server :
    public std::enable_shared_from_this<Server>
{
public:
    Server(asio::io_context &ioc, unsigned short port);
    ~Server();

    void start();

private:
    tcp::acceptor m_acceptor;
};

#endif // _SERVER_H_