#include "thread_pool.h"

#include <iostream>

ThreadPool::~ThreadPool()
{
    stop();
}

// ThreadPool &ThreadPool::getInstance()
// {
//     static ThreadPool pool;
//     return pool;
// }

ThreadPool::ThreadPool(std::size_t size)
{
    for (std::size_t i = 0; i < size; ++i) {
        m_threads.emplace_back([this]() {
            loop();
        });
    }
}

void ThreadPool::loop()
{
    while (m_running) {
        Task task;
        {
            std::unique_lock<std::mutex> lock {m_tasks_que_mtx};
            m_tasks_que_cond.wait(lock, [this]() {
                return !m_running || !m_tasks_que.empty();
            });
            if (!m_running && m_tasks_que.empty()) {
                return;
            }
            task = std::move(m_tasks_que.front());
            m_tasks_que.pop();
        }
        try {
            task();
        }
        catch (const std::exception &e) {
            std::cerr << "Task Exception: " << e.what() << std::endl;
        }
    }
}

void ThreadPool::stop()
{
    m_running = false;
    m_tasks_que_cond.notify_all();
    for (auto &thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}
