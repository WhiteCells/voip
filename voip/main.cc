#include "global.h"
#include "ini.h"
#include "logger.h"
#include "ws_client.h"

int main()
{
    // logger
    Logger::init();
    LOG_INFO("client start");

    // config
    loadINICfg();

    // endpoint
    startEndpointLib(5060);

    net::io_context ioc;
    auto client = std::make_shared<VoipClient>(ioc);
    client->start_ws_client();
    ioc.run();

    while (1) {
    }
    return 0;
}