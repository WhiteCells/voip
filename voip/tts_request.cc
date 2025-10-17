#include "tts_request.h"
#include "logger.h"
#include <iostream>
#include <fstream>
#include <vector>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

std::shared_ptr<TTSPlayer> TTSPlayer::instance_ = nullptr;
std::string TTSPlayer::host_, TTSPlayer::port_, TTSPlayer::target_;

static std::vector<char>read_pcm(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) throw std::runtime_error("无法打开文件: " + filename);

    file.seekg(0, std::ios::end);
    std::streampos size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    file.close();

    return buffer;
}

TTSPlayer::~TTSPlayer() {
    stop();
}

// --- HTTP 请求部分 ---
std::vector<char> TTSPlayer::requestTTS(const std::string& text)
{
    boost::asio::io_context ioc;
    tcp::resolver resolver(ioc);
    tcp::socket socket(ioc);

    auto const results = resolver.resolve(host_, port_);
    boost::asio::connect(socket, results.begin(), results.end());

    // 构造 JSON 请求体
    Json::Value root;
    root["text"] = text;
    Json::StreamWriterBuilder writer;
    std::string body = Json::writeString(writer, root);

    // 构造 HTTP POST 请求
    http::request<http::string_body> req{http::verb::post, target_, 11};
    req.set(http::field::host, host_);
    req.set(http::field::user_agent, "Boost.Beast-TTSClient");
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.prepare_payload();

    // 发送请求
    http::write(socket, req);

    // 接收响应
    boost::beast::flat_buffer buffer;
    http::response<http::vector_body<char>> res;
    http::read(socket, buffer, res);

    socket.shutdown(tcp::socket::shutdown_both);

    if (res.result() != http::status::ok)
        throw std::runtime_error("TTS 请求失败: " + std::to_string(res.result_int()));

    auto pcm_data = res.body();
    if (pcm_data.size() % 2 != 0)
        pcm_data.pop_back();

    return pcm_data;
}

// --- 生产数据 ---
void TTSPlayer::produceTTS(const std::vector<std::string>& texts)
{
    for (auto& text : texts) {
        if (stop_flag_) {
            break;
        }
        try {
//            LOG_INFO("[TTS] 获取内容: {}", text);
            auto pcm = requestTTS(text);
//            auto pcm = read_pcm("pcm_2025_10_14_10_30_09.pcm");
            {
                std::lock_guard<std::mutex> lock(mtx_);
                audio_queue_.push(std::move(pcm));
            }
            LOG_DEBUG("[TTS] 音频大小: {} ", audio_queue_.size());
        } catch (const std::exception& e) {
            LOG_ERROR("[TTS] 生成失败: {}", e.what());
        }
    }

    // 推入一个空标志表示结束
    {
        std::lock_guard<std::mutex> lock(mtx_);
        audio_queue_.push({});
    }
}

// --- 消费数据 ---
bool TTSPlayer::getNextAudio(std::vector<char>& pcm)
{
//    LOG_INFO("[TTS] 获取音频");
    std::lock_guard<std::mutex> lock(mtx_);
//    LOG_INFO("[TTS] 音频队列大小: {}", audio_queue_.size());
    if (audio_queue_.empty()) return false;

    pcm = std::move(audio_queue_.front());
//    LOG_INFO("[TTS] pcm音频大小: {}", pcm.size());
    audio_queue_.pop();
    return !pcm.empty();
}

// --- 停止 ---
void TTSPlayer::stop(){
    stop_flag_ = true;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        while (!audio_queue_.empty()) {
            audio_queue_.pop();
        }
        LOG_INFO("[TTS] stop produce WAV and clear text_list");
    }
}

void TTSPlayer::resume() {
    stop_flag_ = false;
    LOG_INFO("[TTS] resume produce WAV and clear text_list");
}
