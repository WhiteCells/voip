#pragma once

#include <boost/asio.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <json/json.h>
#include <deque>
#include <mutex>
#include <memory>
#include <iostream>
#include <string>
#include <atomic>
#include <chrono>

namespace asio = boost::asio;
namespace ssl = boost::asio::ssl;
namespace beast = boost::beast;
namespace websocket = beast::websocket;

using tcp = boost::asio::ip::tcp;

class ASRWsClient : public std::enable_shared_from_this<ASRWsClient>
{
public:
    ASRWsClient(asio::io_context &ioc,
                const std::string &host,
                const std::string &port,
                const std::string &path,
                bool use_ssl = true)
        : ioc_(ioc)
        , strand_(asio::make_strand(ioc))
        , resolver_(asio::make_strand(ioc))
        , host_(host)
        , port_(port)
        , path_(path)
        , use_ssl_(use_ssl)
        , ssl_ctx_(ssl::context::tlsv12_client)
        , running_(false)
        , writing_(false)
    {
        if (use_ssl_) {
            ssl_ctx_.set_default_verify_paths();
            ssl_ctx_.set_verify_mode(ssl::verify_none); // todo change for prod
        }
    }

    ~ASRWsClient()
    {
        try {
            doClose();
        }
        catch (...) {
        }
    }

    void start()
    {
        asio::post(strand_, [self = shared_from_this()]() {
            if (self->running_) {
                return;
            }
            self->running_ = true;
            self->doResolve();
        });
    }

    void stop()
    {
        asio::post(strand_, [weak_self = weak_from_this()]() {
            if (auto self = weak_self.lock()) {
                if (!self->running_) {
                    return;
                }

                self->running_ = false;
                self->doClose();
            }
        });
    }

    void send(const std::string &data, bool is_text = true)
    {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            send_queue_.emplace_back(Message {data, is_text});
        }
        asio::post(strand_, [self = shared_from_this()]() {
            if (!self->running_) {
                return;
            }
            self->startWrite();
        });
    }

    void send_start_config()
    {
        Json::Value config;
        config["mode"] = "2pass";
        config["wav_name"] = "record";
        config["wav_format"] = "pcm";
        config["audio_fs"] = 16000.0;
        config["is_speaking"] = true;
        config["itn"] = true;
        config["svs_itn"] = true;
        Json::Value chunk_size(Json::arrayValue);
        chunk_size.append(5);
        chunk_size.append(10);
        chunk_size.append(5);
        config["chunk_size"] = chunk_size;

        Json::StreamWriterBuilder builder;
        std::string config_str = Json::writeString(builder, config);
        send(config_str);
    }

    void send_stop_config()
    {
        Json::Value config;
        config["is_speaking"] = "false";

        Json::StreamWriterBuilder builder;
        std::string config_str = Json::writeString(builder, config);
        send(config_str);
    }

