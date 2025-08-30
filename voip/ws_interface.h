#ifndef _WS_INTERFACE_H_
#define _WS_INTERFACE_H_

#include <string>

class IWSSender
{
public:
    virtual ~IWSSender() = default;
    virtual void send(const std::string &msg) = 0;
};

#endif // _WS_INTERFACE_H_