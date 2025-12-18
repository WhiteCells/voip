#pragma once

#include "../event/event.h"
#include "../event/msg.h"
#include "../core/session_core.h"
#include "../logger.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/ssl.hpp>
#include <json/json.h>
#include <atomic>
#include <mutex>
#include <memory>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace ssl = net::ssl;
namespace http = beast::http;
using tcp = net::ip::tcp;

// LLM 客户端
// 当有新的 ASR 文本消息时，需要将上一次请求终止，同时将上一次说的话和当前说的话，作为一次请求，再次请求 LLM
// 如果 LLMTextMsg 已经交给了 TTS 模块，则需要等待 TTS 模块生成音频后，再将 LLMTextMsg 发送给会话管理
class LLMHttpClient : public std::enable_shared_from_this<LLMHttpClient>
{
public:
    LLMHttpClient(net::io_context &ioc,
                  const std::string &host,
                  const std::string &port,
                  const std::string &path,
                  const std::string &ssl_cert)
        : m_ioc(ioc)
        , m_host(host)
        , m_port(port)
        , m_path(path)
        , m_ssl_cert(ssl_cert)
    {
        EventBus::getInstance()->subscribe<ASRTextMsg>([this](const ASRTextMsg &msg) {
            LOG_INFO("recv ASR Text Msg: {}", msg.m_text);
            if (m_processing.load()) {
                // interruptCurrentRequest();
                std::lock_guard<std::mutex> lock(m_pending_text_mtx);
                m_pending_text += msg.m_text;
            }
            else {
                std::lock_guard<std::mutex> lock(m_pending_text_mtx);
                m_pending_text = msg.m_text;
            }

            LOG_INFO("pending text: {}", m_pending_text);

            request(m_pending_text);
        });

        EventBus::getInstance()->subscribe<LLMInterruptMsg>([this](const LLMInterruptMsg &msg) {
            m_processing.store(false);
            // m_stream->shutdown();
        });
    }

    ~LLMHttpClient()
    {
    }

    void interruptCurrentRequest()
    {
        m_processing.store(false);

        if (m_stream) {
            beast::get_lowest_layer(*m_stream).cancel();
            beast::get_lowest_layer(*m_stream).close();
        }
    }

    void request(const std::string &text)
    {
        m_processing.store(true);
        // ++m_request_id;
        // uint64_t cur_request_id = m_request_id;

        // 构建请求体
        auto call_method = SessionCore::getInstance()->getCallMethod();
        auto session_id = SessionCore::getInstance()->getSessionId();
        LOG_INFO("LLMHttpClient request call_method: {}, session_id: {}, text: {}", call_method, session_id, text);
        Json::Value root;
        // root["call_method"] = call_method;
        root["call_method"] = "agent";
        root["role"] = "customer";
        root["text"] = text;
        // auto target = m_path + "/" + session_id;
        auto target = m_path + "/" + "12111143";
        auto body_str = root.toStyledString();

        // 清空响应体
        m_resp = {};

        // resolver
        m_resolver = std::make_shared<tcp::resolver>(m_ioc);
        m_ssl_ctx = std::make_shared<ssl::context>(ssl::context::tls_client);
        m_ssl_ctx->set_verify_mode(ssl::verify_peer);
        m_ssl_ctx->load_verify_file(m_ssl_cert);

        // stream
        m_stream = std::make_shared<ssl::stream<beast::tcp_stream>>(m_ioc, *m_ssl_ctx);

        // request
        m_req.version(11);
        m_req.method(http::verb::post);
        m_req.target(target);
        m_req.set(http::field::host, m_host);
        m_req.set(http::field::content_type, "application/json");
        m_req.set(http::field::user_agent, "voip-llm-client");
        m_req.body() = body_str;
        m_req.prepare_payload();
        // m_request_id = cur_request_id;

        m_resolver->async_resolve(m_host, m_port,
                                  beast::bind_front_handler(&LLMHttpClient::onResolve, shared_from_this()));
    }

private:
    void onResolve(beast::error_code ec, tcp::resolver::results_type results)
    {
        if (ec) {
            LOG_INFO("resolve error: {}", ec.message());
            return;
        }
        if (!m_processing.load()) {
            LOG_INFO("onResolve request not processing, ignore");
            return;
        }
        beast::get_lowest_layer(*m_stream).expires_after(std::chrono::seconds(3));

        beast::get_lowest_layer(*m_stream).async_connect(results,
                                                         beast::bind_front_handler(&LLMHttpClient::onConnect,
                                                                                   shared_from_this()));
    }

