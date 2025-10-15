#ifndef PA_RTP_SERVER_PCM_QUEUE_H
#define PA_RTP_SERVER_PCM_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

class PCMQueue {
public:
    void push(const std::vector<char>& pcm) {
        std::unique_lock<std::mutex> lock(mtx_);
        queue_.push(pcm);
        cv_.notify_one();
    }
    std::vector<char> pop() {
        std::unique_lock<std::mutex> lock(mtx_);
        while (queue_.empty() && !stop_flag_) {
            cv_.wait(lock);
        }
        if (queue_.empty()) {
            return {};
        }
        std::vector<char> pcm = queue_.front();
        queue_.pop();
        return pcm;
    }
    bool empty() {
        std::unique_lock<std::mutex> lock(mtx_);
        return queue_.empty();
    }
    size_t size() {
        std::unique_lock<std::mutex> lock(mtx_);
        return queue_.size();
    }
    void clear() {
        std::unique_lock<std::mutex> lock(mtx_);
        queue_ = {};
    }

private:
    std::queue<std::vector<char>> queue_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> stop_flag_;
};

#endif //PA_RTP_SERVER_PCM_QUEUE_H
