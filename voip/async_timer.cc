#include "async_timer.h"

AsyncTimer::AsyncTimer(asio::io_context &ioc, std::chrono::seconds interval_sec) :
    m_timer(ioc),
    m_interval_sec(interval_sec),
    m_running(false)
{
}

AsyncTimer::~AsyncTimer()
{
}

void AsyncTimer::start(Task task)
{
    if (m_running) {
        return;
    }

    m_task = std::move(task);
    m_running = true;

    // Run once
    // if (task_) {
    //     task_();
    // }

    scheduleTask();
}

void AsyncTimer::stop()
{
    m_running = false;
    m_timer.cancel();
}

void AsyncTimer::reset(std::chrono::seconds interval_sec)
{
    m_interval_sec = interval_sec;
    if (m_running) {
        stop();
        start(m_task);
    }
}

void AsyncTimer::scheduleTask()
{
    m_timer.expires_after(m_interval_sec);
    m_timer.async_wait([this](const boost::system::error_code &ec) {
        if (!ec && m_running) {
            if (m_task) {
                m_task();
            }
            scheduleTask();
        }
    });
}
