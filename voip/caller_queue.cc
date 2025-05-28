#include "caller_queue.h"
#include "vaccount.h"
#include "logger.h"

#include <pjsua2.hpp>

CallerQueue::CallerQueue()
{
}

CallerQueue::~CallerQueue()
{
}

void CallerQueue::addCaller(CallerSPtr caller)
{
    {
        std::unique_lock<std::mutex> lock(m_que_mtx);
        m_que.push(caller);
    }
    m_que_cv.notify_one();
}

CallerQueue::CallerSPtr CallerQueue::getCaller()
{
    std::unique_lock<std::mutex> lock(m_que_mtx);
    m_que_cv.wait(lock, [this]() {
        return !m_que.empty();
    });
    auto vcall = std::move(m_que.front());
    m_que.pop();
    return vcall;
}

void CallerQueue::releaseCaller(CallerSPtr caller)
{
    {
        std::unique_lock<std::mutex> lock(m_que_mtx);
        m_que.push(caller);
        LOG_INFO("release Caller");
    }
    m_que_cv.notify_one();
}

std::size_t CallerQueue::size() const
{
    std::unique_lock<std::mutex> lock(m_que_mtx);
    return m_que.size();
}

bool CallerQueue::empty() const
{
    std::unique_lock<std::mutex> lock(m_que_mtx);
    return m_que.empty();
}
