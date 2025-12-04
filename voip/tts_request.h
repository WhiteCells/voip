#ifndef _TTS_ASYNC_PLAYER_H_
#define _TTS_ASYNC_PLAYER_H_

#include "singleton.hpp"
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <cstdint>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <json/json.h>

class TTSPlayer
{
private:
    static std::shared_ptr<TTSPlayer> instance_;

public:
    std::vector<char> requestTTS(std::string text);

    TTSPlayer() = default;
    ~TTSPlayer();

    static std::shared_ptr<TTSPlayer> getInstance()
    {
        if (instance_ == nullptr) {
            instance_ = std::make_shared<TTSPlayer>();
        }
        return instance_;
    }

    static void init(const std::string &host,
                     const std::string &port,
                     const std::string &target)
    {
        host_ = host;
        port_ = port;
        target_ = target;
    }

    void produceTTS(std::vector<std::string> texts, std::string session_id);
    void produceTTSAsync(std::vector<std::string> texts, std::string session_id);
    bool getNextAudio(std::vector<char> &pcm);

    bool empty();

    void clear();

    void stop();

    void resume();

    bool isStopped() const
    {
        return stop_flag_.load();
    }

    static std::atomic<bool> endendend_flag;
    std::atomic<bool> tts_flag_ = false;
    //    std::atomic<bool> tts_ok_flag_ {false};

private:
    static std::string host_, port_, target_;
    std::queue<std::vector<char>> audio_queue_;
    std::mutex mtx_;
    std::atomic<bool> stop_flag_ = false;
    std::string m_session_id;
    static std::atomic<uint64_t> generation_; // task generation/version
    std::mutex socket_mtx_;                   // protect active_socket_
    std::weak_ptr<boost::asio::ip::tcp::socket> active_socket_;
};

#endif // _TTS_ASYNC_PLAYER_H_
