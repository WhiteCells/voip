#include "coordinator.h"
#include "logger.h"

void Coordinator::notifyCallConfirmed()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    if (!m_confirmed) {
        m_confirmed = true;
        m_winner_tid = getThreadId();
        m_confirmed_cv.notify_all();
        LOG_INFO("one call confirmed");
    }
}

void Coordinator::notifyCallDisconnected()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    m_finished = true;
    m_disconnected_cv.notify_all();
    LOG_INFO("one call disconnected");
}

void Coordinator::waitForWinner()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    m_confirmed_cv.wait(lock, [&]() {
        LOG_INFO("recv confirmed notify to check confirmed");
        return m_confirmed.load();
    });
}

void Coordinator::waitForCallFinished()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    m_disconnected_cv.wait(lock, [&]() {
        LOG_INFO("recv disconnected notify to check finished");
        return m_finished.load();
    });
}

bool Coordinator::isWinner() const
{
    return getThreadId() == m_winner_tid;
}

bool Coordinator::shouldAbort() const
{
    bool res = m_confirmed && !isWinner();
    LOG_INFO("should Abort: {}", res);
    return res;
}

std::thread::id Coordinator::getThreadId() const
{
    return std::this_thread::get_id();
}

void Coordinator::reset()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    m_confirmed = false;
    m_finished = false;
    m_winner_tid = getThreadId();
}