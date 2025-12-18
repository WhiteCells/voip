#pragma once

#include "../event/event.h"
#include "../event/msg.h"
#include "../logger.h"
#include "pcm_que.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <json/json.h>
#include <memory>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = net::ip::tcp;

// 生成 TTS 音频的客户端
// 当接收到新的 LLM 请求时，需要将上一次的 TTS 音频从队列中弹出，计算已播放的文本
class TTSHttpClient : std::enable_shared_from_this<TTSHttpClient>
{
public:
    TTSHttpClient(net::io_context &ioc,
                  const std::string &host,
                  const std::string &port,
                  const std::string &path)
        : m_ioc(ioc)
        , m_host(host)
        , m_port(port)
        , m_path(path)
    {
        EventBus::getInstance()->subscribe<LLMTextMsg>([this](const LLMTextMsg &msg) {
            for (const auto &text : msg.m_texts) {
                LOG_INFO("LLMTextMsg: {}", text);
                request(text);
            }
        });

        EventBus::getInstance()->subscribe<LLMInterruptMsg>([this](const LLMInterruptMsg &msg) {
            // 终止请求
            // 清理音频
        });
    }

    ~TTSHttpClient()
    {
        m_pcm_que->clear();
    }

    void request(const std::string &text)
    {
        Json::Value root;
        root["input"] = text;
        root["voice"] = ""; // todo
        root["response_format"] = "pcm";
        root["sample_rate"] = 16000;
        root["speed"] = 1.0;
        auto target = m_path;
        auto body_str = root.toStyledString();

        m_resp = {};

        m_resolver = std::make_shared<tcp::resolver>(m_ioc);

        m_stream = std::make_shared<beast::tcp_stream>(m_ioc);

        m_req.version(11);
        m_req.method(http::verb::post);
        m_req.target(target);
        m_req.set(http::field::host, m_host);
        m_req.set(http::field::content_type, "application/json");
        m_req.set(http::field::user_agent, "voip-tts-client");
        m_req.body() = body_str;
        m_req.prepare_payload();

        m_resolver->async_resolve(m_host, m_port,
                                  beast::bind_front_handler(&TTSHttpClient::onResolve,
                                                            shared_from_this()));
    }

private:
    void onResolve(beast::error_code ec, tcp::resolver::results_type results)
    {
        if (ec) {
            LOG_INFO("resolve error: {}", ec.message());
            return;
        }
        beast::get_lowest_layer(*m_stream).expires_after(std::chrono::seconds(3));
        beast::get_lowest_layer(*m_stream).async_connect(results,
                                                         beast::bind_front_handler(&TTSHttpClient::onConnect,
                                                                                   shared_from_this()));
    }

    void onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type endpoint)
    {
        if (ec) {
            LOG_INFO("connect error: {}", ec.message());
            return;
        }
        beast::get_lowest_layer(*m_stream).expires_after(std::chrono::seconds(3));

        http::async_write(*m_stream, m_req,
                          beast::bind_front_handler(&TTSHttpClient::onWrite, shared_from_this()));
    }

    void onWrite(beast::error_code ec, std::size_t bytes_transferred)
    {
        if (ec) {
            LOG_INFO("write error: {}", ec.message());
            return;
        }
        http::async_read(*m_stream, m_buffer, m_resp,
                         beast::bind_front_handler(&TTSHttpClient::onRead, shared_from_this()));
    }

    void onRead(beast::error_code ec, std::size_t bytes_transferred)
    {
        if (ec) {
            LOG_INFO("read error: {}", ec.message());
            return;
        }

        if (m_resp.result() != http::status::ok) {
            LOG_INFO("tts response error: {}", m_resp.result_int());
            return;
        }

        // 这里假设 PCM 数据是 base64 或二进制在 body 中
        const std::string &pcm_data = m_resp.body();

        LOG_INFO("TTS PCM data pushed, size={}, body={}", pcm_data.size(), pcm_data);
    }

private:
    net::io_context &m_ioc;
    std::string m_host;
    std::string m_port;
    std::string m_path;

    // Request resources
    std::shared_ptr<tcp::resolver> m_resolver;
    std::shared_ptr<beast::tcp_stream> m_stream;
    http::request<http::string_body> m_req;
    beast::flat_buffer m_buffer;
    http::response<http::string_body> m_resp;

public:
    static std::shared_ptr<PCMQue> m_pcm_que;
};

inline std::shared_ptr<PCMQue> TTSHttpClient::m_pcm_que = std::make_shared<PCMQue>();
