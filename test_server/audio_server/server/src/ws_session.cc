#include "ws_session.h"
#include "ws_session_mgr.h"

void WsSession::on_read_ws(beast::error_code ec, std::size_t bytes)
{
    if (ec == websocket::error::closed) {
        LOG_ERROR("on_read_ws: {} {}", (int)m_type, m_id);
        WsSessionMgr::getInstance()->printSession();
        WsSessionMgr::getInstance()->leave_session(m_type, m_id);
        WsSessionMgr::getInstance()->printSession();
        return;
    }
    if (ec) {
        LOG_ERROR("on_read_ws: {} {}", (int)m_type, m_id);
        WsSessionMgr::getInstance()->printSession();
        WsSessionMgr::getInstance()->leave_session(m_type, m_id);
        WsSessionMgr::getInstance()->printSession();
        return;
    }
    std::string msg = beast::buffers_to_string(m_buffer.data());
    m_buffer.consume(bytes);
    // process msg
    LOG_INFO("recv msg: {}", msg);
    auto friend_session_ptr = WsSessionMgr::getInstance()->get_friend_session(m_type, m_id);
    if (friend_session_ptr) {
        friend_session_ptr->send(msg);
    }
    else {
        LOG_WARN("no friend");
        send("no friend");
    }
    do_read();
}