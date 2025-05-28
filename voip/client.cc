#include "client.h"
#include "io_context_pool.h"
#include "async_timer.h"
#include "request.hpp"
#include "thread_pool.h"
#include "global.h"
#include "logger.h"
#include "vaccount.h"

#include <boost/beast.hpp>
#include <json/json.h>
#include <iostream>
// #include <fstream>
// #include <filesystem>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
namespace json = Json;
using tcp = asio::ip::tcp;

Client::Client(unsigned workers_num) :
    m_running(true),
    m_thread_pool(workers_num),
    m_caller_vec(std::make_shared<CallerVec>())
{
    notify();
    pullAccount();
    // pullDialplan(); // 改为自动拉取
    heartbeat();
    for (unsigned i = 0; i < workers_num /*todo*/; ++i) {
        m_thread_pool.addTask(std::bind(&Client::callTask, this, i));
    }
}

Client::~Client()
{
    m_running = false;
}

void Client::notify()
{
    try {
        std::string target_url = genUrl(URL_NOTIFY);

        auto resp = voip::httpRequest(
            backend_host, backend_port, target_url, http::verb::post);

        LOG_INFO("client connect backend: {}:{} Response: {}", backend_host, backend_port, resp.toStyledString());

        // resp::code
        if (!resp.isMember("code") || !resp["code"].isInt() || resp["code"] != 200) {
            LOG_ERROR("resp::code");
            return;
        }
        // resp::data
        if (!resp.isMember("data") || !resp["data"].isObject()) {
            LOG_ERROR("resp::data");
            return;
        }
        const json::Value &data = resp["data"];
        if (!data.isMember("clientId") || !data["clientId"].isString()) {
            LOG_ERROR("resp::data::clientId");
            return;
        }
        g_client_id = data["clientId"].asString();
        LOG_INFO("current client ID: {}", g_client_id);
    }
    catch (const std::exception &e) {
        LOG_WARN("Exception: {}", e.what());
    }
}

/*
 * {
 *     "code": 200,
 *     "msg": "",
 *     "data": {
 *         "accounts": [
 *             {"user": "1002", "pass": "1002", "host": "192.168.10.51:5060"},
 *             {"user": "1003", "pass": "1003", "host": "192.168.10.51:5060"},
 *         ]
 *     }
 * }
 */
// void Client::pullAccount()
// {
//     const auto target_url = genUrl(URL_ACCOUNTS, m_client_id);

//     std::map<std::string, std::string> params = {{"threadsNum", std::to_string(thread_num)}};

//     auto resp = voip::httpRequest(
//         backend_host, backend_port, target_url,
//         http::verb::get, params);

//     std::cout << "Response: " << resp.toStyledString() << std::endl;
//     LOG_INFO("client pull Account {}", resp.toStyledString());

//     std::cout << target_url << std::endl;
//     std::cout << resp.toStyledString() << std::endl;

//     // resp::code
//     if (!resp.isMember("code") || resp["code"].asInt() != 200) {
//         LOG_ERROR("resp::code");
//         return;
//     }
//     // resp::data
//     const json::Value &data = resp["data"];
//     if (!data.isMember("accounts") || !data["accounts"].isArray()) {
//         LOG_ERROR("resp::data");
//         return;
//     }
//     // resp::data::accounts
//     const json::Value &accounts = data["accounts"];
//     m_caller_que = std::make_shared<CallerQueue>();
//     for (const auto &acc : accounts) {
//         if (!acc.isMember("user") || !acc.isMember("pass") || !acc.isMember("host")) {
//             LOG_ERROR("pull Account failed");
//             continue;
//         }
//         // 创建 VAccount
//         std::string user = acc["user"].asString();
//         std::string pass = acc["pass"].asString();
//         std::string host = acc["host"].asString();
//         auto vaccount = std::make_shared<voip::VAccount>(user, pass, host);
//         // 防止 vaccount 回收
//         m_vacc_vec.push_back(vaccount);
//         // 创建 Caller
//         auto caller = std::make_shared<voip::Caller>(*vaccount);
//         // 更新 m_caller_que
//         m_caller_que->addCaller(caller);
//     }

//     LOG_INFO("caller que size: {}", m_caller_que->size());
// }

