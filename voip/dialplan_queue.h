#ifndef _DIALPLAN_QUE_H_
#define _DIALPLAN_QUE_H_

#include <queue>
#include <mutex>
#include <string>
#include <condition_variable>

class DialPlanQueue
{
public:
    DialPlanQueue();
    ~DialPlanQueue();

    void addDialPlan(const std::string &dialplan);
    std::string getDialPlan();
    void releaseDialPlan(const std::string &dialplan);

private:
    std::queue<std::string> m_que;
    std::mutex m_que_mtx;
    std::condition_variable m_que_cv;
};

#endif // _DIALPLAN_QUE_H_