#pragma once

#include "../logger.h"
#include "../io_context_pool.h"
#include "../client/web_ws_client.h"
#include "ws_session.h"
// #include "ws_interface.h"
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <json/json.h>
#include <unordered_set>
#include <string>

namespace net = boost::asio;

class GuiWsServer
{
public:
    GuiWsServer(net::io_context &ioc, const std::string &addr,
                unsigned int port)
        : m_acceptor(ioc)
        , m_endpoint(net::ip::make_address(addr), port)
    {
        beast::error_code ec;
        if (m_acceptor.open(m_endpoint.protocol(), ec)) {
            LOG_ERROR("open: {}", ec.what());
            return;
        }
        if (m_acceptor.set_option(net::socket_base::reuse_address(true), ec)) {
            LOG_ERROR("set_option: {}", ec.what());
            return;
        }
        if (m_acceptor.bind(m_endpoint, ec)) {
            LOG_ERROR("bind: {}", ec.what());
            return;
        }
        if (m_acceptor.listen(net::socket_base::max_listen_connections, ec)) {
            LOG_ERROR("listen: {}", ec.what());
            return;
        }
        do_accept();
    }

private:
    void do_accept()
    {
        m_acceptor.async_accept([this](beast::error_code ec, tcp::socket socket) {
            if (ec) {
                LOG_ERROR("async_accept: {}", ec.what());
            }
            auto session = std::make_shared<WebSocketSession>(std::move(socket));
            {
                std::unique_lock<std::mutex> lock(m_sessions_mtx);
                m_sessions.insert(session);
            }
            session->run();
            do_accept();
        });
    }

    tcp::acceptor m_acceptor;
    tcp::endpoint m_endpoint;
    std::unordered_set<std::shared_ptr<WebSocketSession>> m_sessions;
    std::mutex m_sessions_mtx;
};
