#ifndef _WS_CLIENT2_H_
#define _WS_CLIENT2_H_

#include "logger.h"
#include "global.h"
#include "io_context_pool.h"
#include "coordinator.h"
#include "thread_pool.h"
#include <boost/beast.hpp>
#include <boost/asio.hpp>
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
    tcp::resolver m_resolver;
    websocket::stream<beast::tcp_stream> m_ws;
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
    std::size_t m_worker_num;
    ThreadPool m_thread_pool;

public:
    VoipClient(net::io_context &ioc = IOContextPool::getInstance()->getIOContext()) :
        m_resolver(net::make_strand(ioc)),
        m_ws(net::make_strand(ioc)),
        m_thread_pool(5)
    {
        m_on_read_handler = [](const std::string &msg) {
            // 程序启动后
            // 1. 接收账号信息
            // 2. 接收拨号信息，存放队列
            LOG_INFO("recv: {}", msg);
            // 使用 jsoncpp 对接收到的数据进行解析
            
        };
    }

    void start_ws_client()
    {
        m_resolver.async_resolve(m_host,
                                 m_port,
                                 beast::bind_front_handler(&VoipClient::on_resolver,
                                                           shared_from_this()));
    }

    void start_call_client()
    {
        while (m_running) {
            std::unique_lock<std::mutex> lock(m_batch_mtx);
            m_batch_remain = m_worker_num;
            auto coordinator = std::make_shared<Coordinator>();
            for (std::size_t i = 0; i < m_worker_num; ++i) {
                m_thread_pool.addTask([this, i, coordinator]() {
                    call_task(i, coordinator);
                    if (--m_batch_remain == 0) {
                        m_batch_cv.notify_one();
                    }
                });
            }
            m_batch_cv.wait(lock, [this]() {
                return m_batch_remain == 0;
            });
            LOG_INFO("batch finish, start next");
        }
    }

    void call_task(std::size_t i, std::shared_ptr<Coordinator> coordinator)
    {
    }

    void set_on_read_handler(std::function<void(const std::string &)> on_read_handler)
    {
        // m_on_read_handler = on_read_handler;
        m_on_read_handler = [](const std::string &msg) {
            // 程序启动后
            // 1. 接收账号信息
            // 2. 接收拨号信息，存放队列
            LOG_INFO("recv: {}", msg);
        };
    }

private:
    void on_resolver(beast::error_code ec, tcp::resolver::results_type results)
    {
        if (ec) {
            return;
        }
        beast::get_lowest_layer(m_ws)
            .async_connect(results,
                           beast::bind_front_handler(&VoipClient::on_connect,
                                                     shared_from_this()));
    }

    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type endpoint)
    {
        if (ec) {
            return;
        }
        beast::get_lowest_layer(m_ws).expires_never();
        m_ws.set_option(websocket::stream_base::decorator([](websocket::request_type &req) {
            req.set(http::field::user_agent, "<ws>");
        }));
        m_host += ":" + std::to_string(endpoint.port());
        m_ws.async_handshake(m_host,
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
        m_ws.async_read(m_buffer,
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

        LOG_INFO("recv: {}", msg);
        if (m_on_read_handler) {
            m_on_read_handler(msg);
        }

        do_read();
    }
};

#endif // _WS_CLIENT2_H_