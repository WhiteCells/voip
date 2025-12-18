#pragma once

#include <queue>
#include <condition_variable>
#include <mutex>

// 音频队列
// 后续改为无阻塞队列
class PCMQue
{
public:
    PCMQue() = default;

    ~PCMQue() = default;

    void push(const std::vector<char> &pcm)
    {
        std::unique_lock<std::mutex> lck(m_mtx);
        m_que.push(pcm);
        m_cv.notify_one();
    }

    std::vector<char> pop()
    {
        std::unique_lock<std::mutex> lck(m_mtx);
        m_cv.wait(lck, [this]() {
            return !m_que.empty();
        });
        auto pcm = m_que.front();
        m_que.pop();
        return pcm;
    }

    std::vector<char> try_pop()
    {
        std::unique_lock<std::mutex> lck(m_mtx);
        if (m_que.empty()) {
            return {};
        }
        auto pcm = m_que.front();
        m_que.pop();
        return pcm;
    }

    void clear()
    {
        std::unique_lock<std::mutex> lck(m_mtx);
        m_que = std::queue<std::vector<char>>();
    }

    bool empty()
    {
        std::unique_lock<std::mutex> lck(m_mtx);
        return m_que.empty();
    }

private:
    std::queue<std::vector<char>> m_que;
    std::mutex m_mtx;
    std::condition_variable m_cv;
};