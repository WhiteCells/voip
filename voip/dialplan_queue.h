#ifndef _DIALPLAN_QUE_H_
#define _DIALPLAN_QUE_H_

#include <queue>
#include <mutex>
#include <string>
#include <condition_variable>
#include <atomic>

/**
 * @brief 拨号计划队列
 * 线程安全
 */
class DialPlanQueue
{
public:
    DialPlanQueue();
    ~DialPlanQueue();

    void addDialPlan(const std::pair<int, std::string> &dialplan);
    std::pair<int, std::string> getDialPlan();
    void releaseDialPlan(const std::pair<int, std::string> &dialplan);

    std::size_t size() const;
    bool empty() const;

private:
    void fetchDialPlan();

private:
    std::queue<std::pair<int, std::string>> m_que;
    mutable std::mutex m_que_mtx;
    std::condition_variable m_que_cv;
    std::atomic_bool m_fetching;
};

#endif // _DIALPLAN_QUE_H_