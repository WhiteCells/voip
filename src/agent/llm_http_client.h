#pragma once

#include <boost/asio.hpp>

namespace net = boost::asio;

class LLMHttpClient
{
public:
    LLMHttpClient(net::io_context &ioc)
        : m_ioc(ioc)
    {
    }
    ~LLMHttpClient()
    {
    }

private:
    net::io_context &m_ioc;
};