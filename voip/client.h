#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "thread_pool.h"
#include "caller_queue.h"
#include "dialplan_queue.h"

#include <string>
#include <vector>
#include <memory>

class Client : public std::enable_shared_from_this<Client>
{
public:
    Client(unsigned workers_num = 1 /*std::thread::hardware_concurrency()*/);
    ~Client();

private:
    // 同步通知服务端，客户端上线通知
    // 获取 m_client_id
    void notify();

    // 同步拉取账户，需要将注册结果返回
    // 更新 m_caller_que
    void pullAccount();

    // 同步推送注册结果
    void pushRegStatus();

    // 同步拉取拨号计划
    // 更新 m_dialplan_que
    void pullDialplan();

    // 异步心跳，客户端状态
    void heartbeat();

    // 异步推送音频文件
    // void pushFile(const std::string &file_path, const std::string &target);

    // 异步推送呼叫状态
    // void pushDialStatus(const std::string &dial, const std::string &status);

private:
    void callTask();

private:
    std::atomic<bool> m_running;
    ThreadPool m_thread_pool;
    std::shared_ptr<CallerQueue> m_caller_que;
    DialPlanQueue m_dialplan_que;
    std::string m_client_id;

    std::vector<std::shared_ptr<voip::VAccount>> m_vacc_vec;

    std::shared_ptr<voip::Caller> m_caller;
};

#endif // _CLIENT_H_