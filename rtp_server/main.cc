#include "rtp_server.h"
#include <thread>

int main()
{
    RTPServer rtpServer;
    rtpServer.init(5004);
    rtpServer.setRemote("127.0.0.1", 5006); // send to client

    // connect mediaPort to PJSUA2 stream
    // ...

    // periodically call
    while (true) {
        rtpServer.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}
