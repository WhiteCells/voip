#include "rtp_session.h"
#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtppacket.h>
#include <jrtplib3/rtpsessionparams.h>
#include <jrtplib3/rtpudpv4transmitter.h>

void RtpSession::start()
{
    using namespace jrtplib;
    RTPSessionParams sessparams;
    sessparams.SetOwnTimestampUnit(1.0 / 8000.0);
    sessparams.SetAcceptOwnPackets(true);

    RTPUDPv4TransmissionParams transparams;
    transparams.SetPortbase(0);

    int status = m_session.Create(sessparams, &transparams);
    if (status < 0) {
        return;
    }
}

void RtpSession::stop()
{
    const char *reason = "session closed";
    size_t reason_len = strlen(reason);
    m_session.BYEDestroy(jrtplib::RTPTime(1.0), reason, reason_len);
}

void RtpSession::set_remote_port(const std::string &addr, uint port)
{
    uint32_t ip = inet_addr(addr.c_str());
    ip = ntohl(ip);
    m_session.AddDestination(jrtplib::RTPIPv4Address(ip, port));
}

uint RtpSession::get_local_port()
{
    auto info = m_session.GetTransmissionInfo();
    if (!info) {
        return 0;
    }
    jrtplib::RTPUDPv4TransmissionInfo *v4_info = dynamic_cast<jrtplib::RTPUDPv4TransmissionInfo *>(info);
    uint port = v4_info->GetRTPPort();
    return port;
}

void RtpSession::poll()
{
    m_poll_thread = std::thread([this]() {
        while (true) {
            m_session.Poll();
            if (m_session.GotoFirstSourceWithData()) {
                do {
                    jrtplib::RTPPacket *packet;
                    while ((packet = m_session.GetNextPacket()) != nullptr) {
                        std::vector<uint8_t> data(packet->GetPayloadData(),
                                                  packet->GetPacketData() + packet->GetPacketLength());
                        {
                            std::lock_guard<std::mutex> lock(m_mtx);
                            m_buffer.push_back(std::move(data));
                            if (m_buffer.size() > 50) {
                                m_buffer.pop_front();
                            }
                        }
                        m_session.DeletePacket(packet);
                    }
                } while (m_session.GotoNextSourceWithData());
            }
            std::this_thread::sleep_for(std::chrono::microseconds(5));
        }
    });
}

void RtpSession::send(std::vector<unsigned char> frame)
{
    if (frame.size() > 0) {
        int status = m_session.SendPacket(frame.data(), frame.size());
        if (status < 0) {
            // LOG_INFO("");
        }
    }
}
