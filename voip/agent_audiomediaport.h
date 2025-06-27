#ifndef _AGENT_AUDIOMEDIAPORT_H_
#define _AGENT_AUDIOMEDIAPORT_H_

#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtppacket.h>
#include <jrtplib3/rtpipv4address.h>
#include <jrtplib3/rtpsessionparams.h>
#include <jrtplib3/rtpudpv4transmitter.h>
#include <pjsua2.hpp>
#include <queue>
#include <thread>

#include "logger.h"

class AgentAudioMediaPort : public pj::AudioMediaPort
{
private:
    jrtplib::RTPSession rtpSession;
    std::mutex bufferMutex;
    std::queue<std::vector<uint8_t>> recvAudioBuffer;
    bool running = false;
    std::thread recvThread;

    // 接收音频数据
    void recvLoop()
    {
        while (running) {
            rtpSession.Poll();
            rtpSession.BeginDataAccess();

            if (rtpSession.GotoFirstSourceWithData()) {
                do {
                    jrtplib::RTPPacket *packet;
                    while ((packet = rtpSession.GetNextPacket()) != nullptr) {
                        std::lock_guard<std::mutex> lock(bufferMutex);
                        recvAudioBuffer.emplace(
                            (uint8_t *)packet->GetPayloadData(),
                            (uint8_t *)packet->GetPayloadData() + packet->GetPayloadLength());
                        rtpSession.DeletePacket(packet);
                    }
                } while (rtpSession.GotoNextSourceWithData());
            }

            rtpSession.EndDataAccess();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

public:
    AgentAudioMediaPort()
    {
        // 配置 RTP 会话参数
        // jrtplib::RTPSessionParams sessionparams;
        // sessionparams.SetOwnTimestampUnit(1.0 / 8000.0); // 采样率8000Hz
        // sessionparams.SetAcceptOwnPackets(true);

        // jrtplib::RTPUDPv4TransmissionParams transparams;
        // transparams.SetPortbase(8000); // 本地端口

        // int status = rtpSession.Create(sessionparams, &transparams);
        // if (status < 0) {
        //     LOG_ERROR("Failed to create RTP session: {}", status);
        //     throw std::runtime_error("RTP session create failed");
        // }

        // 远端地址和端口
        // uint32_t ip = ntohl(inet_addr("127.0.0.1"));
        // jrtplib::RTPIPv4Address addr(ip, 8000);

        // status = rtpSession.AddDestination(addr);
        // if (status < 0) {
        //     LOG_ERROR("Failed to add destination: {}", status);
        //     throw std::runtime_error("RTP add destination failed");
        // }

        // running = true;
        // recvThread = std::thread(&AgentAudioMediaPort::recvLoop, this);
    }
    /*
     * Callbacks
     */
    /**
     * This callback is called to request a frame from this port. On input,
     * frame.size indicates the capacity of the frame buffer and frame.buf
     * will initially be an empty vector. Application can then set the frame
     * type and fill the vector.
     *
     * @param frame       The frame.
     */
    virtual void onFrameRequested(pj::MediaFrame &frame) override;

    /**
     * This callback is called when this port receives a frame. The frame
     * content will be provided in frame.buf vector, and the frame size
     * can be found in either frame.size or the vector's size (both
     * have the same value).
     *
     * @param frame       The frame.
     */
    virtual void onFrameReceived(pj::MediaFrame &frame) override;
};

#endif // _AGENT_AUDIOMEDIAPORT_H_