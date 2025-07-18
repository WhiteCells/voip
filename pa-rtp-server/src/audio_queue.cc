#include "audio_queue.h"

void AudioQueue::push(uint16_t *data, size_t len)
{
    {
        std::unique_lock<std::mutex> lock(m_que_mtx);
        m_que.push(std::make_pair(data, len));
    }
    m_que_cv.notify_one();
}

AudioQueue::FrameType AudioQueue::pop()
{
    std::unique_lock<std::mutex> lock(m_que_mtx);
    m_que_cv.wait(lock, [this]() {
        return !m_que.empty();
    });
    FrameType p = m_que.front();
    m_que.pop();
    return p;
}

std::size_t AudioQueue::size() const
{
    std::unique_lock<std::mutex> lock(m_que_mtx);
    return m_que.size();
}

bool AudioQueue::empty() const
{
    std::unique_lock<std::mutex> lock(m_que_mtx);
    return m_que.empty();
}
