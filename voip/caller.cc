#include "caller.h"
#include "vaccount.h"
#include "request.hpp"
#include "caller_queue.h"
#include "logger.h"
#include "request.hpp"
#include "global.h"

voip::Caller::Caller(voip::VAccount &acc, int call_id) :
    pj::Call(acc, call_id),
    acc_(acc),
    m_aud_media_port(std::make_shared<AgentAudioMediaPort>()),
    m_aud_media_port2(std::make_shared<AgentAudioMediaPort2>())
{
    pj::MediaFormatAudio fmt;
    fmt.type = PJMEDIA_TYPE_AUDIO;
    fmt.id = PJMEDIA_FORMAT_PCMA; // 或 PCM16, PCMA, etc.
    fmt.clockRate = 8000;
    fmt.channelCount = 1;
    fmt.frameTimeUsec = 20000; // 20ms
    fmt.bitsPerSample = 16;

    m_aud_media_port->createPort("media-port", fmt);

    m_aud_media_port2->createPort("media-port2", fmt);
}

voip::Caller::~Caller()
{
    // this->hangup(const CallOpParam &prm);
}

void voip::Caller::call(
    const std::string &phone,
    const std::string &client_id,
    const int dialplan_id)
{
    m_dialplan_id = dialplan_id;
    m_phone = phone;
    m_client_id = client_id;
    aud_media_recorder_.reset();
    aud_media_recorder_ = std::make_shared<pj::AudioMediaRecorder>();
    auto now = std::chrono::system_clock::now();
    now_time = std::chrono::system_clock::to_time_t(now);
    m_filename = phone + "_" +
                 std::to_string(dialplan_id) + "_" +
                 std::to_string(now_time) + ".wav";
    // aud_media_recorder_->createRecorder(m_filename);
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

    switch (ci.state) {
        case PJSIP_INV_STATE_CONNECTING: {
            LOG_INFO(">>> call: {}, phone: {} connecting", ci.id, m_phone);
            break;
        }
        case PJSIP_INV_STATE_NULL: {
            LOG_INFO(">>> call: {}, phone: {} null", ci.id, m_phone);
            break;
        }
        case PJSIP_INV_STATE_CALLING: {
            LOG_INFO(">>> call: {}, phone: {} calling", ci.id, m_phone);
            // 其他线程接收到已经接通的线程的通知后，会结束拨打
            break;
        }
        case PJSIP_INV_STATE_CONFIRMED: {
            LOG_INFO(">>> call: {}, phone: {} confirmed", ci.id, m_phone);
            // 当前线程如果已经接通了，通知其他线程挂断电话
            break;
        }
        case PJSIP_INV_STATE_DISCONNECTED: {
            LOG_INFO(">>> call: {}, phone: {} disconnected", ci.id, m_phone);
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
    LOG_INFO("call: {} Media State Changed, media size: {}", ci.id, ci.media.size());

    pj::AudioMedia *aud_med;
    // pj::AudioMedia aud_med;
    pj::AudDevManager &mgr = pj::Endpoint::instance().audDevManager();
    auto cap_dev_med = mgr.getCaptureDevMedia();
    auto play_dev_med = mgr.getPlaybackDevMedia();

    for (unsigned i = 0; i < ci.media.size(); ++i) {
        if (ci.media[i].type == PJMEDIA_TYPE_AUDIO) {
            LOG_INFO("used media index: {}", i);
            aud_med = (pj::AudioMedia *)getMedia(i);
            // aud_med = getAudioMedia(i);

            // cap_dev_med.startTransmit(*aud_med);
            // aud_med->startTransmit(play_dev_med);

            // LOG_INFO("audio media: {}", aud_med->getPortId());
            // LOG_INFO("audio media port: {}", m_aud_media_port->getPortId());
            // LOG_INFO("audio media2 port: {}", m_aud_media_port2->getPortId());
            // LOG_INFO("cap dev: {}", cap_dev_med.getPortId());
            // LOG_INFO("paly dev: {}", play_dev_med.getPortId());

            // m_aud_media_port2->startTransmit(*aud_med);
            aud_med->startTransmit(*m_aud_media_port);
        }
    }
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
