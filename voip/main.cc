#include "global.h"
#include "ini.h"
#include "logger.h"
#include "web_agent_bridge.h"
#include "gui_ws_server.h"
#include "tts_request.h"
#include <assert.h>

int main()
{
    // logger
    Logger::init();
    LOG_INFO("client start");

    TTSPlayer::init("192.168.2.3", "51006", "/cosyvoice2");

    // config
    loadINICfg();

    // endpoint
    startEndpointLib(5060);

    auto web_ws_client = std::make_shared<WebWsClient>();
    g_agent_ws_client = std::make_shared<AgentWsClient>(asr_server_remote_host, asr_server_remote_port);
    g_manual_ws_client = std::make_shared<AgentWsClient>(asr_server_remote_host, asr_server_remote_port);
    auto gui_ws_server = std::make_shared<GuiWsServer>("0.0.0.0", 8001, web_ws_client);

    web_ws_client->set_server_sender(gui_ws_server);
    g_agent_ws_client->set_server_sender(gui_ws_server);
    g_manual_ws_client->set_server_sender(gui_ws_server);

    web_ws_client->start();
    g_agent_ws_client->start();
    g_manual_ws_client->start();

    auto bridget_agent = std::make_shared<WebAgentBridge>(g_agent_ws_client, web_ws_client);
    auto bridget_manual = std::make_shared<WebAgentBridge>(g_manual_ws_client, web_ws_client);
    web_ws_client->start_call();

    return 0;
}