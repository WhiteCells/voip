#include "thread_pool.h"
#include "global.h"
#include <iostream>
#include <pjsua2.hpp>

ThreadPool::~ThreadPool()
{
    stop();
}

ThreadPool::ThreadPool(std::size_t size, bool is_sip)
    : m_running(true)
    , m_is_sip(is_sip)
{
    for (std::size_t i = 0; i < size; ++i) {
        m_threads.emplace_back([this]() {
            worker();
        });
    }
}

void ThreadPool::worker()
{
    if (m_is_sip) {
        endpoint.libRegisterThread("Worker");
    }
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

// thread_pool
TTSThreadPool::TTSThreadPool(std::size_t size)
    : m_running(true)
{
    for (std::size_t i = 0; i < size; ++i) {
        m_threads.emplace_back([this]() {
            worker();
        });
    }
}

TTSThreadPool::~TTSThreadPool()
{
    m_running = false;
    m_tasks_que_cond.notify_all();

    for (auto &thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void TTSThreadPool::addTask(Task task)
{
    if (!m_running) {
        throw std::runtime_error {"TTSThreadPool is not running"};
    }

    {
        std::lock_guard<std::mutex> lock(m_tasks_que_mtx);
        m_tasks_que.emplace(std::move(task));
    }

    m_tasks_que_cond.notify_one();
}

void TTSThreadPool::worker()
{
    while (m_running) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(m_tasks_que_mtx);
            m_tasks_que_cond.wait(lock, [this]() {
                return !m_tasks_que.empty() || !m_running;
            });

            if (!m_running && m_tasks_que.empty()) {
                break;
            }

            task = std::move(m_tasks_que.front());
            m_tasks_que.pop();
        }

        try {
            task();
        }
        catch (const std::exception &e) {
        }
    }
}
