#include "caller.h"
#include "vaccount.h"
#include "request.hpp"
#include "caller_queue.h"
#include "logger.h"
#include "request.hpp"
#include "global.h"
#include "coordinator.h"
#include "ws_interface.h"

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
    std::shared_ptr<Coordinator> coordinator,
    std::shared_ptr<IWSSender> sender)
{
    m_coordinator = coordinator;
    m_sender = sender;

    m_dialplan_id = dialplan_id;
    m_phone = phone;
    m_client_id = client_id;
    const std::string dst_uri = "sip:" + phone + "@" + acc_.getHost();
    LOG_INFO("dst_uri: {}", dst_uri);
    const pj::CallOpParam prm {true};
    try {
        this->makeCall(dst_uri, prm);
    }
    catch (const pj::Error &err) {
        LOG_ERROR("make call error: {} {}", err.reason, err.info());
    }

    m_coordinator->waitForWinner();

    if (m_coordinator->shouldAbort(shared_from_this())) {
        LOG_WARN("should abort");
        hangup_();
    }

    m_coordinator->waitForCallFinished();
}

void voip::Caller::hangup_()
{
    pj::CallOpParam prm;
    this->hangup(prm);
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
            this->acc_.getId();
            this->acc_.getHost();
            this->acc_.getUser();
            // m_sender->send();
            break;
        }
        case PJSIP_INV_STATE_NULL: {
            LOG_INFO(">>> call: {}, phone: {} null", ci.id, m_phone);
            // 
            break;
        }
        case PJSIP_INV_STATE_CALLING: {
            LOG_INFO(">>> call: {}, phone: {} calling", ci.id, m_phone);
            break;
        }
        case PJSIP_INV_STATE_CONFIRMED: {
            LOG_INFO(">>> call: {}, phone: {} confirmed", ci.id, m_phone);
            // 当前线程如果已经接通了，通知其他线程挂断电话
            m_coordinator->notifyCallConfirmed(shared_from_this());
            break;
        }
        case PJSIP_INV_STATE_DISCONNECTED: {
            LOG_INFO(">>> call: {}, phone: {} disconnected", ci.id, m_phone);
            m_coordinator->notifyCallDisconnected(shared_from_this());
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

            cap_dev_med.startTransmit(*aud_med);
            // aud_med->startTransmit(*aud_media_recorder_); // 录音无噪音
            aud_med->startTransmit(play_dev_med); // 播放设备有明显电流声
        }
    }
}
