#include "thread_pool.h"
#include "global.h"
#include <iostream>
#include <pjsua2.hpp>

ThreadPool::~ThreadPool()
{
    stop();
}

ThreadPool::ThreadPool(std::size_t size)
    : m_running(true)
{
    for (std::size_t i = 0; i < size; ++i) {
        m_threads.emplace_back([this]() {
            worker();
        });
    }
}

void ThreadPool::worker()
{
    endpoint.libRegisterThread("Worker");
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
