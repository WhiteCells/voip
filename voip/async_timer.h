#ifndef _ASYNC_TIMER_H_
#define _ASYNC_TIMER_H_

#include <boost/asio/steady_timer.hpp>
#include <functional>
#include <chrono>

namespace asio = boost::asio;

class AsyncTimer
{
public:
    using Task = std::function<void(void)>;

    AsyncTimer(asio::io_context &io_context, std::chrono::seconds interval);
    ~AsyncTimer();

    void start(Task task);
    void stop();
    void reset(std::chrono::seconds interval_sec);

private:
    void scheduleTask();

private:
    asio::steady_timer m_timer;
    std::chrono::seconds m_interval_sec;
    Task m_task;
    bool m_running;
};

#endif // _ASYNC_TIMER_H_