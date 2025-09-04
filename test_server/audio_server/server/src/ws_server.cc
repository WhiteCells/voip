#include "ws_server.h"
#include "logger.h"
#include "ws_session_mgr.h"

WsServer::WsServer(net::io_context &ioc,
                   const std::string &addr,
                   const unsigned short port) :
    m_acceptor(ioc),
    m_endpoint(net::ip::make_address(addr), port)
{
    beast::error_code ec;
    if (m_acceptor.open(m_endpoint.protocol(), ec)) {
        LOG_ERROR("open");
        return;
    }
    if (m_acceptor.set_option(net::socket_base::reuse_address(true), ec)) {
        LOG_ERROR("set_option");
        return;
    }
    if (m_acceptor.bind(m_endpoint, ec)) {
        LOG_ERROR("bind");
        return;
    }
    if (m_acceptor.listen(net::socket_base::max_listen_connections, ec)) {
        LOG_ERROR("listen");
        return;
    }
    do_accept();
}

WsServer::~WsServer()
{
}

void WsServer::send(const std::string &id, const std::string &msg)
{
    (void)id;
    (void)msg;
    std::lock_guard<std::mutex> lock(m_session_mtx);
    // auto sess = m_sessions.at(id);
    // sess->send(msg);
}

void WsServer::do_accept()
{
    m_acceptor.async_accept([this](beast::error_code ec, tcp::socket socket) {
        if (ec) {
            LOG_ERROR("async_accept: {}", ec.what());
            do_accept();
            return;
        }
        auto session = std::make_shared<WsSession>(std::move(socket));
        session->set_on_ready([session](WsSessionType type, const WsSessionId &id) {
            LOG_INFO("type: {}", (int)type);
            LOG_INFO("id: {}", id);
            WsSessionMgr::getInstance()->join_session(type, id, session);
            WsSessionMgr::getInstance()->match_session(type, id);
            WsSessionMgr::getInstance()->printSession();
            WsSessionMgr::getInstance()->printFriend();
        });
        session->run();
        do_accept();
    });
}
