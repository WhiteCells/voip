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
    m_dialplan_que.push(dialplan);
}

std::string DialPlanQueue::getDialPlan()
{
    std::unique_lock<std::mutex> lock {m_que_mtx};
    if (m_dialplan_que.empty()) {
        return "";
    }
    std::string dialplan = m_dialplan_que.front();
    m_dialplan_que.pop();
    return dialplan;
}

void DialPlanQueue::releaseDialPlan(const std::string &dialplan)
{
    std::unique_lock<std::mutex> lock {m_que_mtx};
    m_dialplan_que.push(dialplan);
}