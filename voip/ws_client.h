#ifndef _WS_CLIENT2_H_
#define _WS_CLIENT2_H_

#include "logger.h"
#include "global.h"
#include "io_context_pool.h"
#include "coordinator.h"
#include "thread_pool.h"
#include "vaccount.h"
#include "request.hpp"
#include "account_check.h"
#include "account_check_manager.h"
#include "caller_queue.h"
#include "dialplan_queue.h"
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <json/json.h>
#include <functional>
#include <memory>
#include <string>
#include <atomic>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class VoipClient :
    public std::enable_shared_from_this<VoipClient>
{
private:
    std::unique_ptr<tcp::resolver> m_resolver;
    std::unique_ptr<websocket::stream<beast::tcp_stream>> m_ws;
    beast::flat_buffer m_buffer;
    std::string m_host = backend_host;
    std::string m_port = backend_port;
    std::string m_client_id = client_id;
    std::string m_target = "/ws/client/" + m_client_id;
    std::function<void(const std::string &)> m_on_read_handler;
    std::atomic<bool> m_running {true};
    std::mutex m_batch_mtx;
    std::condition_variable m_batch_cv;
    std::size_t m_batch_remain;
    std::size_t m_worker_num = 2;
    ThreadPool m_thread_pool;

    // std::atomic<bool> m_running;
    // ThreadPool m_thread_pool;
    std::shared_ptr<CallerQueue> m_caller_que;
    std::shared_ptr<DialPlanQueue> m_dialplan_que;
    std::vector<std::shared_ptr<voip::VAccount>> m_acc_vec;

    // std::atomic<int> m_batch_remain {0};

public:
    VoipClient(net::io_context &ioc = IOContextPool::getInstance()->getIOContext()) :
        // m_resolver(net::make_strand(ioc)),
        // m_ws(net::make_strand(ioc)),
        m_thread_pool(2),
        m_caller_que(std::make_shared<CallerQueue>()),
        m_dialplan_que(std::make_shared<DialPlanQueue>())
    {
        m_on_read_handler = [this](const std::string &msg) {
            static thread_local bool pj_thread_registered = false;
            if (!pj_thread_registered) {
                endpoint.libRegisterThread("Worker");
                pj_thread_registered = true;
            }
            // accounts.clear();
            AccountCheckManager::getInstance()->clear();
            m_acc_vec.clear();

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
                // request_type
                if (!root.isMember("request_type") || !root["request_type"].isInt()) {
                    LOG_ERROR("::request_type");
                    return;
                }

                const Json::Value accounts_array = root["accounts"];
                const std::string nodeIp = root["node"].asString();
                for (const auto &item : accounts_array) {
                    // id
                    const std::string id = item["id"].asString();
                    // user
                    const std::string user = item["user"].asString();
                    // pass
                    const std::string pass = item["pass"].asString();

                    auto acc = std::make_shared<AccountCheck>(id, user, pass, nodeIp);
                    AccountCheckManager::getInstance()->regAccount(acc);
                }
            }
            // 呼叫信息
            /*
                {
                    "node": "192.168.10.51",                // fs 节点 IP
                    "accounts": [
                        {
                            "id": "1wr3-2s2d-r3r2-dff31",   // 分机号 ID
                            "user": "1001",                 // 分机号账号
                            "pass": "1001"                  // 分机号密码
                        }
                    ],
                    "phones": [
                        "018803030202"                      // 线路 + 手机号
                    ],
                    "task_id": "e32rqe-2e2dds-1e23e-34r4f", // 任务 ID
                    "call_type": 0,                          // 0 代表群呼；1 代表单呼
                    "request_type":0                         // 0 代表拨打数据  1 代表分机号校验数据
                }
            */
            else if (request_type == 0) {
                LOG_INFO("callinfo");
                // node
                if (!root.isMember("node") || root.isString()) {
                    LOG_ERROR("::node");
                    return;
                }
                // accounts
                if (!root.isMember("accounts") || root.isArray()) {
                    LOG_ERROR("::accounts");
                    return;
                }
                // phones
                if (!root.isMember("phones") || root.isArray()) {
                    LOG_ERROR("::phones");
                    return;
                }
                // task_id
                if (!root.isMember("task_id") || root.isString()) {
                    LOG_ERROR("::task_id");
                    return;
                }
                // call_type
                if (!root.isMember("call_type") || root.isInt()) {
                    LOG_ERROR("::call_type");
                    return;
                }
                //
                const Json::Value accounts_array = root["accounts"];
                const std::string node = root["node"].asString();
                const std::string task_id = root["task_id"].asString();

                for (const auto &item : accounts_array) {
                    const std::string id = item["id"].asString();
                    const std::string user = item["user"].asString();
                    const std::string pass = item["pass"].asString();
                    auto acc = std::make_shared<voip::VAccount>(id, user, pass, node);
                    m_acc_vec.push_back(acc);
                    auto caller = std::make_shared<voip::Caller>(*acc);
                    m_caller_que->addCaller(caller);
                }

                const Json::Value dialplans_array = root["phones"];
                for (const auto &item : dialplans_array) {
                    m_dialplan_que->addDialPlan(std::pair(1, item.asString()));
                }
            }
            else {
                LOG_ERROR("error request_type");
            }
        };
    }

    void start_ws_client()
    {
        auto &ioc = IOContextPool::getInstance()->getIOContext();
        m_resolver = std::make_unique<tcp::resolver>(net::make_strand(ioc));
        m_ws = std::make_unique<websocket::stream<beast::tcp_stream>>(net::make_strand(ioc));

        m_resolver->async_resolve(m_host,
                                  m_port,
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

    void restart_ws_client()
    {
        stop_ws_client();
        start_call_client();
    }

    // 项目逻辑存在问题，
    // 我需要的是，一次打一批号码
    // 一批打完，再打下一批
    void start_call_client()
    {
        while (m_running) {
            // if (m_caller_que->empty() || m_dialplan_que->empty()) {
            //     LOG_WARN("队列为空，等待数据中...");
            //     std::this_thread::sleep_for(std::chrono::milliseconds(500));
            //     continue;
            // }
            // std::unique_lock<std::mutex> lock(m_batch_mtx);
            m_batch_remain = m_worker_num;
            auto coordinator = std::make_shared<Coordinator>();
            for (std::size_t i = 0; i < m_worker_num; ++i) {
                m_thread_pool.addTask([this, i, coordinator]() {
                    call_task(i, coordinator);
                    {
                        std::unique_lock<std::mutex> lock(m_batch_mtx);
                        --m_batch_remain;
                    }

                    if (m_batch_remain == 0) {
                        m_batch_cv.notify_one();
                    }
                });
            }
            std::unique_lock<std::mutex> lock(m_batch_mtx);
            m_batch_cv.wait(lock, [this]() {
                return m_batch_remain == 0;
            });
            LOG_INFO("batch finish, start next");
        }
    }

    void call_task(std::size_t i, std::shared_ptr<Coordinator> coordinator)
    {
        auto caller = m_caller_que->getCaller();
        auto dialplan = m_dialplan_que->getDialPlan();
        caller->call(dialplan.second, g_client_id, dialplan.first, coordinator);
    }

    void set_on_read_handler(std::function<void(const std::string &)> on_read_handler)
    {
        // m_on_read_handler = on_read_handler;
        // m_on_read_handler = [](const std::string &msg) {
        // };
    }

private:
    void on_resolver(beast::error_code ec, tcp::resolver::results_type results)
    {
        if (ec) {
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
            return;
        }
        beast::get_lowest_layer(*m_ws).expires_never();
        m_ws->set_option(websocket::stream_base::decorator([](websocket::request_type &req) {
            req.set(http::field::user_agent, "<ws>");
        }));
        m_host += ":" + std::to_string(endpoint.port());
        m_ws->async_handshake(m_host,
                              m_target,
                              beast::bind_front_handler(&VoipClient::on_handshake,
                                                        shared_from_this()));
    }

    void on_handshake(beast::error_code ec)
    {
        if (ec) {
            return;
        }
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