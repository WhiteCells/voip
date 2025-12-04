#pragma once

#include <pjsua2.hpp>
#include <iostream>

using namespace pj;

class MyCall : public Call
{
public:
    MyCall(Account &acc, int call_id = PJSUA_INVALID_ID)
        : Call(acc, call_id) {}

    virtual void onCallState(OnCallStateParam &prm) override
    {
        CallInfo ci = getInfo();
        std::cout << "[Call State] " << ci.stateText << std::endl;

        if (ci.state == PJSIP_INV_STATE_INCOMING) {
            std::cout << "Incoming call from: " << ci.remoteUri << std::endl;
            // CallOpParam ansPrm;
            // ansPrm.statusCode = (pjsip_status_code)200;
            // answer(ansPrm);
            // std::cout << "Call answered." << std::endl;
        }
    }

    virtual void onCallMediaState(OnCallMediaStateParam &prm) override
    {
        CallInfo ci = getInfo();
        for (unsigned i = 0; i < ci.media.size(); i++) {
            if (ci.media[i].type == PJMEDIA_TYPE_AUDIO &&
                getMedia(i) != nullptr) {
                AudioMedia *aud_med = (AudioMedia *)getMedia(i);
                AudDevManager &mgr = Endpoint::instance().audDevManager();
//                mgr.getPlaybackDevMedia().adjustRxLevel(2.0);
//                mgr.getCaptureDevMedia().adjustRxLevel(2.0);
                mgr.getPlaybackDevMedia().adjustTxLevel(2.0);
                mgr.getCaptureDevMedia().adjustTxLevel(2.0);
                aud_med->startTransmit(mgr.getPlaybackDevMedia());
                mgr.getCaptureDevMedia().startTransmit(*aud_med);
            }
        }
    }
};
