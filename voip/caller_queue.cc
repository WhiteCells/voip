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
    m_que_cv.wait(lock, [this]() {
        return !m_que.empty();
    });
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