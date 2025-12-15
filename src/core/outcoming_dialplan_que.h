#pragma once

#include <queue>
#include <mutex>
#include <string>
#include <condition_variable>

// 外呼拨号队列
class OutcomingDialPlanQue
{
public:
    OutcomingDialPlanQue()
    {
    }
    ~OutcomingDialPlanQue()
    {
    }

    void addDialPlan(const std::string &dialplan)
    {
        std::lock_guard<std::mutex> lock(m_que_mtx);
        m_que.push(dialplan);
        m_que_cv.notify_one();
    }

    std::string getDialPlan()
    {
        std::unique_lock<std::mutex> lock(m_que_mtx);
        m_que_cv.wait(lock, [this] {
            return !m_que.empty();
        });
        auto dialplan = m_que.front();
        m_que.pop();
        return dialplan;
    }

    void releaseDialPlan(const std::string &dialplan)
    {
        std::lock_guard<std::mutex> lock(m_que_mtx);
        m_que.push(dialplan);
        m_que_cv.notify_one();
    }

    std::size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_que_mtx);
        return m_que.size();
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_que_mtx);
        return m_que.empty();
    }

private:
    std::queue<std::string> m_que;
    mutable std::mutex m_que_mtx;
    std::condition_variable m_que_cv;
};
