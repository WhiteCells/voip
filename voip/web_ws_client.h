#ifndef _WEB_WS_CLIENT_H_
#define _WEB_WS_CLIENT_H_

#include "global.h"
#include "coordinator.h"
#include "thread_pool.h"
#include "vaccount.h"
#include "caller_queue.h"
#include "dialplan_queue.h"
#include "ws_interface.h"
#include "agent_ws_client.h"
#ifdef VOIP_SSL
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl.hpp>
#endif
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
#ifdef VOIP_SSL
namespace ssl = boost::asio::ssl;
#endif
using tcp = net::ip::tcp;

class WebWsClient :
    public std::enable_shared_from_this<WebWsClient>
{
public:
    using AgentWsMsgHandler = std::function<void(const std::string &)>;

    WebWsClient();
    ~WebWsClient() = default;

    void set_server_sender(std::shared_ptr<IWSSender> sender);

    void set_agent_ws_sender(AgentWsMsgHandler handler);

    void restart();

    void start();

    void stop();

    void start_call();

    void single_call(std::shared_ptr<Coordinator> coordinator);

    void group_call(std::size_t i, std::shared_ptr<Coordinator> coordinator);

    void set_on_read_handler(std::function<void(const std::string &)> on_read_handler);

    void send(const std::string &msg);

private:
    void on_resolver(beast::error_code ec, tcp::resolver::results_type results);

    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type endpoint);

#ifdef VOIP_SSL
    void on_tls_handshake(beast::error_code ec);
#endif

    void on_ws_handshake(beast::error_code ec);

    void do_read();

    void on_read(beast::error_code ec, std::size_t len);

private:
    std::unique_ptr<tcp::resolver> m_resolver;

#ifdef VOIP_SSL
    std::unique_ptr<websocket::stream<beast::ssl_stream<beast::tcp_stream>>> m_ws;
    std::unique_ptr<ssl::context> m_ssl_ctx;
#else
    std::unique_ptr<websocket::stream<beast::tcp_stream>> m_ws;
#endif

    beast::flat_buffer m_buffer;
    std::string m_host = backend_host;
    std::string m_port = backend_port;
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
    std::string m_recv_call_type;
    std::string m_call_method;
    std::string m_different;
    AgentWsMsgHandler m_agent_ws_msg_handler;
};

#endif // _WEB_WS_CLIENT_H_
