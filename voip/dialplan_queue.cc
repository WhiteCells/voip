#include "dialplan_queue.h"
#include "request.hpp"
#include "global.h"

DialPlanQueue::DialPlanQueue() :
    m_fetching(false)
{
}

DialPlanQueue::~DialPlanQueue()
{
    m_fetching = false;
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

void DialPlanQueue::fetchDialPlan()
{
    try {
        std::vector<std::pair<int, std::string>> plans;
        voip::pullDialplan(plans, g_client_id);

        {
            std::unique_lock<std::mutex> lock(m_que_mtx);
            for (const auto &plan : plans) {
                m_que.push(plan);
                LOG_INFO("=== push plan: {} ===", plan.second);
            }
            m_fetching.store(false);
            LOG_INFO("m_fetching set to false. Queue size now: {}", m_que.size());
        }

        m_que_cv.notify_all();
    }
    catch (const std::exception &e) {
        LOG_ERROR("{}", e.what());
    }
}
