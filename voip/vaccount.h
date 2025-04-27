#include <pjsua2.hpp>
#include <QObject>

namespace voip {

class VCall;

class VAccount :
    public QObject,
    public pj::Account
{
public:
    VAccount();
    ~VAccount();

    // 注册状态改变
    virtual void onRegState(pj::OnRegStateParam &prm) override;
    // 呼入
    virtual void onIncomingCall(pj::OnIncomingCallParam &iprm) override;

public:
    VCall *cur_call = nullptr;
    std::string phone_num_;

private:
    pj::AccountConfig m_account_config;

public slots:
    void slot_update_account_config(pj::AccountConfig &acc_cfg);
};

} // namespace voip
