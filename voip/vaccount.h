#include <pjsua2.hpp>
#include <vector>

namespace voip {

class Caller;

class VAccount : public pj::Account
{
public:
    VAccount();

    ~VAccount();

    // 注册状态改变
    virtual void onRegState(pj::OnRegStateParam &prm) override;

    // 呼入
    virtual void onIncomingCall(pj::OnIncomingCallParam &iprm) override;

    voip::Caller *cur_call = nullptr;
};

} // namespace voip
