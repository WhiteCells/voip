#ifndef _TASK_RUNNER_H_
#define _TASK_RUNNER_H_

#include <thread>
#include <vector>
#include <queue>
#include <atomic>
#include <functional>
#include <future>
#include <condition_variable>
#include <chrono>
#include <memory>
#include <iostream>

class TaskRunner
{
public:
    using TaskWrapper = std::function<void()>;
    using fn = std::function<void(std::atomic<bool> &, std::shared_ptr<std::promise<void>>)>;

    TaskRunner(size_t thread_count) :
        m_stop(false)
    {
        for (size_t i = 0; i < thread_count; ++i) {
            m_workers.emplace_back([this]() {
                while (true) {
                    TaskWrapper task;
                    {
                        std::unique_lock<std::mutex> lock(m_tasks_mutex);
                        m_tasks_cv.wait(lock, [this]() {
                            return m_stop || !m_tasks.empty();
                        });
                        if (m_stop && m_tasks.empty()) {
                            return;
                        }
                        task = std::move(m_tasks.front());
                        m_tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~TaskRunner()
    {
        {
            std::unique_lock<std::mutex> lock(m_tasks_mutex);
            m_stop = true;
        }
        m_tasks_cv.notify_all();
        for (auto &worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    void addTask(TaskWrapper task)
    {
        {
            std::unique_lock<std::mutex> lock(m_tasks_mutex);
            m_tasks.push(std::move(task));
        }
        m_tasks_cv.notify_one();
    }

    void runBatch(
        const std::vector<fn> task_batch,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000))
    {
        std::atomic<bool> stop_flag = false;
        auto batch_done = std::make_shared<std::promise<void>>();
        auto batch_done_future = batch_done->get_future();

        for (const auto &task : task_batch) {
            addTask([&stop_flag, task, batch_done]() {
                task(stop_flag, batch_done);
            });
        }

        if (batch_done_future.wait_for(timeout) == std::future_status::timeout) {
            std::cout << "[Batch Timeout] No task reached target in time.\n";
            stop_flag = true;
        }
        else {
            std::cout << "[Batch Success] One task reached the target.\n";
        }
    }

private:
    std::vector<std::thread> m_workers;
    std::queue<TaskWrapper> m_tasks;
    std::mutex m_tasks_mutex;
    std::condition_variable m_tasks_cv;
    bool m_stop;
};

#endif // _TASK_RUNNER_H_