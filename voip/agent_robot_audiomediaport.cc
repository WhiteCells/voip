#include "agent_robot_audiomediaport.h"
#include "global.h"
#include "io_context_pool.h"
#include "logger.h"
#include <fstream>
#include "agent/tts_http_client.h"
#include "tts_request.h"
#include "event/event.h"
#include "agent/msg.h"

AgentRobotAudioMediaPort::AgentRobotAudioMediaPort()
    : tts_pos(0)
    , m_end_flag(false)
    , last_pcm(true)
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

    g_event_bus.subscribe<LLMHangupMsg>([&](const LLMHangupMsg &) {
        m_llm_hangup.store(true);
    });

    auto &ioc = IOContextPool::getInstance()->getIOContext();
    m_asr_ws_client = std::make_shared<ASRWsClient>(ioc,
                                                    agent_session_remote_host,
                                                    agent_session_remote_port,
                                                    agent_session_remote_target,
                                                    true);
    m_asr_ws_client->start();
    m_asr_ws_client->send_start_config();

    LOG_INFO("<<< construct {}", __func__);
}

AgentRobotAudioMediaPort::~AgentRobotAudioMediaPort()
{
    LOG_INFO(">>> {}", __func__);
    m_asr_ws_client->send_stop_config();
    tts_buf.clear();
    tts_pos = 0;
    LOG_INFO("<<< {}", __func__);
}

void AgentRobotAudioMediaPort::onFrameRequested(pj::MediaFrame &frame)
{
    // // LOG_INFO("{} frame size: {}", __FUNCTION__, frame.size);
    // const int sampleRate = 16000;
    // const int duration_ms = 20;
    // const int samplesPerFrame = sampleRate * duration_ms / 1000;    // 320
    // const size_t bytesPerFrame = samplesPerFrame * sizeof(int16_t); // 640 bytes

    // frame.type = PJMEDIA_FRAME_TYPE_AUDIO;
    // frame.size = bytesPerFrame;
    // frame.buf.resize(frame.size);

    // if (m_llm_hangup.load()) {
    //     if (last_pcm) {
    //         last_pcm = false;
    //         auto pcm = TTSHTTPClient::m_que->try_pop();
    //         if (pcm.empty()) {
    //             memset(frame.buf.data(), 0, frame.size);
    //             return;
    //         }
    //     }
    // }

    // if (TTSPlayer::getInstance()->isStopped()) {
    //     tts_buf.clear();
    //     tts_pos = 0;
    // }

    // if (tts_pos >= tts_buf.size()) {
    //     auto pcm = TTSHTTPClient::m_que->try_pop();
    //     if (!pcm.empty()) {
    //         size_t samples = pcm.size() / sizeof(int16_t);
    //         tts_buf.resize(samples);
    //         memcpy(tts_buf.data(), pcm.data(), pcm.size());
    //         tts_pos = 0;
    //     }
    //     else {
    //         memset(frame.buf.data(), 0, frame.size);
    //         return;
    //     }
    // }

    // size_t remain = tts_buf.size() - tts_pos;
    // size_t copy_samples = (std::min)((size_t)samplesPerFrame, remain);
    // memcpy(frame.buf.data(), tts_buf.data() + tts_pos, copy_samples * sizeof(int16_t));
    // tts_pos += copy_samples;

    // if (copy_samples < samplesPerFrame) {
    //     memset(frame.buf.data() + copy_samples * sizeof(int16_t), 0,
    //            (samplesPerFrame - copy_samples) * sizeof(int16_t));
    // }
}

void AgentRobotAudioMediaPort::startEndFlagMonitor(std::weak_ptr<AgentRobotAudioMediaPort> weak_self)
{
    std::thread([weak_self]() {
        endpoint.libRegisterThread("Worker");
        LOG_INFO("[Monitor] Start monitoring m_end_flag...");

        while (true) {
            auto self = weak_self.lock();
            if (!self) {
                LOG_WARN("[Monitor] weakSelf expired, exiting monitor thread.");
                break;
            }
            if (self->m_llm_hangup.load()) {
                LOG_INFO("[Monitor] m_llm_hangup detected, hanging up call...");
                endpoint.hangupAllCalls();
                LOG_INFO("monitor hangup over");
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        LOG_INFO("[Monitor] Monitor thread exited.");
    }).detach();
}

// 接收客户音频
// 推送 rtp server 的音频数据
void AgentRobotAudioMediaPort::onFrameReceived(pj::MediaFrame &frame)
{
    // LOG_INFO("{} frame size: {}", __FUNCTION__, frame.size);
    if (m_confirmed.load() == false) {
        // LOG_WARN("call not confirmed, drop frame");
        return;
    }

    if (m_llm_hangup.load()) {
        return;
    }

    static std::ofstream send_audio("client2agent.pcm",
                                    std::ios::binary | std::ios::out | std::ios::trunc);
    if (!send_audio.is_open()) {
        LOG_ERROR("Failed to open client2agent.pcm");
    }

    if (frame.size > 0) {
        send_audio.write(reinterpret_cast<char *>(frame.buf.data()), frame.size);
        auto pcm = std::string(reinterpret_cast<const char *>(frame.buf.data()));
        m_asr_ws_client->send(pcm, false);
    }
}
