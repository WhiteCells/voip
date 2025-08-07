#include "global.h"
#include "ini.h"
#include "logger.h"
#include "ws_client.h"
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

    auto client = std::make_shared<VoipClient>();
    auto server = std::make_shared<WSServer>("0.0.0.0", 8001, client);

    client->set_server_sender(server);
    client->start_ws_client();
    client->start_call_client();

    // while (1) {
    // }
    return 0;
}