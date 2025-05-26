#include "vaccount.h"
#include "caller.h"
#include "request.hpp"
#include "global.h"

#include <iostream>

voip::VAccount::VAccount(
    const std::string &user,
    const std::string &pass,
    const std::string &host) :
    m_user(user),
    m_pass(pass),
    m_host(host)
{
    m_auth_cred_info = pj::AuthCredInfo("digest", "*",
                                        user, 0, pass);
    m_acc_cfg.idUri = "sip:" + user + "@" + host;
    m_acc_cfg.regConfig.registrarUri = "sip:" + host;
    m_acc_cfg.sipConfig.authCreds.push_back(m_auth_cred_info);
    m_acc_cfg.callConfig.timerMinSESec = 90;
    m_acc_cfg.callConfig.timerSessExpiresSec = 1800;
    this->create(m_acc_cfg);
}

voip::VAccount::~VAccount()
{
}

void voip::VAccount::onRegState(pj::OnRegStateParam &prm)
{
    pj::AccountInfo ai = getInfo();
    LOG_INFO("code: {} reason: {} {}", static_cast<int>(prm.code), prm.reason, ai.uri);
    voip::pushRegStatus(m_user, REG_STATE::SUCCESSED, "todo");
}

// void voip::VAccount::onIncomingCall(pj::OnIncomingCallParam &iprm)
// {
//     pj::CallOpParam prm;

//     if (cur_call) {
//         std::cout << ">>> Another call is active. Rejecting incoming call ID "
//                   << iprm.callId << " from " << iprm.rdata.srcAddress << std::endl;

//         Caller *rejectCall = nullptr;
//         std::shared_ptr<Caller> reject_call;
//         try {
//             rejectCall = new Caller(*this, iprm.callId);
//             prm.statusCode = PJSIP_SC_BUSY_HERE;
//             rejectCall->hangup(prm);
//             delete rejectCall;
//             rejectCall = nullptr;
//         }
//         catch (pj::Error &err) {
//             std::cerr << ">>> error rejecting call ID " << iprm.callId << ": " << err.info() << std::endl;
//             if (rejectCall) {
//                 std::cerr << ">>> attempting cleanup of rejectCall object after rejection error." << std::endl;
//                 delete rejectCall;
//             }
//         }
//         return;
//     }

//     std::cout << ">>> incoming call: " << iprm.callId << " from " << iprm.rdata.srcAddress << std::endl;

//     Caller *call = nullptr;
//     try {
//         call = new Caller(*this, iprm.callId);
//         std::cout << ">>> auto-answering incoming call..." << std::endl;
//         prm.statusCode = PJSIP_SC_OK;
//         call->answer(prm);
//         cur_call = call;
//     }
//     catch (pj::Error &err) {
//         std::cerr << ">>> failed to create or answer call ID " << iprm.callId << ": " << err.info() << std::endl;
//         if (call) {
//             delete call;
//         }
//         cur_call = nullptr;
//     }
// }