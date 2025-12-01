#pragma once

#include "agent_robot_audiomediaport.h"
#include "logger.h"
#include <pjsua2.hpp>
#include <pjsua2/call.hpp>

class VCaller : public pj::Call
{
public:
    VCaller(pj::Account &acc, int call_id = PJSUA_INVALID_ID)
        : pj::Call(acc, call_id) {}
    VCaller(const VCaller &) = delete;
    VCaller &operator=(const VCaller &) = delete;
    ~VCaller() = default;

    virtual void onCallState(pj::OnCallStateParam &prm) override
    {
        PJ_UNUSED_ARG(prm);

        pj::CallInfo ci = getInfo();
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
            case PJSIP_INV_STATE_INCOMING:
                break;
            default: {
                break;
            }
        }
    }

    virtual void onCallMediaState(pj::OnCallMediaStateParam &prm) override
    {
        PJ_UNUSED_ARG(prm);

        // todo
        pj::CallInfo ci = getInfo();
        for (unsigned i = 0; i < ci.media.size(); i++) {
            if (ci.media[i].type == PJMEDIA_TYPE_AUDIO &&
                getMedia(i) != nullptr) {
                pj::AudioMedia *aud_med = (pj::AudioMedia *)getMedia(i);
                pj::AudDevManager &mgr = pj::Endpoint::instance().audDevManager();
                mgr.getPlaybackDevMedia().adjustTxLevel(2.0);
                mgr.getCaptureDevMedia().adjustTxLevel(2.0);
                aud_med->startTransmit(mgr.getPlaybackDevMedia());
                mgr.getCaptureDevMedia().startTransmit(*aud_med);
            }
        }
    }
};