#pragma once

#include <pjsua2.hpp>
#include <atomic>
#include <memory>
#include <fstream>

class AgentRobotAudioMediaPort : public pj::AudioMediaPort
{
public:
    AgentRobotAudioMediaPort();
    ~AgentRobotAudioMediaPort();

    static void startEndFlagMonitor(std::weak_ptr<AgentRobotAudioMediaPort> weakSelf);

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
    std::ofstream m_audio_file;
};

