#include "web_ws_client.h"
#include <thread>
#include "logger.h"

int main(int argc, char *argv[])
{
    Logger::init();

    auto client = std::make_shared<WebWsClient>();
    client->start();

    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}