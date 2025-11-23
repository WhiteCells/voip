#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <json/json.h>

#include <deque>
#include <mutex>
#include <memory>
#include <string>
#include <iostream>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <cstdlib>

namespace asio = boost::asio;
namespace ssl = boost::asio::ssl;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

class WsClientBase : public std::enable_shared_from_this<WsClientBase>
{
public:
    WsClientBase(asio::io_context &ioc,
                 const std::string &host,
                 const std::string &port,
                 const std::string &path,
                 bool use_ssl = true)
        : ioc_(ioc)
        , strand_(asio::make_strand(ioc_))
        , resolver_(asio::make_strand(ioc_))
        , host_(host)
        , port_(port)
        , path_(path)
        , use_ssl_(use_ssl)
        , ssl_ctx_(ssl::context::tlsv12_client)
        , reconnect_timer_(ioc_)
        , ping_timer_(ioc_)
        , running_(false)
        , writing_(false)
        , reconnect_delay_s_(1)
    {
        if (use_ssl_) {
            ssl_ctx_.set_default_verify_paths();
            ssl_ctx_.set_verify_mode(ssl::verify_none);
        }
    }

    virtual ~WsClientBase()
    {
        asio::post(strand_, [this]() {
            try {
                running_ = false;
                cancelAll();
                closeSocketsSync();
            }
            catch (...) {
            }
        });
    }

    virtual void onConnected() {}
    virtual void onDisconnected() {}
    virtual void onMessage(const std::string &) {}
    virtual void onBinary(const std::vector<uint8_t> &) {}
    virtual void onError(const std::string &) {}

    void start()
    {
        // thread-safe
        asio::post(strand_, [weak = weak_from_this()]() {
            if (auto self = weak.lock()) {
                if (self->running_)
                    return;
                self->running_ = true;
                self->reconnect_delay_s_ = 1;
                self->doResolve();
            }
        });
    }

    void stop()
    {
        // thread-safe
        asio::post(strand_, [weak = weak_from_this()]() {
            if (auto self = weak.lock()) {
                if (!self->running_)
                    return;
                self->running_ = false;
                self->cancelAll();
                // start an orderly close if socket is open
                self->asyncCloseSockets();
                // notify sub-classes
                self->onDisconnected();
            }
        });
    }

    void sendText(const std::string &txt)
    {
        queueSend(txt, true);
    }

    void sendBinary(const void *data, size_t size)
    {
        queueSend(std::string(reinterpret_cast<const char *>(data), size), false);
    }

protected:
    // helpers that subclasses might call
    bool isRunning() const { return running_; }
    bool isConnected() const
    {
        if (use_ssl_)
            return ws_ssl_ && ws_ssl_->is_open();
        else
            return ws_plain_ && ws_plain_->is_open();
    }

private:
    // Message queue type
    struct Message
    {
        std::string data;
        bool is_text;
    };

    // ---------- members ----------
    asio::io_context &ioc_;
    asio::strand<asio::io_context::executor_type> strand_;
    tcp::resolver resolver_;
    std::string host_;
    std::string port_;
    std::string path_;
    bool use_ssl_;
    ssl::context ssl_ctx_;

    std::unique_ptr<websocket::stream<beast::tcp_stream>> ws_plain_;
    std::unique_ptr<websocket::stream<beast::ssl_stream<beast::tcp_stream>>> ws_ssl_;

    beast::flat_buffer buffer_;

    std::deque<Message> send_queue_;
    std::mutex queue_mutex_;

    asio::executor_work_guard<asio::io_context::executor_type> work_guard_ {asio::make_work_guard(ioc_)};

    asio::steady_timer reconnect_timer_;
    asio::steady_timer ping_timer_;

    std::atomic<bool> running_;
    bool writing_; // only accessed on strand_

    // reconnect backoff (in seconds)
    int reconnect_delay_s_;

    // ---------- implementation ----------

