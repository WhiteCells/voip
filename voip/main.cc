#include "global.h"
#include "ini.h"
#include "logger.h"
#include "web_ws_client.h"
#include "agent_ws_client.h"
#include "bridge.h"
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

    auto web_ws_client = std::make_shared<WebWsClient>();
    auto agent_ws_client = std::make_shared<AgentWsClient>();
    auto server = std::make_shared<WSServer>("0.0.0.0", 8001, web_ws_client);

    agent_ws_client->start();
    web_ws_client->set_server_sender(server);
    web_ws_client->start();
    auto bridget = std::make_shared<Bridge>(agent_ws_client, web_ws_client);
    web_ws_client->start_call_client();

    return 0;
}