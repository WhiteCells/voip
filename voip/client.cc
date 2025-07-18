#include "client.h"
#include "request.hpp"
#include "thread_pool.h"
#include "global.h"
#include "logger.h"
#include "vaccount.h"
#include "coordinator.h"

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
    while (m_running) {
        LOG_INFO("call task");
        auto coordinator = Coordinator::getInstance();
        auto caller = m_caller_que->getCaller();
        auto dialplan = m_dialplan_que.getDialPlan();
        LOG_INFO("tasking: {} {}", dialplan.first, dialplan.second);
        caller->call(dialplan.second, g_client_id, dialplan.first);

        coordinator->waitForWinner();

        if (coordinator->shouldAbort()) {
            LOG_INFO("");
            caller->hangup_();
            continue;
        }

        coordinator->waitForCallFinished();

        // std::this_thread::sleep_for(std::chrono::seconds(1200));
        // std::this_thread::sleep_for(std::chrono::seconds(120));
    }
}

void Client::batchTask()
{
    // auto caller = m_caller_que.get();
}