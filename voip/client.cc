#include "client.h"
#include "request.hpp"
#include "thread_pool.h"
#include "global.h"
#include "logger.h"
#include "vaccount.h"

#include <boost/beast.hpp>
#include <json/json.h>

Client::Client(unsigned workers_num) :
    m_running(true),
    m_thread_pool(workers_num),
    m_caller_que(std::make_shared<CallerQueue>())
{
    voip::notify();
    voip::heartbeat();
    for (unsigned i = 0; i < workers_num /*todo*/; ++i) {
        m_thread_pool.addTask(std::bind(&Client::callTask, this));
    }
}

Client::~Client()
{
    m_running = false;
}

void Client::callTask()
{
    // auto caller = m_caller_vec->getCaller(i);
    while (m_running) {
        LOG_INFO("to get dualplan");
        auto caller = m_caller_que->getCaller();
        auto dialplan = m_dialplan_que.getDialPlan();
        LOG_INFO("tasking: {}", dialplan);
        caller->call(dialplan, g_client_id);
        std::this_thread::sleep_for(std::chrono::seconds(120));
    }
}
