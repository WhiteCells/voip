#include "dialplan_queue.h"
#include "logger.h"

DialPlanQueue::DialPlanQueue()
{
}

DialPlanQueue::~DialPlanQueue()
{
}

void DialPlanQueue::addDialPlan(const std::pair<int, std::string> &dialplan)
{
    {
        std::unique_lock<std::mutex> lock {m_que_mtx};
        m_que.push(dialplan);
    }
    m_que_cv.notify_one();
}

std::pair<int, std::string> DialPlanQueue::getDialPlan()
{
    std::unique_lock<std::mutex> lock(m_que_mtx);
    m_que_cv.wait(lock, [this]() {
        return !m_que.empty();
    });
    auto dialplan = m_que.front();
    m_que.pop();
    LOG_INFO("popped: {} {}", dialplan.first, dialplan.second);
    return dialplan;
}

void DialPlanQueue::releaseDialPlan(const std::pair<int, std::string> &dialplan)
{
    {
        std::unique_lock<std::mutex> lock {m_que_mtx};
        m_que.push(dialplan);
    }
    m_que_cv.notify_one();
}

std::size_t DialPlanQueue::size() const
{
    std::unique_lock<std::mutex> lock {m_que_mtx};
    return m_que.size();
}

bool DialPlanQueue::empty() const
{
    std::unique_lock<std::mutex> lock {m_que_mtx};
    return m_que.empty();
}
