#include "web_ws_client.h"
#include "io_context_pool.h"
#include "logger.h"

WebWsClient::WebWsClient()
{
}

void WebWsClient::restart()
{
    stop();
    start();
}

void WebWsClient::start()
{
    auto &ioc = IOContextPool::getInstance()->getIOContext();
    m_resolver = std::make_unique<tcp::resolver>(net::make_strand(ioc));
    m_ws = std::make_unique<websocket::stream<beast::tcp_stream>>(net::make_strand(ioc));
    m_resolver->async_resolve("127.0.0.1",
                              "8443",
                              beast::bind_front_handler(&WebWsClient::on_resolver,
                                                        shared_from_this()));
}

void WebWsClient::stop()
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

void WebWsClient::send(const std::string &msg)
{
    m_ws->text(true);
    m_ws->async_write(asio::buffer(msg),
                      [self = shared_from_this()](beast::error_code ec, std::size_t) {
                          if (ec) {
                              LOG_ERROR("Web Ws Client Send Failed {}", ec.message());
                              return;
                          }
                      });
}

void WebWsClient::on_resolver(beast::error_code ec, tcp::resolver::results_type results)
{
    if (ec) {
        LOG_ERROR("on_resolver error: {}", ec.message());
        Json::Value resp;
        resp["backend_status"] = "error";

        Json::StreamWriterBuilder builder;
        std::string resp_str = Json::writeString(builder, resp);
        return;
    }
    LOG_INFO("on_resolver");
    beast::get_lowest_layer(*m_ws)
        .async_connect(results,
                       beast::bind_front_handler(&WebWsClient::on_connect,
                                                 shared_from_this()));
}

void WebWsClient::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type endpoint)
{
    if (ec) {
        LOG_ERROR("on_connect error: {}", ec.message());
        Json::Value resp;
        resp["backend_status"] = "error";

        Json::StreamWriterBuilder builder;
        std::string resp_str = Json::writeString(builder, resp);
        return;
    }
    LOG_INFO("on_connect");
    beast::get_lowest_layer(*m_ws).expires_never();
    m_ws->set_option(websocket::stream_base::decorator([](websocket::request_type &req) {
        req.set(http::field::user_agent, "<ws>");
    }));
    m_host = std::string("127.0.0.1") + ":" + std::to_string(endpoint.port());
    m_target = std::string("session") + "/" + std::string("123456");

    m_ws->async_handshake(m_host, m_target,
                          beast::bind_front_handler(&WebWsClient::on_ws_handshake,
                                                    shared_from_this()));
}

void WebWsClient::on_tls_handshake(beast::error_code ec)
{
    if (ec) {
        LOG_ERROR("on_tls_handshake error: {}", ec.message());
        Json::Value resp;
        resp["backend_status"] = "error";

        Json::StreamWriterBuilder builder;
        std::string resp_str = Json::writeString(builder, resp);
        return;
    }
    LOG_INFO("on_tls_handshake");
    m_ws->set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
    m_ws->set_option(websocket::stream_base::decorator([](websocket::request_type &req) {
        req.set(http::field::user_agent, "voip-client");
    }));
    m_ws->async_handshake(m_host, m_target,
                          beast::bind_front_handler(&WebWsClient::on_ws_handshake,
                                                    shared_from_this()));
}

void WebWsClient::on_ws_handshake(beast::error_code ec)
{
    Json::Value resp;
    Json::StreamWriterBuilder builder;
    if (ec) {
        LOG_ERROR("on_ws_handshake error: {}", ec.message());
        resp["backend_status"] = "error";
        std::string resp_str = Json::writeString(builder, resp);
        return;
    }
    LOG_INFO("on_ws_handshake");
    resp["backend_status"] = "connected";
    std::string resp_str = Json::writeString(builder, resp);
    do_read();
}

void WebWsClient::do_read()
{
    m_ws->async_read(m_buffer,
                     beast::bind_front_handler(&WebWsClient::on_read,
                                               shared_from_this()));
}

void WebWsClient::on_read(beast::error_code ec, std::size_t len)
{
    boost::ignore_unused(len);
    if (ec) {
        Json::Value resp;
        resp["backend_status"] = "disconnected";
        Json::StreamWriterBuilder builder;
        std::string resp_str = Json::writeString(builder, resp);
        return;
    }
    std::string msg = beast::buffers_to_string(m_buffer.data());
    m_buffer.consume(m_buffer.size());

    LOG_INFO("Web Ws Client recv: {}", msg);

    do_read();
}
