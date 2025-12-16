#pragma once

#include "outcoming_call.h"
#include <boost/asio.hpp>
#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>

// 外呼队列
class OutcomingCallQue
{
    using CallerSPtr = std::shared_ptr<OutcomingCall>;

public:
    OutcomingCallQue()
    {
    }
    OutcomingCallQue(const OutcomingCallQue &) = delete;
    OutcomingCallQue &operator=(const OutcomingCallQue &) = delete;
    ~OutcomingCallQue()
    {
    }

    void addCaller(CallerSPtr caller)
    {
        std::lock_guard<std::mutex> lock(m_que_mtx);
        m_que.push(caller);
        m_que_cv.notify_one();
    }

    CallerSPtr getCaller()
    {
        std::unique_lock<std::mutex> lock(m_que_mtx);
        m_que_cv.wait(lock, [this]() {
            return !m_que.empty();
        });
        auto caller = m_que.front();
        m_que.pop();
        return caller;
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
    std::queue<CallerSPtr> m_que;
    mutable std::mutex m_que_mtx;
    std::condition_variable m_que_cv;
    std::atomic_bool m_fetching;
};
