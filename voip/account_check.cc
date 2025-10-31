#include "account_check.h"
#include "logger.h"
#include "account_check_manager.h"

AccountCheck::AccountCheck(const std::string &id,
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
}

AccountCheck::~AccountCheck()
{
    LOG_INFO("~AccountCheck");
    this->shutdown();
}

void AccountCheck::onRegState(pj::OnRegStateParam &prm)
{
    LOG_INFO("on reg state");
    AccountCheckManager::getInstance()->onAccountRegState(shared_from_this(),
                                                          m_id,
                                                          static_cast<int>(prm.code),
                                                          prm.reason);
}

std::string AccountCheck::getId() const
{
    return m_id;
}

std::string AccountCheck::getUser() const
{
    return m_user;
}

std::string AccountCheck::getPass() const
{
    return m_pass;
}

std::string AccountCheck::getHost() const
{
    return m_host;
}
