#pragma once

#include "../singleton.hpp"
#include <string>

// 会话核心
// 存储会话 ID、访问令牌、呼叫方法
class SessionCore : public Singleton<SessionCore>
{
    friend class Singleton<SessionCore>;

public:
    ~SessionCore() = default;

    void setSessionId(const std::string &session_id) { m_session_id = session_id; }
    void setAccessToken(const std::string &access_token) { m_access_token = access_token; }
    void setCallMethod(const std::string &call_method) { m_call_method = call_method; }

    std::string getSessionId() const { return m_session_id; }
    std::string getAccessToken() const { return m_access_token; }
    std::string getCallMethod() const { return m_call_method; }

private:
    SessionCore() = default;

private:
    std::string m_session_id;
    std::string m_access_token;
    std::string m_call_method;
};