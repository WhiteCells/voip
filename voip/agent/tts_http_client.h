#pragma once

#include "../event/event.h"
#include "../io_context_pool.h"
#include "../logger.h"
#include "msg.h"
#include "pcm_queue.h"
#include <boost/beast.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/system/detail/error_category.hpp>
#include <json/json.h>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = net::ip::tcp;

class TTSHTTPClient
{
public:
    TTSHTTPClient(EventBus &e,
                  const std::string &host,
                  const std::string &port,
                  const std::string &target)
        : m_event_bus(e)
        , m_host(host)
        , m_port(port)
        , m_target(target)
    {
        m_event_bus.subscribe<LLMTextMsg>([&](const LLMTextMsg &msg) {
            // m_que->clear();
            auto pcm = request(msg.text);
            m_que->push(pcm);
        });
        m_event_bus.subscribe<LLMEndMsg>([&](const LLMEndMsg &msg) {
            // m_que->clear();
            auto pcm = request(msg.text);
            m_que->push(pcm);
            m_event_bus.publish(LLMHangupMsg {});
        });
        //
    }
    ~TTSHTTPClient() = default;

    std::vector<char> request(const std::string &text)
    {
        asio::io_context &ioc = IOContextPool::getInstance()->getIOContext();
        tcp::resolver resolver(ioc);
        tcp::socket socket(ioc);
        boost::system::error_code ec;
        auto results = resolver.resolve(m_host, m_port, ec);
        if (ec) {
            throw std::runtime_error("resolve failed: " + ec.message());
        }
        net::connect(socket, results, ec);
        if (ec) {
            throw std::runtime_error("connect failed: " + ec.message());
        }

        Json::Value root;
        root["text"] = text;
        Json::StreamWriterBuilder writer;
        std::string body = Json::writeString(writer, root);

        http::request<http::string_body> req {http::verb::post, m_target, 11};
        req.set(http::field::content_type, "application/json");
        req.body() = body;
        req.prepare_payload();

        http::write(socket, req, ec);
        if (ec) {
            throw std::runtime_error("write failed: " + ec.message());
        }

        boost::beast::flat_buffer buffer;
        http::response<http::vector_body<char>> res;

        std::atomic<bool> done {false};
        std::thread timeout_thread([&]() {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            if (!done.load()) {
                LOG_WARN("TTS read timeout, cancelling socket");
                socket.cancel();
            }
        });

        http::read(socket, buffer, res, ec);
        done = true;
        timeout_thread.join();

        if (ec == boost::asio::error::operation_aborted) {
            LOG_WARN("TTS timeout");
            return {};
        }
        if (ec) {
            throw std::runtime_error("read failed: " + ec.message());
        }
        if (res.result() != http::status::ok) {
            throw std::runtime_error("bad status: " + std::to_string(res.result_int()));
        }

        return res.body();
    }

public:
    static std::shared_ptr<PCMQueue> m_que;

private:
    EventBus &m_event_bus;
    const std::string &m_host;
    const std::string &m_port;
    const std::string &m_target;
};
