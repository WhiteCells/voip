#include "global.h"

#include <thread>

pj::Endpoint endpoint;

unsigned thread_num = std::thread::hardware_concurrency();

// std::string backend_host = voip::cfg["HOST"];
std::string backend_host = "127.0.0.1";

// std::string backend_port = voip::cfg["PORT"];
std::string backend_port = "5000";

void startEndpointLib(unsigned port)
{
    endpoint.libCreate();

    pj::EpConfig ep_cfg;
    endpoint.libInit(ep_cfg);

    pj::TransportConfig ts_cfg;
    ts_cfg.port = port;
    endpoint.transportCreate(PJSIP_TRANSPORT_UDP, ts_cfg);

    endpoint.libStart();
}