    void doResolve()
    {
        // capture weak
        auto weak = weak_from_this();
        resolver_.async_resolve(host_, port_,
                                asio::bind_executor(strand_,
                                                    [weak](const boost::system::error_code &ec, tcp::resolver::results_type results) {
                                                        if (auto self = weak.lock())
                                                            self->onResolve(ec, results);
                                                    }));
    }

    void onResolve(const boost::system::error_code &ec, tcp::resolver::results_type results)
    {
        if (!running_)
            return;
        if (ec) {
            onError(std::string("[Resolve] ") + ec.message());
            scheduleReconnect();
            return;
        }

        if (use_ssl_) {
            ws_ssl_.reset(new websocket::stream<beast::ssl_stream<beast::tcp_stream>>(ioc_, ssl_ctx_));
            // set SNI if possible
            if (!SSL_set_tlsext_host_name(ws_ssl_->next_layer().native_handle(), host_.c_str())) {
                // non-fatal
            }

            // connect TCP
            beast::get_lowest_layer(*ws_ssl_).expires_never();
            auto weak = weak_from_this();
            beast::get_lowest_layer(*ws_ssl_).async_connect(results,
                                                            asio::bind_executor(strand_,
                                                                                [weak](const boost::system::error_code &ec, auto) {
                                                                                    if (auto self = weak.lock())
                                                                                        self->onConnectSSL(ec);
                                                                                }));
        }
        else {
            ws_plain_.reset(new websocket::stream<beast::tcp_stream>(ioc_));
            beast::get_lowest_layer(*ws_plain_).expires_never();
            auto weak = weak_from_this();
            beast::get_lowest_layer(*ws_plain_).async_connect(results, asio::bind_executor(strand_, [weak](const boost::system::error_code &ec, auto) {
                                                                  if (auto self = weak.lock())
                                                                      self->onConnectPlain(ec);
                                                              }));
        }
    }

    void onConnectPlain(const boost::system::error_code &ec)
    {
        if (!running_)
            return;
        if (ec) {
            onError(std::string("[ConnectPlain] ") + ec.message());
            scheduleReconnect();
            return;
        }
        // Set reasonable timeouts and auto-ping behavior
        if (ws_plain_) {
            websocket::stream_base::timeout opt = websocket::stream_base::timeout::suggested(beast::role_type::client);
            ws_plain_->set_option(opt);
            ws_plain_->control_callback(std::bind(&WsClientBase::onControlCallback, this, std::placeholders::_1, std::placeholders::_2));
        }
        // handshake - include port if non-standard
        std::string host_header = host_;
        if (port_ != "80" && port_ != "443")
            host_header += ":" + port_;
        auto weak = weak_from_this();
        ws_plain_->async_handshake(host_header, path_,
                                   asio::bind_executor(strand_,
                                                       [weak](const boost::system::error_code &ec) {
                                                           if (auto self = weak.lock())
                                                               self->onHandshake(ec);
                                                       }));
    }

    void onConnectSSL(const boost::system::error_code &ec)
    {
        if (!running_)
            return;
        if (ec) {
            onError(std::string("[ConnectSSL] ") + ec.message());
            scheduleReconnect();
            return;
        }
        auto weak = weak_from_this();
        // SSL handshake
        ws_ssl_->next_layer().async_handshake(ssl::stream_base::client,
                                              asio::bind_executor(strand_,
                                                                  [weak](const boost::system::error_code &ec) {
                                                                      if (auto self = weak.lock())
                                                                          self->onSSLHandshake(ec);
                                                                  }));
    }

    void onSSLHandshake(const boost::system::error_code &ec)
    {
        if (!running_)
            return;
        if (ec) {
            onError(std::string("[SSLHandshake] ") + ec.message());
            scheduleReconnect();
            return;
        }
        // Set timeouts and control callback
        if (ws_ssl_) {
            websocket::stream_base::timeout opt = websocket::stream_base::timeout::suggested(beast::role_type::client);
            ws_ssl_->set_option(opt);
            ws_ssl_->control_callback(std::bind(&WsClientBase::onControlCallback, this, std::placeholders::_1, std::placeholders::_2));
        }

        std::string host_header = host_;
        if (port_ != "80" && port_ != "443")
            host_header += ":" + port_;
        auto weak = weak_from_this();
        ws_ssl_->async_handshake(host_header, path_,
                                 asio::bind_executor(strand_,
                                                     [weak](const boost::system::error_code &ec) {
                                                         if (auto self = weak.lock())
                                                             self->onHandshake(ec);
                                                     }));
    }

