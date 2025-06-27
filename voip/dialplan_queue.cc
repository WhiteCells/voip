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

    while (m_que.empty()) {
        bool expected_is_fetching = false;
        if (m_fetching.compare_exchange_strong(expected_is_fetching, true)) {
            // 当前线程负责拉取
            lock.unlock();
            LOG_INFO("initiating fetch");
            fetchDialPlan(); // 拉取结束时自动设置 fetching = false
            LOG_INFO("re-acquired lock after fetch attempt. Queue empty: {}", m_que.empty());
            lock.lock();
        }
        else {
            // 其他线程
            LOG_INFO("waiting as another fetch is in progress. Queue empty: {}", m_que.empty());
            m_que_cv.wait(lock, [this]() {
                return !m_que.empty();
            });
            LOG_INFO("Fetching: {}", m_fetching.load());
        }
        // todo 线程拉取拨号计划为空时需要等待
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

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
