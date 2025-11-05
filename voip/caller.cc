#include "caller.h"
#include "vaccount.h"
#include "request.hpp"
#include "logger.h"
#include "request.hpp"
#include "global.h"
#include "coordinator.h"
#include "ws_interface.h"
#include "agent_ws_client.h"
#include <chrono>

voip::Caller::Caller(voip::VAccount &acc, int call_id)
    : pj::Call(acc, call_id)
    , acc_(acc)
{
}

voip::Caller::~Caller()
{
    LOG_INFO("~Caller");
}

void voip::Caller::group_call(const std::string &phone,
                              const std::string &client_id,
                              const int dialplan_id,
                              std::shared_ptr<Coordinator> coordinator,
                              std::shared_ptr<IWSSender> sender,
                              const std::string &call_method,
                              const std::string &different)
{
    call_type = "group";
    m_coordinator = coordinator;
    m_sender = sender;
    m_dialplan_id = dialplan_id;
    m_phone = phone;
    m_client_id = client_id;
    m_call_method = call_method;
    m_different = different;
    const std::string dst_uri = "sip:" + phone + "@" + acc_.getHost();
    LOG_INFO("dst_uri: {}", dst_uri);
    const pj::CallOpParam prm {true};

    try {
        this->makeCall(dst_uri, prm);
    }
    catch (const pj::Error &err) {
        LOG_ERROR("pj::Error: {} {}", err.reason, err.info());
    }

    // 超时之前等待 winner
    // 是 winner 在 waitForCallFinished 阻塞，直到通话结束
    // 非 winner 或者超时走挂断逻辑
    if (!m_coordinator->waitForWinner(std::chrono::seconds(10)) ||
        m_coordinator->shouldAbort(shared_from_this())) {

        if (m_sender) {
            Json::Value status_msg;
            status_msg["id"] = g_task_id;
            status_msg["phone"] = m_phone;
            status_msg["status"] = "DISCONNECTED";

            Json::StreamWriterBuilder builder;
            builder["indentation"] = "";
            std::string msg = Json::writeString(builder, status_msg);
            m_sender->send(msg);
        }

        LOG_WARN("call {} wait winner time out", m_phone);
        m_call_status = "no_answered";
        LOG_INFO("Caller::call phone {} call_status {} call_type {}", m_phone, m_call_status, call_type);
        voip::pushCallState(m_phone,
                            m_call_status,
                            call_type,
                            "mediator",
                            m_call_method,
                            m_different);
        return;
    }
    m_coordinator->waitForCallFinished();
}

