#include "vaccount.h"
#include "vcall.h"

#include <pjsua2.hpp>
#include <memory>
#include <iostream>
#include <limits>

#define SIP_USER      "1003"
#define SIP_DOMAIN    "192.168.10.51:5060"
#define SIP_PASSWORD  "1003"
#define SIP_REGISTRAR "sip:" SIP_DOMAIN

int main()
{
    pj::Endpoint ep;
    std::unique_ptr<voip::VAccount> acc;

    try {
        std::cout << "initializing Endpoint" << std::endl;
        ep.libCreate();
        pj::EpConfig ep_cfg;
        ep.libInit(ep_cfg);

        pj::TransportConfig tcfg;
        tcfg.port = 5060;
        ep.transportCreate(PJSIP_TRANSPORT_UDP, tcfg);

        ep.libStart();
        std::cout << "Pjsua2 library start" << std::endl;

        try {
            pj::AudDevManager &mgr = ep.audDevManager();
            if (mgr.getDevCount() > 0) {
                std::cout << ">>> default capture device: " << mgr.getCaptureDev() << std::endl;
                std::cout << ">>> default playback device: " << mgr.getPlaybackDev() << std::endl;
            }
            else {
                std::cout << ">>> no audio devices found. Using NULL audio device" << std::endl;
                ep.audDevManager().setNullDev();
            }
        }
        catch (const pj::Error &err) {
            std::cerr << ">>> error setting audio devices: " << err.info() << std::endl;
            try {
                ep.audDevManager().setNullDev();
            }
            catch (...) {
            }
        }

        pj::AccountConfig acc_cfg;
        acc_cfg.idUri = "sip:" SIP_USER "@" SIP_DOMAIN;
        acc_cfg.regConfig.registrarUri = SIP_REGISTRAR;
        pj::AuthCredInfo cred("digest", "*", SIP_USER, 0, SIP_PASSWORD);
        acc_cfg.sipConfig.authCreds.push_back(cred);

        acc = std::make_unique<voip::VAccount>();
        acc->create(acc_cfg);
        std::cout << "*** Account created for " << acc_cfg.idUri << ". Registering..." << std::endl;

        std::cout << "\nCommands:\n";
        std::cout << "  m <sip:user@domain>  : 拨号\n";
        std::cout << "  h                    : 挂断\n";
        std::cout << "  q                    : 退出\n\n";

        char cmd[100];
        while (true) {
            std::cout << "> ";
            if (!std::cin.getline(cmd, sizeof(cmd))) {
                if (std::cin.eof())
                    break;
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cerr << ">>> input error. please try again" << std::endl;
                continue;
            }

            std::string command_line = cmd;
            if (command_line.empty())
                continue;

            char action = command_line[0];

            if (action == 'q') {
                break;
            }
            else if (action == 'm') {
                if (acc->cur_call) {
                    std::cerr << ">>> cannot make a new call. A call is already active." << std::endl;
                    continue;
                }
                if (command_line.length() < 3 || command_line[1] != ' ') {
                    std::cerr << ">>> invalid format. Use: m <sip:user@domain>" << std::endl;
                    continue;
                }
                std::string target_uri = command_line.substr(2);
                std::cout << ">>> placing call to: " << target_uri << std::endl;

                voip::VCall *call = new voip::VCall(*acc);
                pj::CallOpParam prm(true);

                try {
                    call->makeCall(target_uri, prm);
                    acc->cur_call = call;
                }
                catch (const pj::Error &err) {
                    std::cerr << ">>> failed to make call: " << err.info() << std::endl;
                    delete call;
                }
            }
            else if (action == 'h') {
                if (!acc->cur_call) {
                    std::cerr << ">>> no active call to hang up" << std::endl;
                    continue;
                }
                std::cout << ">>> hanging up call" << std::endl;
                pj::CallOpParam prm;
                try {
                    acc->cur_call->hangup(prm);
                }
                catch (const pj::Error &err) {
                    std::cerr << ">>> failed to hang up call: " << err.info() << std::endl;
                }
            }
            else {
                std::cerr << ">>> unknown command: " << action << std::endl;
            }

            if (acc->cur_call) {
                try {
                    pj::CallInfo ci = acc->cur_call->getInfo();
                    if (ci.state == PJSIP_INV_STATE_DISCONNECTED) {
                        std::cout << ">>> detected disconnected call in main loop, attempting cleanup." << std::endl;
                        delete acc->cur_call;
                        acc->cur_call = nullptr;
                    }
                }
                catch (const pj::Error &err) {
                    std::cerr << ">>> error checking call state in main loop (might be already deleted): " << err.info() << std::endl;
                    acc->cur_call = nullptr;
                }
            }
        }

        std::cout << "shutting down" << std::endl;
        if (acc->cur_call) {
            std::cout << ">>> hanging up active call before exit..." << std::endl;
            pj::CallOpParam prm;
            try {
                acc->cur_call->hangup(prm);
            }
            catch (const pj::Error &err) {
            }
            pj_thread_sleep(500);
            delete acc->cur_call;
            acc->cur_call = nullptr;
        }

        acc.reset();

        ep.libDestroy();
        std::cout << "Pjsua2 library destroy" << std::endl;
    }
    catch (const pj::Error &err) {
        std::cerr << "[Exception]: " << err.info() << std::endl;
        try {
            if (ep.libGetState() != PJSUA_STATE_NULL) {
                ep.libDestroy();
            }
        }
        catch (...) {
        }
        return 1;
    }

    return 0;
}