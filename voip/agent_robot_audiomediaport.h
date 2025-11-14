#ifndef _AGENT_ROBOT_AUDIOMEDIAPORT_H_
#define _AGENT_ROBOT_AUDIOMEDIAPORT_H_

#ifdef _WIN32
#include <rtpsession.h>
#include <rtppacket.h>
#include <rtpipv4address.h>
#include <rtpsessionparams.h>
#include <rtpudpv4transmitter.h>
#else
#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtppacket.h>
#include <jrtplib3/rtpsessionparams.h>
#include <jrtplib3/rtpudpv4transmitter.h>
#endif
#include <pjsua2.hpp>
#include <atomic>
#include <memory>

class AgentRobotAudioMediaPort : public pj::AudioMediaPort
{
public:
    AgentRobotAudioMediaPort();
    ~AgentRobotAudioMediaPort();

    static void startEndFlagMonitor(std::shared_ptr<AgentRobotAudioMediaPort> weakSelf);

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

private:
    std::vector<int16_t> tts_buf;
    std::size_t tts_pos;
    std::atomic<bool> m_end_flag;
    std::atomic<bool> m_called_hangup;
};

#endif // _AGENT_ROBOT_AUDIOMEDIAPORT_H_
