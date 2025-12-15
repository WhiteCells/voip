#pragma once

#include "../logger.h"
#include <pjsua2.hpp>
#include <memory>

class OutcomingAcc :
    public pj::Account,
    public std::enable_shared_from_this<OutcomingAcc>
{
public:
    OutcomingAcc(const std::string &id,
                 const std::string &user,
                 const std::string &pass,
                 const std::string &host)
        : m_id(id)
        , m_user(user)
        , m_pass(pass)
        , m_host(host)
    {
        LOG_INFO("register OutcomingAcc: {} {} {} {}", m_id, m_user, m_pass, m_host);
        pj::AuthCredInfo auth_cred_info = pj::AuthCredInfo("digest", "*", user, 0, pass);
        pj::AccountConfig acc_cfg;
        acc_cfg.idUri = "sip:" + user + "@" + host;
        acc_cfg.regConfig.registrarUri = "sip:" + host;
        acc_cfg.sipConfig.authCreds.push_back(auth_cred_info);
        acc_cfg.callConfig.timerMinSESec = 90;
        acc_cfg.callConfig.timerSessExpiresSec = 1800;
        pj::Account::create(acc_cfg);
        LOG_INFO("OutcomingAcc created");
    }

    ~OutcomingAcc()
    {
        LOG_INFO("~OutcomingAcc {} {} {}", m_id, m_user, m_host);
        pj::Account::shutdown();
    }

    virtual void onRegState(pj::OnRegStateParam &prm) override
    {
        LOG_INFO("code: {} reason: {}", static_cast<int>(prm.code), prm.reason);
        // OutcomingAccMgr::getInstance()->onAccountRegState(shared_from_this(), prm.code, prm.reason);
    }

    std::string getId() const { return m_id; }
    std::string getHost() const { return m_host; }
    std::string getUri() const { return "sip:" + m_user + "@" + m_host; }

private:
    std::string m_id;
    std::string m_user;
    std::string m_pass;
    std::string m_host;
};