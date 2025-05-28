#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <thread>
#include <future>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <functional>
#include <condition_variable>

class ThreadPool
{
    using Task = std::function<void()>;

public:
    explicit ThreadPool(std::size_t size = std::thread::hardware_concurrency());
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ~ThreadPool();

public:
    template <typename Func, typename... Args>
    auto addTask(Func &&func, Args &&...)
        -> std::future<std::invoke_result_t<Func, Args...>>;

    template <typename Func, typename... Args>
    auto addTaskWithTimeout(Func &&func, std::chrono::milliseconds timeout, Args &&...args)
        -> std::future<std::invoke_result_t<Func, Args...>>;

private:
    void worker();

    void stop();

    std::vector<std::thread> m_threads;
    std::queue<Task> m_tasks_que;
    std::mutex m_tasks_que_mtx;
    std::condition_variable m_tasks_que_cond;
    std::atomic_bool m_running;
};

template <typename Func, typename... Args>
inline auto ThreadPool::addTask(Func &&func, Args &&...args)
    -> std::future<std::invoke_result_t<Func, Args...>>
{
    using FuncRetType = std::invoke_result_t<Func, Args...>;
    using PackagedTaskType = std::packaged_task<FuncRetType()>;

    if (!m_running) {
        throw std::runtime_error {"ThreadPool is not running"};
    }

    auto task = std::make_shared<PackagedTaskType>(
        std::bind(std::forward<Func>(func), std::forward<Args>(args)...));

    auto future = task->get_future();

    {
        std::lock_guard<std::mutex> lock(m_tasks_que_mtx);
        m_tasks_que.emplace([task]() {
            (*task)();
        });
    }

    m_tasks_que_cond.notify_one();

    return future;
}

template <typename Func, typename... Args>
inline auto ThreadPool::addTaskWithTimeout(Func &&func, std::chrono::milliseconds timeout, Args &&...args)
    -> std::future<std::invoke_result_t<Func, Args...>>
{
    auto future = addTask(std::forward<Func>(func), std::forward<Args>(args)...);

    if (future.wait_for(timeout) == std::future_status::timeout) {
        throw std::runtime_error {"Task time out"};
    }

    return future;
}

#endif // _THREAD_POOL_H_