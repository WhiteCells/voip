#include "server.h"

#include "http.h"
#include "io_context_pool.h"
#include <iostream>

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
    asio::io_context &ioc = IOContextPool::getInstance()->getIOContext();
    std::shared_ptr<Http> conn = std::make_shared<Http>(ioc);
    m_acceptor.async_accept(
        conn->getSocket(),
        [self, conn](beast::error_code ec) {
            try {
                if (ec) {
                    self->start();
                    return;
                }
                conn->start();
                self->start();
            }
            catch (const std::exception &e) {
                std::cerr << "[Exception]: " << e.what() << std::endl;
                self->start();
            }
        });
}