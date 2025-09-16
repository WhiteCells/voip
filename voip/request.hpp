#ifndef _REQUEST_H_
#define _REQUEST_H_

#include "io_context_pool.h"
#include "async_timer.h"
#include "logger.h"
#include "global.h"
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <json/json.h>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace voip {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace ssl = asio::ssl;
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
 * @return Json::Value 响应数据回包
 */
inline Json::Value httpRequest(const std::string &host,
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
        req.set(http::field::content_type, "application/Json");
        req.content_length(body.size());
    }

    http::write(stream, req);

    // Accept return package
    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    // Parse `res` to Json::Value
    Json::Value resp;
    std::string errs;
    std::istringstream iss(res.body());
    Json::CharReaderBuilder reader;
    if (!Json::parseFromStream(reader, iss, &resp, &errs)) {
        return Json::Value {};
    }

    // Close `stream`
    beast::error_code ec;
    ec = stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    if (ec && ec != beast::errc::not_connected) {
        throw beast::system_error {ec};
    }

    return resp;
}

inline Json::Value httpSSLRequest(const std::string &host,
                                  const std::string &port,
                                  const std::string &target,
                                  http::verb method,
                                  const std::map<std::string, std::string> &params = {},
                                  const std::string &body = "")
{
    auto &ioc = IOContextPool::getInstance()->getIOContext();

    // ssl
    ssl::context ctx(ssl::context::sslv23_client);
    ctx.set_verify_mode(ssl::verify_peer); // 启用证书验证
    ctx.load_verify_file(verify_file);     // CA

    // resolve
    tcp::resolver resolver(ioc);
    auto const results = resolver.resolve(host, port);

    // ssl stream
    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

    // SNI
    if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
        beast::error_code ec {static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()};
        throw beast::system_error {ec};
    }

    // tcp connect
    beast::get_lowest_layer(stream).connect(results);

    // tls handshake
    stream.handshake(ssl::stream_base::client);

    // query
    std::string query_string;
    for (const auto &[key, value] : params) {
        query_string += (query_string.empty() ? "?" : "&") + key + "=" + value;
    }
    std::string full_target = target + query_string;

    // request
    http::request<http::string_body> req {method, full_target, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, "voip");

    if (!body.empty() && (method == http::verb::post || method == http::verb::put)) {
        req.body() = body;
        req.set(http::field::content_type, "application/Json");
        req.content_length(body.size());
    }

    // write
    http::write(stream, req);

    // read
    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    // parse Json
    Json::Value resp;
    std::string errs;
    std::istringstream iss(res.body());
    Json::CharReaderBuilder reader;
    if (!Json::parseFromStream(reader, iss, &resp, &errs)) {
        return Json::Value {};
    }

    // close
    beast::error_code ec;
    ec = stream.shutdown(ec);
    if (ec == asio::error::eof) {
        ec.assign(0, ec.category()); // 忽略 EOF
    }
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
    // 读取 g_client_id 文件
    // 如果文件不存在则创建文件，创建 g_client_id(uuid) 并存储至文件
    if (std::filesystem::exists("./g_client_id")) {
        std::ifstream in("./g_client_id");
        if (!in.is_open()) {
            LOG_INFO("open g_client_id file failed");
            return;
        }
        std::getline(in, g_client_id);
        in.close();
    }
    else {
        boost::uuids::random_generator gen;
        boost::uuids::uuid uuid = gen();
        g_client_id = boost::uuids::to_string(uuid);
        LOG_INFO("gen g_client_id: {}", g_client_id);
        std::ofstream out("./g_client_id");
        if (!out.is_open()) {
            LOG_ERROR("open g_client_id file failed");
            return;
        }
        out << g_client_id;
        out.close();
    }
    const auto target_url = genUrl(URL_NOTIFY, g_client_id);
    for (;;) {
        try {
            Json::Value body_json;
            body_json["threads_num"] = g_thread_num;
            Json::StreamWriterBuilder writer;
            writer["indentation"] = "";
            std::string body = Json::writeString(writer, body_json);

            LOG_INFO("Client notify target_url: {}", target_url);

            auto resp = httpRequest(
                backend_host, backend_port,
                target_url, http::verb::post,
                {}, body);
            LOG_INFO("Client connect backend: {}:{} Response: {}", backend_host, backend_port, resp.toStyledString());
            LOG_INFO("body Json: {}", body);

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
            const Json::Value &data = resp["data"];
            if (!data.isMember("client_id")) {
                LOG_ERROR("resp::data::client_id");
                return;
            }

            // update g_client_id
            // g_client_id = data["client_id"].asString();
            LOG_INFO("current client ID: {}", g_client_id);
            break;
        }
        catch (const std::exception &e) {
            LOG_WARN("Exception: {}", e.what());
            std::this_thread::sleep_for(std::chrono::seconds {3});
            LOG_INFO("retry notify");
        }
    }
}

