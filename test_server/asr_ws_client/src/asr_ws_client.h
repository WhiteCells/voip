#pragma once

#include "ws_client.h"
#include "ws_client_base.h"

class ASRWsClient : public WsClient
{
public:
    ASRWsClient(asio::io_context &ioc,
                const std::string &host,
                const std::string &port,
                const std::string &path,
                bool use_ssl = true)
        : WsClient(ioc, host, port, path, use_ssl)
    {
    }
};

class ASRWsClient2 : public WsClientBase
{
public:
    using WsClientBase::WsClientBase;

    virtual void onConnected() override
    {
        std::cout << "ASRWsClient2 connected\n";
    }
};