#include "ws_session_mgr.h"
#include "logger.h"

bool WsSessionMgr::join_session(WsSessionType type, const WsSessionId &id, WsSession::Sptr ptr)
{
    std::unique_lock<std::mutex> lock(s_mtx);
    if (s_voip_session.find(id) != s_voip_session.end() || s_robot_session.find(id) != s_robot_session.end()) {
        return false;
    }
    bool res;
    switch (type) {
        case kVoip: {
            res = join_voip_session(id, ptr);
            update_voip_session_status(id, kFree);
            break;
        }
        case kRobot: {
            res = join_robot_session(id, ptr);
            update_robot_session_status(id, kFree);
            break;
        }
        default: {
            res = false;
            break;
        }
    }
    return res;
}

bool WsSessionMgr::leave_session(WsSessionType type, const WsSessionId &id)
{
    std::unique_lock<std::mutex> lock(s_mtx);
    if (s_voip_session.find(id) == s_voip_session.end() && s_robot_session.find(id) == s_robot_session.end()) {
        return false;
    }
    bool res;
    switch (type) {
        case kVoip: {
            res = leave_voip_session(id);
            break;
        }
        case kRobot: {
            res = leave_robot_session(id);
            break;
        }
        default: {
            res = false;
            break;
        }
    }
    return res;
}

WsSession::Sptr WsSessionMgr::get_friend_session(WsSessionType owner_type, const WsSessionId &id)
{
    std::unique_lock<std::mutex> lock(s_mtx);
    if (s_voip2robot.find(id) == s_voip2robot.end() && s_robot2voip.find(id) == s_robot2voip.end()) {
        return nullptr;
    }
    WsSession::Sptr friend_ptr;
    switch (owner_type) {
        case kVoip: {
            friend_ptr = get_voip_friend_session(id);
            break;
        }
        case kRobot: {
            friend_ptr = get_robot_friend_session(id);
            break;
        }
        default: {
            friend_ptr = nullptr;
            break;
        }
    }
    return friend_ptr;
}

bool WsSessionMgr::match_session(WsSessionType owner_type, const WsSessionId &id)
{
    std::unique_lock<std::mutex> lock(s_mtx);
    if (s_voip_session.find(id) == s_voip_session.end() && s_robot_session.find(id) == s_robot_session.end()) {
        return false;
    }
    bool res;
    switch (owner_type) {
        case kVoip: {
            res = match_robot_session(id);
            break;
        }
        case kRobot: {
            res = match_voip_session(id);
            break;
        }
        default: {
            res = false;
            break;
        }
    }
    return res;
}

void WsSessionMgr::printSession()
{
    std::unique_lock<std::mutex> lock(s_mtx);
    LOG_INFO("--------------------------->");
    LOG_INFO("=== All Session ===");
    LOG_INFO(">>> voip session");
    for (const auto &[k, v] : s_voip_session) {
        LOG_INFO(" * id: {}, type: {}", k, (int)v->getType());
    }
    LOG_INFO(">>> robot session");
    for (const auto &[k, v] : s_robot_session) {
        LOG_INFO(" * id: {}, type: {}", k, (int)v->getType());
    }
    LOG_INFO("<---------------------------");
}

void WsSessionMgr::printFriend()
{
    std::unique_lock<std::mutex> lock(s_mtx);
    LOG_INFO("--------------------------->");
    LOG_INFO("=== Friend Session ===");
    LOG_INFO(">>> robot2voip friend session");
    for (const auto &[k, v] : s_robot2voip) {
        LOG_INFO(" <{}: {}>", k, v);
    }
    LOG_INFO(">>> voip2robot friend session");
    for (const auto &[k, v] : s_voip2robot) {
        LOG_INFO(" <{}: {}>", k, v);
    }
    LOG_INFO("<---------------------------");
}

bool WsSessionMgr::join_voip_session(const WsSessionId &id, WsSession::Sptr ptr)
{
    if (s_voip_session.find(id) != s_voip_session.end()) {
        return false;
    }
    s_voip_session[id] = ptr;
    match_robot_session(id);
    return true;
}

bool WsSessionMgr::join_robot_session(const WsSessionId &id, WsSession::Sptr ptr)
{
    if (s_robot_session.find(id) != s_robot_session.end()) {
        return false;
    }
    s_robot_session[id] = ptr;
    match_voip_session(id);
    return true;
}

