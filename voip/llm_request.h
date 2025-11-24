#ifndef LLM_REQUEST_ASYNC_H
#define LLM_REQUEST_ASYNC_H

#include "global.h"
#include "logger.h"
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <json/json.h>
#include <string>
#include <memory>
#include <chrono>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

class LLMRequestAsync : public std::enable_shared_from_this<LLMRequestAsync>
{
public:
    using Callback = std::function<void(const std::string&)>;

    LLMRequestAsync(net::io_context& ioc,
                    const std::string& host,
                    const std::string& port,
                    const std::string& target)
        : m_ioc(ioc)
        , m_resolver(ioc)
        , m_ctx(ssl::context::tlsv12_client)
        , m_host(host)
        , m_port(port)
        , m_target(target)
    {
        m_ctx.set_verify_mode(ssl::verify_peer);
        m_ctx.load_verify_file(agent_session_verify_file);
    }

    // 每次发送会创建一个新的 ssl::stream 实例，确保不重用已经 shutdown 的流
    void asyncSend(const std::string& user_text,
                   const std::string& call_method,
                   const std::string& role,
                   const std::string& session_id,
                   const std::string& access_token,
                   Callback cb,
                   int timeout_sec = 5)
    {
        // 必须是 shared_ptr 管理的对象
        // 注意：如果不是 shared_ptr 管理，shared_from_this() 会抛或编译失败
        auto self = shared_from_this();

        m_callback = std::move(cb);
        m_session_id = session_id;
        m_access_token = access_token;

        // 构造 JSON body
        Json::Value root;
        root["call_method"] = call_method;
        root["role"] = role;
        root["text"] = user_text;
        Json::StreamWriterBuilder writer;
        m_body = Json::writeString(writer, root);

        m_final_target = m_target + "/" + m_session_id;

        LOG_INFO("[Async] Start request {}{}", m_host, m_final_target);

        // 清理/重置与上次相关的状态
        m_buffer.consume(m_buffer.size()); // 清空 flat_buffer
        m_res = {};                         // 重置 response
        m_req = {};                         // 重置 request

        // 创建新的 ssl stream 实例（每次请求一个新的流）
        // 使用 make_unique 可以在读完后释放并重新创建
        m_stream = std::make_unique<ssl::stream<beast::tcp_stream>>(m_ioc, m_ctx);

        startTimeout(timeout_sec);

        // 开始解析 DNS
        m_resolver.async_resolve(
            m_host,
            m_port,
            [self](beast::error_code ec, tcp::resolver::results_type results) {
                if (ec) {
                    self->fail(ec.message());
                    return;
                }
                self->onResolve(ec, results);
            });
    }

private:
    void startTimeout(int seconds)
    {
        m_timer = std::make_unique<net::steady_timer>(m_ioc);
        m_timer->expires_after(std::chrono::seconds(seconds));

        auto self = shared_from_this();
        m_timer->async_wait([self](beast::error_code ec) {
            if (!ec) {
                LOG_INFO("[Async] Timeout reached");
                self->fail("timeout");
                // release stream so next attempt will recreate
                self->reset_stream();
            }
        });
    }

    void cancelTimeout()
    {
        if (m_timer) {
            beast::error_code ec;
            m_timer->cancel(ec);
        }
    }

    void onResolve(beast::error_code ec, tcp::resolver::results_type results)
    {
        if (ec) return fail(ec.message());

        // connect lowest layer of the current stream
        if (!m_stream) return fail("internal stream not initialized");

        // async_connect expects lowest_layer to be valid
        beast::get_lowest_layer(*m_stream).async_connect(
            results,
            [self = shared_from_this()](beast::error_code ec2, tcp::resolver::results_type::endpoint_type ep) {
                if (ec2) {
                    self->fail(ec2.message());
                    return;
                }
                self->onConnect(ec2, ep);
            });
    }

