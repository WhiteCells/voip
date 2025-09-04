#ifndef _RTP_SESSION_IMPL_H_
#define _RTP_SESSION_IMPL_H_

#include <jrtplib3/rtpsession.h>

class RtpSessionImpl : public jrtplib::RTPSession
{
public:
    RtpSessionImpl();
    ~RtpSessionImpl();

    virtual void OnBYEPacket(jrtplib::RTPSourceData *srcdat);

    void restart();
    void stop();
    void send();
    void poll();
};

#endif // _RTP_SESSION_IMPL_H_