/**
 * @brief 拉取 Account
 *
 * @param accounts 传出参数
 * @param client_id 客户端 ID
 */
inline void pullAccount(std::vector<std::vector<std::string>> &accounts, const std::string &client_id)
{
    const auto target_url = genUrl(URL_ACCOUNTS, client_id);

    try {
        LOG_INFO("Client pull Account target_url: {}", target_url);

        // std::map<std::string, std::string> params = {
        //     {"threadsNum", std::to_string(g_thread_num)},
        // };
        // auto resp = httpRequest(
        //     backend_host, backend_port, target_url,
        //     http::verb::get, params);
        auto resp = httpRequest(
            backend_host, backend_port, target_url,
            http::verb::get);
        LOG_INFO("Client pull Account response: {}", resp.toStyledString());

        // resp::code
        if (!resp.isMember("code") || resp["code"].asInt() != 200) {
            LOG_ERROR("resp::code");
            return;
        }
        // resp::data
        const Json::Value &data = resp["data"];
        if (!data.isMember("accounts") || !data["accounts"].isArray()) {
            LOG_ERROR("resp::data");
            return;
        }
        // resp::data::accounts
        const Json::Value &accs = data["accounts"];
        for (const auto &acc : accs) {
            if (!acc.isMember("name") || !acc.isMember("pwd") || !acc.isMember("host")) {
                LOG_ERROR("pull Account failed");
                continue;
            }
            std::string id = acc["id"].asString();
            std::string user = acc["name"].asString();
            std::string pass = acc["pwd"].asString();
            std::string host = acc["host"].asString();
            std::vector<std::string> acc_info;
            acc_info.push_back(id);
            acc_info.push_back(user);
            acc_info.push_back(pass);
            acc_info.push_back(host);
            accounts.push_back(acc_info);
        }
    }
    catch (const std::exception &e) {
        LOG_WARN("Exception: {}", e.what());
        // std::this_thread::sleep_for(std::chrono::seconds {3});
        // LOG_INFO("retry notify");
    }
}

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

    try {
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
    catch (const std::exception &e) {
        LOG_WARN("Exception: {}", e.what());
    }
}

/**
 * @brief 推送注册状态
 *
 * @param user 注册用户
 * @param state 注册状态
 * @param client_id 客户端 ID
 */
inline void pushRegStatus(const std::string &id, const std::string &state, const std::string &client_id)
{
    const auto target_url = genUrl(URL_REG_STATUS, client_id);
    try {
        Json::Value body_json;
        body_json["account_id"] = id;
        body_json["status"] = state;
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        std::string body = Json::writeString(writer, body_json);
        auto resp = httpRequest(
            backend_host, backend_port, target_url,
            http::verb::put, {}, body);
    }
    catch (const std::exception &e) {
        LOG_WARN("Exception: {}", e.what());
    }
}

/**
 * @brief 推送通话状态
 *
 * @param phone_num 通话手机号
 * @param state 通话状态
 * @param client_id 客户端 ID
 */
inline void pushDialStatus(
    const int dialplan_id,
    const std::string &phone_num,
    const std::string state,
    const std::string &client_id,
    const std::string &account_id)
{
    const auto target_url = genUrl(URL_DIAL_STATUS, client_id);

    try {
        Json::Value body_json;
        body_json["id"] = dialplan_id;
        body_json["phone"] = phone_num;
        body_json["status"] = state;
        body_json["account_id"] = account_id;
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        std::string body = Json::writeString(writer, body_json);
        LOG_INFO("dialplan status: {}", body);
        auto resp = httpRequest(
            backend_host, backend_port, target_url,
            http::verb::put, {}, body);
    }
    catch (const std::exception &e) {
        LOG_WARN("Exception: {}", e.what());
    }
}

