#include "dialplan_queue.h"

DialPlanQueue::DialPlanQueue() :
    m_fetching(false)
{
}

DialPlanQueue::~DialPlanQueue()
{
    m_fetching = false;
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
    // while (m_que.empty()) {
    //     if (!m_fetching.exchange(true)) {
    //         std::thread(&DialPlanQueue::fetchDialPlan, this).detach();
    //     }
    //     m_que_cv.wait(lock, [this]() {
    //         return !m_que.empty();
    //     });
    // }
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

void DialPlanQueue::fetchDialPlan()
{
    // std::vector<
}