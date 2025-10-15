#ifndef _VCALL_H_
#define _VCALL_H_

#ifdef REMINDER
#include "agent_aud_audiomediaport.h"
#include "agent_cap_audiomediaport.h"
#include "agent_robot_audiomediaport.h"
#elif ROBOT
#include "agent_robot_audiomediaport.h"
#endif
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
        Caller(VAccount &acc,int call_id = PJSUA_INVALID_ID);
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
                        std::shared_ptr<IWSSender> sender,
                        const std::string &call_method,
                        const std::string &different);

        void single_call(const std::string &phone,
                         const std::string &client_id,
                         const int dialplan_id,
                         std::shared_ptr<Coordinator> coordinator,
                         std::shared_ptr<IWSSender> sender,
                         const std::string &call_method,
                         const std::string &different);

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
        std::string m_call_status;
        std::string call_type;
        std::string hangup_direction;
        std::string m_call_method;
        std::string m_different;

#ifdef REMINDER
        std::shared_ptr<AgentAudAudioMediaPort> m_agent_aud_media_port;
    std::shared_ptr<AgentCapAudioMediaPort> m_agent_cap_media_port;
    std::shared_ptr<AgentRobotAudioMediaPort> m_agent_robot_media_port;
#elif ROBOT
        std::shared_ptr<AgentRobotAudioMediaPort> m_agent_robot_media_port;
#endif
    };

} // namespace voip

#endif // _VCALL_H_
