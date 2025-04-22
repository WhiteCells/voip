#include <pjsua2.hpp>

namespace voip {

class VCall;

class VAccount : public pj::Account
{
public:
    VAccount();

    ~VAccount();

    // 注册状态改变
    virtual void
    onRegState(pj::OnRegStateParam &prm) override;

    // 呼入
    virtual void
    onIncomingCall(pj::OnIncomingCallParam &iprm) override;

    VCall *cur_call = nullptr;
};

} // namespace voip
