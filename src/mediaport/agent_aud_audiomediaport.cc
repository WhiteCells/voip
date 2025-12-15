#include "agent_aud_audiomediaport.h"
#include "../logger.h"
#include "../agent/agent_ws_client.h"
#include <fstream>

AgentAudAudioMediaPort::AgentAudAudioMediaPort()
{
    LOG_INFO(">>> construct {}", __func__);
    pj::MediaFormatAudio fmt;      //
    fmt.type = PJMEDIA_TYPE_AUDIO; //
    fmt.id = PJMEDIA_FORMAT_PCM;   //
    fmt.clockRate = 16000;         //
    fmt.channelCount = 1;          //
    fmt.bitsPerSample = 16;        //
    fmt.frameTimeUsec = 20000;     //
    fmt.avgBps = 32000;            //
    fmt.maxBps = 32000;            //
    pj::AudioMediaPort::createPort("port", fmt);
    LOG_INFO("<<< construct {}", __func__);
}

AgentAudAudioMediaPort::~AgentAudAudioMediaPort()
{
    LOG_INFO(">>> {}", __func__);
    LOG_INFO("<<< {}", __func__);
}

// 向客户推送音频
// 接收 rtp server 的音频数据
void AgentAudAudioMediaPort::onFrameRequested(pj::MediaFrame &frame)
{
}

// 接收客户音频
// 推送 rtp server 的音频数据
void AgentAudAudioMediaPort::onFrameReceived(pj::MediaFrame &frame)
{
    // LOG_INFO("{} frame size: {}", __FUNCTION__, frame.size);
    // if (m_confirmed.load() == false) {
    //     // LOG_WARN("call not confirmed, drop frame");
    //     return;
    // }
    static std::ofstream send_audio("agent_aud_recv.pcm",
                                    std::ios::binary | std::ios::out | std::ios::trunc);
    if (!send_audio.is_open()) {
        LOG_ERROR("Failed to open agent_aud_recv.pcm");
    }

    if (frame.size > 0) {
        send_audio.write(reinterpret_cast<char *>(frame.buf.data()), frame.size);
        // if (g_agent_ws_client) {
        //     g_agent_ws_client->sendBinary(std::string(reinterpret_cast<const char *>(frame.buf.data()), frame.size), "customer");
        // }
    }
}
