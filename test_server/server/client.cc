#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

void upload_file(const std::string& host, const std::string& port, const std::string& target, const std::string& file_path) {
    asio::io_context ioc;
    tcp::resolver resolver(ioc);
    beast::tcp_stream stream(ioc);

    auto const results = resolver.resolve(host, port);
    stream.connect(results);

    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << file_path << "\n";
        return;
    }

    std::size_t file_size = file.tellg();
    if (file_size == 0) {
        std::cerr << "File is empty: " << file_path << "\n";
        return;
    }

    file.seekg(0);

    // 构建请求
    http::request<http::dynamic_body> req{http::verb::post, target, 11};
    req.set(http::field::host, host);
    req.set("Filename", std::filesystem::path(file_path).filename().string());

    // 将文件数据读取到请求体中
    beast::ostream(req.body()) << file.rdbuf();
    req.prepare_payload();

    try {
        http::write(stream, req);

        // 读取响应
        beast::flat_buffer buffer_res;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer_res, res);

        std::cout << "Response: " << res << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    stream.socket().shutdown(tcp::socket::shutdown_both);
}

int main() {
    std::string host = "127.0.0.1";
    std::string port = "5000";
    std::string target = "/upload";
    std::string file_path = "01046.wav";

    upload_file(host, port, target, file_path);
    return 0;
}
