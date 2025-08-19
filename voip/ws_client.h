#ifndef _WS_CLIENT2_H_
#define _WS_CLIENT2_H_

#include "logger.h"
#include "global.h"
#include "io_context_pool.h"
#include "coordinator.h"
#include "thread_pool.h"
#include "vaccount.h"
#include "account_check.h"
#include "account_check_manager.h"
#include "caller_queue.h"
#include "dialplan_queue.h"
#include "ws_interface.h"
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <json/json.h>
#include <functional>
#include <memory>
#include <string>
#include <atomic>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

class VoipClient :
    public std::enable_shared_from_this<VoipClient>
{
private:
    std::unique_ptr<tcp::resolver> m_resolver;
    std::unique_ptr<websocket::stream<beast::ssl_stream<beast::tcp_stream>>> m_ws;
    beast::flat_buffer m_buffer;
    std::string m_host = backend_host;
    std::string m_port = backend_port;
    std::string m_client_id = client_id;
    std::string m_target;
    std::function<void(const std::string &)> m_on_read_handler;
    std::atomic<bool> m_running {true};
    std::mutex m_batch_mtx;
    std::condition_variable m_batch_cv;
    std::size_t m_batch_remain;
    std::size_t m_worker_num;
    ThreadPool m_thread_pool;

    std::shared_ptr<CallerQueue> m_caller_que;
    std::shared_ptr<DialPlanQueue> m_dialplan_que;
    std::vector<std::shared_ptr<voip::VAccount>> m_acc_vec;
    std::shared_ptr<IWSSender> m_server_sender;
    int m_recv_call_type;

public:
    VoipClient() :
        // m_resolver(net::make_strand(ioc)),
        // m_ws(net::make_strand(ioc)),
        m_thread_pool(5),
        m_caller_que(std::make_shared<CallerQueue>()),
        m_dialplan_que(std::make_shared<DialPlanQueue>())
    {
        m_on_read_handler = [this](const std::string &msg) {
            static thread_local bool pj_thread_registered = false;
            if (!pj_thread_registered) {
                endpoint.libRegisterThread("Worker");
                pj_thread_registered = true;
            }
            // AccountCheckManager::getInstance()->clear();
            // m_acc_vec.clear();

            // 程序启动后
            // 1. 接收账号信息
            // 2. 接收拨号信息，存放队列
            LOG_INFO("recv1: {}", msg);
            Json::CharReaderBuilder reader_builder;
            Json::Value root;
            std::string errs;
            std::istringstream iss(msg);
            bool success = Json::parseFromStream(reader_builder, iss, &root, &errs);
            if (!success) {
                LOG_INFO("parse error");
                return;
            }
            LOG_INFO("recv json format: {}", root.toStyledString());
            const int request_type = root["request_type"].asInt();
            // 经过 1 后才能 0
            // 账号校验
            if (request_type == 1) {
                LOG_INFO("check accounts type");
                // accounts
                if (!root.isMember("accounts") || !root["accounts"].isArray()) {
                    LOG_ERROR("::accounts");
                    return;
                }
                // nodeIp
                if (!root.isMember("node") || !root["node"].isString()) {
                    LOG_ERROR("::node");
                    return;
                }

                const Json::Value accounts_array = root["accounts"];
                const std::string nodeIp = root["node"].asString();
                AccountCheckManager::getInstance()->setRequestRegCnt(accounts_array.size());
                for (const auto &item : accounts_array) {
                    // id
                    const std::string id = item["id"].asString();
                    // user
                    const std::string user = item["extUser"].asString();
                    // pass
                    const std::string pass = item["extPsd"].asString();

                    auto acc = std::make_shared<AccountCheck>(id, user, pass, nodeIp);
                    AccountCheckManager::getInstance()->regAccount(acc);
                }
            }
            else if (request_type == 0) {
                LOG_INFO("callinfo");
                // node
                if (!root.isMember("node") || !root["node"].isString()) {
                    LOG_ERROR("::node");
                    return;
                }
                // accounts
                if (!root.isMember("accounts") || !root["accounts"].isArray()) {
                    LOG_ERROR("::accounts");
                    return;
                }
                // phones
                if (!root.isMember("phones") || !root["phones"].isArray()) {
                    LOG_ERROR("::phones");
                    return;
                }
                // task_id
                if (!root.isMember("task_id") || !root["task_id"].isString()) {
                    LOG_ERROR("::task_id");
                    return;
                }
                // call_type
                if (!root.isMember("call_type") || !root["call_type"].isInt()) {
                    LOG_ERROR("::call_type");
                    return;
                }
                const Json::Value accounts_array = root["accounts"];
                const std::string node = root["node"].asString();
                const std::string task_id = root["task_id"].asString();
                g_task_id = root["task_id"].asString();
                const Json::Value dialplans_array = root["phones"];
                m_worker_num = dialplans_array.size();
                int call_type = root["call_type"].asInt();
                m_recv_call_type = call_type;

                if (m_server_sender) {
                    Json::Value account_info;
                    account_info["type"] = "account_info";
                    account_info["task_id"] = task_id;
                    account_info["call_type"] = call_type;
                    account_info["accounts"] = accounts_array;
                    account_info["phones"] = dialplans_array;

                    Json::StreamWriterBuilder builder;
                    builder["indentation"] = "";
                    std::string message = Json::writeString(builder, account_info);
                    m_server_sender->send(message);
                    LOG_INFO("Sent account info to WebSocket: {}", message);
                }

                if (call_type == 1) {
                    const std::string id = accounts_array[0]["id"].asString();
                    const std::string user = accounts_array[0]["extUser"].asString();
                    const std::string pass = accounts_array[0]["extPsd"].asString();
                    const std::string dialplan = dialplans_array[0].asString();
                    auto acc = std::make_shared<voip::VAccount>(id, user, pass, node);
                    m_acc_vec.push_back(acc);
                    auto caller = std::make_shared<voip::Caller>(*acc);
                    m_caller_que->addCaller(caller);
                    m_dialplan_que->addDialPlan({1, dialplan});
                }
                else if (call_type == 0) {
                    for (Json::ArrayIndex i = 0; i < dialplans_array.size(); ++i) {
                        const Json::Value &item = accounts_array[i];
                        const std::string id = item["id"].asString();
                        const std::string user = item["extUser"].asString();
                        const std::string pass = item["extPsd"].asString();
                        auto acc = std::make_shared<voip::VAccount>(id, user, pass, node);
                        m_acc_vec.push_back(acc);
                        auto caller = std::make_shared<voip::Caller>(*acc);
                        m_caller_que->addCaller(caller);
                    }

                    for (const auto &item : dialplans_array) {
                        m_dialplan_que->addDialPlan(std::pair(1, item.asString()));
                    }
                }
                else {
                    LOG_ERROR("error call_type");
                }
            }
            else {
                LOG_ERROR("error request_type");
            }
        };
    }

    void set_server_sender(std::shared_ptr<IWSSender> sender)
    {
        m_server_sender = sender;
    }

    void restart_ws_client()
    {
        stop_ws_client();
        start_ws_client();
    }

    void start_ws_client()
    {
        auto &ioc = IOContextPool::getInstance()->getIOContext();
        m_resolver = std::make_unique<tcp::resolver>(net::make_strand(ioc));
        ssl::context ssl_ctx(ssl::context::tls_client);
        ssl_ctx.set_verify_mode(ssl::verify_peer);
        ssl_ctx.load_verify_file(verify_file);
        m_ws = std::make_unique<websocket::stream<beast::ssl_stream<beast::tcp_stream>>>(net::make_strand(ioc), ssl_ctx);

        m_resolver->async_resolve(g_gui_cfg.gui_host,
                                  g_gui_cfg.gui_port,
                                  beast::bind_front_handler(&VoipClient::on_resolver,
                                                            shared_from_this()));
    }

    void stop_ws_client()
    {
        if (m_ws->is_open()) {
            beast::error_code ec;
            m_ws->close(websocket::close_code::normal, ec);
            if (ec) {
                LOG_ERROR("WebSocket Close Failed: {}", ec.message());
            }
            else {
                LOG_INFO("WebSocket Close Successfully");
            }
            m_ws.reset();
        }
        m_resolver->cancel();
    }

    void start_call_client()
    {
        while (m_running) {
            m_worker_num = m_dialplan_que->size();
            // LOG_INFO("m_worker_num {}", m_worker_num);
            m_batch_remain = m_dialplan_que->size();
            if (m_batch_remain == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                // LOG_INFO("que size 0");
                continue;
            }
            auto coordinator = std::make_shared<Coordinator>();
            coordinator->reset_();
            if (m_recv_call_type == 1) {
                LOG_INFO("start_call_client single call");
                m_thread_pool.addTask([this, coordinator]() {
                    single_task(coordinator);
                    {
                        std::unique_lock<std::mutex> lock(m_batch_mtx);
                        --m_batch_remain;
                    }

                    if (m_batch_remain == 0) {
                        m_batch_cv.notify_one();
                    }
                });
                std::unique_lock<std::mutex> lock(m_batch_mtx);
                m_batch_cv.wait(lock, [this]() {
                    return m_batch_remain == 0;
                });
                LOG_INFO("single finish, start next");
            }
            else if (m_recv_call_type == 0) {
                LOG_INFO("start_call_client group call");
                for (std::size_t i = 0; i < m_worker_num; ++i) {
                    m_thread_pool.addTask([this, i, coordinator]() {
                        call_task(i, coordinator);
                        {
                            std::unique_lock<std::mutex> lock(m_batch_mtx);
                            --m_batch_remain;
                            LOG_INFO("update remain: {}", m_batch_remain);
                            if (m_batch_remain == 0) {
                                m_batch_cv.notify_one();
                            }
                        }
                    });
                }
                std::unique_lock<std::mutex> lock(m_batch_mtx);
                m_batch_cv.wait(lock, [this]() {
                    LOG_INFO("remain: {}", m_batch_remain);
                    return m_batch_remain == 0;
                });
                LOG_INFO("batch finish, start next");
            }
            else {
                LOG_INFO("error m_recv_call_type");
            }
            AccountCheckManager::getInstance()->clear();
            m_acc_vec.clear();
        }
    }

    void single_task(std::shared_ptr<Coordinator> coordinator)
    {
        auto caller = m_caller_que->getCaller();
        auto dialplan = m_dialplan_que->getDialPlan();
        caller->single_call(dialplan.second, g_client_id, dialplan.first, coordinator, m_server_sender);
    }

    void call_task(std::size_t i, std::shared_ptr<Coordinator> coordinator)
    {
        LOG_INFO("call task {}", i);
        auto caller = m_caller_que->getCaller();
        auto dialplan = m_dialplan_que->getDialPlan();
        caller->call(dialplan.second, g_client_id, dialplan.first, coordinator, m_server_sender);
        LOG_INFO("call {} over", dialplan.second);
    }

    void set_on_read_handler(std::function<void(const std::string &)> on_read_handler)
    {
        m_on_read_handler = on_read_handler;
    }

private:
    void on_resolver(beast::error_code ec, tcp::resolver::results_type results)
    {
        if (ec) {
            Json::Value response;
            response["backend_status"] = "error";

            Json::StreamWriterBuilder writerBuilder;
            std::string responseStr = Json::writeString(writerBuilder, response);
            m_server_sender->send(responseStr);
            return;
        }
        beast::get_lowest_layer(*m_ws)
            .async_connect(results,
                           beast::bind_front_handler(&VoipClient::on_connect,
                                                     shared_from_this()));
    }

    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type endpoint)
    {
        if (ec) {
            Json::Value response;
            response["backend_status"] = "error";

            Json::StreamWriterBuilder writerBuilder;
            std::string responseStr = Json::writeString(writerBuilder, response);
            m_server_sender->send(responseStr);
            return;
        }
        beast::get_lowest_layer(*m_ws).expires_never();
        m_ws->set_option(websocket::stream_base::decorator([](websocket::request_type &req) {
            req.set(http::field::user_agent, "<ws>");
        }));
        m_host = g_gui_cfg.gui_host + ":" + std::to_string(endpoint.port());
        m_target = g_gui_cfg.gui_target + "/" + g_gui_cfg.gui_client_id;
        m_ws->next_layer().async_handshake(ssl::stream_base::client,
                                           beast::bind_front_handler(&VoipClient::on_tls_handshake,
                                                                     shared_from_this()));
    }

    void on_tls_handshake(beast::error_code ec)
    {
        if (ec) {
            Json::Value response;
            response["backend_status"] = "error";
            Json::StreamWriterBuilder writerBuilder;
            std::string responseStr = Json::writeString(writerBuilder, response);
            m_server_sender->send(responseStr);
            return;
        }
        m_ws->set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
        m_ws->set_option(websocket::stream_base::decorator([](websocket::request_type &req) {
            req.set(http::field::user_agent, "voip-client");
        }));
        m_ws->async_handshake(m_host, m_target,
                              beast::bind_front_handler(&VoipClient::on_ws_handshake,
                                                        shared_from_this()));
    }

    void on_ws_handshake(beast::error_code ec)
    {
        if (ec) {
            Json::Value ec_response;
            ec_response["backend_status"] = "error";
            Json::StreamWriterBuilder writerBuilder;
            std::string responseStr = Json::writeString(writerBuilder, ec_response);
            m_server_sender->send(responseStr);
            return;
        }
        Json::Value response;
        response["backend_status"] = "connected";
        Json::StreamWriterBuilder writerBuilder;
        std::string responseStr = Json::writeString(writerBuilder, response);
        m_server_sender->send(responseStr);
        do_read();
    }

    void do_read()
    {
        m_ws->async_read(m_buffer,
                         beast::bind_front_handler(&VoipClient::on_read,
                                                   shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t len)
    {
        boost::ignore_unused(len);
        if (ec) {
            Json::Value response;
            response["backend_status"] = "disconnected";
            Json::StreamWriterBuilder writerBuilder;
            std::string responseStr = Json::writeString(writerBuilder, response);
            m_server_sender->send(responseStr);
            return;
        }
        std::string msg = beast::buffers_to_string(m_buffer.data());
        m_buffer.consume(m_buffer.size());

        // LOG_INFO("recv: {}", msg);
        if (m_on_read_handler) {
            m_on_read_handler(msg);
        }

        do_read();
    }
};

#endif // _WS_CLIENT2_H_