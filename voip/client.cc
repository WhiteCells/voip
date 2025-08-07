#include "client.h"
#include "request.hpp"
#include "thread_pool.h"
#include "global.h"
#include "logger.h"
#include "coordinator.h"
#include <boost/beast.hpp>
#include <json/json.h>

Client::Client(unsigned workers_num) :
    m_running(true),
    m_thread_pool(workers_num),
    m_caller_que(std::make_shared<CallerQueue>()),
    m_workers_num(workers_num)
{
    voip::notify();
    voip::heartbeat();
    callBatch();
}

Client::~Client()
{
    m_running = false;
    m_cv.notify_all();
}

void Client::callBatch()
{
    while (m_running) {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_batch_remain = m_workers_num;
        auto coordinator = std::make_shared<Coordinator>();

        for (std::size_t i = 0; i < m_workers_num; ++i) {
            m_thread_pool.addTask([this, i, coordinator]() {
                callTask(i, coordinator);
                if (--m_batch_remain == 0) {
                    LOG_INFO("m_batch_remain --");
                    // std::lock_guard<std::mutex> cv_lock(m_mtx);
                    m_cv.notify_one();
                }
            });
        }

        m_cv.wait(lock, [this]() {
            LOG_INFO("m_cv wait lock");
            return m_batch_remain == 0;
        });

        LOG_INFO("batch finish");
    }
}

// void Client::callTask(std::size_t i, std::shared_ptr<Coordinator> coordinator)
// {
//     std::cout << i << std::endl;
//     auto caller = m_caller_que->getCaller();
//     auto dialplan = m_dialplan_que.getDialPlan();
//     caller->call(dialplan.second, g_client_id, dialplan.first, coordinator);
// }
