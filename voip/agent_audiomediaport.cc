#include "agent_audiomediaport.h"
#include "logger.h"
#include <fstream>

AgentAudioMediaPort::AgentAudioMediaPort()
{
    pj::MediaFormatAudio fmt;      //
    fmt.type = PJMEDIA_TYPE_AUDIO; //
    fmt.id = PJMEDIA_FORMAT_ULAW;  //
    fmt.clockRate = 8000;          //
    fmt.channelCount = 1;          //
    fmt.bitsPerSample = 8;         //
    fmt.frameTimeUsec = 20000;     //
    fmt.avgBps = 64000;            //
    fmt.maxBps = 64000;            //
    this->createPort("port", fmt);
}

void AgentAudioMediaPort::onFrameRequested(pj::MediaFrame &frame)
{
    static double phase = 0.0;
    static int frameCount = 0;

    const int sampleRate = 8000;
    const int channels = 1;
    const int duration_ms = 20;
    const int samplesPerFrame = sampleRate * duration_ms / 1000;

    frame.type = PJMEDIA_FRAME_TYPE_AUDIO;
    frame.size = samplesPerFrame;
    frame.buf.resize(frame.size);

    const int framesPerCycle = 30;
    int cyclePos = frameCount % framesPerCycle;

    double freq = 0.0;
    if (cyclePos < 10) {
        freq = 300.0;
    }
    else if (cyclePos < 20) {
        freq = 500.0;
    }
    else {
        freq = 0.0; // 静音段
    }
    for (int i = 0; i < samplesPerFrame; ++i) {
        int16_t pcm_sample = 0;

        if (freq > 0.0) {
            pcm_sample = static_cast<int16_t>(std::sin(phase) * 6000); // 振幅
            phase += 2.0 * M_PI * freq / sampleRate;
            if (phase > 2.0 * M_PI) {
                phase -= 2.0 * M_PI;
            }
        }
        frame.buf[i] = pjmedia_linear2ulaw(pcm_sample);
    }
    static std::ofstream pcm_out(
        "input.pcm",
        std::ios::binary | std::ios::out | std::ios::trunc);
    if (!pcm_out.is_open()) {
        LOG_ERROR("Failed to open input_ulaw.pcm");
        return;
    }
    pcm_out.write(reinterpret_cast<char *>(frame.buf.data()), frame.size);

    ++frameCount;
    // LOG_INFO("frame {} with freq {}", frameCount, freq);
}

void AgentAudioMediaPort::onFrameReceived(pj::MediaFrame &frame)
{
    // LOG_INFO("{} frame size: {}", __FUNCTION__, frame.size);

    static std::ofstream pcm_out(
        "output.pcm",
        std::ios::binary | std::ios::out | std::ios::trunc);
    if (!pcm_out.is_open()) {
        LOG_ERROR("Failed to open output.pcm");
        return;
    }
    pcm_out.write(reinterpret_cast<char *>(frame.buf.data()), frame.size);
}
