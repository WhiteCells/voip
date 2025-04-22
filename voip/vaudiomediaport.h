#ifndef _VAUDIOMEDIAPORT_H_
#define _VAUDIOMEDIAPORT_H_

#include <pjsua2.hpp>
#include <pjsua2/media.hpp>

namespace voip {

class VAudioMediaPort : public pj::AudioMediaPort
{
public:
    VAudioMediaPort();
    virtual ~VAudioMediaPort();

    virtual void
    onFrameRequested(pj::MediaFrame &frame) override;

    virtual void
    onFrameReceived(pj::MediaFrame &frame) override;
};

// class VRecvAudioMediaPort : public VAudioMediaPort
// {
// public:
//     VRecvAudioMediaPort();
//     ~VRecvAudioMediaPort();

//     virtual void
//     onFrameRequested(pj::MediaFrame &frame) override;

//     virtual void
//     onFrameReceived(pj::MediaFrame &frame) override;
// };

// class VSendAudioMediaPort : public VAudioMediaPort
// {
// public:
//     VSendAudioMediaPort();
//     ~VSendAudioMediaPort();

//     virtual void
//     onFrameRequested(pj::MediaFrame &frame) override;

//     virtual void
//     onFrameReceived(pj::MediaFrame &frame) override;
// };

} // namespace voip

#endif // _VAUDIOMEDIAPORT_H_