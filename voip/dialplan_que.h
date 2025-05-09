#ifndef _DIALPLAN_QUE_H_
#define _DIALPLAN_QUE_H_

#include <queue>
#include <string>

class DialPlanQue
{
public:
    DialPlanQue();
    ~DialPlanQue();

    void addDialPlan();
    std::string &getDialPlan();

private:
    std::queue<std::string> m_dialplan_que;
};

#endif // _DIALPLAN_QUE_H_