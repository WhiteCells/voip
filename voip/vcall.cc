#include "vcall.h"
#include "vaccount.h"
#include "vaudiomediaport.h"

#include <pjsua2/call.hpp>
#include <iostream>

voip::VCall::VCall(voip::VAccount &acc, int call_id) :
    Call(acc, call_id),
    acc_(acc),
    aud_media_port_(std::make_shared<VAudioMediaPort>()),
    aud_media_recorder_(std::make_shared<pj::AudioMediaRecorder>())
{
    pj::MediaFormatAudio fmt;
    fmt.init(PJMEDIA_FORMAT_PCM, 16000, 1, 20000, 16);
    aud_media_port_->createPort("aud_media_port_", fmt);

    // recorder
    aud_media_recorder_->createRecorder(acc.phone_num_ + ".wav");

    pj::AudDevManager &mgr = pj::Endpoint::instance().audDevManager();
    cap_dev_med_ = mgr.getCaptureDevMedia();
    play_dev_med_ = mgr.getPlaybackDevMedia();
}

voip::VCall::~VCall()
{
    if (acc_.cur_call == this) {
        acc_.cur_call = nullptr;
        std::cout << ">>> Call object destroyed, account call pointer cleared." << std::endl;
    }
    else {
        std::cout << ">>> Call object destroyed (was not the account's active call)." << std::endl;
    }
}

void voip::VCall::onCallState(pj::OnCallStateParam &prm)
{
    PJ_UNUSED_ARG(prm);
    try {
        pj::CallInfo ci = getInfo();
        std::cout << ">>> call " << ci.id << " state: " << ci.stateText;
        if (!ci.lastReason.empty()) {
            std::cout << " (reason: " << ci.lastReason << ")";
        }
        std::cout << std::endl;

        if (ci.state == PJSIP_INV_STATE_DISCONNECTED) {
            std::cout << ">>> call " << ci.id << " disconnected." << std::endl;
            if (acc_.cur_call == this) {
                acc_.cur_call = nullptr;
                std::cout << ">>> account's active call pointer cleared due to DISCONNECTED state." << std::endl;
            }
        }
        else if (ci.state == PJSIP_INV_STATE_CONFIRMED) {
            std::cout << ">>> call " << ci.id << " connected/Confirmed." << std::endl;
        }
    }
    catch (const pj::Error &err) {
        std::cerr << ">>> error getting call info in onCallState: " << err.info() << std::endl;
    }
}

void voip::VCall::onCallMediaState(pj::OnCallMediaStateParam &prm)
{
    PJ_UNUSED_ARG(prm);
    try {
        pj::CallInfo ci = getInfo();
        std::cout << ">>> call " << ci.id << " Media State Changed" << std::endl;

        for (unsigned i = 0; i < ci.media.size(); ++i) {
            if (ci.media[i].type == PJMEDIA_TYPE_AUDIO && getMedia(i)) {
                std::cout << "<<<:" << i << std::endl;
                pj::AudioMedia aud_med = getAudioMedia(i);

                if (ci.media[i].status == PJSUA_CALL_MEDIA_ACTIVE) {
                    try {
                        // cap_dev_med_.startTransmit(aud_med);
                        // aud_med.startTransmit(*aud_media_port_);
                        // aud_med.startTransmit(play_dev_med);
                        // cap_dev_med_.startTransmit(*aud_media_recorder_);
                        aud_med.startTransmit(*aud_media_recorder_);
                        // play_dev_med.startTransmit(*aud_media_recorder_);
                    }
                    catch (pj::Error &err) {
                        std::cerr << ">>> failed to connect audio for call " << ci.id << ": " << err.info() << std::endl;
                    }
                }
            }
            else if (ci.media[i].type != PJMEDIA_TYPE_AUDIO) {
                std::cout << ">>> non-audio media stream detected (type: " << ci.media[i].type << ")" << std::endl;
            }
        }
    }
    catch (const pj::Error &err) {
        std::cerr << ">>> error in onCallMediaState: " << err.info() << std::endl;
    }
}

// void voip::VCall::onStreamCreated(pj::OnStreamCreatedParam &prm)
// {
//     this->onStreamCreated(prm);
//     std::ofstream out_file("stream", std::ios::binary | std::ios::app);
//     if (out_file.is_open()) {
//         out_file.write(reinterpret_cast<const char *>(prm.stream), sizeof(prm.stream));
//         out_file.close();
//     }
// }