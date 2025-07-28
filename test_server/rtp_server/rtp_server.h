#ifndef _RTP_SERVER_H_
#define _RTP_SERVER_H_

#include <jrtplib3/rtpsession.h>
#include <mutex>
#include <queue>
#include <vector>

class RTPServer
{
public:
    bool init(uint16_t localPort);
    void setRemote(const std::string &ip, uint16_t port);

    void sendAudioFrame(const uint8_t *data, size_t size);
    bool receiveAudioFrame(std::vector<uint8_t> &out);

    void poll();

private:
    jrtplib::RTPSession session;
    std::mutex rxMutex;
    std::queue<std::vector<uint8_t>> rxQueue;
};

#endif // _RTP_SERVER_H_