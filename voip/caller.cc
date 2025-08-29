#include "caller.h"
#include "vaccount.h"
#include "request.hpp"
#include "caller_queue.h"
#include "logger.h"
#include "request.hpp"
#include "global.h"
#include "coordinator.h"

voip::Caller::Caller(voip::VAccount &acc, int call_id) :
    pj::Call(acc, call_id),
    acc_(acc),
    m_aud_media_port(std::make_shared<AgentAudioMediaPort>()),
    m_aud_media_player(std::make_shared<pj::AudioMediaPlayer>())
{
    // m_aud_media_player->createPlayer("input.wav");
}

voip::Caller::~Caller()
{
}

void voip::Caller::call(
    const std::string &phone,
    const std::string &client_id,
    const int dialplan_id)
{
    auto coordinator = Coordinator::getInstance();
    coordinator->reset_();

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
    aud_media_recorder_->createRecorder(m_filename);
    const std::string dst_uri = "sip:" + phone + "@" + acc_.getHost();
    LOG_INFO("dst_uri: {}", dst_uri);
    const pj::CallOpParam prm {true};
    this->makeCall(dst_uri, prm);

    coordinator->waitForWinner();

    if (coordinator->shouldAbort(shared_from_this())) {
        LOG_WARN("should abort");
        // hangup_();
    }

    coordinator->waitForCallFinished();
}

void voip::Caller::hangup_()
{
    pj::CallOpParam prm;
    // this->hangup(prm);
}

void voip::Caller::onCallTsxState(pj::OnCallTsxStateParam &prm)
{
    PJ_UNUSED_ARG(prm);
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
            break;
        }
        case PJSIP_INV_STATE_CONFIRMED: {
            LOG_INFO(">>> call: {}, phone: {} confirmed", ci.id, m_phone);
            auto coordinator = Coordinator::getInstance();
            // 当前线程如果已经接通了，通知其他线程挂断电话
            coordinator->notifyCallConfirmed(shared_from_this());
            break;
        }
        case PJSIP_INV_STATE_DISCONNECTED: {
            LOG_INFO(">>> call: {}, phone: {} disconnected", ci.id, m_phone);
            auto coordinator = Coordinator::getInstance();
            coordinator->notifyCallDisconnected(shared_from_this());
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
    LOG_INFO("call: {} Media State Changed: {}, media size: {}", ci.id, ci.stateText, ci.media.size());

    pj::AudioMedia *aud_med;

    pj::AudDevManager &mgr = pj::Endpoint::instance().audDevManager();
    auto cap_dev_med = mgr.getCaptureDevMedia();
    auto play_dev_med = mgr.getPlaybackDevMedia();

    for (unsigned i = 0; i < ci.media.size(); ++i) {
        if (ci.media[i].type == PJMEDIA_TYPE_AUDIO) {
            LOG_INFO("used media index: {}", i);
            aud_med = (pj::AudioMedia *)getMedia(i);

            //
            // m_aud_media_player->startTransmit(*aud_med);
            //
            m_aud_media_port->startTransmit(*aud_med);
            aud_med->startTransmit(*m_aud_media_port);
            // m_aud_media_port->startTransmit(*aud_media_recorder_);
            //
            // cap_dev_med.startTransmit(*m_aud_media_port);
            //
            // cap_dev_med.startTransmit(*aud_med);
            // aud_med->startTransmit(*aud_media_recorder_); // 录音无噪音
            // aud_med->startTransmit(play_dev_med);         // 播放设备有明显电流声
        }
    }
}
