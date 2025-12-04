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

// initialize generation_
std::atomic<uint64_t> TTSPlayer::generation_ {0};

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

std::vector<char> TTSPlayer::requestTTS(std::string text,uint64_t my_gen)
{
    asio::io_context &ioc = IOContextPool::getInstance()->getIOContext();

    auto sock = std::make_shared<tcp::socket>(ioc);
    {
        std::lock_guard<std::mutex> lock(socket_mtx_);
        active_socket_ = sock;
    }

    tcp::resolver resolver(ioc);
    boost::system::error_code ec;

    auto results = resolver.resolve(host_, port_, ec);
    if (ec) {
        LOG_ERROR("http resolve: {}", ec.message());
        return {};
    }

    boost::asio::connect(*sock, results, ec);
    if (ec) {
        LOG_ERROR("http connect: {}", ec.message());
        return {};
    }

    // --- 构建 HTTP body ---
    Json::Value root;
    root["input"] = text;
    root["voice"] = g_tts_voice;
    root["response_format"] = "pcm";
    root["sample_rate"] = 16000;
    root["speed"] = g_tts_speed;

    Json::StreamWriterBuilder writer;
    std::string body = Json::writeString(writer, root);

    http::request<http::string_body> req {http::verb::post, target_, 11};
    req.set(http::field::content_type, "application/json; charset=utf-8");
    req.set(http::field::host, host_ + ":" + port_);
    req.body() = body;
    req.prepare_payload();

    http::write(*sock, req, ec);
    if (ec) {
        LOG_ERROR("http write error: {}", ec.message());
        return {};
    }

    boost::beast::flat_buffer buffer;

    // 用 parser 支持 chunked
    http::response_parser<http::vector_body<char>> parser;
    parser.body_limit((std::numeric_limits<std::uint64_t>::max)());

    // ---- 先读 header ----
    http::read_header(*sock, buffer, parser, ec);
    if (ec) {
        LOG_ERROR("read_header: {}", ec.message());
        return {};
    }

    // 非 200 直接打印错误
    if (parser.get().result() != http::status::ok) {
        http::response<http::vector_body<char>> tmp = parser.release();
        std::string err(tmp.body().begin(), tmp.body().end());
        LOG_ERROR("HTTP {} body: {}", (unsigned)tmp.result(), err);
        return {};
    }

    // ---- 流式读取 chunk ----
    std::vector<char> pcm;
    pcm.reserve(65536);

    while (!parser.is_done()) {
        http::read(*sock, buffer, parser, ec);
        if (ec == http::error::end_of_stream)
            break;
        if (ec) {
            LOG_ERROR("read chunk: {}", ec.message());
            break;
        }
    }

    auto res = parser.release();
    if (!res.body().empty()) {
        pcm.insert(pcm.end(), res.body().begin(), res.body().end());
    }

    // generation 检查
    if (my_gen != generation_) {
        LOG_INFO("[TTS] dropped PCM (new generation)");
        return {};
    }

    if (ec || res.result() != http::status::ok) {
        //        LOG_ERROR("request status error, result: {}, ec: {}", (unsigned)res.result(), ec.message());
        std::string err_body(res.body().begin(), res.body().end());
        LOG_ERROR("request status error, result: {}, body: {}", (unsigned)res.result(), err_body);
        return {};
    }

    return pcm;
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

// produceTTSAsync: 最小化改动，++generation_, cancel old socket, 提交新任务并传 gen
void TTSPlayer::produceTTSAsync(std::vector<std::string> texts, std::string session_id)
{
    tts_flag_.store( true);
    LOG_INFO("[TTS] produceTTSAsync tts_flag {}", tts_flag_.load());

    if (!g_tts_thread_pool) {
        LOG_ERROR("TTS thread pool not initialized");
        return;
    }
    LOG_INFO("[TTS] Thread pool is valid, adding task...");

    //    resume();

    // --- NEW: bump generation and cancel previous active socket ---
    uint64_t my_gen = ++generation_;
    {
        std::lock_guard<std::mutex> lock(socket_mtx_);
        if (auto s = active_socket_.lock()) {
            boost::system::error_code ec;
            //            s->cancel(ec);
            clear();
            LOG_INFO("[TTS] Canceled previous active TTS socket (ec: {})", ec.message());
        }
    }

    g_tts_thread_pool->addTask([texts = std::move(texts), session_id, my_gen]() {
        LOG_INFO("[TTS] Task started in thread pool, calling produceTTS...");
        TTSPlayer::getInstance()->produceTTS(texts, session_id, my_gen);
    });
}

// --- 生产TTS音频 ---
void TTSPlayer::produceTTS(std::vector<std::string> texts, std::string session_id, uint64_t my_gen)
{
    tts_flag_.store( true);
    m_session_id = session_id;

    if (my_gen != generation_) {
        LOG_INFO("[TTS] produceTTS aborted immediately (newer generation exists)");
        return;
    }

    std::uint64_t total_req_cast = 0;
    float total_play_cast = 0.0f;
    std::string tts_text = {};

    for (const auto &t : texts) {
        LOG_INFO("produce TTS text: {}", t);
    }

    LOG_INFO("produce TTSPlay::endendend_flag: {}", TTSPlayer::endendend_flag.load());
    if (!texts.empty() && texts[0].rfind("ENDENDEND:", 0) == 0) {
        TTSPlayer::endendend_flag.store(true);
        LOG_INFO("set endendend_flag {}", TTSPlayer::endendend_flag.load());
        // 去除 ENDENDEND:
        const size_t prefix_len = std::strlen("ENDENDEND:");
        std::string new_text = texts[0].substr(prefix_len);
        texts[0] = new_text;
    }
    LOG_INFO("produce TTSPlay::endendend_flag: {}", TTSPlayer::endendend_flag.load());

    auto start_time = get_current_timestamp_milliseconds();

    for (const std::string &text : texts) {
        // 每次循环开始前检查 generation 和 stop_flag_
        if (my_gen != generation_) {
            LOG_INFO("[TTS] produceTTS aborted mid-way (newer generation)");
            break;
        }

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
            auto pcm = requestTTS(text,my_gen);
            // 如果在请求期间被取消，requestTTS 会返回空（或抛出），因此再次检查 generation
            if (my_gen != generation_) {
                LOG_INFO("[TTS] produceTTS aborted after requestTTS (newer generation)");
                break;
            }

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
                LOG_WARN("[TTS] PCM is empty，maybe timeout or cancelled，terminal TTS create");
                break; // 超时或被 cancel 直接退出当前任务
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

    {
        std::lock_guard<std::mutex> lock(mtx_);
        audio_queue_.push({'T', 'A', 'I', 'L'});
    }
    //    tts_ok_flag_.store(true);

    // 插入特殊标志
    LOG_INFO("to insert end flag produce TTSPlay::endendend_flag: {}", TTSPlayer::endendend_flag.load());
    if (TTSPlayer::endendend_flag.load()) {
        std::lock_guard<std::mutex> lock(mtx_);
        std::vector<char> END_FLAG {'E', 'N', 'D'};
        if (!audio_queue_.empty()) {
            LOG_INFO("insert END_FLAG");
            audio_queue_.push(END_FLAG);
        }
        else {
            LOG_ERROR("audio queue is empty");
        }
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

bool TTSPlayer::empty()
{
    std::lock_guard<std::mutex> lock(mtx_);
//    LOG_INFO("TTSPlayer::empty");
    return audio_queue_.empty();
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
    // optional: bump generation_ to invalidate running tasks immediately
    ++generation_;

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

    // cancel active socket if any
    {
        std::lock_guard<std::mutex> lock(socket_mtx_);
        if (auto s = active_socket_.lock()) {
            boost::system::error_code ec;
            //            s->cancel(ec);
            LOG_INFO("[TTS] stop(): canceled active socket (ec: {})", ec.message());
        }
    }
}

// --- 恢复 ---
void TTSPlayer::resume()
{
    stop_flag_ = false;
    LOG_INFO("[TTS] resume produce WAV and clear text_list");
}
