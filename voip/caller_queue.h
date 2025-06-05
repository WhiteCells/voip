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

/**
 * @brief 呼叫者队列
 * 线程安全
 */
class CallerQueue
{
    using CallerSPtr = std::shared_ptr<voip::Caller>;
    using AccountUPtr = std::unique_ptr<voip::VAccount>;

public:
    CallerQueue();
    CallerQueue(const CallerQueue &) = delete;
    CallerQueue &operator=(const CallerQueue &) = delete;
    ~CallerQueue();

    void addCaller(CallerSPtr caller);
    CallerSPtr getCaller();
    void releaseCaller(CallerSPtr vcall);

    std::size_t size() const;
    bool empty() const;

private:
    void fetchCaller();

private:
    std::queue<CallerSPtr> m_que;
    mutable std::mutex m_que_mtx;
    std::condition_variable m_que_cv;
    std::atomic_bool m_fetching;
    std::vector<std::shared_ptr<voip::VAccount>> m_acc;
};

#endif // _CALLPOOL_H_