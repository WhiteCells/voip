#include "global.h"

pj::Endpoint endpoint;

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