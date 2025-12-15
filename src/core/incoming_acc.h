#pragma once

#include "../logger.h"
#include "incoming_call.h"
#include <pjsua2.hpp>
#include <string>

class IncomingAcc : public pj::Account
{
public:
    IncomingAcc(const std::string &user,
                const std::string &pass,
                const std::string &host)
        : m_user(user)
        , m_pass(pass)
        , m_host(host)
    {
        LOG_INFO("register IncomingAcc: {} {} {}", m_user, m_pass, m_host);
        pj::Endpoint::instance().libRegisterThread("incoming_acc");
        auto auth_cred_info = pj::AuthCredInfo("digest", "*", m_user, 0, m_pass);
        pj::AccountConfig acc_cfg;
        acc_cfg.idUri = "sip:" + user + "@" + host;
        acc_cfg.regConfig.registrarUri = "sip:" + host;
        acc_cfg.sipConfig.authCreds.push_back(auth_cred_info);
        acc_cfg.callConfig.timerMinSESec = 90;
        acc_cfg.callConfig.timerSessExpiresSec = 1800;
        pj::Account::create(acc_cfg);
        LOG_INFO("IncomingAcc created");
    }

    ~IncomingAcc()
    {
        LOG_INFO("~IncomingAcc: {} {} {}", m_user, m_pass, m_host);
        pj::Account::shutdown();
    }

    virtual void onIncomingCall(pj::OnIncomingCallParam &prm) override
    {
        LOG_INFO("incoming call {}, {}, {}", prm.rdata.info, prm.rdata.wholeMsg, prm.rdata.srcAddress);
        // m_incoming_call = std::make_shared<IncomingCall>(*this, prm.callId);
        // todo 通知 SIPCore 接听电话
    }

    virtual void onRegState(pj::OnRegStateParam &prm) override
    {
        LOG_INFO("user {}, reg state {}, {}, {}", m_user, (unsigned)prm.code, prm.reason, prm.expiration);
        // todo 通知 UI 登陆状态
    }

    void answer()
    {
        if (m_incoming_call) {
            pj::CallOpParam prm;
            prm.statusCode = PJSIP_SC_OK;
            m_incoming_call->answer(prm);
            LOG_INFO("answer call code: {}", (unsigned)prm.statusCode);
        }
    }

private:
    std::string m_user;
    std::string m_pass;
    std::string m_host;
    std::shared_ptr<IncomingCall> m_incoming_call;
};