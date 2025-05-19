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

public:
    CallerQueue();
    ~CallerQueue();

    void addCaller(CallerUPtr caller);
    CallerUPtr getCaller();
    void releaseCaller(CallerUPtr vcall);

    unsigned size() const { return m_que.size(); }

private:
    std::queue<CallerUPtr> m_que;
    std::mutex m_que_mtx;
    std::condition_variable m_que_cv;
};

#endif // _CALLPOOL_H_