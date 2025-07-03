#include "agent_audiomediaport.h"
#include "logger.h"
#include <fstream>

void AgentAudioMediaPort::onFrameRequested(pj::MediaFrame &frame)
{
    // static double phase = 0.0;
    // static int frameCount = 0;

    // const int sampleRate = 16000;       // 采样率
    // const int channels = 1;             // 单声道
    // const int duration_ms = 20;         // 每帧20毫秒
    // const int samplesPerFrame = sampleRate * duration_ms / 1000;
    // const int bytesPerSample = 2;
    // const int frameSize = samplesPerFrame * bytesPerSample;

    // frame.size = frameSize;
    // frame.buf.resize(frameSize);

    // const int framesPerCycle = 30;
    // int cyclePos = frameCount % framesPerCycle;

    // double freq = 0.0;
    // if (cyclePos < 10) {
    //     freq = 300.0;
    // } else if (cyclePos < 20) {
    //     freq = 500.0;
    // } else {
    //     freq = 0.0;
    // }

    // for (int i = 0; i < samplesPerFrame; ++i) {
    //     int16_t sample = 0;
    //     if (freq > 0.0) {
    //         sample = static_cast<int16_t>(std::sin(phase) * 6000); // 6000 是振幅
    //         phase += 2.0 * M_PI * freq / sampleRate;
    //         if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
    //     }
    //     frame.buf[i * 2 + 0] = sample & 0xFF;
    //     frame.buf[i * 2 + 1] = (sample >> 8) & 0xFF;
    // }

    // ++frameCount;
    // LOG_INFO("Simulated frame {} with freq {}", frameCount, freq);
}

/**
 * This callback is called when this port receives a frame. The frame
 * content will be provided in frame.buf vector, and the frame size
 * can be found in either frame.size or the vector's size (both
 * have the same value).
 *
 * @param frame       The frame.
 */
void AgentAudioMediaPort::onFrameReceived(pj::MediaFrame &frame)
{
    LOG_INFO(">>> {}", __FUNCTION__);
    LOG_INFO(" frame size: {}", frame.size);
    // LOG_INFO(" frame type: {}", frame.type);
    // for (unsigned i = 0; i < frame.size; ++i) {
    // LOG_INFO(" frame[{}]: {}", i, static_cast<int>(frame.buf[i]));
    // }

    static std::ofstream pcm_out(
        "output.pcm",
        std::ios::binary | std::ios::out | std::ios::trunc);
    if (!pcm_out.is_open()) {
        LOG_ERROR("Failed to open output.pcm");
        return;
    }
    pcm_out.write(reinterpret_cast<char *>(frame.buf.data()), frame.size);
    LOG_INFO("<<< {}", __FUNCTION__);
}