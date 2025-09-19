#include "global.h"
#include <thread>

std::string g_client_id;

pj::Endpoint endpoint;

unsigned g_thread_num = std::thread::hardware_concurrency();

std::string backend_host;

std::string backend_port;

std::string client_id;

std::string verify_file;

cfg_map cfg;

std::string local_hangup = "0";

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
