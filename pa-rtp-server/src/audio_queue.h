#ifndef _AUDIO_QUEUE_H_
#define _AUDIO_QUEUE_H_

#include <queue>
#include <mutex>
#include <condition_variable>

class AudioQueue
{
public:
    using FrameType = std::pair<int16_t *, size_t>;

    AudioQueue() = default;
    AudioQueue(const AudioQueue &) = delete;
    AudioQueue &operator=(const AudioQueue &) = delete;
    virtual ~AudioQueue() = default;

    void push(int16_t *data, size_t len)
    {
        {
            std::unique_lock<std::mutex> lock(m_que_mtx);
            m_que.push(std::make_pair(data, len));
        }
        m_que_cv.notify_one();
    }

    FrameType pop()
    {
        std::unique_lock<std::mutex> lock(m_que_mtx);
        m_que_cv.wait(lock, [this]() {
            return !m_que.empty();
        });
        FrameType p = m_que.front();
        m_que.pop();
        return p;
    }

    size_t size() const
    {
        std::unique_lock<std::mutex> lock(m_que_mtx);
        return m_que.size();
    }

    bool empty() const
    {
        std::unique_lock<std::mutex> lock(m_que_mtx);
        return m_que.empty();
    }

private:
    std::queue<FrameType> m_que;
    mutable std::mutex m_que_mtx;
    std::condition_variable m_que_cv;
};

#endif // _AUDIO_QUEUE_H_
