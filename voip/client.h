#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "thread_pool.h"
#include "account_queue.h"
#include "caller_queue.h"
#include "dialplan_queue.h"

#include <string>

class Client
{
public:
    Client();
    ~Client();

private:
    void heartbeat();
    void notify();
    void uploadFile();
    void uploadStatus();

    ThreadPool thread_pool;
    // VAccountQueue vacc_que;
    CallerQueue caller_que;
    DialPlanQueue dialplan_que;
    std::string m_client_id;
};

#endif // _CLIENT_H_