#include "agent_robot_audiomediaport.h"
#include "global.h"
#include "logger.h"
#include <fstream>
#include "agent_ws_client.h"
//#include "tts_request.h"

AgentRobotAudioMediaPort::AgentRobotAudioMediaPort()
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

    // RTP 会话初始化
    using namespace jrtplib;
    RTPSessionParams sessparams;
    sessparams.SetOwnTimestampUnit(1.0 / 16000.0);
    sessparams.SetAcceptOwnPackets(true);

    RTPUDPv4TransmissionParams transparams;
    transparams.SetPortbase(0); // 本地 RTP 端口

    int status = m_session.Create(sessparams, &transparams);
    if (status < 0) {
        LOG_ERROR("Failed to create RTP session: {}", RTPGetErrorString(status));
        m_running = false;
        return;
    }

    m_session.SetDefaultPayloadType(1);
    m_session.SetDefaultMark(false);
    m_session.SetDefaultTimestampIncrement(320);

    const char *ip_str = robot_remote_host.c_str();
    uint16_t port = static_cast<uint16_t>(std::stoi(robot_remote_port));
    uint32_t ip = inet_addr(ip_str);
    ip = ntohl(ip);
    m_session.AddDestination(jrtplib::RTPIPv4Address(ip, port));

    m_running = true;
    m_rtp_recv_thread = std::thread([this]() {
        while (m_running) {
            m_session.Poll();
            if (m_session.GotoFirstSourceWithData()) {
                do {
                    jrtplib::RTPPacket *packet;
                    while ((packet = m_session.GetNextPacket()) != nullptr) {
                        std::size_t len = packet->GetPayloadLength();
                        const unsigned char *payload = packet->GetPayloadData();
                        int16_t pcm[320];
                        int frame_size = opus_decode(decoder, payload, len, pcm, 320, 0);
                        if (frame_size < 0) {
                            LOG_ERROR("Opus decode failed: {}", opus_strerror(frame_size));
                            m_session.DeletePacket(packet);
                            continue;
                        }
                        std::vector<uint16_t> data(frame_size * sizeof(int16_t));
                        memcpy(data.data(), pcm, frame_size * sizeof(int16_t));
                        {
                            std::lock_guard<std::mutex> lock(m_buffer_mtx);
                            m_rtp_recv_buffer.push_back(std::move(data));
                            // if (m_rtp_recv_buffer.size() > 50) {
                            //     m_rtp_recv_buffer.pop_front(); // 限制缓冲大小
                            //     LOG_INFO("Rtp Recv Buffer pop font");
                            // }
                            LOG_INFO("Recv RTP");
                        }
                        m_session.DeletePacket(packet);
                    }
                } while (m_session.GotoNextSourceWithData());
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
    LOG_INFO("<<< construct {}", __func__);
}

AgentRobotAudioMediaPort::~AgentRobotAudioMediaPort()
{
    LOG_INFO(">>> {}", __func__);
    m_running = false;
    if (m_rtp_recv_thread.joinable()) {
        m_rtp_recv_thread.join();
    }
    const char *reason = "session closed";
    size_t reason_len = strlen(reason);
    m_session.BYEDestroy(jrtplib::RTPTime(1.0), reason, reason_len);
    LOG_INFO("<<< {}", __func__);
}

// 向客户推送音频
// 接收 rtp server 的音频数据
//void AgentRobotAudioMediaPort::onFrameRequested(pj::MediaFrame &frame)
//{
//    static std::ofstream recv_audio("agent2client.pcm",
//                                    std::ios::binary | std::ios::out | std::ios::app);
//    if (!recv_audio.is_open()) {
//        LOG_ERROR("Failed to open recv.pcm");
//    }
//
//    const int sampleRate = 16000;
//    const int channels = 1;
//    const int duration_ms = 20;
//    const int samplesPerFrame = channels * sampleRate * duration_ms / 1000;
//    frame.type = PJMEDIA_FRAME_TYPE_AUDIO;
//    frame.size = samplesPerFrame * sizeof(int16_t);
//    frame.buf.resize(frame.size);
//
//    std::vector<char> pcm;
//    TTSPlayer::getInstance()->getNextAudio(pcm);
////    LOG_INFO("TTS agent_Audio Size: {}", pcm.size());
//
//    std::vector<uint16_t> pcm_uint16(pcm.size() / sizeof(uint16_t));
//    memcpy(pcm_uint16.data(), pcm.data(), pcm.size());
//    m_rtp_recv_buffer.push_back(std::move(pcm_uint16));
//
//    std::lock_guard<std::mutex> lock(m_buffer_mtx);
//    if (!m_rtp_recv_buffer.empty()) {
//        LOG_INFO("Rtp Recv Buffer size: {}", m_rtp_recv_buffer.size());
//        std::vector<uint16_t> &pkt = m_rtp_recv_buffer.front();
//        recv_audio.write(reinterpret_cast<char *>(pkt.data()), pkt.size());
//        size_t copy_size = (std::min)(pkt.size(), frame.buf.size());
//        memcpy(frame.buf.data(), pkt.data(), copy_size);
//        m_rtp_recv_buffer.pop_front();
//    }
//    else {
//        memset(frame.buf.data(), 0, frame.size);
//    }
//}

void AgentRobotAudioMediaPort::onFrameRequested(pj::MediaFrame &frame)
{
    const int sampleRate = 16000;
    const int duration_ms = 20;
    const int samplesPerFrame = sampleRate * duration_ms / 1000; // 320
    const size_t bytesPerFrame = samplesPerFrame * sizeof(int16_t); // 640 bytes

    frame.type = PJMEDIA_FRAME_TYPE_AUDIO;
    frame.size = bytesPerFrame;
    frame.buf.resize(frame.size);

    static std::vector<int16_t> tts_buf;
    static size_t tts_pos = 0;

    if (TTSPlayer::getInstance()->isStopped()) {  // TTS 停止播放
        tts_buf.clear();
        tts_pos = 0;
    }

    // 如果当前缓存不够，尝试拉取新的 TTS 音频
    if (tts_pos >= tts_buf.size()) {
        std::vector<char> pcm;
        if (TTSPlayer::getInstance()->getNextAudio(pcm) && !pcm.empty()) {
            size_t samples = pcm.size() / sizeof(int16_t);
            tts_buf.resize(samples);
            memcpy(tts_buf.data(), pcm.data(), pcm.size());
            tts_pos = 0;
        } else {
            memset(frame.buf.data(), 0, frame.size); // 无音频，静音输出
            return;
        }
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

// 接收客户音频
// 推送 rtp server 的音频数据
void AgentRobotAudioMediaPort::onFrameReceived(pj::MediaFrame &frame)
{
    // LOG_INFO("{} frame size: {}", __FUNCTION__, frame.size);
    if (m_confirmed.load() == false) {
        LOG_WARN("call not confirmed, drop frame");
        return;
    }
    static std::ofstream send_audio("client2agent.pcm", std::ios::binary | std::ios::out | std::ios::trunc);
    if (!send_audio.is_open()) {
        LOG_ERROR("Failed to open client2agent.pcm");
    }

    if (frame.size > 0) {

        send_audio.write(reinterpret_cast<char *>(frame.buf.data()), frame.size);

        if (g_agent_ws_client){
            g_agent_ws_client->sendBinary(std::string(reinterpret_cast<const char*>(frame.buf.data()), frame.size));

        }

        const int max_packet_size = 1500;
        std::vector<unsigned char> encoded(max_packet_size);
        int encoded_bytes = opus_encode(encoder,
                                        (const opus_int16 *)frame.buf.data(),
                                        320,
                                        encoded.data(),
                                        max_packet_size);
        if (encoded_bytes < 0) {
            LOG_ERROR("Opus encode failed: {}", opus_strerror(encoded_bytes));
            return;
        }
        int status = m_session.SendPacket(encoded.data(), encoded_bytes);
        // send_audio.write(reinterpret_cast<char *>(encoded.data()), encoded_bytes);
        // LOG_INFO("SendPacket: {}", std::string(reinterpret_cast<const char*>(frame.buf.data()), frame.size));
        if (status < 0) {
            LOG_INFO("RTP send failed: {}", jrtplib::RTPGetErrorString(status));
            return;
        }
//        LOG_INFO("Send customer RTP");
    }
}
