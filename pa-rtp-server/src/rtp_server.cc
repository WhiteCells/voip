#include "rtp_server.h"
#include <jrtplib3/rtpsessionparams.h>
#include <jrtplib3/rtpudpv4transmitter.h>
#include <jrtplib3/rtppacket.h>
#include <iostream>

RtpServer::RtpServer(AudioQueue &output_que, AudioQueue &input_que,
                     const std::string &remote_ip, uint16_t remote_port, uint16_t local_port,
                     double tsunit, uint32_t tsinc) :
    m_remote_ip(remote_ip),
    m_remote_port(remote_port),
    m_local_port(local_port),
    m_tsunit(tsunit),
    m_tsinc(tsinc),
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
    m_session.SetDefaultPayloadType(0);
    m_session.SetDefaultMark(false);
    m_session.SetDefaultTimestampIncrement(tsinc);
}

RtpServer::~RtpServer()
{
}

void RtpServer::sendFrame(const void *data, size_t len)
{
    m_session.SendPacket(data, len);
    std::cout << "send frame: " << data << std::endl;
}

void RtpServer::send()
{
    std::cout << __func__ << std::endl;
    while (true) {
        if (!m_input_que.empty()) {
            AudioQueue::FrameType p = m_input_que.pop();
            m_session.SendPacket(p.first, p.second);
            std::cout << "Send Payload" << std::endl;
            // std::cout << "Send Payload: " << p.first << ", Samples: " << p.second << std::endl;
        }
    }
}

void RtpServer::poll(std::atomic<bool> &running)
{
    std::cout << __func__ << std::endl;
    while (running) {
        m_session.Poll();
        m_session.BeginDataAccess();
        if (m_session.GotoFirstSourceWithData()) {
            do {
                jrtplib::RTPPacket *pkt;
                while ((pkt = m_session.GetNextPacket()) != nullptr) {
                    uint16_t *data = (uint16_t *)pkt->GetPayloadData();
                    size_t len = pkt->GetPayloadLength();
                    m_output_que.push(data, len);
                    std::cout << "Recv Payload" << std::endl;
                    // std::cout << "Recv Payload: " << data << ", Samples: " << samples << std::endl;
                    m_session.DeletePacket(pkt);
                }
            } while (m_session.GotoNextSourceWithData());
        }
        m_session.EndDataAccess();
    }
}