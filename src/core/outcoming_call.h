#pragma once

#include "../logger.h"
#include "../coordinator.h"
#include "../event/event.h"
#include "../event/msg.h"
#include "../mediaport/agent_aud_audiomediaport.h"
#include "../mediaport/agent_cap_audiomediaport.h"
#include "../mediaport/agent_robot_audiomediaport.h"
#include "outcoming_acc.h"
#include "../core/sip_core.h"
#include <pjsua2/call.hpp>
#include <json/json.h>
#include <string>
#include <memory>

class OutcomingCall :
    public pj::Call,
    public std::enable_shared_from_this<OutcomingCall>
{
public:
    OutcomingCall(OutcomingAcc &acc, int call_id = PJSUA_INVALID_ID)
        : pj::Call(acc, call_id)
        , m_acc(acc)
    {
    }
    OutcomingCall(const OutcomingCall &) = delete;
    OutcomingCall &operator=(const OutcomingCall &) = delete;
    ~OutcomingCall()
    {
        LOG_INFO("~OutcomingCall");
    }

    virtual void onCallState(pj::OnCallStateParam &prm) override
    {
        PJ_UNUSED_ARG(prm);

        pj::CallInfo ci = pj::Call::getInfo();
        LOG_INFO("call id: {} phone: {} state: {} code:{}",
                 ci.id, m_phone, ci.stateText, (int)ci.lastStatusCode);

        if (ci.lastStatusCode == PJSIP_SC_NOT_FOUND) {
            LOG_ERROR("call last status code: {}", (int)ci.lastStatusCode);
            return;
        }
        if (!ci.lastReason.empty()) {
            LOG_INFO("call reason: {}", ci.lastReason);
        }

        switch (ci.state) {
            case PJSIP_INV_STATE_CALLING: {
                LOG_INFO("call {} CALLING state: {}", m_phone, ci.stateText);
                // 通知 GuiWsServer 呼叫中
                Json::Value call_state;
                call_state["id"] = "111"; // todo
                call_state["phone"] = m_phone;
                call_state["status"] = "CALLING";
                call_state["type"] = "call_status";
                EventBus::getInstance()->publish(OutcomingCallStateMsg {call_state.toStyledString()});
                break;
            }
            case PJSIP_INV_STATE_CONFIRMED: {
                LOG_INFO("call {} CONFIRMED state: {}", m_phone, ci.stateText);
                m_coordinator->notifyCallConfirmed(shared_from_this());
                // 通知 GuiWsServer 已接通
                Json::Value call_state;
                call_state["id"] = "111"; // todo
                call_state["phone"] = m_phone;
                call_state["status"] = "CONFIRMED";
                call_state["type"] = "call_status";
                EventBus::getInstance()->publish(OutcomingCallStateMsg {call_state.toStyledString()});
                // todo 通知 WebHttpClient 已接通
                // 更新 SipCore 呼叫状态
                SIPCore::getInstance()->setState(SIPCore::State::CALLING);
                break;
            }
            case PJSIP_INV_STATE_DISCONNECTED: {
                LOG_INFO("call {} DISCONNECTED state: {}", m_phone, ci.stateText);
                // 通知 GuiWsServer 已挂断
                Json::Value call_state;
                call_state["id"] = "111"; // todo
                call_state["phone"] = m_phone;
                call_state["status"] = "DISCONNECTED";
                call_state["type"] = "call_status";
                EventBus::getInstance()->publish(OutcomingCallStateMsg {call_state.toStyledString()});
                // todo 通知 WebWsClient 已挂断
                // 更新 SipCore 呼叫状态
                SIPCore::getInstance()->setState(SIPCore::State::IDLE);
                m_coordinator->notifyCallDisconnected(shared_from_this());
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
        LOG_INFO("call {} media state: {}, media size: {}", m_phone, ci.stateText, ci.media.size());

        if (SIPCore::getInstance()->getState() != SIPCore::State::CALLING) {
            LOG_INFO("SIPCore state: {}, not CALLING, ignore", (int)SIPCore::getInstance()->getState());
            return;
        }

        pj::AudioMedia *aud_med = nullptr;
        auto &dev_mgr = pj::Endpoint::instance().audDevManager();
        auto cap_dev_med = dev_mgr.getCaptureDevMedia();
        auto play_dev_med = dev_mgr.getPlaybackDevMedia();

        for (unsigned i = 0; i < ci.media.size(); ++i) {
            if (ci.media[i].type == PJMEDIA_TYPE_AUDIO) {
                aud_med = (pj::AudioMedia *)pj::Call::getMedia(i);

                if (m_call_method == "manual") {
                    // manual
                    // m_agent_aud_med_port.reset();
                    // m_agent_cap_med_port.reset();
                    // m_agent_aud_med_port = std::make_shared<AgentAudAudioMediaPort>();
                    // m_agent_cap_med_port = std::make_shared<AgentCapAudioMediaPort>();

                    // aud_med->startTransmit(*m_agent_aud_med_port);
                    // cap_dev_med.startTransmit(*m_agent_cap_med_port);

                    aud_med->startTransmit(play_dev_med);
                    cap_dev_med.startTransmit(*aud_med);
                }
                else if (m_call_method == "agent") {
                    // agent
                    // m_agent_robot_aud_med_port.reset();
                    // m_agent_robot_aud_med_port = std::make_shared<AgentRobotAudioMediaPort>();
                    // aud_med->startTransmit(*m_agent_robot_aud_med_port);
                    // m_agent_robot_aud_med_port->startTransmit(*aud_med);
                }
                break;
            }
        }
    }

    void makeGroupCall(const std::string &phone,
                       const std::string &call_method,
                       const std::string &different,
                       std::shared_ptr<Coordinator> coordinator)
    {
        m_call_type = "group";
        m_phone = phone;
        m_call_method = call_method;
        m_differentl = different;
        m_coordinator = coordinator;

        const std::string dst_uri = "sip:" + phone + "@" + m_acc.getHost();
        LOG_INFO("dst_uri: {}", dst_uri);

        try {
            const pj::CallOpParam param {true};
            pj::Call::makeCall(dst_uri, param);
        }
        catch (const pj::Error &e) {
            LOG_ERROR("makeGroupCall error: {}, {}", e.reason, e.info());
            return;
        }

        if (!m_coordinator->waitForWinner(std::chrono::seconds(10)) || m_coordinator->shouldAbort(shared_from_this())) {
            LOG_INFO("call {} aborted", m_phone);
            // 通知 GuiWsServer 已挂断
            Json::Value call_state;
            call_state["id"] = "111"; // todo
            call_state["phone"] = m_phone;
            call_state["status"] = "DISCONNECTED";
            call_state["type"] = "call_status";
            EventBus::getInstance()->publish(OutcomingCallStateMsg {call_state.toStyledString()});
            // todo 通知 WebWsClient 已挂断
            return;
        }
        // 阻塞等待通话结束
        m_coordinator->waitForCallFinished();
        LOG_INFO("call {} finished", m_phone);
    }

    void makeSingleCall(const std::string &phone,
                        const std::string &call_method,
                        const std::string &different,
                        std::shared_ptr<Coordinator> coordinator)
    {
        m_call_type = "single";
        m_phone = phone;
        m_call_method = call_method;
        m_differentl = different;
        m_coordinator = coordinator;

        const std::string dst_uri = "sip:" + phone + "@" + m_acc.getHost();
        LOG_INFO("dst_uri: {}", dst_uri);

        try {
            const pj::CallOpParam param {true};
            pj::Call::makeCall(dst_uri, param);
        }
        catch (const pj::Error &e) {
            LOG_ERROR("makeSingleCall error: {}, {}", e.reason, e.info());
            return;
        }

        if (!m_coordinator->waitForSingleCallConfirmed(std::chrono::seconds(10))) {
            LOG_INFO("call {} aborted", m_phone);
            // todo 通知 GuiWsServer 已挂断
            Json::Value call_state;
            m_acc.getId();
            call_state["id"] = "111"; // todo
            call_state["phone"] = m_phone;
            call_state["status"] = "DISCONNECTED";
            call_state["type"] = "call_status";
            EventBus::getInstance()->publish(OutcomingCallStateMsg {call_state.toStyledString()});
            // todo 通知 WebWsClient 已挂断
            return;
        }
        // 阻塞等待通话结束
        m_coordinator->waitForCallFinished();
    }

private:
    OutcomingAcc &m_acc;
    std::string m_call_type; // group or single
    std::string m_phone;
    std::string m_call_method; // agent or manual
    std::string m_differentl;
    std::shared_ptr<Coordinator> m_coordinator;
    // mediaport
    std::shared_ptr<AgentAudAudioMediaPort> m_agent_aud_med_port;
    std::shared_ptr<AgentCapAudioMediaPort> m_agent_cap_med_port;
    std::shared_ptr<AgentRobotAudioMediaPort> m_agent_robot_aud_med_port;
};