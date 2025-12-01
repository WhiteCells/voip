#ifndef _ACCOUNT_H_
#define _ACCOUNT_H_

#include "vcaller.h"
#include "logger.h"
#include <pjsua2.hpp>
#include <pjsua2/endpoint.hpp>

class Account : public pj::Account
{
public:
    Account(const std::string &user = "1000",
            const std::string &pass = "1000",
            const std::string &host = "192.168.10.51")
    {
        pj::Endpoint::instance().libRegisterThread("Account");
        m_auth_cred_info = pj::AuthCredInfo("digest", "*",
                                            user, 0, pass);
        m_acc_cfg.idUri = "sip:" + user + "@" + host;
        m_acc_cfg.regConfig.registrarUri = "sip:" + host;
        m_acc_cfg.sipConfig.authCreds.push_back(m_auth_cred_info);
        m_acc_cfg.callConfig.timerMinSESec = 90;
        m_acc_cfg.callConfig.timerSessExpiresSec = 1800;
        pj::Account::create(m_acc_cfg);
    }

    ~Account()
    {
        pj::Account::shutdown();
    }

    virtual void onIncomingCall(pj::OnIncomingCallParam &prm) override
    {
        LOG_INFO("incoming call {}, {}, {}", prm.rdata.info, prm.rdata.wholeMsg, prm.rdata.srcAddress);
        m_cur_caller = std::make_unique<VCaller>(*this, prm.callId);

        answerCall();
    }

    void answerCall()
    {
        if (m_cur_caller) {
            pj::CallOpParam prm;
            prm.statusCode = PJSIP_SC_OK;
            m_cur_caller->answer(prm);
            LOG_INFO("answer call {}", (unsigned)prm.statusCode);
        }
    }

private:
    pj::AuthCredInfo m_auth_cred_info;
    pj::AccountConfig m_acc_cfg;
    std::unique_ptr<VCaller> m_cur_caller;
};

#endif // _ACCOUNT_H_