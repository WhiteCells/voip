#pragma once

#include "../logger.h"
#include "../coordinator.h"
#include "outcoming_acc.h"
#include <pjsua2.hpp>
#include <pjsua2/call.hpp>
#include <pjsua2/endpoint.hpp>
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
            case PJSIP_INV_STATE_CONNECTING: {
                LOG_INFO("call {} state: {}", m_phone, ci.stateText);
                break;
            }
            case PJSIP_INV_STATE_CONFIRMED: {
                LOG_INFO("call {} state: {}", m_phone, ci.stateText);
                break;
            }
            case PJSIP_INV_STATE_DISCONNECTED: {
                LOG_INFO("call {} state: {}", m_phone, ci.stateText);

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

        // if (m_confirmed)
        pj::AudioMedia *aud_med = nullptr;
        auto &aud_mgr = pj::Endpoint::instance().audDevManager();
        auto cap_dev_med = aud_mgr.getCaptureDevMedia();
        auto play_dev_med = aud_mgr.getPlaybackDevMedia();
        cap_dev_med.adjustRxLevel(2.0);
        play_dev_med.adjustRxLevel(2.0);

        for (unsigned i = 0; i < ci.media.size(); ++i) {
            if (ci.media[i].type == PJMEDIA_TYPE_AUDIO) {
                aud_med = (pj::AudioMedia *)pj::Call::getMedia(i);

                if (m_call_method == "manual") {
                    // manual
                }
                else if (m_call_method == "agent") {
                    // agent
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

        if (m_coordinator->waitForWinner(std::chrono::seconds(10)) || m_coordinator->shouldAbort(shared_from_this())) {
            // todo 向后端发送挂断状态
            return;
        }
        // 阻塞等待通话结束
        m_coordinator->waitForCallFinished();
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

        if (m_coordinator->waitForSingleCallConfirmed(std::chrono::seconds(10))) {
            // todo 向后端发送挂断状态
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
};