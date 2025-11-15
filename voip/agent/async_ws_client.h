#pragma once

#include "../io_context_pool.h"
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <deque>
#include <mutex>
#include <atomic>
#include <chrono>
#include <functional>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace ws = beast::websocket;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
using tcp = asio::ip::tcp;
using namespace std::chrono_literals;

class AsyncWsClient : public std::enable_shared_from_this<AsyncWsClient>
{
public:
    using MessageHandler = std::function<void(std::string)>;
    using OpenHandler = std::function<void()>;

    AsyncWsClient(std::string host,
                  std::string port,
                  std::string target = "/",
                  MessageHandler on_msg = nullptr)
        : host_(std::move(host))
        , port_(std::move(port))
        , target_(std::move(target))
        , on_message_(std::move(on_msg))
        , stopping_(false)
        , writing_(false)
        , connected_(false)
    {
    }

    ~AsyncWsClient()
    {
        stop();
    }

    // start connection loop (non-blocking)
    void start()
    {
        auto &ioc = IOContextPool::getInstance()->getIOContext();
        co_spawn(ioc, [self = shared_from_this()]() -> awaitable<void> {
            co_await self->connection_loop();
        },
                 detached);
    }

    void stop()
    {
        bool expected = false;
        if (!stopping_.compare_exchange_strong(expected, true))
            return;

        asio::post(ws_->get_executor(), [self = shared_from_this()]() {
            beast::error_code ec;
            if (self->ws_->is_open()) {
                self->ws_->close(ws::close_code::normal, ec);
            }
        });
    }

    void set_on_open(OpenHandler cb)
    {
        on_open_ = std::move(cb);
    }

    void send(std::string msg)
    {
        bool is_connected = connected_.load(std::memory_order_acquire);
        {
            std::lock_guard<std::mutex> lg(queue_mutex_);
            if (!is_connected) {
                pending_queue_.push_back(std::move(msg));
                return; // message buffered until connect
            }
            else {
                send_queue_.push_back(std::move(msg));
            }
        }

        asio::post(ws_->get_executor(), [self = shared_from_this()]() {
            if (!self->writing_) {
                self->writing_ = true;
                co_spawn(self->ws_->get_executor(), [self]() -> awaitable<void> {
                    co_await self->write_worker();
                },
                         detached);
            }
        });
    }

private:
    awaitable<void> connection_loop()
    {
        auto executor = co_await asio::this_coro::executor;
        std::uint32_t reconnect_attempt = 0;

        while (!stopping_) {
            try {
                co_await perform_connect();

                connected_.store(true, std::memory_order_release);

                if (on_open_) {
                    try {
                        on_open_();
                    }
                    catch (...) {
                    }
                }

                {
                    std::lock_guard<std::mutex> lg(queue_mutex_);
                    while (!pending_queue_.empty()) {
                        send_queue_.push_back(std::move(pending_queue_.front()));
                        pending_queue_.pop_front();
                    }
                }
                asio::post(ws_->get_executor(), [self = shared_from_this()]() {
                    if (!self->writing_) {
                        self->writing_ = true;
                        co_spawn(self->ws_->get_executor(), [self]() -> awaitable<void> {
                            co_await self->write_worker();
                        },
                                 detached);
                    }
                });

                reconnect_attempt = 0;

                co_await read_loop();

                beast::error_code ec;
                ws_->close(ws::close_code::normal, ec);
            }
            catch (const std::exception &ex) {
                std::cerr << "[ASRWsClient] connection error: " << ex.what() << "\n";
            }

            connected_.store(false, std::memory_order_release);

            if (stopping_) {
                break;
            }

            ++reconnect_attempt;
            auto backoff_ms = std::min(30000u, (1u << std::min(10u, reconnect_attempt)) * 500u);
            std::cerr << "[ASRWsClient] reconnecting in " << backoff_ms << " ms\n";
            asio::steady_timer timer(executor);
            timer.expires_after(std::chrono::milliseconds(backoff_ms));
            co_await timer.async_wait(use_awaitable);
        }
    }

    awaitable<void> perform_connect()
    {
        auto results = co_await resolver_->async_resolve(host_, port_, use_awaitable);
        co_await asio::async_connect(ws_->next_layer(), results, use_awaitable);

        ws_->set_option(ws::stream_base::timeout::suggested(beast::role_type::client));
        std::string host_header = host_ + ":" + port_;
        co_await ws_->async_handshake(host_header, target_, use_awaitable);

        std::cerr << "[ASRWsClient] connected to " << host_ << ":" << port_ << target_ << "\n";
    }

    awaitable<void> read_loop()
    {
        beast::flat_buffer buffer;
        while (!stopping_) {
            co_await ws_->async_read(buffer, use_awaitable);
            std::string msg {static_cast<const char *>(buffer.data().data()), buffer.size()};
            buffer.consume(buffer.size());

            if (on_message_) {
                try {
                    on_message_(std::move(msg));
                }
                catch (...) {
                }
            }
            else {
                std::cout << "[ASRWsClient] recv: " << msg << "\n";
            }
        }
    }

    awaitable<void> write_worker()
    {
        try {
            for (;;) {
                std::string msg;
                {
                    std::lock_guard<std::mutex> lg(queue_mutex_);
                    if (send_queue_.empty()) {
                        writing_ = false;
                        co_return;
                    }
                    msg = std::move(send_queue_.front());
                    send_queue_.pop_front();
                }

                // Ensure we are connected before writing
                if (!connected_.load(std::memory_order_acquire)) {
                    // push back to pending for retry after reconnect and exit writer
                    std::lock_guard<std::mutex> lg(queue_mutex_);
                    pending_queue_.push_front(std::move(msg));
                    writing_ = false;
                    co_return;
                }

                // perform the async write
                co_await ws_->async_write(asio::buffer(msg), use_awaitable);
            }
        }
        catch (const std::exception &ex) {
            std::cerr << "[ASRWsClient] write error: " << ex.what() << "\n";

            // On write failure, mark disconnected and move current send_queue_ front back to pending (best-effort)
            connected_.store(false, std::memory_order_release);

            // Note: we may not have the message here because it was moved; best we can do is ensure future sends are buffered.
            // For robustness, you might store a copy before writing if message requeue is critical.

            writing_ = false;
            co_return;
        }
    }

private:
    std::unique_ptr<tcp::resolver> resolver_;
    std::unique_ptr<ws::stream<tcp::socket>> ws_;

    const std::string host_;
    const std::string port_;
    const std::string target_;
    MessageHandler on_message_;
    OpenHandler on_open_;

    std::deque<std::string> pending_queue_;
    std::deque<std::string> send_queue_;
    std::mutex queue_mutex_;

    std::atomic<bool> stopping_;
    std::atomic<bool> writing_;
    std::atomic<bool> connected_;
};
