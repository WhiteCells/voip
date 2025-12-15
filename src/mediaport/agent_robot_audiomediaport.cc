#include "agent_robot_audiomediaport.h"
#include "../logger.h"
#include "../agent/agent_ws_client.h"
#include <chrono>
#include <fstream>
#include <filesystem>

AgentRobotAudioMediaPort::AgentRobotAudioMediaPort()
    : tts_pos(0)
    , m_end_flag(false)
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
    // audio file
    std::filesystem::create_directories("pcm");
    std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    std::string aud_path = "pcm/client2agent_" + std::to_string(now.count()) + ".pcm";
    LOG_INFO("aud file path: {}", aud_path);
    m_audio_file = std::ofstream(aud_path,
                                 std::ios::binary | std::ios::out | std::ios::trunc);
    LOG_INFO("<<< construct {}", __func__);
}

AgentRobotAudioMediaPort::~AgentRobotAudioMediaPort()
{
    LOG_INFO(">>> {}", __func__);
    tts_buf.clear();
    tts_pos = 0;
    // g_agent_ws_client->clear_llm_msg_list();
    LOG_INFO("<<< {}", __func__);
}

void AgentRobotAudioMediaPort::onFrameRequested(pj::MediaFrame &frame)
{
    //    LOG_INFO("{} frame size: {}", __FUNCTION__, frame.size);
    const int sampleRate = 16000;
    const int duration_ms = 20;
    const int samplesPerFrame = sampleRate * duration_ms / 1000;    // 320
    const size_t bytesPerFrame = samplesPerFrame * sizeof(int16_t); // 640 bytes

    frame.type = PJMEDIA_FRAME_TYPE_AUDIO;
    frame.size = bytesPerFrame;
    frame.buf.resize(frame.size);

    // if (TTSPlayer::getInstance()->isStopped()) {
    //     tts_buf.clear();
    //     tts_pos = 0;
    //     //        memset(frame.buf.data(), 0, frame.size);
    //     //        return;
    // }

    // 如果当前缓存不够，尝试拉取新的 TTS 音频
    if (tts_pos >= tts_buf.size()) {
        std::vector<char> pcm;
        // if (TTSPlayer::getInstance()->getNextAudio(pcm) && !pcm.empty()) {
        //     LOG_INFO("pcm {}", std::string(pcm.data()));
        //     if (pcm == std::vector<char> {'E', 'N', 'D'}) {
        //         LOG_INFO("set m_end_flag to true");
        //         m_end_flag = true;
        //         pcm.clear();
        //     }
        //     if (!pcm.empty()) {
        //         size_t samples = pcm.size() / sizeof(int16_t);
        //         tts_buf.resize(samples);
        //         memcpy(tts_buf.data(), pcm.data(), pcm.size());
        //         tts_pos = 0;
        //     }
        // }
        // else {
        //     memset(frame.buf.data(), 0, frame.size);
        //     return;
        // }
    }

    // 从缓冲中取 20ms 数据
    size_t remain = tts_buf.size() - tts_pos;
    size_t copy_samples = (std::min)((size_t)samplesPerFrame, remain);
    memcpy(frame.buf.data(), tts_buf.data() + tts_pos, copy_samples * sizeof(int16_t));
    tts_pos += copy_samples;

    // 如果不满一帧，补零
    if (copy_samples < samplesPerFrame) {
        memset(frame.buf.data() + copy_samples * sizeof(int16_t), 0,
               (samplesPerFrame - copy_samples) * sizeof(int16_t));
    }
}

void AgentRobotAudioMediaPort::startEndFlagMonitor(std::weak_ptr<AgentRobotAudioMediaPort> weakSelf)
{
    std::thread([weakSelf]() {
        // endpoint.libRegisterThread("Worker");
        LOG_INFO("[Monitor] Start monitoring m_end_flag...");

        while (true) {
            auto self = weakSelf.lock();
            if (!self) {
                LOG_WARN("[Monitor] weakSelf expired, exiting monitor thread.");
                break;
            }
            if (self->m_end_flag.load()) {
                //            if (TTSPlayer::endendend_flag.load()) {
                LOG_INFO("[Monitor] m_end_flag detected, hanging up call...");
                //                std::this_thread::sleep_for(std::chrono::seconds(30));
                self->m_end_flag.store(false);
                //                TTSPlayer::getInstance()->clear();
                // TTSPlayer::getInstance()->stop();

                // if (TTSPlayer::endendend_flag.load()) {
                //     endpoint.hangupAllCalls();
                //     LOG_INFO("monitor hangup over");
                // }
                // TTSPlayer::endendend_flag.store(false);
                // break;
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
    // if (m_confirmed.load() == false) {
    //     // LOG_WARN("call not confirmed, drop frame");
    //     return;
    // }

    // if (TTSPlayer::endendend_flag.load()) {
    //     return;
    // }

    if (m_end_flag) {
        return;
    }

    if (!m_audio_file.is_open()) {
        LOG_ERROR("Failed to open client2agent.pcm");
    }

    if (frame.size > 0) {
        m_audio_file.write(reinterpret_cast<char *>(frame.buf.data()), frame.size);
        // if (g_agent_ws_client) {
        //     g_agent_ws_client->sendBinary(std::string(reinterpret_cast<const char *>(frame.buf.data()), frame.size), "customer");
        // }
    }
}
