#ifndef _CALLPOOL_H_
#define _CALLPOOL_H_

#include "vcall.h"
#include "vaccount.h"

#include <boost/asio.hpp>
#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>

namespace asio = boost::asio;

class AsyncTimer;

class CallerQueue
{
    using VCallUPtr = std::unique_ptr<voip::VCall>;
    using VAccountUPtr = std::unique_ptr<voip::VAccount>;
    // using AsyncTimerSPtr = std::shared_ptr<AsyncTimer>;

public:
    CallerQueue(asio::io_context &ioc);
    ~CallerQueue();

    void addVCall(VCallUPtr vcall);
    VCallUPtr getVCall();
    void recycleVCall(VCallUPtr vcall);

private:
    void fetch();
    VAccountUPtr createAccount(
        const std::string &sip_user,
        const std::string &sip_domain,
        const std::string &sip_password);
    VCallUPtr createCall(VAccountUPtr vaccount);
    void initEndpoint();

private:
    asio::io_context &m_ioc;
    std::queue<VCallUPtr> m_que;
    std::mutex m_que_mtx;
    std::condition_variable m_que_cond;
    std::shared_ptr<AsyncTimer> m_fetch_timer;
};

#endif // _CALLPOOL_H_