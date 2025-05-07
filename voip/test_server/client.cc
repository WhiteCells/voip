#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <json/json.h>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

void send_heartbeat(websocket::stream<tcp::socket> &ws)
{
    Json::Value root;
    root["type"] = "heartbeat";

    Json::StreamWriterBuilder writer;
    std::string message = Json::writeString(writer, root);

    ws.write(asio::buffer(message));
}

int main()
{
    try {
        asio::io_context ioc;
        tcp::resolver resolver(ioc);
        websocket::stream<tcp::socket> ws(ioc);

        auto const results = resolver.resolve("localhost", "8765");
        auto ep = asio::connect(ws.next_layer(), results);

        ws.handshake(ep.address().to_string() + ":" + std::to_string(ep.port()), "/");

        std::cout << "Connected to WebSocket server." << std::endl;

        std::thread heartbeat_thread([&ws]() {
            while (true) {
                try {
                    if (ws.is_open()) {
                        send_heartbeat(ws);
                        std::cout << "Sent heartbeat" << std::endl;
                    }
                    else {
                        std::cout << "ws client close" << std::endl;
                    }
                }
                catch (const std::exception &e) {
                    std::cerr << "Heartbeat error: " << e.what() << std::endl;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        });

        heartbeat_thread.join();
    }
    catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
