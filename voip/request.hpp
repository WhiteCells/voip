#ifndef _REQUEST_H_
#define _REQUEST_H_

#include "io_context_pool.h"
#include "logger.h"
#include "global.h"

#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <json/json.h>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace voip {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = Json;
using tcp = asio::ip::tcp;

/**
 * @brief 封装 http 1.1 请求
 *
 * @param host 请求主机名
 * @param port 请求端口
 * @param target 请求路径
 * @param method 请求方法
 * @param params 可选请求路径查询参数
 * @param body 可选请求体内容
 * @return json::Value 响应数据回包
 */
inline json::Value httpRequest(
    const std::string &host,
    const std::string &port,
    const std::string &target,
    http::verb method,
    const std::map<std::string, std::string> &params = {},
    const std::string &body = "")
{
    auto &ioc = IOContextPool::getInstance()->getIOContext();
    tcp::resolver resolver(ioc);
    const auto endpoint = resolver.resolve(host, port);

    // Connect endpoint
    beast::tcp_stream stream(ioc);
    stream.connect(endpoint);

    // Params
    std::string query_string;
    for (const auto &[key, value] : params) {
        query_string += (query_string.empty() ? "?" : "&") + key + "=" + value;
    }

    std::string full_target = target + query_string;

    // Request
    http::request<http::string_body> req {method, full_target, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, "voip");

    // Body
    if (!body.empty() && (method == http::verb::post || method == http::verb::put)) {
        req.body() = body;
        req.set(http::field::content_type, "application/json");
        req.content_length(body.size());
    }

    http::write(stream, req);

    // Accept return package
    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    // Parse `res` to json::Value
    json::Value resp;
    std::string errs;
    std::istringstream iss(res.body());
    json::CharReaderBuilder reader;
    if (!json::parseFromStream(reader, iss, &resp, &errs)) {
        return json::Value {};
    }

    // Close `stream`
    beast::error_code ec;
    ec = stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    if (ec && ec != beast::errc::not_connected) {
        throw beast::system_error {ec};
    }

    return resp;
}

/**
 * @brief 通知服务端该客户端上线，同时获取客户端 ID
 */
inline void notify()
{
    const auto target_url = genUrl(URL_NOTIFY);
    LOG_INFO("Client notify target_url: {}", target_url);    

    auto resp = httpRequest(backend_host, backend_port, target_url, http::verb::get);
    LOG_INFO("Client connect backend: {}:{} Response: {}", backend_host, backend_port, resp.toStyledString());

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

    // update g_client_id
    g_client_id = data["clientId"].asString();
    LOG_INFO("current client ID: {}", g_client_id);
}

// inline void pullAccount(std::shared_ptr<>, const std::string &client_id)
// {
//     const auto target_url = genUrl(URL_ACCOUNTS, client_id);
//     LOG_INFO("Client pull Account target_url: {}", target_url);

//     std::map<std::string, std::string> params = {
//         {"threadsNum", std::to_string(g_thread_num)},
//     };

//     auto resp = httpRequest(
//         backend_host, backend_port, target_url,
//         http::verb::get, params);
//     LOG_INFO("Client pull Account response: {}", resp.toStyledString());

//         // resp::code
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
//         // // 创建 Caller
//         auto caller = std::make_shared<voip::Caller>(*vaccount);
//         m_caller_vec->push(caller);
//     }
//     // LOG_INFO("caller vec size: {}", m_caller_vec->size());
// }

/**
 * @brief 推送音频文件
 *
 * @param file_path 文件路径
 * @param client_id 客户端 ID
 *
 * @todo 断点续传
 * @todo 分片传输
 */
inline void pushFile(const std::string file_path, const std::string &client_id)
{
    const auto target_url = genUrl(URL_DIAL_WAV, client_id);
    LOG_INFO("Client push File target_url: {}", target_url);

    auto &ioc = IOContextPool::getInstance()->getIOContext();
    tcp::resolver resolver(ioc);
    beast::tcp_stream stream(ioc);

    auto const results = resolver.resolve(backend_host, backend_port);
    stream.connect(results);

    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LOG_WARN("Failed to open file: {}", file_path);
        return;
    }

    std::size_t file_size = file.tellg();
    if (file_size == 0) {
        LOG_WARN("File is empty: {}", file_path);
        return;
    }

    file.seekg(0);

    http::request<http::dynamic_body> req {
        http::verb::post, target_url, 11};
    req.set(http::field::host, backend_host);
    req.set("filename", std::filesystem::path(file_path).filename().string());

    beast::ostream(req.body()) << file.rdbuf();
    req.prepare_payload();

    try {
        http::write(stream, req);
        beast::flat_buffer buffer_res;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer_res, res);
    }
    catch (const std::exception &e) {
        LOG_WARN("Exception: ", e.what());
    }

    stream.socket().shutdown(tcp::socket::shutdown_both);

    LOG_INFO("Push file success");
}

/**
 * @brief 推送注册状态
 *
 * @param user 注册用户
 * @param state 注册状态
 * @param client_id 客户端 ID
 */
inline void pushRegStatus(const std::string &user, REG_STATE state, const std::string &client_id)
{
    const auto target_url = genUrl(URL_REG_STATUS, client_id);

    json::Value body_json;
    body_json["user"] = user;
    body_json["status"] = state;
    json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string body = json::writeString(writer, body_json);
    auto resp = httpRequest(
        backend_host, backend_port, target_url,
        http::verb::post, {}, body);
}

/**
 * @brief 推送通话状态
 *
 * @param phone_num 通话手机号
 * @param state 通话状态
 * @param client_id 客户端 ID
 */
inline void pushDialStatus(const std::string &phone_num, DIAL_STATE state, const std::string &client_id)
{
    const auto target_url = genUrl(URL_DIAL_STATUS, client_id);

    // std::string body = R"({"phoneNum": "dial", "dialStatus": "status"})";
    json::Value body_json;
    body_json["phoneNum"] = phone_num;
    body_json["status"] = state;
    json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string body = json::writeString(writer, body_json);
    auto resp = httpRequest(
        backend_host, backend_port, target_url,
        http::verb::post, {}, body);
}

/**
 * @brief 拉取拨号计划
 *
 * @param plans 传出参数，存储拨号计划
 * @param client_id 客户端 ID
 */
inline void pullDialplan(std::vector<std::string> &plans /* & */, const std::string &client_id)
{
    const auto target_url = genUrl(URL_DIALPLANS, client_id);

    auto resp = httpRequest(backend_host, backend_port, target_url, http::verb::get);
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
        plans.push_back(phone_num);
        // LOG_INFO("fetch pull phone: {}", phone_num); // 日志误导 bug
    }
}

} // namespace voip

#endif // _REQUEST_H_