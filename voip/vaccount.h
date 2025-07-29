#ifndef _VACCOUNT_H_
#define _VACCOUNT_H_

#include <pjsua2.hpp>
#include <string>
#include <memory>

namespace voip {

class Caller;

struct AccResult;

/**
 * @brief SIP 用户对象
 *
 */
class VAccount : public pj::Account
{
public:
    VAccount(
        const std::string &id,
        const std::string &user,
        const std::string &pass,
        const std::string &host);

    ~VAccount();

    // 注册状态改变
    virtual void onRegState(pj::OnRegStateParam &prm) override;

    void set_acc_result(std::shared_ptr<voip::AccResult> acc_results);

    void create_();

    // 呼入
    // virtual void onIncomingCall(pj::OnIncomingCallParam &iprm) override;
    std::string getId() const { return m_id; };
    std::string getUser() const { return m_user; };
    std::string getPass() const { return m_pass; };
    std::string getHost() const { return m_host; };

private:
    pj::AuthCredInfo m_auth_cred_info;
    pj::AccountConfig m_acc_cfg;

    std::string m_id;
    std::string m_user;
    std::string m_pass;
    std::string m_host;
    std::shared_ptr<voip::AccResult> m_acc_result;
};

} // namespace voip

#endif // _VACCOUNT_H_
