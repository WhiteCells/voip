#include "server.h"
#include "client.h"
#include "global.h"
#include "ini.h"
#include "logger.h"

#include <iostream>

int main()
{
    // logger
    Logger::init();
    // LOG_INFO("client start");
    LOG_INFO("client start");

    // config
    loadINICfg();

    // endpoint
    startEndpointLib(5060);

    Client client;

    try {
        asio::io_context ioc(1);
        asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](boost::system::error_code ec, int signal_num) {
            if (ec) {
                std::cerr << "[Signal Number]: " << signal_num << std::endl;
                return;
            }
            ioc.stop();
        });
        std::make_shared<Server>(ioc, 8001)->start();
        ioc.run();
    }
    catch (const std::exception &e) {
        std::cerr << "[Exception]: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}