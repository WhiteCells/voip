#include <pjsua2.hpp>
#include <vector>
#include <string>

namespace voip {

class Caller;

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

    // voip::Caller *cur_call = nullptr;

    pj::AuthCredInfo m_auth_cred_info;
    pj::AccountConfig m_acc_cfg;
};

} // namespace voip
