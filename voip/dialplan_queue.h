#ifndef _DIALPLAN_QUE_H_
#define _DIALPLAN_QUE_H_

#include <queue>
#include <mutex>
#include <string>
#include <functional>
#include <condition_variable>
#include <atomic>

class DialPlanQueue
{
    using FetchFunc = std::function<bool(std::vector<std::string> &)>;

public:
    DialPlanQueue();
    ~DialPlanQueue();

    void addDialPlan(const std::string &dialplan);
    std::string getDialPlan();
    void releaseDialPlan(const std::string &dialplan);

    unsigned size() const { return m_que.size(); }

private:
    void fetchDialPlan();

private:
    std::queue<std::string> m_que;
    std::mutex m_que_mtx;
    std::condition_variable m_que_cv;
    std::atomic<bool> m_fetching;
    FetchFunc m_fetch_func;
};

#endif // _DIALPLAN_QUE_H_