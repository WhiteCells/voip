#ifndef _RTP_SERVER_H_
#define _RTP_SERVER_H_

#include "audio_queue.h"
#ifdef _WIN32
#include <rtpsession.h>
#include <rtppacket.h>
#include <rtpipv4address.h>
#include <rtpsessionparams.h>
#include <rtpudpv4transmitter.h>
#include <opus.h>
#else
#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtppacket.h>
#include <jrtplib3/rtpsessionparams.h>
#include <jrtplib3/rtpudpv4transmitter.h>
#include <opus/opus.h>
#endif
#include <string>
#include <cstdint>
#include <cstring>
#include <functional>
#include <atomic>
#include <iostream>
#include <thread>
#include <fstream>

class RtpServer
{
public:
    using RecvCallback = std::function<void(const float *, size_t)>;
    RtpServer(AudioQueue &output_que, AudioQueue &input_que,
              const std::string &remote_ip,
              uint16_t remote_port, uint16_t local_port,
              double tsunit, uint32_t tsinc) :
        m_remote_ip(remote_ip),
        m_output_que(output_que),
        m_input_que(input_que)
    {
        m_encoder = opus_encoder_create(16000, 1, OPUS_APPLICATION_VOIP, nullptr);
        m_decoder = opus_decoder_create(16000, 1, nullptr);

        jrtplib::RTPSessionParams sess_prm;
        sess_prm.SetOwnTimestampUnit(tsunit);
        sess_prm.SetAcceptOwnPackets(true);

        // local
        jrtplib::RTPUDPv4TransmissionParams ts_prm;
        ts_prm.SetPortbase(local_port);

        int status = m_session.Create(sess_prm, &ts_prm);
        if (status) {
            std::cerr << "ERROR: " << jrtplib::RTPGetErrorString(status) << std::endl;
            exit(-1);
        }

        std::cout << "listen port: " << local_port << std::endl;

        // remote
        uint32_t ipval = inet_addr(remote_ip.c_str());
        ipval = ntohl(ipval);

        jrtplib::RTPIPv4Address addr(ipval, remote_port);
        m_session.AddDestination(addr);
        m_session.SetDefaultPayloadType(1);
        m_session.SetDefaultMark(false);
        m_session.SetDefaultTimestampIncrement(tsinc);
    }
    virtual ~RtpServer() = default;

    void sendFrame(const void *data, size_t len)
    {
        m_session.SendPacket(data, len);
        std::cout << "send frame: " << data << std::endl;
    }

    void send(std::atomic<bool> &running)
    {
        std::cout << __func__ << std::endl;
        while (running) {
            // std::cout << __func__ << " loop" << std::endl;
            if (!m_input_que.empty()) {
                const int max_packet_size = 1500;
                std::vector<unsigned char> encoded(max_packet_size);
                AudioQueue::FrameType p = m_input_que.pop();
                int bytes = opus_encode(m_encoder,
                                        (const opus_int16 *)p.first,
                                        320,
                                        encoded.data(),
                                        max_packet_size);
                if (bytes < 0) {
                    std::cerr << "encode failed: " << bytes << std::endl;
                    continue;
                }
                m_session.SendPacket(encoded.data(), bytes);
                // std::cout << "Send Payload" << std::endl;
                // std::cout << "Send Payload: " << p.first << ", Samples: " << p.second << std::endl;
            }
        }
    }

    void poll(std::atomic<bool> &running)
    {
        std::cout << __func__ << std::endl;
        while (running) {
            // std::cout << __func__ << " loop" << std::endl;
            m_session.Poll();
            if (m_session.GotoFirstSourceWithData()) {
                do {
                    jrtplib::RTPPacket *pkt;
                    while ((pkt = m_session.GetNextPacket()) != nullptr) {
                        size_t len = pkt->GetPayloadLength();
                        const unsigned char *payload = pkt->GetPayloadData();
                        int16_t pcm[320];
                        int frame_size = opus_decode(m_decoder, payload, len, pcm, 320, 0);
                        if (frame_size < 0) {
                            std::cerr << "decode failed: " << frame_size << std::endl;
                            m_session.DeletePacket(pkt);
                            continue;
                        }
                        int16_t *copy = new int16_t[frame_size];
                        std::memcpy(copy, pcm, frame_size * sizeof(int16_t));
                        // save client audio to pcm file
                        static std::ofstream recv_audio("client.pcm",
                                                        std::ios::binary | std::ios::out | std::ios::trunc);
                        recv_audio.write(reinterpret_cast<char *>(copy), frame_size * sizeof(int16_t));                        
                        std::cout << "write client audio" << std::endl;
                        // 
                        m_output_que.push(copy, frame_size * sizeof(int16_t));
                        std::cout << "Recv Payload (decoded)" << std::endl;
                    }
                } while (m_session.GotoNextSourceWithData());
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

private:
    jrtplib::RTPSession m_session;
    std::string m_remote_ip;
    std::atomic<bool> m_running;
    AudioQueue &m_output_que;
    AudioQueue &m_input_que;
    OpusEncoder *m_encoder {nullptr};
    OpusDecoder *m_decoder {nullptr};
};

#endif // _RTP_SERVER_H_