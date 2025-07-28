#ifndef _COORDINATOR_H_
#define _COORDINATOR_H_

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

namespace voip {
class Caller;
}

class Coordinator
{
public:
    Coordinator() = default;
    ~Coordinator() = default;

    void notifyCallConfirmed(std::shared_ptr<voip::Caller> winner);
    void notifyCallDisconnected(std::shared_ptr<voip::Caller> winner);

    void waitForWinner();
    void waitForCallFinished();

    bool isWinner(std::shared_ptr<voip::Caller> winner) const;
    bool shouldAbort(std::shared_ptr<voip::Caller> winner) const;
    std::thread::id getThreadId() const;

    void reset_();

private:
    mutable std::mutex m_mtx;
    std::condition_variable m_confirmed_cv;
    std::condition_variable m_disconnected_cv;
    std::atomic<bool> m_confirmed = false;
    std::atomic<bool> m_finished = false;
    // std::thread::id m_winner_tid;
    std::shared_ptr<voip::Caller> m_winner_caller;
};

#endif // _COORDINATOR_H_