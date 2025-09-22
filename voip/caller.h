#ifndef _VCALL_H_
#define _VCALL_H_

#include "agent_audiomediaport.h"
#include "agent_cap_audiomediaport.h"
#include "coordinator.h"
#include <pjsua2.hpp>
#include <string>
#include <memory>
#include <ctime>

class CallerQueue;

class IWSSender;

namespace voip {

class VAccount;

/**
 * @brief 呼叫者
 */
class Caller :
    public pj::Call,
    public std::enable_shared_from_this<Caller>
{
public:
    Caller(VAccount &acc, int call_id = PJSUA_INVALID_ID);
    Caller(const Caller &) = delete;
    Caller &operator=(const Caller &) = delete;
    ~Caller();

    // 状态吗改变
    virtual void onCallTsxState(pj::OnCallTsxStateParam &prm) override;

    // 呼叫状态改变
    virtual void onCallState(pj::OnCallStateParam &prm) override;

    // 呼叫状态改变
    virtual void onCallMediaState(pj::OnCallMediaStateParam &prm) override;

    // virtual void onStreamCreated(pj::OnStreamCreatedParam &prm) override;

    void group_call(const std::string &phone,
                    const std::string &client_id,
                    const int dialplan_id,
                    std::shared_ptr<Coordinator> coordinator,
                    std::shared_ptr<IWSSender> sender);

    void single_call(const std::string &phone,
                     const std::string &client_id,
                     const int dialplan_id,
                     std::shared_ptr<Coordinator> coordinator,
                     std::shared_ptr<IWSSender> sender);

    void hangup_();

private:
    int m_dialplan_id;
    std::time_t now_time;
    std::string m_filename;
    VAccount &acc_;

    std::string m_phone;
    std::string m_client_id;
    std::shared_ptr<Coordinator> m_coordinator;
    std::shared_ptr<IWSSender> m_sender;
    int m_call_status = 0;
    int call_type;
    std::string hangup_direction;

    std::shared_ptr<AgentAudioMediaPort> m_agent_media_port;
    std::shared_ptr<AgentCapAudioMediaPort> m_cap_agent_media_port;
};

} // namespace voip

#endif // _VCALL_H_
