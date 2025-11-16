#pragma once

#include "../io_context_pool.h"
#include "../global.h"
#include "../logger.h"
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <memory>
#include <string>
#include <deque>
#include <mutex>
#include <atomic>
#include <functional>

namespace asio = boost::asio;
namespace ssl = asio::ssl;
namespace beast = boost::beast;
namespace ws = beast::websocket;
using tcp = asio::ip::tcp;

class ASRWsClient : public std::enable_shared_from_this<ASRWsClient>
{
public:
    using MessageHandler = std::function<void(std::string)>;
    using OpenHandler = std::function<void()>;

    ASRWsClient()
        : host_(asr_server_remote_host)
        , port_(asr_server_remote_port)
        , target_("/")
        , io_context_(IOContextPool::getInstance()->getIOContext())
        , ssl_ctx_(ssl::context::tlsv12_client)
        , resolver_(std::make_unique<tcp::resolver>(io_context_))
        , ws_(nullptr)
        , stopping_(false)
        , writing_(false)
        , connected_(false)
    {
        LOG_INFO(">>> ASRWsClient()");
        ssl_ctx_.set_default_verify_paths();
        ssl_ctx_.set_verify_mode(ssl::verify_peer);
        LOG_INFO("<<< ASRWsClient()");
    }

    ~ASRWsClient()
    {
        LOG_INFO(">>> ~ASRWsClient()");
        stop();
        LOG_INFO("<<< ~ASRWsClient()");
    }

    void set_on_open(OpenHandler cb) { on_open_ = std::move(cb); }
    void set_on_message(MessageHandler cb) { on_message_ = std::move(cb); }

    void start()
    {
        LOG_INFO("start connect");
        connect();
    }

    void stop()
    {
        LOG_INFO("to stop");
        bool expected = false;
        if (!stopping_.compare_exchange_strong(expected, true)) {
            LOG_INFO("aleay stop");
            return;
        }
        if (ws_) {
            asio::post(ws_->get_executor(),
                       [self = shared_from_this()]() {
                           beast::error_code ec;
                           if (self->ws_ && self->ws_->is_open()) {
                               self->ws_->close(ws::close_code::normal, ec);
                           }
                       });
        }
    }

    void send(std::string msg)
    {
        // LOG_INFO("send to asr: {}", msg);
        bool is_conn = connected_.load(std::memory_order_acquire);
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (!is_conn) {
                LOG_WARN("ws not connect");
                pending_queue_.push_back(std::move(msg));
                return;
            }
            else {
                send_queue_.push_back(std::move(msg));
            }
        }

        asio::post(ws_->get_executor(),
                   [self = shared_from_this()]() {
                       if (!self->writing_) {
                           self->writing_ = true;
                           self->write_next();
                       }
                   });
    }

