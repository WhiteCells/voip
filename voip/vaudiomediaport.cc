#include "vaudiomediaport.h"

#include <fstream>
#include <iostream>

voip::VAudioMediaPort::VAudioMediaPort()
{
}

voip::VAudioMediaPort::~VAudioMediaPort()
{
}

void voip::VAudioMediaPort::onFrameRequested(pj::MediaFrame &frame)
{
    std::cout << "frame send" << std::endl;
}

void voip::VAudioMediaPort::onFrameReceived(pj::MediaFrame &frame)
{
    std::cout << "frame recv" << std::endl;
    std::fstream recv_frame("recv.pcm", std::ios::binary | std::ios::app);
    if (recv_frame.is_open()) {
        recv_frame.write(reinterpret_cast<const char *>(frame.buf.data()), frame.size);
        recv_frame.close();
    }
}

// voip::VRecvAudioMediaPort::VRecvAudioMediaPort()
// {
// }

// voip::VRecvAudioMediaPort::~VRecvAudioMediaPort()
// {
// }

// void voip::VRecvAudioMediaPort::onFrameRequested(pj::MediaFrame &frame)
// {
//     std::ofstream outFile("output.pcm", std::ios::binary | std::ios::app);
//     if (outFile.is_open()) {
//         outFile.write(reinterpret_cast<const char *>(frame.buf.data()), frame.size);
//         outFile.close();
//     }
// }

// void voip::VRecvAudioMediaPort::onFrameReceived(pj::MediaFrame &frame)
// {
//     std::ofstream outFile("received_output.pcm", std::ios::binary | std::ios::app);
//     if (outFile.is_open()) {
//         outFile.write(reinterpret_cast<const char *>(frame.buf.data()), frame.size);
//         outFile.close();
//     }
// }

// voip::VSendAudioMediaPort::VSendAudioMediaPort()
// {
// }

// voip::VSendAudioMediaPort::~VSendAudioMediaPort()
// {
// }

// void voip::VSendAudioMediaPort::onFrameRequested(pj::MediaFrame &frame)
// {
//     std::ofstream outFile("output.pcm", std::ios::binary | std::ios::app);
//     if (outFile.is_open()) {
//         outFile.write(reinterpret_cast<const char *>(frame.buf.data()), frame.size);
//         outFile.close();
//     }
// }

// void voip::VSendAudioMediaPort::onFrameReceived(pj::MediaFrame &frame)
// {
//     std::ofstream outFile("received_output.pcm", std::ios::binary | std::ios::app);
//     if (outFile.is_open()) {
//         outFile.write(reinterpret_cast<const char *>(frame.buf.data()), frame.size);
//         outFile.close();
//     }
// }