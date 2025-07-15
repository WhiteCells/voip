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

    // RTP 会话初始化
    using namespace jrtplib;
    RTPSessionParams sessparams;
    sessparams.SetOwnTimestampUnit(1.0 / 8000.0);
    sessparams.SetAcceptOwnPackets(true);

    RTPUDPv4TransmissionParams transparams;
    transparams.SetPortbase(8002); // 本地 RTP 端口

    int status = m_session.Create(sessparams, &transparams);
    if (status < 0) {
        LOG_ERROR("Failed to create RTP session: {}", RTPGetErrorString(status));
        m_running = false;
        return;
    }

    uint32_t ip = inet_addr("127.0.0.1");
    ip = ntohl(ip);
    m_session.AddDestination(jrtplib::RTPIPv4Address(ip, 8000)); // 远程服务器 IP:端口

    m_running = true;
    m_rtp_recv_thread = std::thread([this]() {
        while (m_running) {
            m_session.Poll();
            if (m_session.GotoFirstSourceWithData()) {
                do {
                    jrtplib::RTPPacket *packet;
                    while ((packet = m_session.GetNextPacket()) != nullptr) {
                        std::vector<uint8_t> data(
                            packet->GetPayloadData(),
                            packet->GetPayloadData() + packet->GetPayloadLength());
                        {
                            std::lock_guard<std::mutex> lock(m_buffer_mtx);
                            m_rtp_recv_buffer.push_back(std::move(data));
                            if (m_rtp_recv_buffer.size() > 50) {
                                m_rtp_recv_buffer.pop_front(); // 限制缓冲大小
                            }
                        }
                        m_session.DeletePacket(packet);
                    }
                } while (m_session.GotoNextSourceWithData());
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
}

// 向客户推送音频
// 接收 rtp server 的音频数据
void AgentAudioMediaPort::onFrameRequested(pj::MediaFrame &frame)
{
    frame.type = PJMEDIA_FRAME_TYPE_AUDIO;
    frame.size = 160;
    frame.buf.resize(frame.size);

    std::lock_guard<std::mutex> lock(m_buffer_mtx);
    if (!m_rtp_recv_buffer.empty()) {
        std::vector<uint8_t> &pkt = m_rtp_recv_buffer.front();
        size_t copy_size = std::min(pkt.size(), frame.buf.size());
        memcpy(frame.buf.data(), pkt.data(), copy_size);
        m_rtp_recv_buffer.pop_front();
    }
    else {
        memset(frame.buf.data(), 0xFF, frame.size);
    }
    // static double phase = 0.0;
    // static int frameCount = 0;

    // const int sampleRate = 8000;
    // const int channels = 1;
    // const int duration_ms = 20;
    // const int samplesPerFrame = sampleRate * duration_ms / 1000;

    // frame.type = PJMEDIA_FRAME_TYPE_AUDIO;
    // frame.size = samplesPerFrame;
    // frame.buf.resize(frame.size);

    // const int framesPerCycle = 30;
    // int cyclePos = frameCount % framesPerCycle;

    // double freq = 0.0;
    // if (cyclePos < 10) {
    //     freq = 300.0;
    // }
    // else if (cyclePos < 20) {
    //     freq = 500.0;
    // }
    // else {
    //     freq = 0.0; // 静音段
    // }
    // for (int i = 0; i < samplesPerFrame; ++i) {
    //     int16_t pcm_sample = 0;

    //     if (freq > 0.0) {
    //         pcm_sample = static_cast<int16_t>(std::sin(phase) * 6000); // 振幅
    //         phase += 2.0 * M_PI * freq / sampleRate;
    //         if (phase > 2.0 * M_PI) {
    //             phase -= 2.0 * M_PI;
    //         }
    //     }
    //     frame.buf[i] = pjmedia_linear2ulaw(pcm_sample);
    // }
    // static std::ofstream pcm_out(
    //     "input.pcm",
    //     std::ios::binary | std::ios::out | std::ios::trunc);
    // if (!pcm_out.is_open()) {
    //     LOG_ERROR("Failed to open input_ulaw.pcm");
    //     return;
    // }
    // pcm_out.write(reinterpret_cast<char *>(frame.buf.data()), frame.size);

    // ++frameCount;
    // LOG_INFO("frame {} with freq {}", frameCount, freq);
}

// 接收客户音频
// 推送 rtp server 的音频数据
void AgentAudioMediaPort::onFrameReceived(pj::MediaFrame &frame)
{
    // LOG_INFO("{} frame size: {}", __FUNCTION__, frame.size);
    if (frame.size > 0) {
        int status = m_session.SendPacket(frame.buf.data(), frame.size);
        if (status < 0) {
            LOG_INFO("RTP send failed: {}", jrtplib::RTPGetErrorString(status));
        }
    }

    static std::ofstream pcm_out(
        "output.pcm",
        std::ios::binary | std::ios::out | std::ios::trunc);
    if (!pcm_out.is_open()) {
        LOG_ERROR("Failed to open output.pcm");
        return;
    }
    pcm_out.write(reinterpret_cast<char *>(frame.buf.data()), frame.size);
}
