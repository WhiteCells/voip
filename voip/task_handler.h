#ifndef _TASK_HANDLER_H_
#define _TASK_HANDLER_H_

#include "caller_queue.h"
#include "dialplan_queue.h"

#include <memory>

class TaskHandler
{
public:
    TaskHandler(std::shared_ptr<CallerQueue> caller_que,
                std::shared_ptr<DialPlanQueue> dialplan_que);
    ~TaskHandler();

    void operator()();

private:
    std::shared_ptr<CallerQueue> m_caller_que;
    std::shared_ptr<DialPlanQueue> m_dialplan_que;
};

#endif // _TASK_HANDLER_H_