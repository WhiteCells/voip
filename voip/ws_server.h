#ifndef _WS_SERVER_H_
#define _WS_SERVER_H_

#include "logger.h"
#include "io_context_pool.h"
#include "global.h"
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <json/json.h>
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
    explicit WebSocketSession(tcp::socket &&socket) :
        m_stream(std::move(socket))
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

    void send(std::string message)
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
        if (m_req.target() != "ws://127.0.0.1:8088/ws/client/1d6616dc-bcef-4927-80e9-72a186b22ee6") {
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
                handleConfigMessage(root);
            }
            else if (type == "command") {
                // 处理命令消息
                handleCommandMessage(root);
            }
        }

        // 继续读取下一个消息
        m_buffer.consume(bytes);
        do_read();
    }

    void handleConfigMessage(const Json::Value& root)
    {
        // 提取配置信息并保存到GUIConfig结构体
        if (root.isMember("host")) {
            g_gui_cfg.gui_host = root["host"].asString();
        }
        if (root.isMember("port")) {
            g_gui_cfg.gui_port = root["port"].asString();
        }
        if (root.isMember("client_id")) {
            g_gui_cfg.gui_client_id = root["client_id"].asString();
            client_id = root["client_id"].asString();
            g_client_id = root["client_id"].asString();
        }
        if (root.isMember("route")) {
            g_gui_cfg.gui_target = root["route"].asString();
        }

        // std::cout << "GUI Config updated - Host: " << g_gui_cfg.gui_host
        //           << ", Port: " << g_gui_cfg.gui_port
        //           << ", Target: " << g_gui_cfg.gui_target
        //           << ", ClientID: " << g_gui_cfg.gui_client_id << std::endl;
    }

    void handleCommandMessage(const Json::Value& root)
    {
        if (root.isMember("action")) {
            std::string action = root["action"].asString();

            if (action == "hangup") {
                // 尝试执行挂断所有呼叫操作
                endpoint.hangupAllCalls();

                Json::Value response;
                response["close_status"] = "success";

                Json::StreamWriterBuilder writerBuilder;
                std::string responseStr = Json::writeString(writerBuilder, response);

                send(responseStr);
            }
        }
        // on_read_ws_handler

        // config
        //  host
        //  port
        //  url
        //  client_id
        // command
        //  hangup
        // endpoint.hangupAllCalls();
        // LOG_INFO("on_read_ws: {}", msg);
    }

    void do_write()
    {
        m_stream.text(true);
        m_stream.async_write(boost::asio::buffer(m_write_que.front()),
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
};

class WSServer
{
public:
    WSServer(boost::asio::io_context &ioc, std::string addr, unsigned int port) :
        m_acceptor(ioc),
        m_endpoint(asio::ip::make_address(addr), port)
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

private:
    void do_accept()
    {
        m_acceptor.async_accept([this](beast::error_code ec, tcp::socket socket) {
            if (ec) {
                LOG_ERROR("async_accept: {}", ec.what());
            }
            std::make_shared<WebSocketSession>(std::move(socket))->run();
            do_accept();
        });
    }

    tcp::acceptor m_acceptor;
    tcp::endpoint m_endpoint;
};

#endif // _WS_SERVER_H_