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
    LOG_INFO("~Caller");
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

    // 超时之前等待 winner
    // 是 winner 在 waitForCallFinished 阻塞，直到通话结束
    // 非 winner 或者超时走挂断逻辑
    if (!m_coordinator->waitForWinner(std::chrono::seconds(10)) ||
        m_coordinator->shouldAbort(shared_from_this())) {
        LOG_WARN("call {} wait winner time out", m_phone);
        m_call_status = 2;
        hangup_();
        LOG_INFO("Caller::call phone {} call_status {} call_type {}", m_phone, m_call_status, call_type);
        voip::pushCallState(
            std::to_string(m_dialplan_id), // task_id
            m_phone,                       // phone
            m_call_status,                 // status (断开连接)
            call_type,                     // call_type
            "1"                            // hangup_direction
        );
        return;
    }

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
    if (!m_coordinator->waitForSingleCallConfirmed(std::chrono::seconds(10))) {
        LOG_WARN("call {} wait winner time out", m_phone);
        m_call_status = 2;
        hangup_();
        voip::pushCallState(
                std::to_string(m_dialplan_id), // task_id
                m_phone,                       // phone
                m_call_status,                 // status (断开连接)
                call_type,                     // call_type
                "1"                            // hangup_direction
        );
        return;
    }
    m_coordinator->waitForSingleCallFinished();
}

void voip::Caller::hangup_()
{
    pj::CallOpParam prm;
    prm.statusCode = PJSIP_SC_OK;
//    pj::Call::hangup(prm);
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
    if (ci.lastStatusCode == 404) {
        LOG_INFO("call last status code: {}", (int)ci.lastStatusCode);
        return;
    }
    if (!ci.lastReason.empty()) {
        LOG_INFO("call reason: {}", ci.lastReason);
    }

    switch (ci.state) {
        case PJSIP_INV_STATE_CONNECTING: {
            LOG_INFO(">>> call: {}, phone: {} connecting", ci.id, m_phone);
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
            // voip::pushCallState(
            //     std::to_string(m_dialplan_id), // task_id
            //     m_phone,                       // phone
            //     0,                             // status (通话中)
            //     call_type,                     // call_type
            //     ""                             // hangup_direction
            // );
            break;
        }
        case PJSIP_INV_STATE_NULL: {
            LOG_INFO(">>> call: {}, phone: {} null", ci.id, m_phone);
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
            // voip::pushCallState(
            //     std::to_string(m_dialplan_id), // task_id
            //     m_phone,                       // phone
            //     0,                             // status (通话中)
            //     call_type,                     // call_type
            //     ""                             // hangup_direction
            // );
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

            LOG_INFO(">>> pushCallState PJSIP_INV_STATE_CONFIRMED call: {}, phone: {}, call_type: {}",std::to_string(m_dialplan_id), m_phone, call_type);

            voip::pushCallState(
                std::to_string(m_dialplan_id), // task_id
                m_phone,                       // phone
                0,                             // status (已确认)
                call_type,                     // call_type
                ""                             // hangup_direction
            );
            break;
        }
        case PJSIP_INV_STATE_DISCONNECTED: {
            LOG_INFO(">>> call: {}, phone: {} disconnected", ci.id, m_phone);
            m_coordinator->notifyCallDisconnected(shared_from_this());

            if (local_hangup == "1") {
                // 主叫方挂断
                LOG_INFO("{}: 主叫方挂断", m_phone);
                hangup_direction = "1";
                local_hangup = "0";
            }
            else if (local_hangup == "0") {
                // 被叫方挂断
                LOG_INFO("{}: 被叫方挂断", m_phone);
                hangup_direction = "0";
            }
            LOG_INFO(">>>phone {},hangup_direction {}", m_phone, hangup_direction);

            std::string tmp_phone1 = m_phone;

            if (m_sender) {
                json::Value status_msg;
                status_msg["id"] = acc_.getUser();
                status_msg["phone"] = tmp_phone1;
                status_msg["status"] = "DISCONNECTED";

                Json::StreamWriterBuilder builder;
                builder["indentation"] = "";
                std::string msg = json::writeString(builder, status_msg);
                m_sender->send(msg);
            }

            if (m_call_status != 2) {
                m_call_status = 1;
            }

            LOG_INFO(">>> pushCallState PJSIP_INV_STATE_DISCONNECTED call: {}, phone: {}, status: {}, call_type: {}, hangup_direction: {}",std::to_string(m_dialplan_id), m_phone, m_call_status, call_type, hangup_direction);

            std::string tmp_phone = tmp_phone1;
            std::string tmp_hangup_direction = "0";

            voip::pushCallState(
                std::to_string(m_dialplan_id), // task_id
                tmp_phone,                       // phone
                m_call_status,                 // status (断开连接)
                call_type,                     // call_type
                tmp_hangup_direction               // hangup_direction
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
            aud_med->startTransmit(play_dev_med);
        }
    }
}
