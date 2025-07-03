#ifndef _AGENT_AUDIOMEDIAPORT2_H_
#define _AGENT_AUDIOMEDIAPORT2_H_

#include <pjsua2.hpp>

class AgentAudioMediaPort2 : public pj::AudioMediaPort
{
public:
    virtual void onFrameRequested(pj::MediaFrame &frame) override;
    virtual void onFrameReceived(pj::MediaFrame &frame) override;
};

#endif // _AGENT_AUDIOMEDIAPORT2_H_