#pragma once

#include "../logger.h"
#include "../mediaport/agent_robot_audiomediaport.h"
// #include "../core/sip_core.h"
#include <pjsua2/account.hpp>
#include <pjsua2/call.hpp>
#include <pjsua2/media.hpp>
#include <pjsua2/endpoint.hpp>

class IncomingCall : public pj::Call
{
public:
    IncomingCall(pj::Account &acc, int call_id = PJSUA_INVALID_ID)
        : pj::Call(acc, call_id)
    {
    }
    IncomingCall(const IncomingCall &) = delete;
    IncomingCall &operator=(const IncomingCall &) = delete;
    ~IncomingCall() = default;

    virtual void onCallState(pj::OnCallStateParam &prm) override
    {
        PJ_UNUSED_ARG(prm);

        pj::CallInfo ci = pj::Call::getInfo();
        LOG_INFO("call id: {} state: {} code:{}",
                 ci.id, ci.stateText, (int)ci.lastStatusCode);
        if (ci.lastStatusCode == 404) {
            LOG_ERROR("call last status code: {}", (int)ci.lastStatusCode);
            return;
        }
        if (!ci.lastReason.empty()) {
            LOG_INFO("call reason: {}", ci.lastReason);
        }

        switch (ci.state) {
            case PJSIP_INV_STATE_INCOMING: {
                // 向会话管理注册 ID
                // 更新呼叫方式
                break;
            }
            case PJSIP_INV_STATE_CONFIRMED: {
                // 更新 SipCore 呼叫状态
                // SIPCore::getInstance()->setState(SIPCore::State::CALLING);
                break;
            }
            case PJSIP_INV_STATE_DISCONNECTED: {
                // 更新 SipCore 呼叫状态
                // SIPCore::getInstance()->setState(SIPCore::State::IDLE);
                break;
            }
            default: {
                break;
            }
        }
    }

    virtual void onCallMediaState(pj::OnCallMediaStateParam &prm) override
    {
        PJ_UNUSED_ARG(prm);
        pj::CallInfo ci = pj::Call::getInfo();
        for (unsigned i = 0; i < ci.media.size(); i++) {
            if (ci.media[i].type == PJMEDIA_TYPE_AUDIO && pj::Call::getMedia(i) != nullptr) {
                auto *aud_med = (pj::AudioMedia *)pj::Call::getMedia(i);
                m_agent_robot_media_port = std::make_shared<AgentRobotAudioMediaPort>();
                aud_med->startTransmit(*m_agent_robot_media_port);
                m_agent_robot_media_port->startTransmit(*aud_med);
            }
        }
    }

private:
    // mediaport
    std::shared_ptr<AgentRobotAudioMediaPort> m_agent_robot_media_port;
};