#include "global.h"
#include "ini.h"
#include "logger.h"
#include "web_ws_client.h"
#include "ws_server.h"

int main()
{
    // logger
    Logger::init();
    LOG_INFO("client start");

    // config
    loadINICfg();

    // endpoint
    startEndpointLib(5060);

    auto voip_client = std::make_shared<WebWsClient>();
    auto server = std::make_shared<WSServer>("0.0.0.0", 8001, voip_client);
    // auto agent_client

    voip_client->set_server_sender(server);
    voip_client->start_ws_client();
    voip_client->start_call_client();

    return 0;
}