    void onHandshake(const boost::system::error_code &ec)
    {
        if (!running_)
            return;
        if (ec) {
            onError(std::string("[Handshake] ") + ec.message());
            scheduleReconnect();
            return;
        }

        // connected
        reconnect_delay_s_ = 1; // reset backoff
        onConnected();

        doRead();
        // todo ping
        startWrite();
    }

    void doRead()
    {
        if (!running_)
            return;
        buffer_.consume(buffer_.size());
        auto weak = weak_from_this();
        if (use_ssl_ && ws_ssl_) {
            ws_ssl_->async_read(buffer_,
                                asio::bind_executor(strand_,
                                                    [weak](const boost::system::error_code &ec, std::size_t bytes) {
                                                        if (auto self = weak.lock())
                                                            self->onRead(ec, bytes);
                                                    }));
        }
        else if (!use_ssl_ && ws_plain_) {
            ws_plain_->async_read(buffer_,
                                  asio::bind_executor(strand_,
                                                      [weak](const boost::system::error_code &ec, std::size_t bytes) {
                                                          if (auto self = weak.lock())
                                                              self->onRead(ec, bytes);
                                                      }));
        }
    }

    void onRead(const boost::system::error_code &ec, std::size_t bytes_transferred)
    {
        if (!running_)
            return;
        if (ec) {
            onError(std::string("[Read] ") + ec.message());
            scheduleReconnect();
            return;
        }

        // Distinguish text/binary
        bool got_text = false;
        if (use_ssl_ && ws_ssl_) {
            got_text = ws_ssl_->got_text();
        }
        if (!use_ssl_ && ws_plain_) {
            got_text = ws_plain_->got_text();
        }

        if (got_text) {
            std::string s = beast::buffers_to_string(buffer_.data());
            onMessage(s);
        }
        else {
            std::vector<uint8_t> v(bytes_transferred);
            asio::buffer_copy(asio::buffer(v), buffer_.data());
            onBinary(v);
        }

        // continue reading
        doRead();
    }

    void queueSend(const std::string &data, bool is_text)
    {
        {
            std::lock_guard<std::mutex> lg(queue_mutex_);
            send_queue_.push_back({data, is_text});
        }
        asio::post(strand_, [weak = weak_from_this()]() {
            if (auto self = weak.lock()) {
                if (!self->running_)
                    return;
                self->startWrite();
            }
        });
    }

    void startWrite()
    {
        if (!running_ || writing_ || !isConnected()) {
            return;
        }

        Message msg;
        {
            std::lock_guard<std::mutex> lg(queue_mutex_);
            if (send_queue_.empty()) {
                return;
            }
            msg = send_queue_.front();
            send_queue_.pop_front();
        }

        writing_ = true;
        auto buf = std::make_shared<std::string>(msg.data);
        auto weak = weak_from_this();

        if (use_ssl_) {
            if (!ws_ssl_ || !ws_ssl_->is_open()) {
                writing_ = false;
                return;
            }
            ws_ssl_->text(msg.is_text);
            ws_ssl_->async_write(asio::buffer(*buf),
                                 asio::bind_executor(strand_,
                                                     [weak, buf](const boost::system::error_code &ec, std::size_t bytes) {
                                                         if (auto self = weak.lock())
                                                             self->onWrite(ec, bytes);
                                                     }));
        }
        else {
            if (!ws_plain_ || !ws_plain_->is_open()) {
                writing_ = false;
                return;
            }
            ws_plain_->text(msg.is_text);
            ws_plain_->async_write(asio::buffer(*buf),
                                   asio::bind_executor(strand_,
                                                       [weak, buf](const boost::system::error_code &ec, std::size_t bytes) {
                                                           if (auto self = weak.lock())
                                                               self->onWrite(ec, bytes);
                                                       }));
        }
    }

