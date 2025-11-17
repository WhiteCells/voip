#include "global.h"
#include <thread>

std::string g_client_id;

pj::Endpoint endpoint;

unsigned g_thread_num = std::thread::hardware_concurrency();

std::string backend_host;

std::string backend_port;

std::string client_id;

std::string backend_verify_file;

std::string reminder_consumer_remote_host;

std::string reminder_consumer_remote_port;

std::string reminder_mediator_remote_host;

std::string reminder_mediator_remote_port;

std::string robot_remote_host;

std::string robot_remote_port;

std::string asr_server_remote_host;

std::string asr_server_remote_port;

std::string asr_server_verify_file;

std::string agent_session_remote_host;

std::string agent_session_remote_port;

std::string agent_session_remote_target;

std::string agent_session_verify_file;

std::string tts_server_remote_host;

std::string tts_server_remote_port;

std::string tts_server_remote_target;

cfg_map cfg;

std::string local_hangup = "customer";

std::shared_ptr<AgentWsClient> g_agent_ws_client;

std::shared_ptr<AgentWsClient> g_manual_ws_client;

std::unique_ptr<TTSThreadPool> g_tts_thread_pool = nullptr;

EventBus g_event_bus;

void startEndpointLib(unsigned port)
{
    endpoint.libCreate();

    pj::EpConfig ep_cfg;
    ep_cfg.medConfig.sndClockRate = 16000;
    ep_cfg.medConfig.channelCount = 1;
    ep_cfg.medConfig.clockRate = 16000;
    ep_cfg.medConfig.sndAutoCloseTime = -1;
    ep_cfg.uaConfig.maxCalls = 1600;
    ep_cfg.uaConfig.threadCnt = 5;
    ep_cfg.logConfig.level = 5;

    endpoint.libInit(ep_cfg);

    pj::TransportConfig ts_cfg;
    ts_cfg.port = port;
    endpoint.transportCreate(PJSIP_TRANSPORT_UDP, ts_cfg);

    endpoint.libStart();

    encoder = opus_encoder_create(16000, 1, OPUS_APPLICATION_VOIP, nullptr);
    decoder = opus_decoder_create(16000, 1, nullptr);
}

GUIConfig g_gui_cfg;

std::string g_task_id;

OpusEncoder *encoder;
OpusDecoder *decoder;

// std::condition_variable m_confirmed_cv;
// std::mutex m_confirmed_mtx;
std::atomic<bool> m_confirmed {false};