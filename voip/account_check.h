#ifndef _ACCOUNT_CHECK_H_
#define _ACCOUNT_CHECK_H_

#include <pjsua2.hpp>
#include <memory>
#include <string>

class AccountCheckManager;

class AccountCheck :
    public std::enable_shared_from_this<AccountCheck>,
    public pj::Account
{
public:
    AccountCheck(const std::string &id,
                 const std::string &user,
                 const std::string &pass,
                 const std::string &host);

    ~AccountCheck();

    virtual void onRegState(pj::OnRegStateParam &prm) override;

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

#endif // _ACCOUNT_CHECK_H_