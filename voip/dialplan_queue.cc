#include "dialplan_queue.h"

DialPlanQueue::DialPlanQueue()
{
}

DialPlanQueue::~DialPlanQueue()
{
}

void DialPlanQueue::addDialPlan(const std::string &dialplan)
{
    std::unique_lock<std::mutex> lock {m_que_mtx};
    m_que.push(dialplan);
    m_que_cv.notify_one();
}

std::string DialPlanQueue::getDialPlan()
{
    std::unique_lock<std::mutex> lock {m_que_mtx};
    m_que_cv.wait(lock, [this]() {
        return !m_que.empty();
    });
    auto dialplan = m_que.front();
    m_que.pop();
    return dialplan;
}

void DialPlanQueue::releaseDialPlan(const std::string &dialplan)
{
    std::unique_lock<std::mutex> lock {m_que_mtx};
    m_que.push(dialplan);
    m_que_cv.notify_one();
}