private:
    void doResolve()
    {
        // resolve host:port
        resolver_.async_resolve(host_, port_,
                                asio::bind_executor(
                                    strand_,
                                    std::bind(&ASRWsClient::onResolve, shared_from_this(),
                                              std::placeholders::_1, std::placeholders::_2)));
    }

    void onResolve(const boost::system::error_code &ec, tcp::resolver::results_type results)
    {
        if (ec == asio::error::operation_aborted) {
            return; // operation_aborted is expected when stopping
        }
        if (!running_) {
            return;
        }
        if (ec) {
            std::cerr << "[onResolve] error: " << ec.message() << "\n";
            scheduleReconnect();
            return;
        }

        // create appropriate websocket stream
        if (use_ssl_) {
            // create SSL websocket stream
            ws_ssl_.reset(new websocket::stream<beast::ssl_stream<beast::tcp_stream>>(ioc_, ssl_ctx_));
            // set SNI hostname (optional but recommended)
            if (!SSL_set_tlsext_host_name(ws_ssl_->next_layer().native_handle(), host_.c_str())) {
                boost::system::error_code ec2 {static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
                std::cerr << "[onResolve] SNI error: " << ec2.message() << "\n";
            }

            // connect TCP
            // beast::get_lowest_layer(*ws_ssl_).expires_after(std::chrono::seconds(30));
            beast::get_lowest_layer(*ws_ssl_).expires_never();
            beast::get_lowest_layer(*ws_ssl_).async_connect(
                results,
                asio::bind_executor(
                    strand_,
                    std::bind(&ASRWsClient::onConnectSSL, shared_from_this(), std::placeholders::_1)));
        }
        else {
            ws_plain_.reset(new websocket::stream<beast::tcp_stream>(ioc_));
            // beast::get_lowest_layer(*ws_plain_).expires_after(std::chrono::seconds(30));
            beast::get_lowest_layer(*ws_plain_).expires_never();
            beast::get_lowest_layer(*ws_plain_).async_connect(results, asio::bind_executor(strand_, std::bind(&ASRWsClient::onConnectPlain, shared_from_this(), std::placeholders::_1)));
        }
    }

    void onConnectPlain(const boost::system::error_code &ec)
    {
        if (ec == asio::error::operation_aborted) {
            return; // operation_aborted is expected when stopping
        }
        if (!running_) {
            return;
        }
        if (ec) {
            std::cerr << "[onConnectPlain] error: " << ec.message() << "\n";
            scheduleReconnect();
            return;
        }

        // Perform WebSocket handshake (plain)
        ws_plain_->async_handshake(host_, path_,
                                   asio::bind_executor(
                                       strand_,
                                       std::bind(&ASRWsClient::onHandshake, shared_from_this(), std::placeholders::_1)));
    }

    void onConnectSSL(const boost::system::error_code &ec)
    {
        if (ec == asio::error::operation_aborted) {
            return; // operation_aborted is expected when stopping
        }
        if (!running_) {
            return;
        }
        if (ec) {
            std::cerr << "[onConnectSSL] error: " << ec.message() << "\n";
            scheduleReconnect();
            return;
        }

        // Perform SSL handshake first
        ws_ssl_->next_layer().async_handshake(ssl::stream_base::client,
                                              asio::bind_executor(
                                                  strand_,
                                                  std::bind(&ASRWsClient::onSSLHandshake, shared_from_this(), std::placeholders::_1)));
    }

    void onSSLHandshake(const boost::system::error_code &ec)
    {
        if (ec == asio::error::operation_aborted) {
            return; // operation_aborted is expected when stopping
        }
        if (!running_) {
            return;
        }
        if (ec) {
            std::cerr << "[onSSLHandshake] error: " << ec.message() << "\n";
            scheduleReconnect();
            return;
        }

        // Now perform WebSocket handshake over SSL
        ws_ssl_->async_handshake(host_, path_,
                                 asio::bind_executor(
                                     strand_,
                                     std::bind(&ASRWsClient::onHandshake, shared_from_this(), std::placeholders::_1)));
    }

    void onHandshake(const boost::system::error_code &ec)
    {
        if (ec == asio::error::operation_aborted) {
            return; // operation_aborted is expected when stopping
        }
        if (!running_) {
            return;
        }
        if (ec) {
            std::cerr << "[onHandshake] error: " << ec.message() << "\n";
            scheduleReconnect();
            return;
        }

        std::cout << "[onHandshake] connected to " << host_ << (use_ssl_ ? " (wss)" : " (ws)") << "\n";

        // start reading
        doRead();

        // start sending queued messages if any
        startWrite();
    }

    void doRead()
    {
        if (!running_) {
            return;
        }
        buffer_.consume(buffer_.size()); // clear buffer
        if (use_ssl_) {
            ws_ssl_->async_read(buffer_,
                                asio::bind_executor(
                                    strand_,
                                    std::bind(&ASRWsClient::onRead, shared_from_this(),
                                              std::placeholders::_1, std::placeholders::_2)));
        }
        else {
            ws_plain_->async_read(buffer_,
                                  asio::bind_executor(
                                      strand_,
                                      std::bind(&ASRWsClient::onRead, shared_from_this(),
                                                std::placeholders::_1, std::placeholders::_2)));
        }
    }

    void onRead(const boost::system::error_code &ec, std::size_t bytes_transferred)
    {
        (void)bytes_transferred;
        if (ec == asio::error::operation_aborted) {
            return; // operation_aborted is expected when stopping
        }
        if (!running_) {
            return;
        }
        if (ec) {
            std::cerr << "[onRead] error: " << ec.message() << "\n";
            scheduleReconnect();
            return;
        }

        // Print received message
        std::string msg = beast::buffers_to_string(buffer_.data());
        std::cout << "[Received] " << msg << "\n";

        // Continue reading
        doRead();
    }

    void startWrite()
    {
        if (!running_) {
            return;
        }
        if (writing_) {
            return;
        }
        if (!isConnected()) {
            return;
        }

        // pop a message
        Message m;
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (send_queue_.empty()) {
                return;
            }
            m = send_queue_.front();
            send_queue_.pop_front();
        }

        writing_ = true;

        // prepare buffer
        auto buf = std::make_shared<std::string>(m.data);

        if (use_ssl_) {
            if (!ws_ssl_ || !ws_ssl_->is_open()) {
                writing_ = false;
                return;
            }

            ws_ssl_->text(m.is_text);
            ws_ssl_->async_write(asio::buffer(*buf),
                                 asio::bind_executor(
                                     strand_,
                                     std::bind(&ASRWsClient::onWrite, shared_from_this(),
                                               std::placeholders::_1, std::placeholders::_2, buf)));
        }
        else {
            if (!ws_plain_ || !ws_plain_->is_open()) {
                writing_ = false;
                return;
            }

            ws_plain_->text(m.is_text);
            ws_plain_->async_write(asio::buffer(*buf),
                                   asio::bind_executor(
                                       strand_,
                                       std::bind(&ASRWsClient::onWrite, shared_from_this(),
                                                 std::placeholders::_1, std::placeholders::_2, buf)));
        }
    }

    void onWrite(const boost::system::error_code &ec, std::size_t bytes_transferred,
                 std::shared_ptr<std::string> /* buffer_holder */)
    {
        (void)bytes_transferred;
        if (ec == asio::error::operation_aborted) {
            return; // operation_aborted is expected when stopping
        }
        if (!running_) {
            return;
        }
        if (ec) {
            std::cerr << "[onWrite] error: " << ec.message() << "\n";
            writing_ = false;
            scheduleReconnect();
            return;
        }

        // finished writing one message, allow next
        writing_ = false;

        // start next write
        startWrite();
    }

    bool isConnected() const
    {
        if (use_ssl_) {
            return ws_ssl_ && ws_ssl_->is_open();
        }
        else {
            return ws_plain_ && ws_plain_->is_open();
        }
    }

    void doClose()
    {
        if (use_ssl_) {
            if (ws_ssl_ && ws_ssl_->is_open()) {
                beast::get_lowest_layer(*ws_ssl_).close();
            }
            ws_ssl_.reset();
        }
        else {
            if (ws_plain_ && ws_plain_->is_open()) {
                beast::get_lowest_layer(*ws_plain_).close();
            }
            ws_plain_.reset();
        }
    }

    void scheduleReconnect()
    {
        if (!running_) {
            return;
        }
        try {
            if (use_ssl_) {
                if (ws_ssl_) {
                    beast::get_lowest_layer(*ws_ssl_).close();
                }
                ws_ssl_.reset();
            }
            else {
                if (ws_plain_) {
                    beast::get_lowest_layer(*ws_plain_).close();
                }
                ws_plain_.reset();
            }
        }
        catch (...) {
        }

        auto timer = std::make_shared<asio::steady_timer>(ioc_, std::chrono::seconds(1));
        timer->async_wait(asio::bind_executor(
            strand_,
            [self = shared_from_this(), timer](const boost::system::error_code &ec) {
                if (ec) {
                    return;
                }
                if (!self->running_)
                    return;
                self->doResolve();
            }));
    }

private:
    struct Message
    {
        std::string data;
        bool is_text {true};
    };

    asio::io_context &ioc_;
    asio::strand<asio::io_context::executor_type> strand_;
    tcp::resolver resolver_;
    std::string host_;
    std::string port_;
    std::string path_;
    bool use_ssl_;

    // For plain websocket
    std::unique_ptr<websocket::stream<beast::tcp_stream>> ws_plain_;
    // For ssl websocket
    std::unique_ptr<websocket::stream<beast::ssl_stream<beast::tcp_stream>>> ws_ssl_;

    ssl::context ssl_ctx_;

    beast::flat_buffer buffer_;

    std::deque<Message> send_queue_;
    std::mutex queue_mutex_;

    std::atomic<bool> running_;
    std::atomic<bool> writing_;

public:
    static std::string s_call_method;
    static std::string s_session_id;
    static std::string s_access_token;
};
