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
{
}

voip::Caller::~Caller()
{
}

void voip::Caller::call(
    const std::string &phone,
    const std::string &client_id,
    const int dialplan_id,
    std::shared_ptr<CallerQueue> que,
    std::shared_ptr<Caller> caller)
{
    m_dialplan_id = dialplan_id;
    m_phone = phone;
    m_client_id = client_id;
    m_que = que;
    m_caller = caller;
    aud_media_recorder_.reset();
    aud_media_recorder_ = std::make_shared<pj::AudioMediaRecorder>();
    // boost::uuids::random_generator gen;
    // boost::uuids::uuid uuid;
    // std::string uuid_str = boost::uuids::to_string(uuid);
    auto now = std::chrono::system_clock::now();
    now_time = std::chrono::system_clock::to_time_t(now);
    m_filename = phone + "_" +
                 std::to_string(dialplan_id) + "_" +
                 std::to_string(now_time) + ".wav";
    aud_media_recorder_->createRecorder(m_filename);
    const std::string dst_uri = "sip:" + phone + "@" + acc_.getHost();
    LOG_INFO("dst_uri: {}", dst_uri);
    const pj::CallOpParam prm {true};
    this->makeCall(dst_uri, prm);
}

void voip::Caller::onCallTsxState(pj::OnCallTsxStateParam &prm)
{
    PJ_UNUSED_ARG(prm);

    // pj::CallInfo ci = getInfo();

    // int statusCode = ci.lastStatusCode;
    // std::string statusText = ci.lastReason;

    // LOG_INFO("call status: {} {}", statusCode, statusText);

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
    LOG_INFO("call id: {} phone: {} state: {} code:{}",
             ci.id, m_phone, ci.stateText, (int)ci.lastStatusCode);
    if (!ci.lastReason.empty()) {
        LOG_INFO("call reason: {}", ci.lastReason);
    }
    std::cout << std::endl;

    switch (ci.state) {
        case PJSIP_INV_STATE_CALLING:
            std::cout << ">>> call" << ci.id << " calling" << std::endl;
            // 推送 processing 状态
            voip::pushDialStatus(
                m_dialplan_id,
                m_phone,
                STATUS_DIALPLAN_PROCESSING,
                m_client_id,
                acc_.getId());
            LOG_INFO("calling: {}", m_phone);
            break;
        case PJSIP_INV_STATE_CONFIRMED: {
            std::cout << ">>> call " << ci.id << " connected/Confirmed." << std::endl;
            // 挂断电话
            pj::CallOpParam prm;
            this->hangup(prm);
            // 推送文件
            voip::pushFile(m_filename, g_client_id);
            // 推送状态
            voip::pushDialStatus(
                m_dialplan_id,
                m_phone,
                STATUS_DIALPLAN_FINISH,
                m_client_id,
                acc_.getId());
            LOG_INFO("hangup: {}", m_phone);
            break;
        }
        case PJSIP_INV_STATE_DISCONNECTED: {
            std::cout << ">>> call " << ci.id << " disconnected." << std::endl;
            // 推送文件
            voip::pushFile(m_filename, g_client_id);
            // 推送状态
            voip::pushDialStatus(
                m_dialplan_id,
                m_phone,
                STATUS_DIALPLAN_FINISH,
                m_client_id,
                acc_.getId());
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
//     pj::Call::onStreamCreated(prm);
//     std::ofstream out_file("stream", std::ios::binary | std::ios::app);
//     if (out_file.is_open()) {
//         out_file.write(reinterpret_cast<const char *>(prm.stream), sizeof(prm.stream));
//         out_file.close();
//     }
// }