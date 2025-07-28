// websocket_client.h
#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <thread>
#include <iostream>
#include <string>
#include "thread_safe_queue.h"

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class WebSocketClient {
public:
    WebSocketClient(net::io_context& ioc, ThreadSafeQueue<std::string>& queue)
        : resolver_(ioc), ws_(ioc), queue_(queue) {}

    void connect(const std::string& host, const std::string& port, const std::string& target = "/") {
        resolver_.async_resolve(host, port,
            [this, host, target](beast::error_code ec, tcp::resolver::results_type results) {
                if (ec) {
                    std::cerr << "Resolve: " << ec.message() << "\n";
                    return;
                }

                net::async_connect(ws_.next_layer(), results.begin(), results.end(),
                    [this, host, target](beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
                        if (ec) {
                            std::cerr << "Connect: " << ec.message() << "\n";
                            return;
                        }

                        ws_.async_handshake(host, target,
                            [this](beast::error_code ec) {
                                if (ec) {
                                    std::cerr << "Handshake: " << ec.message() << "\n";
                                    return;
                                }
                                read_loop(); // Start reading
                            });
                    });
            });
    }

private:
    void read_loop() {
        ws_.async_read(buffer_,
            [this](beast::error_code ec, std::size_t bytes_transferred) {
                boost::ignore_unused(bytes_transferred);

                if (ec) {
                    std::cerr << "Read: " << ec.message() << "\n";
                    return;
                }

                std::string message = beast::buffers_to_string(buffer_.data());
                buffer_.consume(buffer_.size());

                queue_.push(message);  // Push to thread-safe queue
                read_loop(); // Continue reading
            });
    }

    tcp::resolver resolver_;
    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
    ThreadSafeQueue<std::string>& queue_;
};
