#include "rtp_server.h"
#include <jrtplib3/rtpsessionparams.h>
#include <jrtplib3/rtpudpv4transmitter.h>
#include <jrtplib3/rtppacket.h>

bool RTPServer::init(uint16_t localPort)
{
    using namespace jrtplib;

    RTPSessionParams sessParams;
    sessParams.SetOwnTimestampUnit(1.0 / 8000.0);
    sessParams.SetAcceptOwnPackets(true);

    RTPUDPv4TransmissionParams transParams;
    transParams.SetPortbase(localPort);

    int status = session.Create(sessParams, &transParams);
    return status >= 0;
}

void RTPServer::setRemote(const std::string &ip, uint16_t port)
{
    using namespace jrtplib;
    uint32_t ipAddr = inet_addr(ip.c_str());
    session.AddDestination(RTPIPv4Address(ntohl(ipAddr), port));
}

void RTPServer::sendAudioFrame(const uint8_t *data, size_t size)
{
    session.SendPacket(data, size, 0, true, 160); // PT=0, 20ms at 8000Hz
}

bool RTPServer::receiveAudioFrame(std::vector<uint8_t> &out)
{
    std::lock_guard<std::mutex> lock(rxMutex);
    if (rxQueue.empty())
        return false;
    out = std::move(rxQueue.front());
    rxQueue.pop();
    return true;
}

void RTPServer::poll()
{
    using namespace jrtplib;
    session.BeginDataAccess();
    if (session.GotoFirstSourceWithData()) {
        do {
            RTPPacket *pkt;
            while ((pkt = session.GetNextPacket()) != nullptr) {
                std::lock_guard<std::mutex> lock(rxMutex);
                rxQueue.push(std::vector<uint8_t>(
                    pkt->GetPayloadData(),
                    pkt->GetPayloadData() + pkt->GetPayloadLength()));
                session.DeletePacket(pkt);
            }
        } while (session.GotoNextSourceWithData());
    }
    session.EndDataAccess();
}
