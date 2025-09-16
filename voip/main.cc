#include "global.h"
#include "ini.h"
#include "logger.h"
#include "web_agent_bridge.h"
#include "gui_ws_server.h"

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
    auto gui_ws_server = std::make_shared<GuiWsServer>("0.0.0.0", 8001, web_ws_client);

    web_ws_client->set_server_sender(gui_ws_server);
    agent_ws_client->set_server_sender(gui_ws_server);

    web_ws_client->start();
    agent_ws_client->start();

    auto bridget = std::make_shared<WebAgentBridge>(agent_ws_client, web_ws_client);

    web_ws_client->start_call();

    return 0;
}