void Client::pullAccount()
{
    const auto target_url = genUrl(URL_ACCOUNTS, g_client_id);

    std::map<std::string, std::string> params = {{"threadsNum", std::to_string(g_thread_num)}};

    auto resp = voip::httpRequest(
        backend_host, backend_port, target_url,
        http::verb::get, params);

    std::cout << "Response: " << resp.toStyledString() << std::endl;
    LOG_INFO("client pull Account {}", resp.toStyledString());

    std::cout << target_url << std::endl;
    std::cout << resp.toStyledString() << std::endl;

    // resp::code
    if (!resp.isMember("code") || resp["code"].asInt() != 200) {
        LOG_ERROR("resp::code");
        return;
    }
    // resp::data
    const json::Value &data = resp["data"];
    if (!data.isMember("accounts") || !data["accounts"].isArray()) {
        LOG_ERROR("resp::data");
        return;
    }
    // resp::data::accounts
    const json::Value &accounts = data["accounts"];
    m_caller_que = std::make_shared<CallerQueue>();
    for (const auto &acc : accounts) {
        if (!acc.isMember("user") || !acc.isMember("pass") || !acc.isMember("host")) {
            LOG_ERROR("pull Account failed");
            continue;
        }
        // 创建 VAccount
        std::string user = acc["user"].asString();
        std::string pass = acc["pass"].asString();
        std::string host = acc["host"].asString();
        auto vaccount = std::make_shared<voip::VAccount>(user, pass, host);
        // 防止 vaccount 回收
        m_vacc_vec.push_back(vaccount);
        // // 创建 Caller
        auto caller = std::make_shared<voip::Caller>(*vaccount);
        m_caller_vec->push(caller);
    }

    // LOG_INFO("caller vec size: {}", m_caller_vec->size());
}

/*
 * {
 *     "code": 200,
 *     "msg": "",
 *     "data": {
 *         "dialplans": [
 *             "",
 *         ]
 *     }
 * }
 */
void Client::pullDialplan()
{
    const auto target_url = genUrl(URL_DIALPLANS, g_client_id);

    auto resp = voip::httpRequest(
        backend_host, backend_port, target_url, http::verb::get);

    LOG_INFO("pull dialplan: {}", resp.toStyledString());

    // resp::code
    if (!resp.isMember("code") || resp["code"] != 200) {
        LOG_ERROR("resp::code");
        return;
    }
    // resp::data
    if (!resp.isMember("data")) {
        LOG_ERROR("resp::data");
        return;
    }
    const json::Value data = resp["data"];
    if (!data.isMember("dialplans") || !data["dialplans"].isArray()) {
        LOG_ERROR("resp::data::dialplans");
        return;
    }
    // resp::data::dialplans
    const json::Value &dialplans = data["dialplans"];
    for (const auto &dialplan : dialplans) {
        if (!dialplan.isString()) {
            LOG_ERROR("resp::data::dialplans format");
            continue;
        }
        std::string phone_num = dialplan.asString();
        std::cout << "[input dialplan]: " << std::endl;
        m_dialplan_que.addDialPlan(phone_num);
    }

    LOG_INFO("dialplan que size: {}", m_dialplan_que.size());
}

void Client::heartbeat()
{
    const auto target_url = genUrl(URL_HEARTBEAT, g_client_id);

    auto &ioc = IOContextPool::getInstance()->getIOContext();
    auto timer = std::make_shared<AsyncTimer>(ioc, std::chrono::seconds {5});
    timer->start([timer, target_url]() {
        try {
            auto resp = voip::httpRequest(
                backend_host, backend_port, target_url, http::verb::post);
            LOG_INFO("Client send heartbeat");
        }
        catch (const std::exception &e) {
            LOG_WARN("Client send heartbeat Exception: {}", e.what());
        }
    });
}

void Client::callTask(unsigned i)
{
    auto caller = m_caller_vec->getCaller(i);
    while (m_running) {
        LOG_INFO("to get dualplan");
        auto dialplan = m_dialplan_que.getDialPlan();
        LOG_INFO("tasking: {}", dialplan);
        caller->call(dialplan, g_client_id);
        sleep(120);
        // std::this_thread::sleep_for(std::chrono::seconds(120));
    }
}
