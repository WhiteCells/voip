#ifndef _TTS_ASYNC_PLAYER_H_
#define _TTS_ASYNC_PLAYER_H_

#include "singleton.hpp"
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <json/json.h>

class TTSPlayer
{
private:
    static std::shared_ptr<TTSPlayer> instance_;

public:
    std::vector<char> requestTTS(const std::string &text);

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

    // 同步调用，依次请求 TTS 并推入队列
    void produceTTS(const std::vector<std::string> &texts);

    // 安全地从队列取一条音频数据（阻塞等待）
    bool getNextAudio(std::vector<char> &pcm);

    // 停止生产或消费
    void stop();

    void resume();

    bool isStopped() const
    {
        return stop_flag_.load();
    }

private:
    static std::string host_, port_, target_;
    std::queue<std::vector<char>> audio_queue_;
    std::mutex mtx_;
    std::atomic<bool> stop_flag_ = false;
};

#endif // _TTS_ASYNC_PLAYER_H_
