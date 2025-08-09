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
    call_type = 0;
    m_coordinator = coordinator;
    m_sender = sender;

    m_dialplan_id = dialplan_id;
    m_phone = phone;
    m_client_id = client_id;
    const std::string dst_uri = "sip:" + phone + "@" + acc_.getHost();
    LOG_INFO("dst_uri: {}", dst_uri);
    const pj::CallOpParam prm {true};
    // try {
    //     this->makeCall(dst_uri, prm);
    // }
    // catch (const pj::Error &err) {
    //     LOG_ERROR("make call error: {} {}", err.reason, err.info());
    // }

    try {
        this->makeCall(dst_uri, prm);
    }
    catch (const pj::Error &err) {
        LOG_ERROR("pj::Error: {} {}", err.reason, err.info());
    }
    catch (const std::exception &ex) {
        LOG_ERROR("std::exception: {}", ex.what());
    }
    catch (...) {
        LOG_ERROR("Unknown exception caught!");
    }

    m_coordinator->waitForWinner();
    if (!m_coordinator->waitForWinner(std::chrono::seconds(10)) || m_coordinator->shouldAbort(shared_from_this())) {
        LOG_WARN("call {} wait winner time out", m_phone);
        hangup_();
        return;
    }

    // if () {
    //     LOG_WARN("call {} should abort", m_phone);
    //     hangup_();
    //     return;
    // }

    m_coordinator->waitForCallFinished();
}

void voip::Caller::single_call(const std::string &phone,
                               const std::string &client_id,
                               const int dialplan_id,
                               std::shared_ptr<Coordinator> coordinator,
                               std::shared_ptr<IWSSender> sender)
{
    call_type = 1;
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
    m_coordinator->waitForCallFinished();
}

void voip::Caller::hangup_()
{
    pj::CallOpParam prm;
    prm.statusCode = PJSIP_SC_OK;
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

            if (m_sender) {
                json::Value status_msg;
                status_msg["id"] = acc_.getUser();
                status_msg["phone"] = m_phone;
                status_msg["status"] = "CONNECTING";

                Json::StreamWriterBuilder builder;
                builder["indentation"] = "";
                std::string msg = json::writeString(builder, status_msg);
                m_sender->send(msg);
            }

            voip::pushCallState(
                    std::to_string(m_dialplan_id),  // task_id
                    m_phone,                        // phone
                    0,                         // status (通话中)
                    call_type,                              // call_type
                    ""                       // hangup_direction
            );

            break;
        }
        case PJSIP_INV_STATE_NULL: {
            LOG_INFO(">>> call: {}, phone: {} null", ci.id, m_phone);
            m_coordinator->notifyCallConfirmed(shared_from_this());
            if (m_sender) {
                json::Value status_msg;
                status_msg["id"] = acc_.getUser();
                status_msg["phone"] = m_phone;
                status_msg["status"] = "NULL";

                Json::StreamWriterBuilder builder;
                builder["indentation"] = "";
                std::string msg = json::writeString(builder, status_msg);
                m_sender->send(msg);
            }
            break;
        }
        case PJSIP_INV_STATE_CALLING: {
            LOG_INFO(">>> call: {}, phone: {} calling", ci.id, m_phone);

            if (m_sender) {
                json::Value status_msg;
                status_msg["id"] = acc_.getUser();
                status_msg["phone"] = m_phone;
                status_msg["status"] = "CALLING";

                Json::StreamWriterBuilder builder;
                builder["indentation"] = "";
                std::string msg = json::writeString(builder, status_msg);
                m_sender->send(msg);
            }
            voip::pushCallState(
                    std::to_string(m_dialplan_id),  // task_id
                    m_phone,                        // phone
                    0,                         // status (通话中)
                    call_type,                      // call_type
                    ""                       // hangup_direction
            );

            break;
        }
        case PJSIP_INV_STATE_CONFIRMED: {
            LOG_INFO(">>> call: {}, phone: {} confirmed", ci.id, m_phone);
            // 当前线程如果已经接通了，通知其他线程挂断电话
            m_coordinator->notifyCallConfirmed(shared_from_this());

            if (m_sender) {
                json::Value status_msg;
                status_msg["id"] = acc_.getUser();
                status_msg["phone"] = m_phone;
                status_msg["status"] = "CONFIRMED";

                Json::StreamWriterBuilder builder;
                builder["indentation"] = "";
                std::string msg = json::writeString(builder, status_msg);
                m_sender->send(msg);
            }

            voip::pushCallState(
                    std::to_string(m_dialplan_id),  // task_id
                    m_phone,                        // phone
                    0,                      // status (已确认)
                    call_type,                              // call_type
                    ""                       // hangup_direction
            );
            break;
        }
        case PJSIP_INV_STATE_DISCONNECTED: {
            LOG_INFO(">>> call: {}, phone: {} disconnected", ci.id, m_phone);
            // if (!m_coordinator->isWinner(shared_from_this())) {
            //     LOG_WARN(">>> call: {}, phone: {} DISCONNECTED without CONFIRMED, doing fallback confirm", ci.id, m_phone);
            //     m_coordinator->notifyCallConfirmed(shared_from_this()); // 补偿触发
            // }
            m_coordinator->notifyCallDisconnected(shared_from_this());
            if (m_sender) {
                json::Value status_msg;
                status_msg["id"] = acc_.getUser();
                status_msg["phone"] = m_phone;
                status_msg["status"] = "DISCONNECTED";

                Json::StreamWriterBuilder builder;
                builder["indentation"] = "";
                std::string msg = json::writeString(builder, status_msg);
                m_sender->send(msg);
            }

            int status_code = 1; // 默认为挂断

            // 根据 lastStatusCode 判断具体的断开原因
            if (ci.lastStatusCode == PJSIP_SC_REQUEST_TIMEOUT ||
                ci.lastStatusCode == PJSIP_SC_TEMPORARILY_UNAVAILABLE ||
                ci.lastStatusCode == PJSIP_SC_NOT_FOUND) {
                // 可能是无人接听的情况
                status_code = 2; // 无人接听
            } else if (ci.lastStatusCode == PJSIP_SC_OK || ci.lastStatusCode == PJSIP_SC_BUSY_HERE) {
                // 正常挂断或忙线
                status_code = 1; // 挂断
            }

            voip::pushCallState(
                    std::to_string(m_dialplan_id),  // task_id
                    m_phone,                        // phone
                    status_code,                         // status (断开连接)
                    call_type,                              // call_type
                    "hangup_direction"                      // hangup_direction
            );

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
