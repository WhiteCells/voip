#ifndef _ASYNC_TIMER_H_
#define _ASYNC_TIMER_H_

#include <boost/asio/steady_timer.hpp>
#include <functional>
#include <chrono>
#include <atomic>

namespace asio = boost::asio;

/**
 * @brief 异步定时器
 *
 */
class AsyncTimer
{
    using Task = std::function<void(void)>;

public:
    /**
     * @brief Construct a new Async Timer:: Async Timer object
     *
     * @param ioc asio io context
     * @param interval 定时时间(秒)
     */
    AsyncTimer(asio::io_context &io_context, std::chrono::seconds interval);
    AsyncTimer(const AsyncTimer &) = delete;
    AsyncTimer &operator=(const AsyncTimer &) = delete;
    ~AsyncTimer();

public:
    /**
     * @brief 启动定时器
     * @param task 定时任务
     */
    void start(Task task);

    /**
     * @brief 停止定时器
     */
    void stop();

    /**
     * @brief 重置定时器时间，并重新启动
     * @param interval 定时时间
     */
    void reset(std::chrono::seconds interval);

private:
    /**
     * @brief 安排下一次定时任务的执行，执行绑定的定时任务
     * 使用 boost::asio::steady_timer 实现异步定时
     */
    void scheduleTask();

private:
    asio::steady_timer m_timer;          // 定时器
    std::chrono::seconds m_interval_sec; // 定时时间
    Task m_task;                         // 定时任务
    std::atomic_bool m_running;          // 运行标志
};

#endif // _ASYNC_TIMER_H_