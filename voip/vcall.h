#ifndef _VCALL_H_
#define _VCALL_H_

#include "vaudiomediaport.h"
#include <pjsua2.hpp>
#include <memory>

namespace voip {

class VAccount;
class VAudioMediaPort;

class VCall : public pj::Call
{
public:
    VCall(VAccount &acc, int call_id = PJSUA_INVALID_ID);
    ~VCall();

    // 呼叫状态改变
    virtual void
    onCallState(pj::OnCallStateParam &prm) override;

    // 呼叫状态改变
    virtual void
    onCallMediaState(pj::OnCallMediaStateParam &prm) override;

    // virtual void
    // onStreamCreated(pj::OnStreamCreatedParam &prm) override;

private:
    VAccount &acc_;
    std::shared_ptr<VAudioMediaPort> aud_media_port_;

    std::shared_ptr<pj::AudioMediaRecorder> aud_media_recorder_;

    pj::AudioMedia cap_dev_med_;
    pj::AudioMedia play_dev_med_;
};

} // namespace voip

#endif // _VCALL_H_
