#include <rtpsession.h>
#include <rtpudpv4transmitter.h>
#include <rtpipv4address.h>
#include <rtpsessionparams.h>
#include <rtperrors.h>
#include <rtpsourcedata.h>
#include <iostream>
#include <thread>
#include <cmath>
#include <vector>
#include <atomic>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace jrtplib;

void checkerror(int rtperr) {
    if (rtperr < 0) {
        std::cerr << "ERROR: " << RTPGetErrorString(rtperr) << std::endl;
        exit(-1);
    }
}

std::atomic<bool> running(true);
std::atomic<bool> sending(false);  // 发送开关

void sender(RTPSession *sess) {
    const int sampleRate = 8000;
    const double freq = 440.0; // A4
    const double amplitude = 30000.0;
    const int frameSize = 160; // 20ms

    double phase = 0.0;
    double step = 2 * M_PI * freq / sampleRate;

    while (running) {
        if (sending) {
            std::vector<int16_t> buffer(frameSize);
            for (int i = 0; i < frameSize; i++) {
                buffer[i] = static_cast<int16_t>(amplitude * sin(phase));
                phase += step;
                if (phase > 2 * M_PI) phase -= 2 * M_PI;
            }

            int status = sess->SendPacket((void*)buffer.data(),
                                          buffer.size() * sizeof(int16_t),
                                          96,   // dynamic payload type
                                          false,
                                          10);
            checkerror(status);
//            std::cout << "Sent packet: " << frameSize << " bytes" << std::endl;

            RTPTime::Wait(RTPTime(0,20000)); // 每20ms发一次
        } else {
            // 如果暂停，则稍微sleep避免空转CPU
            RTPTime::Wait(RTPTime(0,50000));
        }
    }
}

void receiver(RTPSession *sess) {
    while (running) {
        sess->Poll();
        sess->BeginDataAccess();
        if (sess->GotoFirstSourceWithData()) {
            do {
                RTPPacket *packet;
                while ((packet = sess->GetNextPacket()) != nullptr) {
                    std::string payload((char *)packet->GetPayloadData(), packet->GetPayloadLength());
                    std::cout << "Received packet: "
                              << packet->GetPayloadLength() << "bytes " << "Received: " << payload << std::endl;
                    sess->DeletePacket(packet);
                }
            } while (sess->GotoNextSourceWithData());
        }
        sess->EndDataAccess();
        RTPTime::Wait(RTPTime(0,1000));
    }
}

int main() {
#ifdef RTP_SOCKETTYPE_WINSOCK
    WSADATA dat;
    WSAStartup(MAKEWORD(2,2),&dat);
#endif

    RTPSession sess;
    RTPSessionParams sessparams;
    RTPUDPv4TransmissionParams transparams;

    sessparams.SetOwnTimestampUnit(1.0/8000.0);
    sessparams.SetAcceptOwnPackets(true);
    transparams.SetPortbase(8000);
    checkerror(sess.Create(sessparams, &transparams));

    uint32_t destip = ntohl(inet_addr("127.0.0.1"));
    uint16_t destport = 8002;
    RTPIPv4Address addr(destip,destport);
    checkerror(sess.AddDestination(addr));

    std::thread sendThread(sender, &sess);
    std::thread recvThread(receiver, &sess);

    std::cout << "Press Enter to toggle sending (start/stop). Press Ctrl+C to quit." << std::endl;

    while (running) {
        std::cin.get();  // 等待回车
        sending = !sending;  // 切换状态
        std::cout << (sending ? "[Sending started]" : "[Sending stopped]") << std::endl;
    }

    running = false;
    sendThread.join();
    recvThread.join();

    return 0;
}
