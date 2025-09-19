#include "agent_audiomediaport.h"
#include "global.h"
#include "logger.h"
#include "ini.h"
#include <fstream>

AgentAudioMediaPort::AgentAudioMediaPort()
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
    transparams.SetPortbase(8002); // 本地 RTP 端口

    int status = m_session.Create(sessparams, &transparams);
    if (status < 0) {
        LOG_ERROR("Failed to create RTP session: {}", RTPGetErrorString(status));
        m_running = false;
        return;
    }

    m_session.SetDefaultPayloadType(0);
    m_session.SetDefaultMark(false);
    m_session.SetDefaultTimestampIncrement(320);

    const char* ip_str = remote_host.c_str();
    uint16_t port =  static_cast<uint16_t>(std::stoi(remote_port));
    uint32_t ip = inet_addr(ip_str);
    ip = ntohl(ip);
    m_session.AddDestination(jrtplib::RTPIPv4Address(ip, port)); // 远程服务器 IP:端口

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
                        std::vector<uint8_t> data(frame_size * sizeof(int16_t));
                        memcpy(data.data(), pcm, frame_size * sizeof(int16_t));
                        {
                            std::lock_guard<std::mutex> lock(m_buffer_mtx);
                            m_rtp_recv_buffer.push_back(std::move(data));
                            if (m_rtp_recv_buffer.size() > 50) {
                                m_rtp_recv_buffer.pop_front(); // 限制缓冲大小
                                // LOG_INFO("Rtp Recv Buffer pop font");
                            }
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

AgentAudioMediaPort::~AgentAudioMediaPort()
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
void AgentAudioMediaPort::onFrameRequested(pj::MediaFrame &frame)
{
    const int sampleRate = 16000;
    const int channels = 1;
    const int duration_ms = 20;
    const int samplesPerFrame = sampleRate * duration_ms / 1000;
    frame.type = PJMEDIA_FRAME_TYPE_AUDIO;
    frame.size = samplesPerFrame * sizeof(int16_t);
    frame.buf.resize(frame.size);

    std::lock_guard<std::mutex> lock(m_buffer_mtx);
    if (!m_rtp_recv_buffer.empty()) {
        std::vector<uint8_t> &pkt = m_rtp_recv_buffer.front();
        size_t copy_size = (std::min)(pkt.size(), frame.buf.size());
        memcpy(frame.buf.data(), pkt.data(), copy_size);
        m_rtp_recv_buffer.pop_front();
    }
    else {
        memset(frame.buf.data(), 0, frame.size);
    }
}

// 接收客户音频
// 推送 rtp server 的音频数据
void AgentAudioMediaPort::onFrameReceived(pj::MediaFrame &frame)
{
    // LOG_INFO("{} frame size: {}", __FUNCTION__, frame.size);
    if (frame.size > 0) {
        const int max_packet_size = 1500;
        std::vector<unsigned char> encoded(max_packet_size);
        int encoded_bytes = opus_encode(encoder,
                                        (const int16_t *)frame.buf.data(),
                                        320,
                                        encoded.data(),
                                        max_packet_size);
        if (encoded_bytes < 0) {
            LOG_ERROR("Opus encode failed: {}", opus_strerror(encoded_bytes));
            return;
        }
        int status = m_session.SendPacket(encoded.data(), encoded_bytes);
        // LOG_INFO("SendPacket: {}", std::string(reinterpret_cast<const char*>(frame.buf.data()), frame.size));
        if (status < 0) {
            LOG_INFO("RTP send failed: {}", jrtplib::RTPGetErrorString(status));
            return;
        }
        LOG_INFO("Send RTP");
    }

    static std::ofstream pcm_out("output.pcm",
                                 std::ios::binary | std::ios::out | std::ios::trunc);
    if (!pcm_out.is_open()) {
        LOG_ERROR("Failed to open output.pcm");
        return;
    }
    pcm_out.write(reinterpret_cast<char *>(frame.buf.data()), frame.size);
}
