#pragma once

#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>

class PCMQueue
{
public:
    void push(const std::vector<char> &data)
    {
        {
            std::lock_guard<std::mutex> lock(m_pcm_que_mtx);
            m_pcm_que.push(data);
        }
        m_pcm_que_cv.notify_one();
    }

    std::vector<char> try_pop()
    {
        std::lock_guard<std::mutex> lock(m_pcm_que_mtx);
        if (m_pcm_que.empty()) {
            return {};
        }

        auto data = m_pcm_que.front();
        m_pcm_que.pop();
        return data;
    }

    std::vector<char> wait_and_pop()
    {
        std::unique_lock<std::mutex> lock(m_pcm_que_mtx);
        m_pcm_que_cv.wait(lock, [this] {
            return !m_pcm_que.empty();
        });

        auto data = m_pcm_que.front();
        m_pcm_que.pop();
        return data;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(m_pcm_que_mtx);
        std::queue<std::vector<char>> empty;
        std::swap(m_pcm_que, empty);
    }

    bool empty()
    {
        std::lock_guard<std::mutex> lock(m_pcm_que_mtx);
        return m_pcm_que.empty();
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock(m_pcm_que_mtx);
        return m_pcm_que.size();
    }

private:
    std::queue<std::vector<char>> m_pcm_que;
    std::mutex m_pcm_que_mtx;
    std::condition_variable m_pcm_que_cv;
};
