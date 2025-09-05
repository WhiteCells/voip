#ifndef _RTP_SESSION_MGR_H_
#define _RTP_SESSION_MGR_H_

#include "singleton.hpp"
#include "rtp_session_impl.h"

class RtpSessionMgr :
    public Singleton<RtpSessionMgr>
{
public:
    ~RtpSessionMgr();

    void restart();
    void stop();
    void send();
    void poll();
    void waitForRtpSessionReady();

private:
    // RtpSessionImpl 
};

#endif // _RTP_SESSION_MGR_H_