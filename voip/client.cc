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
#include <fstream>
#include <filesystem>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
namespace json = Json;
using tcp = asio::ip::tcp;

Client::Client(unsigned workers_num) :
    m_running(true),
    m_thread_pool(workers_num) // todo thread_num
{
    notify();
    pullAccount();
    pullDialplan();
    heartbeat();
    for (unsigned i = 0; i < 2; ++i) {
        std::cout << "[addTask]" << std::endl;
        m_thread_pool.addTask(std::bind(&Client::callTask, this));
    }
}

Client::~Client()
{
    m_running = false;
}

void Client::notify()
{
    std::cout << "[notify]" << std::endl;
    auto target_url = "/notify";
    std::cout << backend_host << ":" << backend_port << std::endl;

    auto resp = voip::httpRequest(
        backend_host, backend_port, target_url, http::verb::post);

    std::cout << "Response: " << resp.toStyledString() << std::endl;
    Logger::info("{} client connect backend: {}:{}", __FUNCTION__, backend_host, backend_port);

    // resp::code
    if (!resp.isMember("code") || !resp["code"].isInt() || resp["code"] != 200) {
        std::cerr << "code Invalid response format" << std::endl;
        Logger::error("{} resp::code", __FUNCTION__);
        return;
    }
    // resp::data
    if (!resp.isMember("data") || !resp["data"].isObject()) {
        std::cerr << "data Invalid response format" << std::endl;
        Logger::error("{} resp::data", __FUNCTION__);
        return;
    }
    const json::Value &data = resp["data"];
    if (!data.isMember("clientId") || !data["clientId"].isString()) {
        std::cerr << "data Invalid response format" << std::endl;
        Logger::error("{} resp::data::clientId", __FUNCTION__);
        return;
    }
    m_client_id = data["clientId"].asString();
    std::cerr << "[client]: " << m_client_id << std::endl;
    Logger::info("{} current client ID: {}", __FUNCTION__, m_client_id);
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
void Client::pullAccount()
{
    auto target_url = "/accounts/" + m_client_id;
    std::map<std::string, std::string> params = {{"threadsNum", std::to_string(thread_num)}};

    auto resp = voip::httpRequest(
        backend_host, backend_port, target_url,
        http::verb::get, params);

    std::cout << "Response: " << resp.toStyledString() << std::endl;
    Logger::info("{} client pull Account {}", __FUNCTION__, resp.toStyledString());

    std::cout << target_url << std::endl;
    std::cout << resp.toStyledString() << std::endl;

    // resp::code
    if (!resp.isMember("code") || resp["code"].asInt() != 200) {
        std::cerr << "code Client::pullAccount failed: Invalid response format" << std::endl;
        Logger::error("{} resp::code", __FUNCTION__);
        return;
    }
    // resp::data
    const json::Value &data = resp["data"];
    if (!data.isMember("accounts") || !data["accounts"].isArray()) {
        std::cerr << "accounts Client::pullAccount failed: Invalid response format" << std::endl;
        Logger::error("{} resp::data", __FUNCTION__);
        return;
    }
    // resp::data::accounts
    const json::Value &accounts = data["accounts"];
    m_caller_que = std::make_shared<CallerQueue>();
    for (const auto &acc : accounts) {
        if (!acc.isMember("user") || !acc.isMember("pass") || !acc.isMember("host")) {
            std::cerr << "acc Client::pullAccount failed: Invalid response format" << std::endl;
            Logger::error("{} pull Account failed", __FUNCTION__);
            continue;
        }
        // 创建 VAccount
        std::string user = acc["user"].asString();
        std::string pass = acc["pass"].asString();
        std::string host = acc["host"].asString();
        auto vaccount = std::make_shared<voip::VAccount>(user, pass, host);
        // 防止 vaccount 回收
        m_vacc_vec.push_back(vaccount);
        // 创建 Caller
        auto caller = std::make_shared<voip::Caller>(*vaccount);
        // 更新 m_caller_que
        m_caller_que->addCaller(caller);
    }

    std::cout << "caller que size: " << m_caller_que->size() << std::endl;
    Logger::info("{} caller que size: {}", __FUNCTION__, m_caller_que->size());
}

void Client::pushRegStatus()
{
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
    std::cout << ">>> pullDialplan" << std::endl;
    auto target_url = "/dialplans/" + m_client_id;
    auto resp = voip::httpRequest(
        backend_host, backend_port, target_url, http::verb::get);

    std::cout << "Response: " << resp.toStyledString() << std::endl;
    Logger::info("{} pull dialplan: {}", __FUNCTION__, resp.toStyledString());

    // resp::code
    if (!resp.isMember("code") || resp["code"] != 200) {
        std::cerr << "Client::pullDialplan failed: Invalid response format" << std::endl;
        Logger::error("{} resp::code", __FUNCTION__);
        return;
    }
    // resp::data
    if (!resp.isMember("data")) {
        std::cerr << "Client::pullDialplan failed: Invalid response format" << std::endl;
        Logger::error("{} resp::data", __FUNCTION__);
        return;
    }
    const json::Value data = resp["data"];
    if (!data.isMember("dialplans") || !data["dialplans"].isArray()) {
        std::cerr << "Invalid response format" << std::endl;
        Logger::error("{} resp::data::dialplans", __FUNCTION__);
        return;
    }
    // resp::data::dialplans
    const json::Value &dialplans = data["dialplans"];
    for (const auto &dialplan : dialplans) {
        if (!dialplan.isString()) {
            std::cerr << "Invalid dialplan format" << std::endl;
            Logger::error("{} resp::data::dialplans format", __FUNCTION__);
            continue;
        }
        std::string phone_num = dialplan.asString();
        std::cout << "[input dialplan]: " << std::endl;
        m_dialplan_que.addDialPlan(phone_num);
    }

    std::cout << "dialplan que size: " << m_dialplan_que.size() << std::endl;
    Logger::info("{} dialplan que size: {}", __FUNCTION__, m_dialplan_que.size());
}

void Client::heartbeat()
{
    auto &ioc = IOContextPool::getInstance()->getIOContext();
    auto timer = std::make_shared<AsyncTimer>(ioc, std::chrono::seconds {5});
    auto target_url = "/heartbeat/" + m_client_id;
    timer->start([timer, target_url]() {
        try {
            auto resp = voip::httpRequest(
                backend_host, backend_port, target_url, http::verb::post);
            std::cout << "heartbeat" << std::endl;
            Logger::info("Client send heartbeat");
        }
        catch (const std::exception &e) {
            std::cout << "Exception: " << e.what() << std::endl;
            Logger::warn("Client send heartbeat Exception: {}", e.what());
        }
    });
}

/*
 * 目前只做简单的tcp文件传输
 * todo:
 *  1. 断点续传
 *  2. 分片传输
 */
void Client::pushFile(const std::string &file_path, const std::string &target)
{
    auto &ioc = IOContextPool::getInstance()->getIOContext();
    tcp::resolver resolver(ioc);
    beast::tcp_stream stream(ioc);

    auto const results = resolver.resolve(backend_host, backend_port);
    stream.connect(results);

    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return;
    }

    std::size_t file_size = file.tellg();
    if (file_size == 0) {
        std::cerr << "File is empty: " << file_path << std::endl;
        return;
    }

    file.seekg(0);

    http::request<http::dynamic_body> req {http::verb::post, target, 11};
    req.set(http::field::host, backend_host);
    req.set("filename", std::filesystem::path(file_path).filename().string());

    beast::ostream(req.body()) << file.rdbuf();
    req.prepare_payload();

    try {
        http::write(stream, req);
        beast::flat_buffer buffer_res;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer_res, res);
        std::cout << "Response: " << res << std::endl;
    }
    catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        Logger::warn("Client send file Exception: {}", e.what());
    }

    stream.socket().shutdown(tcp::socket::shutdown_both);
}

void Client::pushDialStatus(const std::string &dial, const std::string &status)
{
    auto target_url = "/status/" + m_client_id;
    // std::string body = R"({"phoneNum": "dial", "dialStatus": "status"})";
    json::Value body_json;
    body_json["phoneNum"] = dial;
    body_json["dialStatus"] = status;
    json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string body = Json::writeString(writer, body_json);
    auto resp = voip::httpRequest(
        backend_host, backend_port, target_url,
        http::verb::post, {}, body);
    //
}

void Client::callTask()
{
    while (m_running) {
        std::cout << ">>> [callTask]: " << __FUNCTION__ << std::endl;
        // 这里可能出现竟态
        auto caller = m_caller_que->getCaller();
        auto dialplan = m_dialplan_que.getDialPlan();
        std::cout << "<<< [callTask]: " << __FUNCTION__ << std::endl;
        std::cout << "[Dialplan]: " << dialplan << std::endl;
        if (caller && !dialplan.empty()) {
            std::cout << "[callTask]" << std::endl;
            caller->call(dialplan, m_caller_que, caller);
            // m_caller_que->releaseCaller(std::move(caller));
        }
    }
}
