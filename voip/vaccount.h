#include <pjsua2.hpp>
#include <vector>

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

    voip::VCall *cur_call = nullptr;
    std::string phone_num_;
};

} // namespace voip
