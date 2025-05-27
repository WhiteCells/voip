#include "dialplan_queue.h"
#include "request.hpp"

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
    {
        std::unique_lock<std::mutex> lock {m_que_mtx};
        m_que.push(dialplan);
    }
    m_que_cv.notify_one();
}

std::string DialPlanQueue::getDialPlan()
{
    std::unique_lock<std::mutex> lock(m_que_mtx);

    // while (m_que.empty()) {
    //     if (!m_fetching.exchange(true)) {
    //         // 当前线程负责拉取
    //         lock.unlock();
    //         fetchDialPlan(); // 拉取结束时自动设置 fetching = false
    //         lock.lock();
    //     }
    //     else {
    //         // 其他线程等待
    //         m_que_cv.wait(lock, [this]() {
    //             return !m_que.empty();
    //         });
    //     }
    // }

    m_que_cv.wait(lock, [this]() {
        // if (m_que.empty()) {
        //     fetchDialPlan();
        //     return false;
        // }
        return !m_que.empty();
    });
    // if (m_que.empty()) {
    //     return "";
    // }

    auto dialplan = m_que.front();
    m_que.pop();
    return dialplan;
}

void DialPlanQueue::releaseDialPlan(const std::string &dialplan)
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
    std::vector<std::string> plans;
    voip::pullDialplan(plans, g_client_id);

    {
        std::unique_lock<std::mutex> lock(m_que_mtx);
        for (const auto &plan : plans) {
            m_que.push(plan);
            LOG_INFO("=== push plan ===");
        }
    }

    m_fetching = false;
    m_que_cv.notify_all();
}
