#ifndef _WS_SESSION_MGR_H_
#define _WS_SESSION_MGR_H_

#include "singleton.hpp"
#include "types.h"
#include "ws_session.h"
#include <mutex>

class WsSessionMgr :
    public Singleton<WsSessionMgr>
{
public:
    bool join_session(WsSessionType type, const WsSessionId &id, WsSession::Sptr ptr);
    bool leave_session(WsSessionType type, const WsSessionId &id);
    WsSession::Sptr get_friend_session(WsSessionType owner_type, const WsSessionId &id);
    bool match_session(WsSessionType owner_type, const WsSessionId &id);

    void printSession();
    void printFriend();

private:
    bool join_voip_session(const WsSessionId &id, WsSession::Sptr ptr);
    bool join_robot_session(const WsSessionId &id, WsSession::Sptr ptr);

    bool leave_voip_session(const WsSessionId &id);
    bool leave_robot_session(const WsSessionId &id);

    WsSession::Sptr get_voip_friend_session(const WsSessionId &id);
    WsSession::Sptr get_robot_friend_session(const WsSessionId &id);

    WsSessionStatus get_session_status(WsSessionType type, const WsSessionId &id);

    bool update_voip_session_status(const WsSessionId &id, WsSessionStatus status);
    bool update_robot_session_status(const WsSessionId &id, WsSessionStatus status);

    bool relate_session(const WsSessionId &id1, const WsSessionId &id2);

    WsSessionStatus get_voip_session_status(const WsSessionId &id);
    WsSessionStatus get_robot_session_status(const WsSessionId &id);

    bool match_voip_session(const WsSessionId &id);
    bool match_robot_session(const WsSessionId &id);

private:
    std::unordered_map<WsSessionId, WsSession::Sptr> s_voip_session;         // <voip_id, session>
    std::unordered_map<WsSessionId, WsSessionStatus> s_voip_session_status;  // <voip_id, status>
    std::unordered_map<WsSessionId, WsSession::Sptr> s_robot_session;        // <robot_id, session>
    std::unordered_map<WsSessionId, WsSessionStatus> s_robot_session_status; // <robot_id, status>
    std::unordered_map<WsSessionId, WsSessionId> s_robot2voip;               // <robot_id, voip_id>
    std::unordered_map<WsSessionId, WsSessionId> s_voip2robot;               // <voip_id, robot_id>
    std::mutex s_mtx;
};

#endif // _WS_SESSION_MGR_H_