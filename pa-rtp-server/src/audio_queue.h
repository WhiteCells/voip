#ifndef _AUDIO_QUEUE_H_
#define _AUDIO_QUEUE_H_

#include <queue>
#include <mutex>
#include <condition_variable>

class AudioQueue
{
public:
    using FrameType = std::pair<uint16_t *, size_t>;

    AudioQueue() = default;
    AudioQueue(const AudioQueue &) = delete;
    AudioQueue &operator=(const AudioQueue &) = delete;
    virtual ~AudioQueue() = default;

    void push(uint16_t *data, size_t len);
    FrameType pop();

    size_t size() const;
    bool empty() const;

private:
    std::queue<FrameType> m_que;
    mutable std::mutex m_que_mtx;
    std::condition_variable m_que_cv;
};

#endif // _AUDIO_QUEUE_H_
