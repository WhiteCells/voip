#include "coordinator.h"
#include "logger.h"
#include "caller.h"

void Coordinator::notifyCallConfirmed(std::shared_ptr<voip::Caller> winner)
{
    std::unique_lock<std::mutex> lock(m_mtx);
    if (!m_confirmed) {
        m_confirmed = true;
        // m_winner_tid = getThreadId();
        m_winner_caller = winner;
        m_confirmed_cv.notify_all();
        LOG_WARN("one call confirmed");
    }
}

void Coordinator::notifyCallDisconnected(std::shared_ptr<voip::Caller> winner)
{
    std::unique_lock<std::mutex> lock(m_mtx);
    if (winner == m_winner_caller) {
        m_finished = true;
        m_disconnected_cv.notify_all();
        LOG_WARN("one call disconnected");
    }
}

void Coordinator::waitForWinner()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    m_confirmed_cv.wait(lock, [&]() {
        LOG_WARN("recv confirmed notify to check confirmed");
        return m_confirmed.load();
    });
}

void Coordinator::waitForCallFinished()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    m_disconnected_cv.wait(lock, [&]() {
        LOG_WARN("recv disconnected notify to check finished");
        return m_finished.load();
    });
}

bool Coordinator::isWinner(std::shared_ptr<voip::Caller> winner) const
{
    // return getThreadId() == m_winner_tid;
    return m_winner_caller == winner;
}

bool Coordinator::shouldAbort(std::shared_ptr<voip::Caller> winner) const
{
    bool res = m_confirmed && !isWinner(winner);
    LOG_WARN("should Abort: {}", res);
    return res;
}

std::thread::id Coordinator::getThreadId() const
{
    return std::this_thread::get_id();
}

void Coordinator::reset_()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    m_confirmed = false;
    m_finished = false;
    // m_winner_tid = getThreadId();
    m_winner_caller.reset();
}