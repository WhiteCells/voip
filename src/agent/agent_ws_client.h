#pragma once

#include "../logger.h"
// #include "../ws_interface.h"
// #include "llm_request.h"
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <json/json.h>
#include <memory>
#include <functional>
#include <vector>
#include <chrono>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

class AgentWsClient :
    public std::enable_shared_from_this<AgentWsClient>
{
public:
    using WebWsMsgHandler = std::function<void(const std::string &)>;

    explicit AgentWsClient(const std::string &host, const std::string &port);

    ~AgentWsClient();

    void set_web_ws_sender(WebWsMsgHandler handler);
    // void set_server_sender(std::shared_ptr<IWSSender> sender);

    void send(const std::string &msg);

    void restart();

    void start();

    void stop();

    void start_config_send();

    void end_config_send();

    void sendBinary(const std::string &data, const std::string &role);

    void start_llm_style();

    void clear_llm_msg_list();

    void get_session_id(const std::string &call_method,
                        const std::string &session_id,
                        const std::string &access_token);

    void process_buffer_if_timeout();

    bool m_is_hangup {false};

private:
    void on_resolver(beast::error_code ec, tcp::resolver::results_type results);

    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);

    void on_tls_handshake(beast::error_code ec);

    void on_ws_handshake(beast::error_code ec);

    void do_read();

    void do_write();

    void on_read(beast::error_code ec, std::size_t bytes_transferred);

    void agent_asr_with_llm(const std::string &text);

    void manual_asr_with_llm(const std::string &text);

    void start_timeout_check(std::chrono::steady_clock::time_point timeout_time, int timeout_seconds);

    void end_timeout_check();

    void tts_create(std::string response);

    void on_agent_llm_response(const std::string &response);

    // 新增：取消当前的LLM和TTS请求
    void cancel_current_requests();

    static void hangup_call()
    {
        // endpoint.libRegisterThread("Worker");
        // endpoint.hangupAllCalls();
        LOG_INFO("Hangup all calls");
    }

private:
    std::unique_ptr<tcp::resolver> m_resolver;
    std::unique_ptr<websocket::stream<beast::ssl_stream<beast::tcp_stream>>> m_ws;
    beast::flat_buffer m_buffer;
    std::string m_host;
    std::string m_port;
    std::string m_target;

    // std::shared_ptr<IWSSender> m_gui_server_sender;
    // std::shared_ptr<LLMRequestAsync> m_llm_client;

    std::string m_llm_manual_text;
    std::string m_llm_agent_text;
    std::vector<std::string> m_llm_msg_list;
    std::chrono::steady_clock::time_point m_llm_start_time;
    static constexpr int LLM_TIMEOUT_SECONDS = 60;
    std::thread m_llm_timer_thread;
    std::atomic<bool> m_llm_timer_running {false};
    std::string m_session_id;
    std::string m_access_token;
    std::string m_role;
    std::string m_call_method;
    WebWsMsgHandler m_ws_msg_handler;
    std::optional<net::strand<net::io_context::executor_type>> m_strand;
    std::deque<std::string> m_send_queue;
    bool m_writing {false};
    bool m_llm_start {false};
    //    std::atomic<bool> llm_ok_flag_ {false};

    std::string text_buffer_;                              // 累积文本
    std::string pending_text_;                             // 待处理的文本
    std::chrono::steady_clock::time_point last_text_time_; // 上一次收到文本时间
    std::mutex text_mutex_;
    std::atomic<bool> timer_running_ {false};

    // 新增：用于跟踪请求状态的变量
    std::atomic<bool> m_llm_request_active {false}; // 是否有活跃的LLM请求
    std::atomic<bool> m_tts_request_active {false}; // 是否有活跃的TTS请求

    bool m_vad_flag = false;
    bool m_first_flag = true;
};
