#pragma once

#include "../logger.h"
#include "check_acc_mgr.h"
#include <pjsua2.hpp>
#include <memory>
#include <string>

class AccountCheck :
    public std::enable_shared_from_this<AccountCheck>,
    public pj::Account
{
public:
    AccountCheck(const std::string &id,
                 const std::string &user,
                 const std::string &pass,
                 const std::string &host)
        : m_id(id)
        , m_user(user)
        , m_pass(pass)
        , m_host(host)
    {
        LOG_INFO("AccountCheck");
        m_auth_cred_info = pj::AuthCredInfo("digest", "*",
                                            user, 0, pass);
        m_acc_cfg.idUri = "sip:" + user + "@" + host;
        m_acc_cfg.regConfig.registrarUri = "sip:" + host;
        m_acc_cfg.sipConfig.authCreds.push_back(m_auth_cred_info);
        m_acc_cfg.callConfig.timerMinSESec = 90;
        m_acc_cfg.callConfig.timerSessExpiresSec = 1800;
        pj::Account::create(m_acc_cfg);
        LOG_INFO("AccountCheck created");
    }

    ~AccountCheck();

    virtual void onRegState(pj::OnRegStateParam &prm) override
    {
        LOG_INFO("on reg state");
        AccountCheckManager::getInstance()->onAccountRegState(shared_from_this(),
                                                              m_id,
                                                              static_cast<int>(prm.code),
                                                              prm.reason);
    }

    std::string getId() const;
    std::string getUser() const;
    std::string getPass() const;
    std::string getHost() const;

private:
    std::string m_id;
    std::string m_user;
    std::string m_pass;
    std::string m_host;
    pj::AuthCredInfo m_auth_cred_info;
    pj::AccountConfig m_acc_cfg;
};
