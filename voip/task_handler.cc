#include "task_handler.h"

#include <iostream>

TaskHandler::TaskHandler(std::shared_ptr<CallerQueue> caller_que,
                         std::shared_ptr<DialPlanQueue> dialplan_que) :
    m_caller_que(caller_que),
    m_dialplan_que(dialplan_que)
{
}

TaskHandler::~TaskHandler()
{
}

void TaskHandler::operator()()
{
    while (true) {
        auto caller = m_caller_que->getCaller();
        auto dialplan = m_dialplan_que->getDialPlan();
        caller->call(dialplan);
        std::cout << "caller: " << dialplan << std::endl;
    }
}
