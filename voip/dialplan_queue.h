#ifndef _DIALPLAN_QUE_H_
#define _DIALPLAN_QUE_H_

#include <queue>
#include <mutex>
#include <string>
#include <condition_variable>
#include <atomic>

class DialPlanQueue
{
public:
    DialPlanQueue();
    ~DialPlanQueue();

    void addDialPlan(const std::string &dialplan);
    std::string getDialPlan();
    void releaseDialPlan(const std::string &dialplan);

    std::size_t size() const;
    bool empty() const;

// private:
    void fetchDialPlan();

private:
    std::queue<std::string> m_que;
    mutable std::mutex m_que_mtx;
    std::condition_variable m_que_cv;
    std::atomic<bool> m_fetching;
};

#endif // _DIALPLAN_QUE_H_