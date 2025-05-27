#include "caller.h"
#include "vaccount.h"
#include "request.hpp"
#include "caller_queue.h"
#include "logger.h"
#include "request.hpp"
#include "global.h"

#include <pjsua2/call.hpp>
#include <iostream>

voip::Caller::Caller(voip::VAccount &acc, int call_id) :
    pj::Call(acc, call_id),
    acc_(acc)
// aud_media_port_(std::make_shared<VAudioMediaPort>()),
// aud_media_recorder_(std::make_shared<pj::AudioMediaRecorder>())
{
    // pj::MediaFormatAudio fmt;
    // fmt.init(PJMEDIA_FORMAT_PCM, 16000, 1, 20000, 16);
    // aud_media_port_->createPort("aud_media_port_", fmt);

    // pj::AudDevManager &mgr = pj::Endpoint::instance().audDevManager();
    // cap_dev_med_ = mgr.getCaptureDevMedia();
    // play_dev_med_ = mgr.getPlaybackDevMedia();
}

voip::Caller::~Caller()
{
    // if (acc_.cur_call == this) {
    //     acc_.cur_call = nullptr;
    //     std::cout << ">>> Call object destroyed, account call pointer cleared." << std::endl;
    // }
    // else {
    //     std::cout << ">>> Call object destroyed (was not the account's active call)." << std::endl;
    // }
}

void voip::Caller::call(
    const std::string &phone,
    const std::string &client_id,
    std::shared_ptr<CallerQueue> que,
    std::shared_ptr<Caller> caller)
{
    m_phone = phone;
    m_client_id = client_id;
    m_que = que;
    m_caller = caller;
    aud_media_recorder_.reset();
    aud_media_recorder_ = std::make_shared<pj::AudioMediaRecorder>();
    aud_media_recorder_->createRecorder(phone + ".wav");
    const std::string dst_uri = "sip:" + phone + "@" + acc_.getHost();
    std::cout << dst_uri << std::endl;
    const pj::CallOpParam prm {true};
    this->makeCall(dst_uri, prm);
}

void voip::Caller::onCallTsxState(pj::OnCallTsxStateParam &prm)
{
    PJ_UNUSED_ARG(prm);

    pj::CallInfo ci = getInfo();

    int statusCode = ci.lastStatusCode;
    std::string statusText = ci.lastReason;

    LOG_INFO("call status: {} {}", statusCode, statusText);

    // if (statusCode == 404) {
    // }
    // else if (statusCode == 486) {
    // }
    // else if (statusCode == 603) {
    // }
    // else if (statusCode >= 400) {
    // }
}

void voip::Caller::onCallState(pj::OnCallStateParam &prm)
{
    PJ_UNUSED_ARG(prm);

    pj::CallInfo ci = getInfo();
    LOG_INFO("call id: {} state: {}", ci.id, ci.stateText);
    if (!ci.lastReason.empty()) {
        LOG_INFO("call reason: {}", ci.lastReason);
    }
    std::cout << std::endl;

    switch (ci.state) {
        case PJSIP_INV_STATE_CALLING:
            std::cout << ">>> call" << ci.id << " calling" << std::endl;
            //
            voip::pushDialStatus(
                m_phone,
                DIAL_STATE::DURING,
                m_client_id);
            LOG_INFO("calling: {}", m_phone);
            break;
        case PJSIP_INV_STATE_CONFIRMED: {
            std::cout << ">>> call " << ci.id << " connected/Confirmed." << std::endl;
            // 挂断电话
            pj::CallOpParam prm;
            this->hangup(prm);
            //
            voip::pushDialStatus(
                m_phone,
                DIAL_STATE::CONFIRMED,
                m_client_id);
            LOG_INFO("hangup: {}", m_phone);
            break;
        }
        case PJSIP_INV_STATE_DISCONNECTED: {
            std::cout << ">>> call " << ci.id << " disconnected." << std::endl;
            // 推送文件
            voip::pushFile(m_phone + ".wav", "00001");
            // single 回收
            // m_que->releaseCaller(m_caller);
            LOG_INFO("phone: {} disconnected", m_phone);
            break;
        }
        default:
            break;
    }
}

void voip::Caller::onCallMediaState(pj::OnCallMediaStateParam &prm)
{
    PJ_UNUSED_ARG(prm);

    pj::CallInfo ci = getInfo();
    LOG_INFO("call: {} Media State Changed", ci.id);
    LOG_INFO("media size: {}", ci.media.size());

    pj::AudioMedia *aud_med;

    for (unsigned i = 0; i < ci.media.size(); ++i) {
        if (ci.media[i].type == PJMEDIA_TYPE_AUDIO) {
            LOG_INFO("used media index: {}", i);
            aud_med = (pj::AudioMedia *)getMedia(i);
        }
    }
    aud_med->startTransmit(*aud_media_recorder_);
}

// void voip::Caller::onStreamCreated(pj::OnStreamCreatedParam &prm)
// {
//     this->onStreamCreated(prm);
//     std::ofstream out_file("stream", std::ios::binary | std::ios::app);
//     if (out_file.is_open()) {
//         out_file.write(reinterpret_cast<const char *>(prm.stream), sizeof(prm.stream));
//         out_file.close();
//     }
// }