void voip::Caller::single_call(const std::string &phone,
                               const std::string &client_id,
                               const int dialplan_id,
                               std::shared_ptr<Coordinator> coordinator,
                               std::shared_ptr<IWSSender> sender,
                               const std::string &call_method,
                               const std::string &different)
{
    call_type = "single";
    m_coordinator = coordinator;
    m_sender = sender;
    m_dialplan_id = dialplan_id;
    m_phone = phone;
    m_client_id = client_id;
    m_call_method = call_method;
    m_different = different;
    const std::string dst_uri = "sip:" + phone + "@" + acc_.getHost();
    LOG_INFO("dst_uri: {}", dst_uri);
    const pj::CallOpParam prm {true};

    try {
        this->makeCall(dst_uri, prm);
    }
    catch (const pj::Error &err) {
        LOG_ERROR("pj::Error: {} {}", err.reason, err.info());
    }
    if (!m_coordinator->waitForSingleCallConfirmed(std::chrono::seconds(10))) {

        if (m_sender) {
            Json::Value status_msg;
            status_msg["id"] = g_task_id;
            status_msg["phone"] = m_phone;
            status_msg["status"] = "DISCONNECTED";

            Json::StreamWriterBuilder builder;
            builder["indentation"] = "";
            std::string msg = Json::writeString(builder, status_msg);
            m_sender->send(msg);
        }

        LOG_WARN("call {} wait winner time out", m_phone);
        m_call_status = "no_answered";
        voip::pushCallState(m_phone,
                            m_call_status,
                            call_type,
                            "mediator",
                            m_call_method,
                            m_different);
        return;
    }
    m_coordinator->waitForSingleCallFinished();
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
                Json::Value status_msg;
                status_msg["id"] = g_task_id;
                status_msg["phone"] = m_phone;
                status_msg["status"] = "CONNECTING";

                Json::StreamWriterBuilder builder;
                builder["indentation"] = "";
                std::string msg = Json::writeString(builder, status_msg);
                m_sender->send(msg);
            }
            break;
        }
        case PJSIP_INV_STATE_NULL: {
            LOG_INFO(">>> call: {}, phone: {} null", ci.id, m_phone);
            if (m_sender) {
                Json::Value status_msg;
                status_msg["id"] = g_task_id;
                status_msg["phone"] = m_phone;
                status_msg["status"] = "NULL";

                Json::StreamWriterBuilder builder;
                builder["indentation"] = "";
                std::string msg = Json::writeString(builder, status_msg);
                m_sender->send(msg);
            }
            break;
        }
        case PJSIP_INV_STATE_CALLING: {
            LOG_INFO(">>> call: {}, phone: {} calling", ci.id, m_phone);
            if (m_sender) {
                Json::Value status_msg;
                status_msg["id"] = g_task_id;
                status_msg["phone"] = m_phone;
                status_msg["status"] = "CALLING";

                Json::StreamWriterBuilder builder;
                builder["indentation"] = "";
                std::string msg = Json::writeString(builder, status_msg);
                m_sender->send(msg);
            }
            break;
        }
        case PJSIP_INV_STATE_CONFIRMED: {
            LOG_INFO(">>> call: {}, phone: {} confirmed", ci.id, m_phone);
            m_confirmed = true;
            LOG_INFO("set m_confirmed = true");
            // 当前线程如果已经接通了，通知其他线程挂断电话
            m_coordinator->notifyCallConfirmed(shared_from_this());
            if (m_sender) {
                Json::Value status_msg;
                status_msg["id"] = g_task_id;
                status_msg["phone"] = m_phone;
                status_msg["status"] = "CONFIRMED";

                Json::StreamWriterBuilder builder;
                builder["indentation"] = "";
                std::string msg = Json::writeString(builder, status_msg);
                m_sender->send(msg);
            }

            if (g_agent_ws_client && g_manual_ws_client) {
                g_agent_ws_client->start_config_send(); //   发送asr启动配置
                if (m_call_method == "manual") {
                    g_manual_ws_client->start_config_send();
                }
            }
            else {
                LOG_ERROR("m_agent_ws_client is null");
            }

            LOG_INFO(">>> pushCallState PJSIP_INV_STATE_CONFIRMED call: {}, phone: {}, call_type: {}", std::to_string(m_dialplan_id), m_phone, call_type);

            voip::pushCallState(m_phone,
                                "connect",
                                call_type,
                                "",
                                m_call_method,
                                m_different);
            break;
        }
        case PJSIP_INV_STATE_DISCONNECTED: {
            LOG_INFO(">>> call: {}, phone: {} disconnected", ci.id, m_phone);
            m_confirmed = false;
            LOG_INFO("set m_confirmed = false");
            m_coordinator->notifyCallDisconnected(shared_from_this());

            if (local_hangup == "mediator") {
                // 主叫方挂断
                LOG_INFO("{}: 主叫方挂断", m_phone);
                hangup_direction = "mediator";
                local_hangup = "customer";
            }
            else if (local_hangup == "customer") {
                // 被叫方挂断
                LOG_INFO("{}: 被叫方挂断", m_phone);
                hangup_direction = "customer";
            }
            LOG_INFO(">>>phone {},hangup_direction {}", m_phone, hangup_direction);

            std::string phone = m_phone;
            std::string type = call_type;
            std::string direction = hangup_direction;
            std::string method = m_call_method;
            std::string diff = m_different;

//            std::string tmp_phone1 = m_phone;
//            std::string hangup_direction1 = hangup_direction;

            if (m_sender) {
                Json::Value status_msg;
                status_msg["id"] = g_task_id;
                status_msg["phone"] = phone;
                status_msg["status"] = "DISCONNECTED";

                Json::StreamWriterBuilder builder;
                builder["indentation"] = "";
                std::string msg = Json::writeString(builder, status_msg);
                m_sender->send(msg);
            }

            if (m_call_status != "no_answered") {
                m_call_status = "hangup";
            }
            std::string status= m_call_status;

            if (g_agent_ws_client && g_manual_ws_client) {

                g_agent_ws_client->end_config_send(); // 发送asr结束配置
                g_agent_ws_client->clear_llm_msg_list();

                if (m_call_method == "manual") {
                    g_manual_ws_client->end_config_send();
                    g_agent_ws_client->clear_llm_msg_list();
                }
            }
            else {
                LOG_INFO("m_agent_ws_client is null");
            }

            LOG_INFO(">>> pushCallState PJSIP_INV_STATE_DISCONNECTED call: {}, phone: {}, status: {}, call_type: {}, hangup_direction: {}", std::to_string(m_dialplan_id), m_phone, m_call_status, call_type, hangup_direction);

//            std::string tmp_phone = tmp_phone1;
//            std::string tmp_hangup_direction = hangup_direction1;

            voip::pushCallState(phone,
                                status,
                                type,
                                direction,
                                method,
                                diff);
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

#ifdef REMINDER
            if (m_call_method == "manual") {
                m_agent_aud_media_port = std::make_shared<AgentAudAudioMediaPort>();
                m_agent_cap_media_port = std::make_shared<AgentCapAudioMediaPort>();
                // interact with ai
                aud_med->startTransmit(*m_agent_aud_media_port);
                cap_dev_med.startTransmit(*m_agent_cap_media_port);
                // interact with local audio device
                aud_med->startTransmit(play_dev_med);
                cap_dev_med.startTransmit(*aud_med);
            }
            else if (m_call_method == "agent") {
                m_agent_robot_media_port = std::make_shared<AgentRobotAudioMediaPort>();
                aud_med->startTransmit(*m_agent_robot_media_port);
                m_agent_robot_media_port->startTransmit(*aud_med);
            }
#elif ROBOT
            aud_med->startTransmit(*m_agent_robot_media_port);
            m_agent_robot_media_port->startTransmit(*aud_med);
#endif
        }
    }

    // aud_med->startTransmit(*m_audio_media_recorder);
    //    m_agent_robot_media_port->startTransmit(*m_audio_media_recorder);
}
