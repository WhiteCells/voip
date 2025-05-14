#include "client.h"
#include "io_context_pool.h"
#include "async_timer.h"
#include "request.hpp"

#include <boost/beast.hpp>
#include <iostream>

namespace http = boost::beast::http;

Client::Client()
{
    notify();
    heartbeat();

}

Client::~Client()
{
}


/*
{
    ""
}
*/
void Client::heartbeat()
{
    auto &ioc = IOContextPool::getInstance()->getIOContext();
    auto timer = std::make_shared<AsyncTimer>(ioc, std::chrono::seconds {5});
    timer->start([timer, &ioc, this]() {
        auto resp = voip::httpRequest(ioc, "localhost", "5000", "/heartbeat/" + m_client_id, http::verb::post);
        std::cout << "heartbeat" << std::endl;
        // update `caller_que` `dialplan_que`

    });
}

void Client::notify()
{
    auto &ioc = IOContextPool::getInstance()->getIOContext();
    auto resp = voip::httpRequest(ioc, "localhost", "5000", "/notify", http::verb::get);
    if (resp.isMember("id")) {
        m_client_id = resp["id"].asString();
        std::cout << "client id: " << m_client_id << std::endl;
    }
}

void Client::uploadFile()
{
}

void Client::uploadStatus()
{
}
