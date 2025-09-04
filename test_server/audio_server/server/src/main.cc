#include "ws_server.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    try {
        Logger::init();
        net::io_context ioc {1};
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](boost::system::error_code ec, int signal_num) {
            LOG_INFO("Signal Number: {}", signal_num);
            if (ec) {
                LOG_ERROR("Signal Number: {}", signal_num);
            }
            ioc.stop();
        });
        auto ws_server = std::make_shared<WsServer>(ioc, "0.0.0.0", 8001);
        LOG_INFO("Ws Server Start ...");
        ioc.run();
    }
    catch (const std::exception &e) {
        LOG_ERROR("Exception: {}", e.what());
        return 1;
    }
    return 0;
}