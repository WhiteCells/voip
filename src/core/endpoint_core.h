#pragma once

#include <pjsua2.hpp>

void startEndpoint(unsigned port = 5060)
{
    static pj::Endpoint endpoint;

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
}