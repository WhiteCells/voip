#ifndef _DIALPLAN_QUE_H_
#define _DIALPLAN_QUE_H_

#include <queue>
#include <mutex>
#include <string>

class DialPlanQueue
{
public:
    DialPlanQueue();
    ~DialPlanQueue();

    void addDialPlan(const std::string &dialplan);
    std::string getDialPlan();
    void releaseDialPlan(const std::string &dialplan);

private:
    std::queue<std::string> m_dialplan_que;
    std::mutex m_que_mtx;
};

#endif // _DIALPLAN_QUE_H_