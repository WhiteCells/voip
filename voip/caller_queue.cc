#include "caller_queue.h"
#include "vaccount.h"
#include "logger.h"
#include "request.hpp"

#include <pjsua2.hpp>

CallerQueue::CallerQueue() :
    m_fetching(false)
{
}

CallerQueue::~CallerQueue()
{
    m_fetching = false;
}

void CallerQueue::addCaller(CallerSPtr caller)
{
    {
        std::unique_lock<std::mutex> lock(m_que_mtx);
        m_que.push(caller);
    }
    m_que_cv.notify_one();
}

CallerQueue::CallerSPtr CallerQueue::getCaller()
{
    std::unique_lock<std::mutex> lock(m_que_mtx);

    while (m_que.empty()) {
        bool expected_is_fetching = false;
        if (m_fetching.compare_exchange_strong(expected_is_fetching, true)) {
            // 当前线程负责拉取
            lock.unlock();
            LOG_INFO("init fetch");
            fetchCaller();
            LOG_INFO("re-acquired lock after fetch attempt. Queue empty: {}", m_que.empty());
            lock.lock();
        }
        else {
            // 其他线程
            LOG_INFO("waiting as another fetch is in progress. Queue empty: {}", m_que.empty());
            m_que_cv.wait(lock, [this]() {
                return !m_que.empty();
            });
            LOG_INFO("Fetching: {}", m_fetching.load());
        }
        // todo 线程拉取账号为空时需要等待
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    auto vcall = std::move(m_que.front());
    m_que.pop();
    return vcall;
}

void CallerQueue::releaseCaller(CallerSPtr caller)
{
    {
        std::unique_lock<std::mutex> lock(m_que_mtx);
        m_que.push(caller);
        LOG_INFO("release Caller");
    }
    m_que_cv.notify_one();
}

std::size_t CallerQueue::size() const
{
    std::unique_lock<std::mutex> lock(m_que_mtx);
    return m_que.size();
}

bool CallerQueue::empty() const
{
    std::unique_lock<std::mutex> lock(m_que_mtx);
    return m_que.empty();
}

void CallerQueue::fetchCaller()
{
    try {
        std::vector<std::vector<std::string>> accounts;
        voip::pullAccount(accounts, g_client_id);

        {
            std::unique_lock<std::mutex> lock(m_que_mtx);
            for (const auto &acc : accounts) {
                std::string id = acc[0];
                std::string user = acc[1];
                std::string pass = acc[2];
                std::string host = acc[3];
                auto vaccount = std::make_shared<voip::VAccount>(id, user, pass, host);
                m_acc.push_back(vaccount);
                auto caller = std::make_shared<voip::Caller>(*vaccount);
                m_que.push(caller);
                LOG_INFO("=== pull caller: {}:{}:{} ===", user, pass, host);
            }
            m_fetching.store(false);
            LOG_INFO("m_fetching set to false, Caller Queue size now: {}", m_que.size());
        }

        m_que_cv.notify_all();
    }
    catch (const std::exception &e) {
        LOG_ERROR("{}", e.what());
    }
}