#ifndef _RTP_SERVER_H_
#define _RTP_SERVER_H_

#include "audio_queue.h"
#include <jrtplib3/rtpsession.h>
#include <string>
#include <cstdint>
#include <functional>
#include <atomic>

class RtpServer
{
public:
    using RecvCallback = std::function<void(const float *, size_t)>;
    RtpServer(AudioQueue &output_que, AudioQueue &input_que,
              const std::string &remote_ip,
              uint16_t remote_port, uint16_t local_port,
              double tsunit, uint32_t tsinc);
    virtual ~RtpServer();

    void sendFrame(const void *data, size_t len);
    void send();
    void poll(std::atomic<bool> &);

private:
    jrtplib::RTPSession m_session;
    std::string m_remote_ip;
    uint16_t m_remote_port;
    uint16_t m_local_port;
    double m_tsunit;
    uint32_t m_tsinc;
    std::atomic<bool> m_running;
    AudioQueue &m_output_que;
    AudioQueue &m_input_que;
};

#endif // _RTP_SERVER_H_