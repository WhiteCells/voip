#ifndef _RTP_SERVER_H_
#define _RTP_SERVER_H_

#include "audio_queue.h"
#include <rtpsession.h>
#include <rtpsessionparams.h>
#include <rtpudpv4transmitter.h>
#include <rtppacket.h>
#include <string>
#include <cstdint>
#include <cstring>
#include <functional>
#include <atomic>
#include <iostream>

class RtpServer
{
public:
    using RecvCallback = std::function<void(const float *, size_t)>;
    RtpServer(AudioQueue &output_que, AudioQueue &input_que,
              const std::string &remote_ip,
              int16_t remote_port, int16_t local_port,
              double tsunit, uint32_t tsinc) :
        m_remote_ip(remote_ip),
        m_output_que(output_que),
        m_input_que(input_que)
    {
        jrtplib::RTPSessionParams sess_prm;
        sess_prm.SetOwnTimestampUnit(tsunit);
        sess_prm.SetAcceptOwnPackets(true);

        // local
        jrtplib::RTPUDPv4TransmissionParams ts_prm;
        ts_prm.SetPortbase(local_port);

        int status = m_session.Create(sess_prm, &ts_prm);
        if (status) {
            return;
        }

        std::cout << "listen port: " << local_port << std::endl;

        // remote
        uint32_t ipval = inet_addr(remote_ip.c_str());
        ipval = ntohl(ipval);

        jrtplib::RTPIPv4Address addr(ipval, remote_port);
        m_session.AddDestination(addr);
        m_session.SetDefaultPayloadType(96);
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
            if (!m_input_que.empty()) {
                AudioQueue::FrameType p = m_input_que.pop();
                m_session.SendPacket(p.first, p.second);
                std::cout << "Send Payload" << std::endl;
                // std::cout << "Send Payload: " << p.first << ", Samples: " << p.second << std::endl;
            }
        }
    }

    void poll(std::atomic<bool> &running)
    {
        std::cout << __func__ << std::endl;
        while (running) {
            m_session.Poll();
            m_session.BeginDataAccess();
            if (m_session.GotoFirstSourceWithData()) {
                do {
                    jrtplib::RTPPacket *pkt;
                    while ((pkt = m_session.GetNextPacket()) != nullptr) {
                        size_t len = pkt->GetPayloadLength();
                        int16_t *copy = new int16_t[len / sizeof(int16_t)];
                        std::memcpy(copy, pkt->GetPayloadData(), len);
                        // int16_t *data = (int16_t *)pkt->GetPayloadData();
                        m_output_que.push(copy, len);
                        std::cout << "Recv Payload" << std::endl;
                        // std::cout << "Recv Payload: " << data << ", Samples: " << samples << std::endl;
                        m_session.DeletePacket(pkt);
                    }
                } while (m_session.GotoNextSourceWithData());
            }
            m_session.EndDataAccess();
        }
    }

private:
    jrtplib::RTPSession m_session;
    std::string m_remote_ip;
    std::atomic<bool> m_running;
    AudioQueue &m_output_que;
    AudioQueue &m_input_que;
};

#endif // _RTP_SERVER_H_