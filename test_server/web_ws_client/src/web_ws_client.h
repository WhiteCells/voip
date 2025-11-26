#ifndef _WEB_WS_CLIENT_H_
#define _WEB_WS_CLIENT_H_

#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl.hpp>
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
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

class WebWsClient :
    public std::enable_shared_from_this<WebWsClient>
{
public:
    using AgentWsMsgHandler = std::function<void(const std::string &)>;

    WebWsClient();
    ~WebWsClient() = default;

    void restart();

    void start();

    void stop();

    void start_call();

    void send(const std::string &msg);

private:
    void on_resolver(beast::error_code ec, tcp::resolver::results_type results);

    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type endpoint);

    void on_tls_handshake(beast::error_code ec);

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
    std::string m_host;
    std::string m_port;
    std::string m_client_id;
    std::string m_target;
    std::atomic<bool> m_running {true};
    std::mutex m_batch_mtx;
    std::condition_variable m_batch_cv;
    std::size_t m_batch_remain;
    std::size_t m_worker_num;

    std::string m_recv_call_type;
    std::string m_call_method;
};

#endif // _WEB_WS_CLIENT_H_
