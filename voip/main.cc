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
    g_agent_ws_client = std::make_shared<AgentWsClient>("127.0.0.1","10095");
    auto gui_ws_server = std::make_shared<GuiWsServer>("0.0.0.0", 8001, web_ws_client);

    web_ws_client->set_server_sender(gui_ws_server);
    g_agent_ws_client->set_server_sender(gui_ws_server);

    web_ws_client->start();
    g_agent_ws_client->start();

    auto bridget = std::make_shared<WebAgentBridge>(g_agent_ws_client, web_ws_client);

    web_ws_client->start_call();

//    auto t1 = TTSPlayer::getInstance();
//    auto t2 = TTSPlayer::getInstance();
//
//    auto l1 = IOContextPool::getInstance();
//    auto l3 = IOContextPool::getInstance();
//
//    printf("t1: %p, t2: %p\n", l1.get(), l3.get());

    return 0;
}