    void onWrite(const boost::system::error_code &ec, std::size_t /*bytes*/)
    {
        if (!running_)
            return;
        if (ec) {
            writing_ = false;
            onError(std::string("[Write] ") + ec.message());
            scheduleReconnect();
            return;
        }
        writing_ = false;
        // send next queued
        startWrite();
    }

    // todo ping

    void onControlCallback(websocket::frame_type kind, boost::beast::string_view payload)
    {
        (void)kind;
        (void)payload;
    }

    void asyncCloseSockets()
    {
        auto weak = weak_from_this();
        if (use_ssl_ && ws_ssl_ && ws_ssl_->is_open()) {
            ws_ssl_->async_close(websocket::close_code::normal,
                                 asio::bind_executor(strand_,
                                                     [weak](const boost::system::error_code &ec) {
                                                         if (auto self = weak.lock()) {
                                                             if (ec)
                                                                 self->onError(std::string("[CloseSSL] ") + ec.message());
                                                         }
                                                     }));
        }
        if (!use_ssl_ && ws_plain_ && ws_plain_->is_open()) {
            ws_plain_->async_close(websocket::close_code::normal,
                                   asio::bind_executor(strand_,
                                                       [weak](const boost::system::error_code &ec) {
                                                           if (auto self = weak.lock()) {
                                                               if (ec)
                                                                   self->onError(std::string("[ClosePlain] ") + ec.message());
                                                           }
                                                       }));
        }
    }

    void closeSocketsSync()
    {
        if (use_ssl_) {
            if (ws_ssl_) {
                try {
                    auto &layer = beast::get_lowest_layer(*ws_ssl_);
                    layer.socket().shutdown(tcp::socket::shutdown_both);
                    layer.socket().close();
                }
                catch (...) {
                }
                ws_ssl_.reset();
            }
        }
        else {
            if (ws_plain_) {
                try {
                    auto &layer = beast::get_lowest_layer(*ws_plain_);
                    layer.socket().shutdown(tcp::socket::shutdown_both);
                    layer.socket().close();
                }
                catch (...) {
                }
                ws_plain_.reset();
            }
        }
    }

    void cancelAll()
    {
        reconnect_timer_.cancel();
        ping_timer_.cancel();
        resolver_.cancel();
        if (use_ssl_ && ws_ssl_) {
            beast::get_lowest_layer(*ws_ssl_).cancel();
        }
        if (!use_ssl_ && ws_plain_) {
            beast::get_lowest_layer(*ws_plain_).cancel();
        }
    }

    void scheduleReconnect()
    {
        if (!running_)
            return;

        try {
            if (use_ssl_ && ws_ssl_) {
                auto &layer = beast::get_lowest_layer(*ws_plain_);
                layer.socket().shutdown(tcp::socket::shutdown_both);
                layer.socket().close();
            }
            if (!use_ssl_ && ws_plain_) {
                auto &layer = beast::get_lowest_layer(*ws_plain_);
                layer.socket().shutdown(tcp::socket::shutdown_both);
                layer.socket().close();
            }
        }
        catch (...) {
        }
        ws_ssl_.reset();
        ws_plain_.reset();

        // exponential backoff
        auto delay = std::chrono::seconds(reconnect_delay_s_);
        reconnect_delay_s_ = std::min(reconnect_delay_s_ * 2, 60); // cap at 60s

        auto weak = weak_from_this();
        reconnect_timer_.expires_after(delay);
        reconnect_timer_.async_wait(asio::bind_executor(strand_, [weak](const boost::system::error_code &ec) {
            if (auto self = weak.lock()) {
                if (ec) {
                    return;
                }
                if (!self->running_) {
                    return;
                }
                self->doResolve();
            }
        }));
    }
};