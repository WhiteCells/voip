#pragma once

#include "../logger.h"
#include "../client/web_ws_client.h"
#include "../io_context_pool.h"
#include "../core/sip_core.h"
#include "../event/event.h"
#include "../event/msg.h"
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <json/json.h>
#include <pjsua2.hpp>
#include <queue>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class WebSocketSession :
    public std::enable_shared_from_this<WebSocketSession>
{
public:
    explicit WebSocketSession(tcp::socket &&socket)
        : m_stream(std::move(socket))
    {
        EventBus::getInstance()->subscribe<IncomingAccRegStateMsg>([this](const IncomingAccRegStateMsg &msg) {
            Json::Value resp;
            resp["type"] = "registration_status";
            resp["code"] = std::to_string(msg.m_code);
            LOG_INFO("registration_status: {}", resp.toStyledString());
            this->send(resp.toStyledString());
        });

        EventBus::getInstance()->subscribe<WebConnStateMsg>([this](const WebConnStateMsg &msg) {
            LOG_INFO("backend_status: {}", msg.m_state);
            this->send(msg.m_state);
        });
    }

    ~WebSocketSession() = default;

    void run()
    {
        beast::get_lowest_layer(m_stream).expires_never();
        http::async_read(m_stream.next_layer(),
                         m_buffer, m_req,
                         beast::bind_front_handler(&WebSocketSession::on_read_http,
                                                   shared_from_this()));
    }

    void send(const std::string &message)
    {
        boost::asio::post(m_stream.get_executor(),
                          [self = shared_from_this(), msg = std::move(message)]() {
                              bool write_processing = !self->m_write_que.empty();
                              self->m_write_que.push(std::move(msg));
                              if (!write_processing) {
                                  self->do_write();
                              }
                          });
    }

private:
    void on_read_http(beast::error_code ec, std::size_t)
    {
        if (ec) {
            return;
        }
        if (m_req.target() != "/ws") {
            return;
        }
        m_stream.async_accept(m_req,
                              beast::bind_front_handler(&WebSocketSession::on_accept,
                                                        shared_from_this()));
    }

    void on_accept(beast::error_code ec)
    {
        if (ec) {
            return;
        }
        do_read();
    }

    void do_read()
    {
        m_stream.async_read(m_buffer,
                            beast::bind_front_handler(&WebSocketSession::on_read_ws,
                                                      shared_from_this()));
    }

    void on_read_ws(beast::error_code ec, std::size_t bytes)
    {
        if (ec == websocket::error::closed) {
            return;
        }
        if (ec) {
            return;
        }

        std::string msg = beast::buffers_to_string(m_buffer.data());
        m_buffer.consume(bytes);

        // 解析JSON消息
        Json::CharReaderBuilder reader_builder;
        Json::Value root;
        std::string errs;
        std::istringstream iss(msg);
        bool success = Json::parseFromStream(reader_builder, iss, &root, &errs);
        if (!success) {
            LOG_ERROR("parse error");
            do_read();
            return;
        }

        // 根据type字段处理不同类型的JSON消息
        if (root.isMember("type")) {
            std::string type = root["type"].asString();

            if (type == "config") {
                // 处理配置消息
                LOG_INFO("recv gui config: {}", root.toStyledString());
                handleConfigMessage(root);
            }
            else if (type == "command") {
                // 处理命令消息
                LOG_INFO("recv gui command: {}", root.toStyledString());
                handleCommandMessage(root);
            }
            else if (type == "sip_config") {
                // sip 配置
                LOG_INFO("recv account info: {}", root.toStyledString());
                handleAccountMessage(root);
            }
        }

        do_read();
    }

    void handleConfigMessage(const Json::Value &root)
    {
        try {
            auto host = root["host"].asString();
            auto port = root["port"].asString();
            auto route = root["route"].asString();
            auto client_id = root["client_id"].asString();
            auto &ioc = IOContextPool::getInstance()->getIOContext();
            m_client.reset();
            m_client = std::make_shared<WebWsClient>(ioc, host, port, route + "/" + client_id, "crt/backend-server.crt"); // todo
            m_client->start();
        }
        catch (const std::exception &e) {
            LOG_ERROR("handleConfigMessage: {}", e.what());
        }
    }

    void handleCommandMessage(const Json::Value &root)
    {
        pj::Endpoint::instance().libRegisterThread("Worker");
        if (root.isMember("action")) {
            std::string action = root["action"].asString();

            if (action == "hangup" || action == "close") {
                // todo 通知 SIPCore 挂断电话
                EventBus::getInstance()->publish(HangupEvent {});

                // local_hangup = "mediator";

                Json::Value resp;
                resp["status"] = "success";
                resp["type"] = "close_status";
                send(resp.toStyledString());
            }
        }
    }

    void handleAccountMessage(const Json::Value &root)
    {
        if (!root.isMember("user") || !root["user"].isString()) {
            LOG_ERROR("::user");
            return;
        }

        if (!root.isMember("password") || !root["password"].isString()) {
            LOG_ERROR("::password");
            return;
        }

        if (!root.isMember("host") || !root["host"].isString()) {
            LOG_ERROR("::host");
            return;
        }

        // todo 通知 SIPCore 注册账号
        auto user = root["user"].asString();
        auto pass = root["password"].asString();
        auto host = root["host"].asString();
        SIPCore::getInstance()->registerAccount(user, pass, host);

        // m_account.reset();
        // std::string user = root["user"].asString();
        // std::string pass = root["password"].asString();
        // std::string host = root["host"].asString();
        // // m_account = std::make_unique<Account>();
        // m_account = std::make_unique<Account>(user, pass, host);

        // // 设置注册状态回调函数
        // if (m_account) {
        //     m_account->setRegistrationCallback([self = shared_from_this()](const std::string &code) {
        //         self->sendRegistrationStatus(code);
        //     });
        // }
    }

    void sendRegistrationStatus(const std::string &code)
    {
        // 构建JSON响应
        Json::Value response;
        response["type"] = "registration_status";
        response["code"] = code;

        Json::StreamWriterBuilder writerBuilder;
        std::string responseStr = Json::writeString(writerBuilder, response);

        // 发送注册状态到前端
        send(responseStr);
    }

    void do_write()
    {
        m_stream.text(true);
        m_stream.async_write(net::buffer(m_write_que.front()),
                             [self = shared_from_this()](beast::error_code ec, std::size_t) {
                                 if (ec) {
                                     return;
                                 }
                                 self->m_write_que.pop();
                                 if (!self->m_write_que.empty()) {
                                     self->do_write(); // continue
                                 }
                             });
    }

private:
    websocket::stream<beast::tcp_stream> m_stream;
    beast::flat_buffer m_buffer;
    http::request<http::string_body> m_req;
    std::queue<std::string> m_write_que;
    std::shared_ptr<WebWsClient> m_client;
};