#ifndef _RTP_SESSION_H_
#define _RTP_SESSION_H_

#include <jrtplib3/rtpsession.h>
#include <thread>
#include <deque>
#include <vector>
#include <mutex>
#include <atomic>

class RtpSession
{
public:
    void start();
    void stop();
    void set_remote_port(const std::string &addr, uint port);
    uint get_local_port();
    void poll();
    void send(std::vector<unsigned char> frame);

private:
    jrtplib::RTPSession m_session;
    std::thread m_poll_thread;
    std::atomic<bool> m_running;
    std::mutex m_mtx;
    std::deque<std::vector<uint8_t>> m_buffer;
};

#endif // _RTP_SESSION_H_