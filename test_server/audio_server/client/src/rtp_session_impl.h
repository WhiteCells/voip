#ifndef _SESSION_IMPL_H_
#define _SESSION_IMPL_H_

#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtppacket.h>
#include <jrtplib3/rtpipv4address.h>
#include <jrtplib3/rtpipv6address.h>
#include <jrtplib3/rtpbyteaddress.h>
#include <jrtplib3/rtptcpaddress.h>

class RtpSessionImpl : public jrtplib::RTPSession
{
private:
    const jrtplib::RTPAddress *m_address;

public:
    explicit RtpSessionImpl();
    virtual ~RtpSessionImpl();

    virtual void OnRTPPacket(jrtplib::RTPPacket *pack, const jrtplib::RTPTime &receivetime, const jrtplib::RTPAddress *senderaddress)
    {
        m_address = senderaddress;
        auto address_type = senderaddress->GetAddressType();
        switch (address_type) {
            case jrtplib::RTPAddress::IPv4Address: {
                jrtplib::RTPIPv4Address *ipv4 = (jrtplib::RTPIPv4Address *)senderaddress;
                auto ip = ipv4->GetIP();
                auto port = ipv4->GetPort();
                auto rtcp_port = ipv4->GetRTCPSendPort();
            }
            case jrtplib::RTPAddress::IPv6Address: {
                jrtplib::RTPIPv6Address *ipv6 = (jrtplib::RTPIPv6Address *)senderaddress;
                auto ip = ipv6->GetIP();
                auto port = ipv6->GetPort();
            }
            case jrtplib::RTPAddress::ByteAddress: {
                jrtplib::RTPByteAddress *byte = (jrtplib::RTPByteAddress *)senderaddress;
                auto ip = byte->GetHostAddress();
                auto port = byte->GetPort();
            }
            case jrtplib::RTPAddress::UserDefinedAddress: {
            }
            case jrtplib::RTPAddress::TCPAddress: {
            }
        }
    }
    virtual void OnBYEPacket(jrtplib::RTPSourceData *srcdat)
    {
        
    }

    const jrtplib::RTPAddress *getAddress() const
    {
        return m_address;
    }
};

#endif // _SESSION_IMPL_H_