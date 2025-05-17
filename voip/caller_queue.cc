#include "caller_queue.h"

#include <pjsua2.hpp>

CallerQueue::CallerQueue()
{
}

CallerQueue::~CallerQueue()
{
}

void CallerQueue::addCaller(CallerUPtr caller)
{
    std::unique_lock<std::mutex> lock(m_que_mtx);
    m_que.push(std::move(caller));
    m_que_cv.notify_one();
}

CallerQueue::CallerUPtr CallerQueue::getCaller()
{
    std::unique_lock<std::mutex> lock(m_que_mtx);
    m_que_cv.wait(lock, [this]() {
        return !m_que.empty();
    });
    auto vcall = std::move(m_que.front());
    m_que.pop();
    return vcall;
}

void CallerQueue::releaseCaller(CallerUPtr caller)
{
    std::unique_lock<std::mutex> lock(m_que_mtx);
    m_que.push(std::move(caller));
    m_que_cv.notify_one();
}
