#include "vaccount.h"
#include "caller.h"
#include "logger.h"

voip::VAccount::VAccount(const std::string &id,
                         const std::string &user,
                         const std::string &pass,
                         const std::string &host)
    : m_id(id)
    , m_user(user)
    , m_pass(pass)
    , m_host(host)
{
    LOG_INFO("register Account: {} {} {} {}", id, user, pass, host);
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
    LOG_INFO("~VAccount");
    pj::Account::shutdown();
}

void voip::VAccount::onRegState(pj::OnRegStateParam &prm)
{
    pj::AccountInfo ai = getInfo();
    LOG_INFO("code: {} reason: {} {}", static_cast<int>(prm.code), prm.reason, ai.uri);
    if (m_acc_result) {
        m_acc_result->m_code = prm.code;
        m_acc_result->m_msg = prm.reason;
        LOG_INFO("m_acc_result: {}, m_acc_result: {}", m_acc_result->m_code, m_acc_result->m_msg);
    }
}

void voip::VAccount::create_()
{
    try {
        this->create(m_acc_cfg);
        LOG_INFO("account::create");
    }
    catch (const pj::Error &err) {
        LOG_ERROR("create err: {}", err.info());
    }
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