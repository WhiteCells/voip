#include "agent_audiomediaport.h"

void AgentAudioMediaPort::onFrameRequested(pj::MediaFrame &frame)
{
    // std::lock_guard<std::mutex> lock(bufferMutex);

    // if (!recvAudioBuffer.empty()) {
    //     auto &audioChunk = recvAudioBuffer.front();
    //     size_t copyLen = std::min<size_t>(frame.size, audioChunk.size());
    //     memcpy(frame.buf.data(), audioChunk.data(), copyLen);

    //     if (copyLen < frame.size) {
    //         memset(frame.buf.data() + copyLen, 0, frame.size - copyLen);
    //     }
    //     recvAudioBuffer.pop();
    // } else {
    //     memset(frame.buf.data(), 0, frame.size);
    // }
    LOG_INFO("onFrameRequested");
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
    LOG_INFO("onFrameReceived");
    // const void* audioData = frame.buf.data();
    // size_t len = frame.size;

    // // 发送JRTPLib RTP包
    // rtpSession.SendPacket(audioData, len, 0, false, len / 2);
}