bool WsSessionMgr::leave_voip_session(const WsSessionId &id)
{
    if (s_voip_session.find(id) == s_voip_session.end()) {
        return false;
    }
    WsSessionId robot_id {};
    s_voip_session.erase(id);
    if (s_voip2robot.find(id) != s_voip2robot.end()) {
        robot_id = s_voip2robot.at(id);
        s_voip2robot.erase(id);
    }
    if (s_robot2voip.find(robot_id) != s_robot2voip.end()) {
        s_robot2voip.erase(robot_id);
        update_robot_session_status(robot_id, kFree);
    }
    return true;
}

bool WsSessionMgr::leave_robot_session(const WsSessionId &id)
{
    if (s_robot_session.find(id) == s_robot_session.end()) {
        return false;
    }
    WsSessionId voip_id {};
    s_robot_session.erase(id);
    if (s_robot2voip.find(id) != s_robot2voip.end()) {
        voip_id = s_robot2voip.at(id);
        s_robot2voip.erase(id);
    }
    if (s_voip2robot.find(voip_id) != s_voip2robot.end()) {
        s_voip2robot.erase(voip_id);
        update_voip_session_status(voip_id, kFree);
    }
    return true;
}

WsSession::Sptr WsSessionMgr::get_voip_friend_session(const WsSessionId &id)
{
    if (s_voip2robot.find(id) == s_voip2robot.end()) {
        return nullptr;
    }
    auto robot_id = s_voip2robot.at(id);
    if (s_robot_session.find(robot_id) == s_robot_session.end()) {
        LOG_ERROR("voip friend should exist");
        return nullptr;
    }
    auto robot_ptr = s_robot_session.at(robot_id);
    return robot_ptr;
}

WsSession::Sptr WsSessionMgr::get_robot_friend_session(const WsSessionId &id)
{
    if (s_robot2voip.find(id) == s_robot2voip.end()) {
        return nullptr;
    }
    auto voip_id = s_robot2voip.at(id);
    if (s_voip_session.find(voip_id) == s_voip_session.end()) {
        LOG_ERROR("robot friend should exist");
        return nullptr;
    }
    auto voip_ptr = s_voip_session.at(voip_id);
    return voip_ptr;
}

bool WsSessionMgr::update_voip_session_status(const WsSessionId &id, WsSessionStatus status)
{
    if (s_voip_session.find(id) == s_voip_session.end()) {
        return false;
    }
    s_voip_session_status[id] = status;
    return true;
}

bool WsSessionMgr::update_robot_session_status(const WsSessionId &id, WsSessionStatus status)
{
    if (s_robot_session.find(id) == s_robot_session.end()) {
        return false;
    }
    s_robot_session_status[id] = status;
    return true;
}

bool WsSessionMgr::relate_session(const WsSessionId &voip_id, const WsSessionId &robot_id)
{
    if (s_voip2robot.find(voip_id) != s_voip2robot.end() || s_robot2voip.find(robot_id) != s_robot2voip.end()) {
        return false;
    }
    s_voip2robot[voip_id] = robot_id;
    s_robot2voip[robot_id] = voip_id;
    return true;
}

WsSessionStatus WsSessionMgr::get_session_status(WsSessionType type, const WsSessionId &id)
{
    std::unique_lock<std::mutex> lock(s_mtx);
    WsSessionStatus status;
    switch (type) {
        case kVoip: {
            status = get_voip_session_status(id);
            break;
        }
        case kRobot: {
            status = get_robot_session_status(id);
            break;
        }
        default: {
            status = kNone;
            break;
        }
    }
    return status;
}

WsSessionStatus WsSessionMgr::get_voip_session_status(const WsSessionId &id)
{
    if (s_voip_session_status.find(id) == s_voip_session_status.end()) {
        return kNone;
    }
    return s_voip_session_status.at(id);
}

WsSessionStatus WsSessionMgr::get_robot_session_status(const WsSessionId &id)
{
    if (s_robot_session_status.find(id) == s_robot_session_status.end()) {
        return kNone;
    }
    return s_robot_session_status.at(id);
}

bool WsSessionMgr::match_voip_session(const WsSessionId &robot_id)
{
    // robot to match voip session
    for (const auto &[voip_id, _] : s_voip_session) {
        if (get_voip_session_status(voip_id) == kFree) {
            relate_session(voip_id, robot_id);
            update_voip_session_status(voip_id, kUsed);
            return true;
        }
    }
    return false;
}

bool WsSessionMgr::match_robot_session(const WsSessionId &voip_id)
{
    // voip to match robot session
    for (const auto &[robot_id, _] : s_robot_session) {
        if (get_robot_session_status(robot_id) == kFree) {
            relate_session(voip_id, robot_id);
            update_robot_session_status(robot_id, kUsed);
            return true;
        }
    }
    return false;
}
