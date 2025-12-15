#pragma once

#include <boost/asio.hpp>

namespace net = boost::asio;

class TTSHttpClient
{
public:
    TTSHttpClient(net::io_context &ioc)
        : m_ioc(ioc)
    {
    }
    ~TTSHttpClient()
    {
    }

private:
    net::io_context &m_ioc;
};