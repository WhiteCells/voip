#include "client.h"
#include "io_context_pool.h"
#include "async_timer.h"
#include "request.hpp"
#include "thread_pool.h"

#include <boost/beast.hpp>
#include <json/json.h>
#include <iostream>

#define HOST "localhost"
#define PORT "5000"

namespace http = boost::beast::http;
namespace json = Json;

Client::Client(unsigned workers_num) :
    m_thread_pool(workers_num)
{
    notify();
    pullAccount();
    pullDialplan();
    heartbeat();
    for (unsigned i = 0; i < workers_num; ++i) {
        m_thread_pool.addTask(std::bind(&Client::callTask, this));
    }
}

Client::~Client()
{
}

void Client::notify()
{
    auto resp = voip::httpRequest(HOST, PORT, "/notify", http::verb::get);
    if (resp.isMember("id")) {
        m_client_id = resp["id"].asString();
        std::cout << "client id: " << m_client_id << std::endl;
    }
}

/*
 * {
 *     "accounts": [
 *         {"user": "1002", "pass": "1002", "host": "192.168.10.51:5060"},
 *         {"user": "1003", "pass": "1003", "host": "192.168.10.51:5060"},
 *     ]
 * }
 */
void Client::pullAccount()
{
    auto resp = voip::httpRequest(HOST, PORT, "/accounts/" + m_client_id, http::verb::get);
    // 解析 resp
    if (!resp.isMember("accounts") || !resp["accounts"].isArray()) {
        std::cerr << "Invalid response format" << std::endl;
        return;
    }
    const json::Value &accounts = resp["accounts"];
    for (const auto &acc : accounts) {
        if (!acc.isMember("user") || !acc.isMember("pass") || !acc.isMember("host")) {
            std::cerr << "Invalid response format" << std::endl;
            continue;
        }
        // 创建 VAccount
        std::string user = accounts["user"].asString();
        std::string pass = accounts["pass"].asString();
        std::string host = accounts["host"].asString();
        auto vaccount = std::make_shared<voip::VAccount>(user, pass, host);
        // 创建 Caller
        auto caller = std::make_unique<voip::Caller>(*vaccount);
        // 更新 m_caller_que
        m_caller_que.addCaller(std::move(caller));
    }
}

void Client::pushRegStatus()
{
}

/*
{
    "dialplans": [
        "111",
        "222",
    ]
}
*/
void Client::pullDialplan()
{
    auto resp = voip::httpRequest(HOST, PORT, "/dialplans/" + m_client_id, http::verb::get);
    if (!resp.isMember("dialplans") || !resp["dialplans"].isArray()) {
        std::cerr << "Invalid response format" << std::endl;
        return;
    }
    const json::Value &dialplans = resp["dialplan"];
    for (const auto &dialplan : dialplans) {
        if (!dialplan.isString()) {
            std::cerr << "Invalid dialplan format" << std::endl;
            continue;
        }
        std::string phone_num = dialplan.asString();
        m_dialplan_que.addDialPlan(phone_num);
    }
}

void Client::heartbeat()
{
    auto &ioc = IOContextPool::getInstance()->getIOContext();
    auto timer = std::make_shared<AsyncTimer>(ioc, std::chrono::seconds {5});
    timer->start([timer, this]() {
        auto resp = voip::httpRequest(HOST, PORT, "/heartbeat/" + m_client_id, http::verb::post);
        std::cout << "heartbeat" << std::endl;
    });
}

void Client::pushFile()
{
}

void Client::pushDialStatus()
{
}

void Client::callTask()
{
    while (true) {
        auto caller = m_caller_que.getCaller();
        auto dialplan = m_dialplan_que.getDialPlan();
        if (caller && !dialplan.empty()) {
            caller->call(dialplan);
            m_caller_que.releaseCaller(std::move(caller));
        }
    }
}
