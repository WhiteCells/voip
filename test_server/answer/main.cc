#include <pjsua2.hpp>
#include "account.hpp"
#include <iostream>

using namespace pj;

Endpoint endpoint;

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
}

int main()
{
    startEndpointLib(5060);

    try {
        MyAccount *acc = new MyAccount();

        AccountConfig acfg;
        acfg.idUri = "sip:1000@192.168.10.51";
        acfg.regConfig.registrarUri = "sip:192.168.10.51";

        AuthCredInfo cred("digest", "*", "1000", 0, "1000");
        acfg.sipConfig.authCreds.push_back(cred);

        acc->create(acfg);

        std::cout << "Account created. Waiting for calls..." << std::endl;

        while (true) {
            pj_thread_sleep(1000);
        }

        endpoint.libDestroy();
    }
    catch (Error &err) {
        std::cout << "Exception: " << err.info() << std::endl;
        return 1;
    }
}
