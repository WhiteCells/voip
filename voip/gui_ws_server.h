#ifndef _GUI_WS_SERVER_H_
#define _GUI_WS_SERVER_H_

#include "logger.h"
#include "io_context_pool.h"
#include "global.h"
#include "web_ws_client.h"
#include "ws_interface.h"
#include "account.h"
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <json/json.h>
#include <queue>
#include <unordered_set>
#include <functional>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class WebSocketSession :
    public std::enable_shared_from_this<WebSocketSession>
{
public:
    explicit WebSocketSession(tcp::socket &&socket,
                              std::shared_ptr<WebWsClient> client)
        : m_stream(std::move(socket))
        , m_client(client)
    {
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
                LOG_INFO("recv account info: {}", root.toStyledString());
                handleAccountMessage(root);
            }
        }

        do_read();
    }

    void handleConfigMessage(const Json::Value &root)
    {
        if (!root.isMember("host") || !root["host"].isString()) {
            LOG_ERROR("::host");
            return;
        }

        if (!root.isMember("port") || !root["port"].isString()) {
            LOG_ERROR("::port");
            return;
        }

        if (!root.isMember("client_id") || !root["client_id"].isString()) {
            LOG_ERROR("::client_id");
            return;
        }

        if (!root.isMember("route") || !root["route"].isString()) {
            LOG_ERROR("::route");
            return;
        }

        g_gui_cfg.gui_host = root["host"].asString();
        g_gui_cfg.gui_port = root["port"].asString();
        g_gui_cfg.gui_client_id = root["client_id"].asString();
        g_gui_cfg.gui_target = root["route"].asString();

        m_client->restart();
    }

    void handleCommandMessage(const Json::Value &root)
    {
        endpoint.libRegisterThread("Worker");
        if (root.isMember("action")) {
            std::string action = root["action"].asString();

            if (action == "hangup" || action == "close") {
                local_hangup = "mediator";
                // 尝试执行挂断所有呼叫操作
                endpoint.hangupAllCalls();

                Json::Value response;
                response["status"] = "success";
                response["type"] = "close_status";

                Json::StreamWriterBuilder writerBuilder;
                std::string responseStr = Json::writeString(writerBuilder, response);

                send(responseStr);
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

        m_account.reset();
        std::string user = root["user"].asString();
        std::string pass = root["password"].asString();
        std::string host = root["host"].asString();
        // m_account = std::make_unique<Account>();
        m_account = std::make_unique<Account>(user, pass, host);
        
        // 设置注册状态回调函数
        if (m_account) {
            m_account->setRegistrationCallback([self = shared_from_this()](const std::string& code) {
                self->sendRegistrationStatus(code);
            });
        }
    }
    
    void sendRegistrationStatus(const std::string& code)
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
        m_stream.async_write(asio::buffer(m_write_que.front()),
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
    std::unique_ptr<Account> m_account;
};

class GuiWsServer :
    public IWSSender
{
public:
    GuiWsServer(const std::string &addr,
                unsigned int port,
                std::shared_ptr<WebWsClient> client)
        : m_acceptor(IOContextPool::getInstance()->getIOContext())
        , m_endpoint(asio::ip::make_address(addr), port)
        , m_client(client)
    {
        beast::error_code ec;
        if (m_acceptor.open(m_endpoint.protocol(), ec)) {
            LOG_ERROR("open: {}", ec.what());
            return;
        }
        if (m_acceptor.set_option(net::socket_base::reuse_address(true), ec)) {
            LOG_ERROR("set_option: {}", ec.what());
            return;
        }
        if (m_acceptor.bind(m_endpoint, ec)) {
            LOG_ERROR("bind: {}", ec.what());
            return;
        }
        if (m_acceptor.listen(net::socket_base::max_listen_connections, ec)) {
            LOG_ERROR("listen: {}", ec.what());
            return;
        }
        do_accept();
    }

    // ws server 发送
    virtual void send(const std::string &msg) override
    {
        std::unique_lock<std::mutex> lock(m_sessions_mtx);
        for (const auto &session : m_sessions) {
            session->send(msg);
        }
    }

private:
    void do_accept()
    {
        m_acceptor.async_accept([this](beast::error_code ec, tcp::socket socket) {
            if (ec) {
                LOG_ERROR("async_accept: {}", ec.what());
            }
            auto session = std::make_shared<WebSocketSession>(std::move(socket), m_client);
            {
                std::unique_lock<std::mutex> lock(m_sessions_mtx);
                m_sessions.insert(session);
            }
            session->run();
            do_accept();
        });
    }

    tcp::acceptor m_acceptor;
    tcp::endpoint m_endpoint;
    std::shared_ptr<WebWsClient> m_client;
    std::unordered_set<std::shared_ptr<WebSocketSession>> m_sessions;
    std::mutex m_sessions_mtx;
};

#endif // _GUI_WS_SERVER_H_