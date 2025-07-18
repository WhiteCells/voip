#include "global.h"

#include <thread>

std::string g_client_id;

pj::Endpoint endpoint;

unsigned g_thread_num = std::thread::hardware_concurrency();

std::string backend_host;

std::string backend_port;

cfg_map cfg;

void startEndpointLib(unsigned port)
{
    endpoint.libCreate();

    pj::EpConfig ep_cfg;
    ep_cfg.medConfig.clockRate = 48000;
    ep_cfg.uaConfig.maxCalls = 16;
    ep_cfg.uaConfig.threadCnt = 16;
    ep_cfg.logConfig.level = 5;

    endpoint.libInit(ep_cfg);
    // endpoint.audDevManager().setNullDev();

    pj::TransportConfig ts_cfg;
    ts_cfg.port = port;
    endpoint.transportCreate(PJSIP_TRANSPORT_UDP, ts_cfg);

    endpoint.libStart();
}