    void onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type endpoint)
    {
        if (ec) {
            LOG_INFO("connect error: {}", ec.message());
            return;
        }
        if (!m_processing.load()) {
            LOG_INFO("onConnect request not processing, ignore");
            return;
        }
        beast::get_lowest_layer(*m_stream).expires_never();

        // 连接到服务器
        m_stream->async_handshake(ssl::stream_base::client,
                                  beast::bind_front_handler(&LLMHttpClient::onTLSHandshake,
                                                            shared_from_this()));
    }

    void onTLSHandshake(beast::error_code ec)
    {
        if (ec) {
            LOG_INFO("tsl handshake error: {}", ec.message());
            return;
        }
        if (!m_processing.load()) {
            LOG_INFO("onTLSHandshake request not processing, ignore");
            return;
        }
        // 发送请求
        http::async_write(*m_stream, m_req,
                          beast::bind_front_handler(&LLMHttpClient::onWrite, shared_from_this()));
    }

    void onWrite(beast::error_code ec, std::size_t bytes_transferred)
    {
        if (ec) {
            LOG_INFO("write error: {}", ec.message());
            return;
        }
        if (!m_processing.load()) {
            LOG_INFO("onWrite request not processing, ignore");
            return;
        }
        // 接收响应
        http::async_read(*m_stream, m_buffer, m_resp,
                         beast::bind_front_handler(&LLMHttpClient::onRead, shared_from_this()));
    }

    void onRead(beast::error_code ec, std::size_t bytes_transferred)
    {
        if (ec) {
            LOG_INFO("read error: {}", ec.message());
            return;
        }
        if (!m_processing.load()) {
            LOG_INFO("onRead request not processing, ignore");
            return;
        }
        if (m_resp.result() != http::status::ok) {
            LOG_INFO("read error: {}", m_resp.result_int());
            m_processing.store(false);
            return;
        }

        // 处理响应
        std::string body_str = m_resp.body();
        LOG_INFO("read body: {}", body_str);

        Json::Value root;
        Json::CharReaderBuilder resp_builder;
        std::shared_ptr<Json::CharReader> resp_reader(resp_builder.newCharReader());
        std::string json_parse_err;
        if (!resp_reader->parse(body_str.c_str(), body_str.c_str() + body_str.size(), &root, &json_parse_err)) {
            return;
        }
        std::vector<std::string> llm_texts;
        const auto &data = root["data"];
        for (const auto &item : data["text"]) {
            llm_texts.push_back(item.asString());
        }

        EventBus::getInstance()->publish(LLMTextMsg {m_request_id, llm_texts});

        m_processing.store(false);
        m_pending_text.clear();
    }

private:
    net::io_context &m_ioc;
    std::string m_host;
    std::string m_port;
    std::string m_path;
    std::string m_ssl_cert;

    std::atomic<bool> m_processing {false};
    std::mutex m_pending_text_mtx;
    std::string m_pending_text;

    // Request resources
    std::shared_ptr<tcp::resolver> m_resolver;
    std::shared_ptr<ssl::context> m_ssl_ctx;
    std::shared_ptr<ssl::stream<beast::tcp_stream>> m_stream;
    http::request<http::string_body> m_req;
    http::response<http::string_body> m_resp;
    beast::flat_buffer m_buffer;
    uint64_t m_request_id = 0;

    // Json
    // Json::CharReaderBuilder m_json_reader_builder;
};