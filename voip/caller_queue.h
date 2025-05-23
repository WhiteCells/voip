#ifndef _CALLPOOL_H_
#define _CALLPOOL_H_

#include "caller.h"

#include <boost/asio.hpp>
#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>

namespace voip {
class VAccount;
}

class CallerQueue
{
    using CallerSPtr = std::shared_ptr<voip::Caller>;
    using AccountUPtr = std::unique_ptr<voip::VAccount>;

public:
    CallerQueue();
    ~CallerQueue();

    void addCaller(CallerSPtr caller);
    CallerSPtr getCaller();
    void releaseCaller(CallerSPtr vcall);

    unsigned size() const { return m_que.size(); }

private:
    std::queue<CallerSPtr> m_que;
    std::mutex m_que_mtx;
    std::condition_variable m_que_cv;
};

#endif // _CALLPOOL_H_