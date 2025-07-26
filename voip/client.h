#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "thread_pool.h"
#include "caller_queue.h"
#include "dialplan_queue.h"
#include "coordinator.h"
#include <vector>
#include <memory>
#include <atomic>
#include <condition_variable>

class Client : public std::enable_shared_from_this<Client>
{
public:
    // Client(unsigned workers_num = std::thread::hardware_concurrency());
    Client(unsigned workers_num = 5);
    ~Client();

private:
    void callBatch();
    void callTask(std::size_t i, std::shared_ptr<Coordinator> coordinator);

private:
    std::atomic_bool m_running;                              // 运行标志
    ThreadPool m_thread_pool;                                // 线程池
    std::shared_ptr<CallerQueue> m_caller_que;               // 呼叫者队列
    DialPlanQueue m_dialplan_que;                            // 拨号计划队列
    std::vector<std::shared_ptr<voip::VAccount>> m_vacc_vec; // SIP 账号容器

    std::atomic<int> m_batch_remain {0};
    std::size_t m_workers_num;
    std::mutex m_mtx;
    std::condition_variable m_cv;
};

#endif // _CLIENT_H_