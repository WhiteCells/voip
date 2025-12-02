#pragma once

#include <pjsua2.hpp>
#include <pjsua2/call.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <string>
#include "agent_robot_audiomediaport.h"
#include "logger.h"
#include "global.h"
#include "agent_ws_client.h"
#include "register_request.h"


class VCaller : public pj::Call
{
public:
    VCaller(pj::Account &acc, int call_id = PJSUA_INVALID_ID)
        : pj::Call(acc, call_id)
        ,m_register_request(std::make_shared<RegisterRequest>("192.168.10.5", "8000", "/session/reg"))
    {
    }
    VCaller(const VCaller &) = delete;
    VCaller &operator=(const VCaller &) = delete;
    ~VCaller() = default;

    virtual void onCallState(pj::OnCallStateParam &prm) override
    {
        PJ_UNUSED_ARG(prm);

        pj::CallInfo ci = getInfo();
        LOG_INFO("call id: {} state: {} code:{}",
                 ci.id, ci.stateText, (int)ci.lastStatusCode);
        if (ci.lastStatusCode == 404) {
            LOG_ERROR("call last status code: {}", (int)ci.lastStatusCode);
            return;
        }
        if (!ci.lastReason.empty()) {
            LOG_INFO("call reason: {}", ci.lastReason);
        }

        switch (ci.state) {
            case PJSIP_INV_STATE_INCOMING: {
                std::string uuid = uuid_v4();
                std::string response = m_register_request->Request(uuid, "", "call", "single");
                if (response.empty()) {
                    return;
                }
                Json::Value response_json;
                Json::CharReaderBuilder response_builder;
                std::unique_ptr<Json::CharReader> response_reader(response_builder.newCharReader());
                std::string response_errors;

                if (response_reader->parse(response.c_str(), response.c_str() + response.size(), &response_json, &response_errors)){
                    if (response_json.isMember("data") && response_json["data"].isObject()) {
                        if (response_json["data"].isMember("access_token"))
                        {
                            std::string access_token = response_json["data"]["access_token"].asString();
                            g_agent_ws_client->get_session_id("incoming_agent", uuid, access_token);
                        }
                    }
                }
                break;
            }
            case PJSIP_INV_STATE_CONFIRMED: {
                m_confirmed = true;
                g_agent_ws_client->start_config_send(); //   发送asr启动配置
                g_agent_ws_client->m_is_hangup = false;
                pj::OnCallMediaStateParam prm_{};
                onCallMediaState(prm_);
                break;
            }
            case PJSIP_INV_STATE_DISCONNECTED: {
                g_agent_ws_client->end_config_send(); // 发送asr结束配置
                g_agent_ws_client->clear_llm_msg_list();
                g_agent_ws_client->m_is_hangup = true;
                break;
            }
            default: {
                break;
            }
        }
    }

    virtual void onCallMediaState(pj::OnCallMediaStateParam &prm) override
    {
        PJ_UNUSED_ARG(prm);

        // todo
        pj::CallInfo ci = getInfo();
        for (unsigned i = 0; i < ci.media.size(); i++) {
            if (ci.media[i].type == PJMEDIA_TYPE_AUDIO &&
                getMedia(i) != nullptr) {
                pj::AudioMedia *aud_med = (pj::AudioMedia *)getMedia(i);
                pj::AudDevManager &mgr = pj::Endpoint::instance().audDevManager();
                mgr.getPlaybackDevMedia().adjustTxLevel(2.0);
                mgr.getCaptureDevMedia().adjustTxLevel(2.0);
//                aud_med->startTransm it(mgr.getPlaybackDevMedia());
//                mgr.getCaptureDevMedia().startTransmit(*aud_med);
                LOG_INFO("start transmit");
                m_agent_robot_media_port = std::make_shared<AgentRobotAudioMediaPort>();
                AgentRobotAudioMediaPort::startEndFlagMonitor(m_agent_robot_media_port);
                aud_med->startTransmit(*m_agent_robot_media_port);
                m_agent_robot_media_port->startTransmit(*aud_med);
            }
        }
    }

    std::string uuid_v4()
    {
        static boost::uuids::random_generator_mt19937 gen;
        return boost::uuids::to_string(gen());
    }


private:
    std::shared_ptr<AgentRobotAudioMediaPort> m_agent_robot_media_port;
    std::shared_ptr<RegisterRequest> m_register_request;
};