#ifndef _VACCOUNT_H_
#define _VACCOUNT_H_

#include <pjsua2.hpp>
#include <json/json.h>
#include <string>
#include <memory>

namespace voip {

class Caller;

// Singal Account Result
struct AccResult
{
    std::string m_id;
    std::string m_user;
    std::string m_pass;
    std::string m_node_ip;
    int m_code;
    std::string m_msg;

    Json::Value toJson() const
    {
        Json::Value obj;
        obj["id"] = m_id;
        obj["user"] = m_user;
        obj["pass"] = m_pass;
        obj["nodeIp"] = m_node_ip;
        obj["code"] = m_code;
        obj["msg"] = m_msg;
        return obj;
    }
};

/**
 * @brief SIP 用户对象
 *
 */
class VAccount : public pj::Account
{
public:
    VAccount(const std::string &id,
             const std::string &user,
             const std::string &pass,
             const std::string &host);

    ~VAccount();

    // 注册状态改变
    virtual void onRegState(pj::OnRegStateParam &prm) override;

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
