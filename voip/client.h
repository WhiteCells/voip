#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "thread_pool.h"
#include "caller_queue.h"
#include "dialplan_queue.h"

#include <string>

class Client
{
public:
    Client(
        unsigned client_port = 5060,
        unsigned workers_num = std::thread::hardware_concurrency());
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

    // 异步推送音频文件，失败后需要重试
    void pushFile();

    // 异步推送呼叫状态
    void pushDialStatus();

private:
    void startEndpointLib(unsigned port);
    void callTask();

private:
    ThreadPool m_thread_pool;
    CallerQueue m_caller_que;
    DialPlanQueue m_dialplan_que;
    std::string m_client_id;
    pj::Endpoint m_endpoint;
};

#endif // _CLIENT_H_