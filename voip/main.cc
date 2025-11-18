#include "global.h"
#include "ini.h"
#include "logger.h"
#include "web_agent_bridge.h"
#include "gui_ws_server.h"
#include "tts_request.h"
#include <assert.h>
#ifdef _WIN32
#include <windows.h>
#endif

int main()
{
#ifdef _WIN32
    // 设置控制台为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    // logger
    Logger::init();
    LOG_INFO("client start");

    // config
    loadINICfg();

    g_tts_thread_pool = std::make_unique<TTSThreadPool>(2);

    TTSPlayer::init(tts_server_remote_host, tts_server_remote_port, tts_server_remote_target);

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

    // auto bridget_agent = std::make_shared<WebAgentBridge>(g_agent_ws_client, web_ws_client);
    // auto bridget_manual = std::make_shared<WebAgentBridge>(g_manual_ws_client, web_ws_client);

    web_ws_client->start_call();

    return 0;
}