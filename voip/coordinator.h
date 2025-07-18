#ifndef _COORDINATOR_H_
#define _COORDINATOR_H_

#include "singleton.hpp"
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

class Coordinator : public Singleton<Coordinator>
{
public:
    friend class Singleton<Coordinator>;
    ~Coordinator() = default;

    void notifyCallConfirmed();
    void notifyCallDisconnected();

    void waitForWinner();
    void waitForCallFinished();

    bool isWinner() const;
    bool shouldAbort() const;
    std::thread::id getThreadId() const;

    void reset();

private:
    mutable std::mutex m_mtx;
    std::condition_variable m_confirmed_cv;
    std::condition_variable m_disconnected_cv;
    std::atomic<bool> m_confirmed = false;
    std::atomic<bool> m_finished = false;
    std::thread::id m_winner_tid;
};

#endif // _COORDINATOR_H_