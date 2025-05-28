#include <pjsua2.hpp>
#include <string>

namespace voip {

class Caller;

/**
 * @brief SIP 用户对象
 * 
 */
class VAccount : public pj::Account
{
public:
    VAccount(
        const std::string &user,
        const std::string &pass,
        const std::string &host);

    ~VAccount();

    // 注册状态改变
    virtual void onRegState(pj::OnRegStateParam &prm) override;

    // 呼入
    // virtual void onIncomingCall(pj::OnIncomingCallParam &iprm) override;

    std::string getUser() const { return m_user; };
    std::string getPass() const { return m_pass; };
    std::string getHost() const { return m_host; };

private:
    pj::AuthCredInfo m_auth_cred_info;
    pj::AccountConfig m_acc_cfg;

    std::string m_user;
    std::string m_pass;
    std::string m_host;
};

} // namespace voip
