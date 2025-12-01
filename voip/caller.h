#pragma once

#include "agent_aud_audiomediaport.h"
#include "agent_cap_audiomediaport.h"
#include "agent_robot_audiomediaport.h"
#include "coordinator.h"
#include <pjsua2.hpp>
#include <string>
#include <memory>
#include <ctime>

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

    // 呼叫状态改变
    virtual void onCallState(pj::OnCallStateParam &prm) override;

    // 呼叫状态改变
    virtual void onCallMediaState(pj::OnCallMediaStateParam &prm) override;

    // virtual void onStreamCreated(pj::OnStreamCreatedParam &prm) override;

    void group_call(const std::string &phone,
                    const int dialplan_id,
                    std::shared_ptr<Coordinator> coordinator,
                    std::shared_ptr<IWSSender> sender,
                    const std::string &call_method,
                    const std::string &different);

    void single_call(const std::string &phone,
                     const int dialplan_id,
                     std::shared_ptr<Coordinator> coordinator,
                     std::shared_ptr<IWSSender> sender,
                     const std::string &call_method,
                     const std::string &different);

private:
    int m_dialplan_id;
    std::time_t now_time;
    std::string m_filename;
    VAccount &acc_;

    std::string m_phone;
    std::shared_ptr<Coordinator> m_coordinator;
    std::shared_ptr<IWSSender> m_sender;
    std::string m_call_status;
    std::string call_type;
    std::string hangup_direction;
    std::string m_call_method;
    std::string m_different;
    // std::shared_ptr<pj::AudioMediaRecorder> m_audio_media_recorder;

    std::shared_ptr<AgentAudAudioMediaPort> m_agent_aud_media_port;
    std::shared_ptr<AgentCapAudioMediaPort> m_agent_cap_media_port;
    std::shared_ptr<AgentRobotAudioMediaPort> m_agent_robot_media_port;
};

} // namespace voip
