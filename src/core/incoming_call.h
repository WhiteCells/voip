#pragma once

#include "../logger.h"
#include <pjsua2.hpp>
#include <pjsua2/call.hpp>

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
                break;
            }
            case PJSIP_INV_STATE_DISCONNECTED: {
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
            }
        }
    }

private:
    // mediaport
};