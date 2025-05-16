#ifndef _CALLPOOL_H_
#define _CALLPOOL_H_

#include "caller.h"
#include "vaccount.h"

#include <boost/asio.hpp>
#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>

class CallerQueue
{
    using CallerUPtr = std::unique_ptr<voip::Caller>;
    using AccountUPtr = std::unique_ptr<voip::VAccount>;
    // using AsyncTimerSPtr = std::shared_ptr<AsyncTimer>;

public:
    CallerQueue();
    ~CallerQueue();

    void addCaller(CallerUPtr caller);
    CallerUPtr getCaller();
    void releaseCaller(CallerUPtr vcall);

private:
    std::queue<CallerUPtr> m_que;
    std::mutex m_que_mtx;
    std::condition_variable m_que_cond;
};

#endif // _CALLPOOL_H_