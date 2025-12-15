#include "coordinator.h"
#include "logger.h"
#include "core/outcoming_call.h"

void Coordinator::notifyCallConfirmed(std::shared_ptr<OutcomingCall> winner)
{
    std::unique_lock<std::mutex> lock(m_mtx);
    // 确保唯一 winner
    if (!m_confirmed) {
        m_confirmed = true;
        m_winner_call = winner;
        m_confirmed_cv.notify_all();
        LOG_WARN("one call confirmed");
    }
}

void Coordinator::notifyCallDisconnected(std::shared_ptr<OutcomingCall> winner)
{
    // std::unique_lock<std::mutex> lock(m_mtx);
    // // 只有 winner 才可以通知通话结束
    // if (winner.get() == m_winner_caller.get()) {
    //     m_finished = true;
    //     m_disconnected_cv.notify_all();
    //     LOG_WARN("one call disconnected");
    // }
    std::unique_lock<std::mutex> lock(m_mtx);
    auto cur = m_winner_call.lock();
    // 如果有 winner（weak -> shared 成功），或者直接比较裸指针
    if (cur && winner.get() == cur.get()) {
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

bool Coordinator::waitForWinner(std::chrono::seconds timeout)
{
    std::unique_lock<std::mutex> lock(m_mtx);
    bool ok = m_confirmed_cv.wait_for(lock, timeout, [&]() {
        LOG_WARN("recv confirmed notify to check confirmed");
        return m_confirmed.load();
    });
    if (!ok) {
        LOG_WARN("wait for winner time out");
    }
    return ok;
}

void Coordinator::waitForCallFinished()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    m_disconnected_cv.wait(lock, [&]() {
        LOG_WARN("recv disconnected notify to check finished");
        return m_finished.load();
    });
}

bool Coordinator::waitForSingleCallConfirmed(std::chrono::seconds timeout)
{
    std::unique_lock<std::mutex> lock(m_mtx);
    bool ok = m_confirmed_cv.wait_for(lock, timeout, [&]() {
        LOG_WARN("wait for single call confirmed");
        return m_confirmed.load();
    });
    if (!ok) {
        LOG_WARN("wait for single confirmed timeout");
    }
    return ok;
}

void Coordinator::waitForSingleCallFinished()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    m_disconnected_cv.wait(lock, [&]() {
        LOG_WARN("wait for notify");
        return m_finished.load();
    });
}

bool Coordinator::isWinner(std::shared_ptr<OutcomingCall> winner) const
{
    auto cur = m_winner_call.lock();
    return cur && cur.get() == winner.get();
}

bool Coordinator::shouldAbort(std::shared_ptr<OutcomingCall> winner) const
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
    m_winner_call.reset();
}