private:
    void connect()
    {
        if (stopping_) {
            LOG_ERROR("ws stop");
            return;
        }

        ws_ = std::make_unique<ws::stream<beast::ssl_stream<tcp::socket>>>(io_context_, ssl_ctx_);

        auto self = shared_from_this();
        resolver_->async_resolve(host_, port_,
                                 [this, self](beast::error_code ec, tcp::resolver::results_type results) {
                                     this->on_resolve(ec, results);
                                 });
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results)
    {
        if (ec) {
            LOG_ERROR("resolve error: {}", ec.message());
            schedule_reconnect();
            return;
        }

        auto self = shared_from_this();
        asio::async_connect(ws_->next_layer().next_layer(), results,
                            [this, self](beast::error_code ec, const tcp::endpoint &) {
                                this->on_connect(ec);
                            });
    }

    void on_connect(beast::error_code ec)
    {
        if (ec) {
            LOG_ERROR("connect error: {}", ec.message());
            schedule_reconnect();
            return;
        }

        auto self = shared_from_this();
        ws_->next_layer().async_handshake(ssl::stream_base::client,
                                          [this, self](beast::error_code ec) {
                                              this->on_ssl_handshake(ec);
                                          });
    }

    void on_ssl_handshake(beast::error_code ec)
    {
        if (ec) {
            LOG_ERROR("SSL handshake error: {}", ec.message());
            schedule_reconnect();
            return;
        }

        // websocket handshake
        std::string host_header = host_ + ":" + port_;
        auto self = shared_from_this();
        ws_->async_handshake(host_header, target_,
                             [this, self](beast::error_code ec) {
                                 this->on_handshake(ec);
                             });
    }

    void on_handshake(beast::error_code ec)
    {
        if (ec) {
            LOG_ERROR("websocket handshake error: {}", ec.message());
            schedule_reconnect();
            return;
        }

        connected_.store(true, std::memory_order_release);

        if (on_open_) {
            try {
                on_open_();
            }
            catch (...) {
                LOG_WARN("on_open exec error");
            }
        }

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            while (!pending_queue_.empty()) {
                send_queue_.push_back(std::move(pending_queue_.front()));
                pending_queue_.pop_front();
            }
        }

        start_read();
        write_next();
    }

    void start_read()
    {
        if (stopping_) {
            LOG_ERROR("ws is stop");
            return;
        }

        auto self = shared_from_this();
        ws_->async_read(buffer_,
                        [this, self](beast::error_code ec, std::size_t bytes_transferred) {
                            this->on_read(ec, bytes_transferred);
                        });
    }

    void on_read(beast::error_code ec, std::size_t /*bytes_transferred*/)
    {
        if (ec) {
            LOG_ERROR("read error: {}", ec.message());
            connected_.store(false, std::memory_order_release);
            schedule_reconnect();
            return;
        }

        std::string msg = beast::buffers_to_string(buffer_.data());
        buffer_.consume(buffer_.size());

        if (on_message_) {
            try {
                on_message_(std::move(msg));
            }
            catch (...) {
            }
        }
        else {
            LOG_ERROR("recv: ", msg);
        }

        start_read();
    }

    void write_next()
    {
        if (stopping_)
            return;

        std::string msg;
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (send_queue_.empty()) {
                writing_ = false;
                return;
            }
            msg = std::move(send_queue_.front());
            send_queue_.pop_front();
        }

        auto self = shared_from_this();
        auto msg_sp = std::make_shared<std::string>(std::move(msg));

        ws_->async_write(asio::buffer(*msg_sp),
                         [this, self, msg_sp](beast::error_code ec, std::size_t /*bytes_transferred*/) {
                             this->on_write(ec, *msg_sp);
                         });
    }

    void on_write(beast::error_code ec, const std::string &msg)
    {
        if (ec) {
            LOG_ERROR("write error: {}", ec.message());
            connected_.store(false, std::memory_order_release);

            std::lock_guard<std::mutex> lock(queue_mutex_);
            pending_queue_.push_front(msg);

            schedule_reconnect();
            writing_ = false;
            return;
        }

        write_next();
    }

    void schedule_reconnect()
    {
        if (stopping_) {
            return;
        }

        static unsigned reconnect_attempt = 0;
        ++reconnect_attempt;
        unsigned backoff_ms = std::min(30000u, (1u << std::min(10u, reconnect_attempt)) * 500u);

        LOG_ERROR("reconnecting: {}", backoff_ms);
        auto timer = std::make_shared<asio::steady_timer>(io_context_);
        timer->expires_after(std::chrono::milliseconds(backoff_ms));
        auto self = shared_from_this();
        timer->async_wait([this, self, timer](const beast::error_code &ecTimer) {
            if (!ecTimer) {
                this->connect();
            }
        });
    }

private:
    std::string host_;
    std::string port_;
    std::string target_;

    asio::io_context &io_context_;
    ssl::context ssl_ctx_;
    std::unique_ptr<tcp::resolver> resolver_;
    std::unique_ptr<ws::stream<beast::ssl_stream<tcp::socket>>> ws_;

    MessageHandler on_message_;
    OpenHandler on_open_;

    std::deque<std::string> pending_queue_;
    std::deque<std::string> send_queue_;
    std::mutex queue_mutex_;

    beast::flat_buffer buffer_;
    std::atomic<bool> stopping_;
    std::atomic<bool> writing_;
    std::atomic<bool> connected_;
};
