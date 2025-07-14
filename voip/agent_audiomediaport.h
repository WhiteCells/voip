#ifndef _AGENT_AUDIOMEDIAPORT_H_
#define _AGENT_AUDIOMEDIAPORT_H_

// #include <jrtplib3/rtpsession.h>
// #include <jrtplib3/rtppacket.h>
// #include <jrtplib3/rtpipv4address.h>
// #include <jrtplib3/rtpsessionparams.h>
// #include <jrtplib3/rtpudpv4transmitter.h>
#include <pjsua2.hpp>

class AgentAudioMediaPort : public pj::AudioMediaPort
{
public:
    AgentAudioMediaPort();
    ~AgentAudioMediaPort() = default;

    /*
     * Callbacks
     */
    /**
     * This callback is called to request a frame from this port. On input,
     * frame.size indicates the capacity of the frame buffer and frame.buf
     * will initially be an empty vector. Application can then set the frame
     * type and fill the vector.
     *
     * @param frame       The frame.
     */
    virtual void onFrameRequested(pj::MediaFrame &frame) override;

    /**
     * This callback is called when this port receives a frame. The frame
     * content will be provided in frame.buf vector, and the frame size
     * can be found in either frame.size or the vector's size (both
     * have the same value).
     *
     * @param frame       The frame.
     */
    virtual void onFrameReceived(pj::MediaFrame &frame) override;
};

#endif // _AGENT_AUDIOMEDIAPORT_H_