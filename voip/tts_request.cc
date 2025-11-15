#include "tts_request.h"
#include "global.h"
#include "io_context_pool.h"
#include "logger.h"
#include "request.hpp"
#include <boost/asio/io_context.hpp>
#include <exception>
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

std::atomic<bool> TTSPlayer::endendend_flag {false};

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

std::vector<char> TTSPlayer::requestTTS(std::string text)
{
    asio::io_context &ioc = IOContextPool::getInstance()->getIOContext();
    tcp::resolver resolver(ioc);
    tcp::socket socket(ioc);
    boost::system::error_code ec;

    auto results = resolver.resolve(host_, port_, ec);
    if (ec)
        throw std::runtime_error("resolve failed: " + ec.message());
    boost::asio::connect(socket, results, ec);
    if (ec)
        throw std::runtime_error("connect failed: " + ec.message());

    Json::Value root;
    root["text"] = text;
    Json::StreamWriterBuilder writer;
    std::string body = Json::writeString(writer, root);

    http::request<http::string_body> req {http::verb::post, target_, 11};
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.prepare_payload();

    // 写请求
    http::write(socket, req, ec);
    if (ec)
        throw std::runtime_error("write failed: " + ec.message());

    boost::beast::flat_buffer buffer;
    http::response<http::vector_body<char>> res;

    // 超时控制
    std::atomic<bool> done {false};
    std::thread timeout_thread([&]() {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        if (!done.load()) {
            LOG_WARN("TTS read timeout, cancelling socket");
            socket.cancel();
        }
    });

    // 同步读取响应
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

inline std::int64_t get_current_timestamp_seconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

inline std::int64_t get_current_timestamp_milliseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

static float count_pcm_time(std::size_t pcm_len, unsigned int sample_rate,
                            unsigned int bits_per_sample, unsigned int num_channels)
{
    return static_cast<float>(pcm_len) /
           (sample_rate * num_channels * (bits_per_sample / 8.0f));
}

void TTSPlayer::produceTTSAsync(std::vector<std::string> texts, std::string session_id)
{
    if (!g_tts_thread_pool) {
        LOG_ERROR("TTS thread pool not initialized");
        return;
    }

    g_tts_thread_pool->addTask([texts = std::move(texts), session_id]() {
        TTSPlayer::getInstance()->produceTTS(texts, session_id);
    });
}

void TTSPlayer::requestTTS2(const std::string &text, const std::string &session_id)
{
    m_session_id = session_id;
    float total_play_cast = 0.0f;
    std::string tts_text = {};

    auto start_time = get_current_timestamp_milliseconds();
    try {
        auto req_start_time = get_current_timestamp_milliseconds();
        auto pcm = requestTTS(text);
        auto req_end_time = get_current_timestamp_milliseconds();
        auto req_time = static_cast<float>(req_end_time - req_start_time);
        auto play_time = count_pcm_time(pcm.size(), 16000, 16, 1);
    }
    catch (const std::exception &e) {
    }
}

// --- 生产TTS音频 ---
void TTSPlayer::produceTTS(std::vector<std::string> texts, std::string session_id)
{
    m_session_id = session_id;

    std::uint64_t total_req_cast = 0;
    float total_play_cast = 0.0f;
    std::string tts_text = {};

    for (const auto &t : texts) {
        LOG_INFO("produce TTS text: {}", t);
    }

    LOG_INFO("produce TTSPlay::endendend_flag: {}", TTSPlayer::endendend_flag.load());
    if (!texts.empty() && texts[0].rfind("ENDENDEND:", 0) == 0) {
        TTSPlayer::endendend_flag.store(true);
        // 去除 ENDENDEND:
        const size_t prefix_len = std::strlen("ENDENDEND:");
        std::string new_text = texts[0].substr(prefix_len);
        texts[0] = new_text;
    }
    LOG_INFO("produce TTSPlay::endendend_flag: {}", TTSPlayer::endendend_flag.load());

    auto start_time = get_current_timestamp_milliseconds();

    for (const std::string &text : texts) {
        if (stop_flag_) {
            LOG_INFO("[TTS] 停止TTS");
            break;
        }

        try {
            LOG_INFO("request TTS");

            // start req time
            auto req_start_time = get_current_timestamp_milliseconds();
            // request
            std::string backup = text;
            auto pcm = requestTTS(text);
            if (text != backup) {
                LOG_ERROR("!!! text changed after requestTTS !!!");
            }
            // end req time
            auto req_end_time = get_current_timestamp_milliseconds();
            // cal req time
            auto req_time = static_cast<float>(req_end_time - req_start_time);
            // cal play time
            auto play_time = count_pcm_time(pcm.size(), 16000, 16, 1);

            total_play_cast += play_time;
            total_req_cast += req_time;
            tts_text += text;

            // auto pcm = read_pcm("pcm_2025_10_14_10_30_09.pcm");
            if (stop_flag_) {
                LOG_INFO("[TTS] 丢弃TTS");
                break;
            }

            if (pcm.empty()) {
                LOG_WARN("[TTS] PCM is empty，maybe timeout，terminal TTS create");
                break; // 超时直接退出
            }

            {
                std::lock_guard<std::mutex> lock(mtx_);
                audio_queue_.push(std::move(pcm));
                LOG_DEBUG("[TTS] 音频队列大小: {} ", audio_queue_.size());
            }
        }
        catch (const std::exception &e) {
            LOG_ERROR("[TTS] 生成失败: {}", e.what());
        }
    }

    // 插入特殊标志
    if (TTSPlayer::endendend_flag.load()) {
        std::lock_guard<std::mutex> lock(mtx_);
        std::vector<char> END_FLAG {'E', 'N', 'D'};
        audio_queue_.push(END_FLAG);
    }

    voip::pushTTSStart(m_session_id, tts_text, start_time, total_play_cast, total_req_cast);
    LOG_INFO("push tts start, m_session_id: {}, tts_text: {}, start_time: {}, total_play_cast: {}, total_req_cast: {}", m_session_id, tts_text, start_time, total_play_cast, total_req_cast);
}

// --- 消费音频数据 ---
bool TTSPlayer::getNextAudio(std::vector<char> &pcm)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (audio_queue_.empty()) {
        return false;
    }

    pcm = std::move(audio_queue_.front());
    audio_queue_.pop();
    return !pcm.empty();
}

void TTSPlayer::clear()
{
    std::lock_guard<std::mutex> lock(mtx_);
    LOG_INFO("TTSPlayer::clear");
    while (!audio_queue_.empty()) {
        audio_queue_.pop();
    }
}

// --- 停止TTS ---
void TTSPlayer::stop()
{
    stop_flag_ = true;
    if (TTSPlayer::endendend_flag.load()) {
        LOG_INFO("no need to stop");
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mtx_);
        {
            // clear pcm audio pcm
            LOG_INFO("clear pcm audio queue");
            while (!audio_queue_.empty()) {
                audio_queue_.pop();
            }
        }
        auto stop_time = get_current_timestamp_milliseconds();
        voip::pushTTSStop(m_session_id, stop_time);
        LOG_INFO("push tts stop request, m_session_id: {}, stop_time: {}", m_session_id, stop_time);
    }
}

// --- 恢复 ---
void TTSPlayer::resume()
{
    stop_flag_ = false;
    LOG_INFO("[TTS] resume produce WAV and clear text_list");
}
