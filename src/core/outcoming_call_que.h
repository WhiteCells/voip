#pragma once

#include "outcoming_call.h"
#include <boost/asio.hpp>
#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>

// 外呼队列
class OutcomingCallQue
{
    using CallerSPtr = std::shared_ptr<OutcomingCall>;

public:
    OutcomingCallQue()
    {
    }
    OutcomingCallQue(const OutcomingCallQue &) = delete;
    OutcomingCallQue &operator=(const OutcomingCallQue &) = delete;
    ~OutcomingCallQue()
    {
    }

    void addCaller(CallerSPtr caller);
    CallerSPtr getCaller();
    void releaseCaller(CallerSPtr vcall);

    std::size_t size() const;
    bool empty() const;

private:
    std::queue<CallerSPtr> m_que;
    mutable std::mutex m_que_mtx;
    std::condition_variable m_que_cv;
    std::atomic_bool m_fetching;
    // std::vector<std::shared_ptr<voip::VAccount>> m_acc;
};
