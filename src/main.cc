// #include "server/gui_ws_server.h"
#include "env.h"
#include "logger.h"

int main()
{
    Env::instance().load(".env");

    Logger::init();

    // auto server = std::make_shared<GuiWsServer>(Env::instance().getStr("GUI_WS_ADDR"),
    //                                             Env::instance().getInt("GUI_WS_PORT"));

    return 0;
}