    void onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
    {
        if (ec) return fail(ec.message());

        // SNI
        if (!SSL_set_tlsext_host_name(m_stream->native_handle(), m_host.c_str())) {
            beast::error_code ec2{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            return fail(ec2.message());
        }

        auto self = shared_from_this();
        m_stream->async_handshake(
            ssl::stream_base::client,
            [self](beast::error_code ec2) {
                if (ec2) {
                    self->fail(ec2.message());
                    return;
                }
                self->onHandshake(ec2);
            });
    }

    void onHandshake(beast::error_code ec)
    {
        if (ec) return fail(ec.message());

        // 构造 HTTP 请求
        m_req = {};
        m_req.method(http::verb::post);
        m_req.target(m_final_target);
        m_req.version(11);
        m_req.set(http::field::host, m_host);
        m_req.set(http::field::content_type, "application/json; charset=utf-8");
        m_req.set(http::field::authorization, "Bearer " + m_access_token);
        m_req.body() = m_body;
        m_req.prepare_payload();

        auto self = shared_from_this();
        http::async_write(
            *m_stream,
            m_req,
            [self](beast::error_code ec2, std::size_t) {
                if (ec2) {
                    self->fail(ec2.message());
                    return;
                }
                self->onWrite(ec2);
            });
    }

    void onWrite(beast::error_code ec)
    {
        if (ec) return fail(ec.message());

        auto self = shared_from_this();
        http::async_read(
            *m_stream,
            m_buffer,
            m_res,
            [self](beast::error_code ec2, std::size_t) {
                if (ec2) {
                    self->fail(ec2.message());
                    return;
                }
                self->onRead(ec2);
            });
    }

    void onRead(beast::error_code ec)
    {
        if (ec) return fail(ec.message());

        cancelTimeout();

        Json::Value json;
        Json::CharReaderBuilder rb;
        std::string errs;
        std::istringstream ss(m_res.body());
        std::string out;

        if (Json::parseFromStream(rb, ss, &json, &errs)) {
            Json::StreamWriterBuilder wb;
            wb["emitUTF8"] = true;
            out = Json::writeString(wb, json);
        } else {
            out = m_res.body();
        }

        // 回调用户
        if (m_callback) m_callback(out);

        // 优雅关闭 SSL 连接 — 忽略 not_connected / short read 等非致命错误
        beast::error_code ec_shutdown;
        if (m_stream) {
            m_stream->shutdown(ec_shutdown);
            if (ec_shutdown && ec_shutdown != net::error::eof) {
                LOG_INFO("[Async] shutdown returned: {}", ec_shutdown.message());
            }
        }

        // 释放底层流，确保下一次请求从新流开始
        reset_stream();
    }

    // 释放并重置 m_stream（safe to call multiple times）
    void reset_stream()
    {
        try {
            if (m_stream) {
                // try to close lowest layer socket if still open
                beast::error_code ec;
                beast::get_lowest_layer(*m_stream).socket().close(ec);
                (void)ec;
            }
        } catch (...) { }

        m_stream.reset();
    }

    void fail(const std::string& msg)
    {
        cancelTimeout();
        LOG_INFO("[Async] Error: {}", msg.c_str());
        if (m_callback) m_callback(""); // 通知空结果或错误
        // 确保释放流以便下次重试
        reset_stream();
    }

private:
    net::io_context& m_ioc;
    tcp::resolver m_resolver;
    ssl::context m_ctx;

    // 每次请求我们都会创建/销毁该流
    std::unique_ptr<ssl::stream<beast::tcp_stream>> m_stream;

    std::unique_ptr<net::steady_timer> m_timer;

    std::string m_host, m_port, m_target;
    std::string m_session_id, m_access_token;
    std::string m_final_target;
    std::string m_body;

    http::request<http::string_body> m_req;
    beast::flat_buffer m_buffer;
    http::response<http::string_body> m_res;

    Callback m_callback;
};

#endif // LLM_REQUEST_ASYNC_H