/**
 * @brief 拉取拨号计划
 *
 * @param plans 传出参数，存储拨号计划
 * @param client_id 客户端 ID
 */
inline void pullDialplan(std::vector<std::pair<int, std::string>> &plans /* & */, const std::string &client_id)
{
    const auto target_url = genUrl(URL_DIALPLANS, client_id);

    try {
        auto resp = httpRequest(
            backend_host, backend_port,
            target_url, http::verb::get);
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
        const Json::Value data = resp["data"];
        if (!data.isMember("dialplans") || !data["dialplans"].isArray()) {
            LOG_ERROR("resp::data::dialplans");
            return;
        }
        // resp::data::dialplans
        const Json::Value &dialplans = data["dialplans"];
        for (const auto &dialplan : dialplans) {
            if (!dialplan.isMember("phone") || !dialplan.isMember("id")) {
                LOG_ERROR("resp::data::dialplans::phone");
                continue;
            }
            int id = dialplan["id"].asInt();
            std::string phone = dialplan["phone"].asString();
            // if (!dialplan.isString()) {
            //     LOG_ERROR("resp::data::dialplans format");
            //     continue;
            // }
            // std::string phone_num = dialplan.asString();
            std::pair p = std::make_pair(id, phone);
            plans.push_back(p);
        }
    }
    catch (const std::exception &e) {
        LOG_WARN("Exception: {}", e.what());
    }
}

/**
 * @brief 客户端心跳
 */
inline void heartbeat()
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

inline void pushAccountsRegState(const std::vector<AccountsRegState> &accounts_reg_state)
{
    const auto target_url = genUrl(URL_ACCOUNTS_REGSTATE, g_gui_cfg.gui_client_id);
    try {
        Json::Value accounts_array(Json::arrayValue);
        for (const auto &account : accounts_reg_state) {
            Json::Value body_json;
            body_json["account_id"] = account.account_id;
            body_json["status"] = account.status;
            accounts_array.append(body_json);
        }
        Json::Value root;
        root["accounts"] = accounts_array;
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        std::string body = Json::writeString(writer, root);
        LOG_INFO("push call state body: {}", body);
        auto resp = httpSSLRequest(backend_host, backend_port, target_url,
                                   http::verb::post, {}, body);
    }
    catch (const std::exception &e) {
        LOG_WARN("Exception: {}", e.what());
    }
}

inline void pushCallState(const std::string &phone,
                          const int status,
                          const int call_type,
                          const std::string &hangup_direction)
{
    const auto target_url = genUrl(URL_CALL_STATE, g_gui_cfg.gui_client_id);
    try {
        Json::Value body_json;
        body_json["task_id"] = g_task_id;
        body_json["phone"] = phone;
        body_json["status"] = status;
        body_json["call_type"] = call_type;
        body_json["hangup_direction"] = hangup_direction;

        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        std::string body = Json::writeString(writer, body_json);
        LOG_INFO("push call state body: {}", body);
        auto resp = httpSSLRequest(backend_host, backend_port, target_url,
                                   http::verb::post, {}, body);
    }
    catch (const std::exception &e) {
        LOG_WARN("Exception: {}", e.what());
    }
}

inline void pushGroupCallFinished(bool is_push)
{
    const auto target_url = genUrl(URL_GROUP_CALL_STATE, g_gui_cfg.gui_client_id);
    try {
        std::map<std::string, std::string> params;
        params["is_push"] = is_push ? "true" : "false";
        auto resp = httpSSLRequest(backend_host, backend_port, target_url,
                                   http::verb::post, params);
        LOG_INFO("push group call finished");
    }
    catch (const std::exception &e) {
        LOG_WARN("Exception: {}", e.what());
    }
}

} // namespace voip

#endif // _REQUEST_H_