#include "server.h"
#include "io_context_pool.h"

Server::Server(asio::io_context &ioc, unsigned short port) :
    m_acceptor(ioc, tcp::endpoint(tcp::v4(), port))
{
}

Server::~Server()
{
}

void Server::start()
{
    std::shared_ptr<Server> self = shared_from_this();
    asio::io_context &io_context = IOContextPool::getInstance()->getIOContext();
    std::shared_ptr<
}