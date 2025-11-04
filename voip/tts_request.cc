#include "tts_request.h"
#include "logger.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/steady_timer.hpp>
#include <json/json.h>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

std::shared_ptr<TTSPlayer> TTSPlayer::instance_ = nullptr;
std::string TTSPlayer::host_, TTSPlayer::port_, TTSPlayer::target_;

// --- 工具函数：读取PCM文件 ---
static std::vector<char> read_pcm(const std::string &filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
        throw std::runtime_error("无法打开文件: " + filename);

    file.seekg(0, std::ios::end);
    std::streampos size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    file.close();

    return buffer;
}

TTSPlayer::~TTSPlayer()
{
    stop();
}

std::vector<char> TTSPlayer::requestTTS(const std::string &text)
{
    boost::asio::io_context ioc;
    tcp::resolver resolver(ioc);
    tcp::socket socket(ioc);
    boost::system::error_code ec;

    // 解析域名
    auto const results = resolver.resolve(host_, port_, ec);
    if (ec) {
        throw std::runtime_error("TTS 解析失败: " + ec.message());
    }

    // 连接服务器
    boost::asio::connect(socket, results.begin(), results.end(), ec);
    if (ec) {
        throw std::runtime_error("TTS 连接失败: " + ec.message());
    }

    // 构造 JSON 请求体
    Json::Value root;
    root["text"] = text;
    Json::StreamWriterBuilder writer;
    std::string body = Json::writeString(writer, root);

    // 构造 HTTP POST 请求
    http::request<http::string_body> req {http::verb::post, target_, 11};
    req.set(http::field::host, host_);
    req.set(http::field::user_agent, "Boost.Beast-TTSClient");
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.prepare_payload();

    // 发送请求
    http::write(socket, req, ec);
    if (ec) {
        throw std::runtime_error("TTS 请求发送失败: " + ec.message());
    }

    // --- 设置超时定时器 ---
    boost::asio::steady_timer timer(ioc);
    bool timeout = false;

    timer.expires_after(std::chrono::seconds(3)); // 设置超时时间为3秒
    timer.async_wait([&](const boost::system::error_code &e) {
        if (!e) {
            timeout = true;
            socket.cancel(); // 强制中断 read()
        }
    });

    // 异步接收响应
    boost::beast::flat_buffer buffer;
    http::response<http::vector_body<char>> res;

    http::async_read(socket, buffer, res,
                     [&](const boost::system::error_code &e, std::size_t) {
                         ec = e;
                         timer.cancel(); // 成功读完就取消定时器
                     });

    // 运行事件循环
    ioc.run();

    // 关闭连接
    socket.shutdown(tcp::socket::shutdown_both, ec);

    if (timeout) {
        LOG_WARN("[TTS] requestTTS 超时，未收到PCM数据");
        return {}; // 返回空数据
    }

    if (ec && ec != boost::asio::error::operation_aborted) {
        throw std::runtime_error("TTS 响应读取失败: " + ec.message());
    }

    if (res.result() != http::status::ok) {
        throw std::runtime_error("TTS 请求失败: " + std::to_string(res.result_int()));
    }

    auto pcm_data = res.body();
    if (pcm_data.size() % 2 != 0)
        pcm_data.pop_back();

    return pcm_data;
}

// --- 生产TTS音频 ---
void TTSPlayer::produceTTS(const std::vector<std::string> &texts)
{
    for (auto &text : texts) {
        if (stop_flag_)
            break;

        try {
            auto pcm = requestTTS(text);
            //            auto pcm = read_pcm("pcm_2025_10_14_10_30_09.pcm");
            if (stop_flag_)
                break;

            if (pcm.empty()) {
                LOG_WARN("[TTS] PCM is empty，maybe timeout，terminal TTS create");
                break; // 超时直接退出
            }

            {
                std::lock_guard<std::mutex> lock(mtx_);
                audio_queue_.push(std::move(pcm));
            }
            LOG_DEBUG("[TTS] 音频队列大小: {} ", audio_queue_.size());
        }
        catch (const std::exception &e) {
            LOG_ERROR("[TTS] 生成失败: {}", e.what());
        }
    }

    // 推入空标志表示结束
    {
        std::lock_guard<std::mutex> lock(mtx_);
        audio_queue_.push({});
    }
}

// --- 消费音频数据 ---
bool TTSPlayer::getNextAudio(std::vector<char> &pcm)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (audio_queue_.empty())
        return false;

    pcm = std::move(audio_queue_.front());
    audio_queue_.pop();
    return !pcm.empty();
}

// --- 停止TTS ---
void TTSPlayer::stop()
{
    stop_flag_ = true;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        while (!audio_queue_.empty()) {
            audio_queue_.pop();
        }
        LOG_INFO("[TTS] stop produce WAV and clear text_list");
    }
}

// --- 恢复 ---
void TTSPlayer::resume()
{
    stop_flag_ = false;
    LOG_INFO("[TTS] resume produce WAV and clear text_list");
}
