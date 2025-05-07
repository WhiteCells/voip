// #define TEST
#include "server.h"
#ifdef TEST
#include "core.h"
#endif
#include "request.hpp"

#include <iostream>

int main()
{
#ifdef TEST
    auto core = Core::getInstance();
    core->makeCall("18871357225");
#endif
    try {
        auto res = voip::httpRequest("localhost", "5000", "/get_account", voip::http::verb::get);
        std::cout << res.toStyledString() << std::endl;
    }
    catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
    }
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