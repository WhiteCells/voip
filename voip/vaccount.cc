#include "vaccount.h"
#include "vcall.h"

#include <iostream>
#include <memory>

voip::VAccount::VAccount()
{
}

voip::VAccount::~VAccount()
{
}

void voip::VAccount::onRegState(pj::OnRegStateParam &prm)
{
    pj::AccountInfo ai = getInfo();
    std::cout << (ai.regIsActive ? ">>> Registered:" : ">>> Unregistered:")
              << " code=" << prm.code
              << " reason=" << prm.reason
              << " (" << ai.uri << ")"
              << std::endl;
}

void voip::VAccount::onIncomingCall(pj::OnIncomingCallParam &iprm)
{
    pj::CallOpParam prm;

    if (cur_call) {
        std::cout << ">>> Another call is active. Rejecting incoming call ID "
                  << iprm.callId << " from " << iprm.rdata.srcAddress << std::endl;

        VCall *rejectCall = nullptr;
        std::shared_ptr<VCall> reject_call;
        try {
            rejectCall = new VCall(*this, iprm.callId);
            prm.statusCode = PJSIP_SC_BUSY_HERE;
            rejectCall->hangup(prm);
            delete rejectCall;
            rejectCall = nullptr;
        }
        catch (pj::Error &err) {
            std::cerr << ">>> error rejecting call ID " << iprm.callId << ": " << err.info() << std::endl;
            if (rejectCall) {
                std::cerr << ">>> attempting cleanup of rejectCall object after rejection error." << std::endl;
                delete rejectCall;
            }
        }
        return;
    }

    std::cout << ">>> incoming call: " << iprm.callId << " from " << iprm.rdata.srcAddress << std::endl;

    VCall *call = nullptr;
    try {
        call = new VCall(*this, iprm.callId);
        std::cout << ">>> auto-answering incoming call..." << std::endl;
        prm.statusCode = PJSIP_SC_OK;
        call->answer(prm);
        cur_call = call;
    }
    catch (pj::Error &err) {
        std::cerr << ">>> failed to create or answer call ID " << iprm.callId << ": " << err.info() << std::endl;
        if (call) {
            delete call;
        }
        cur_call = nullptr;
    }
}