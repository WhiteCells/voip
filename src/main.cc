#include "io_context_pool.h"
#include "server/gui_ws_server.h"
#include "core/outcoming_center.h"
#include "core/endpoint_core.h"
#include "logger.h"
#include "env.h"

int main()
{
    Env::instance().load();

    Logger::init();

    startEndpoint();

    auto &ioc = IOContextPool::getInstance()->getIOContext();

    auto server = std::make_shared<GuiWsServer>(ioc,
                                                Env::instance().getStr("GUI_WS_HOST"),
                                                Env::instance().getInt("GUI_WS_PORT"));

    OutcomingCenter::getInstance()->start();
}