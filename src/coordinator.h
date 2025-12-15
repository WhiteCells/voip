#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <chrono>

class OutcomingCall;

class Coordinator
{
public:
    Coordinator() = default;
    ~Coordinator() = default;

    void notifyCallConfirmed(std::shared_ptr<OutcomingCall> winner);
    void notifyCallDisconnected(std::shared_ptr<OutcomingCall> winner);

    void waitForWinner();
    bool waitForWinner(std::chrono::seconds timeout);
    void waitForCallFinished();

    bool waitForSingleCallConfirmed(std::chrono::seconds timeout);
    void waitForSingleCallFinished();

    bool isWinner(std::shared_ptr<OutcomingCall> winner) const;
    bool shouldAbort(std::shared_ptr<OutcomingCall> winner) const;
    std::thread::id getThreadId() const;

    void reset_();

private:
    mutable std::mutex m_mtx;
    std::condition_variable m_confirmed_cv;
    std::condition_variable m_disconnected_cv;
    std::atomic<bool> m_confirmed = false;
    std::atomic<bool> m_finished = false;
    std::weak_ptr<OutcomingCall> m_winner_call;
};
