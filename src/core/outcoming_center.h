#pragma once

#include "../event/event.h"
#include "../event/msg.h"
#include "../logger.h"
#include "../singleton.hpp"
#include "outcoming_acc.h"
#include "outcoming_call.h"
#include "outcoming_call_que.h"
#include "check_acc_mgr.h"
#include "check_acc.h"
#include "outcoming_dialplan_que.h"
#include "session_core.h"
#include "../thread_pool.h"
#include <json/json.h>
#include <pjsua2/endpoint.hpp>
#include <vector>
#include <sstream>
#include <atomic>
#include <mutex>
#include <condition_variable>

class OutcomingCenter : public Singleton<OutcomingCenter>
{
    friend class Singleton<OutcomingCenter>;

public:
    ~OutcomingCenter() = default;

    void start()
    {
        while (m_running) {
            std::size_t worker_num = m_dialplan_que->size(); // 需要的线程数
            m_batch_remain = worker_num;                     // 剩余的任务数
            if (m_batch_remain == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            auto coordinator = std::make_shared<Coordinator>();
            if (m_call_type == "single") {
                LOG_INFO("start single call");
                m_thread_pool.addTask([this, coordinator]() {
                    makeSingleCall(coordinator);
                    {
                        std::lock_guard<std::mutex> lock(m_batch_mutex);
                        --m_batch_remain;
                        if (m_batch_remain == 0) {
                            m_batch_cv.notify_all();
                        }
                    }
                });
                std::unique_lock<std::mutex> lock(m_batch_mutex);
                m_batch_cv.wait(lock, [this]() {
                    return m_batch_remain == 0;
                });
                LOG_INFO("single call done");
            }
            else if (m_call_type == "group") {
                LOG_INFO("start group call");
                for (std::size_t i = 0; i < worker_num; ++i) {
                    m_thread_pool.addTask([this, i, coordinator]() {
                        LOG_INFO("group call worker: {}", i);
                        makeGroupCall(coordinator);
                        {
                            std::lock_guard<std::mutex> lock(m_batch_mutex);
                            --m_batch_remain;
                            if (m_batch_remain == 0) {
                                m_batch_cv.notify_all();
                            }
                        }
                    });
                }
                std::unique_lock<std::mutex> lock(m_batch_mutex);
                m_batch_cv.wait(lock, [this]() {
                    LOG_INFO("batch remain: {}", m_batch_remain);
                    return m_batch_remain == 0;
                });
                LOG_INFO("group call done");
            }
            // 释放分机号 m_accs
            for (auto &acc : m_accs) {
                acc.reset();
            }
        }
    }

    void makeSingleCall(std::shared_ptr<Coordinator> coordinator)
    {
        LOG_INFO("make single call");
        pj::Endpoint::instance().libRegisterThread("make_single_call");
        auto call = m_call_que->getCaller();
        auto dialplan = m_dialplan_que->getDialPlan();
        call->makeSingleCall(dialplan, m_call_method, m_different, coordinator);
        LOG_INFO("make single call done");
    }

    void makeGroupCall(std::shared_ptr<Coordinator> coordinator)
    {
        LOG_INFO("make group call");
        pj::Endpoint::instance().libRegisterThread("make_group_call");
        auto call = m_call_que->getCaller();
        auto dialplan = m_dialplan_que->getDialPlan();
        call->makeGroupCall(dialplan, m_call_method, m_different, coordinator);
        LOG_INFO("make group call done");
    }

    void handleOutcomingEvent(const OutcomingEvent &event)
    {
        // LOG_INFO("OutcomingEvent: {}", event.m_msg);
        Json::Value root;
        std::string errs;
        std::istringstream iss(event.m_msg);
        if (!Json::parseFromStream(m_reader_builder, iss, &root, &errs)) {
            LOG_ERROR("Failed to parse JSON: {}", errs);
            return;
        }
        LOG_INFO("recv outcoming json: {}", root.toStyledString());
        const std::string request_type = root["request_type"].asString();
        if (request_type == "check") {
            // 处理分机号检验
            LOG_INFO("check accounts type");
            const Json::Value accounts_array = root["accounts"];
            const std::string node = root["node"].asString();
            AccountCheckManager::getInstance()->setRequestRegCnt(accounts_array.size());
            for (const auto &item : accounts_array) {
                const std::string id = item["id"].asString();
                const std::string user = item["extUser"].asString();
                const std::string pass = item["extPass"].asString();
                auto acc = std::make_shared<AccountCheck>(id, user, pass, node);
                AccountCheckManager::getInstance()->regAccount(acc);
            }
        }
        else if (request_type == "dialplan") {
            // 处理拨号计划
            LOG_INFO("dialplan");
            const Json::Value accounts_array = root["accounts"];
            const std::string node = root["node"].asString();
            const std::string task_id = root["task_id"].asString();
            const Json::Value dialplans_array = root["phones"];
            const std::string call_type = root["call_type"].asString();
            const std::string call_method = root["call_method"].asString();
            const std::string different = root["different"].asString();
            m_call_type = call_type;
            m_call_method = call_method;
            m_different = different;

            if (call_type == "single") {
                // 处理单呼
                const std::string id = accounts_array[0]["id"].asString();
                const std::string user = accounts_array[0]["extUser"].asString();
                const std::string pass = accounts_array[0]["extPsd"].asString();
                const std::string dialplan = dialplans_array[0].asString();
                LOG_INFO("to make OutcomingAcc id: {}, user: {}", id, user);
                auto acc = std::make_shared<OutcomingAcc>(id, user, pass, node);
                m_accs.push_back(acc);
                auto call = std::make_shared<OutcomingCall>(*acc);
                m_call_que->addCaller(call);
                LOG_INFO("to push dialplan: {}", dialplan);
                m_dialplan_que->addDialPlan(dialplan);
            }
            else if (call_type == "group") {
                // 处理群呼
                LOG_INFO("dialplans_array size: {}", dialplans_array.size());
                // 处理分机号与呼叫不匹配情况
                auto max_call_cnt = std::min(accounts_array.size(), dialplans_array.size());
                for (Json::ArrayIndex i = 0; i < max_call_cnt; ++i) {
                    const Json::Value &item = accounts_array[i];
                    const std::string id = item["id"].asString();
                    const std::string user = item["extUser"].asString();
                    const std::string pass = item["extPsd"].asString();
                    LOG_INFO("to make OutcomingAcc id: {}, user: {}", id, user);
                    auto acc = std::make_shared<OutcomingAcc>(id, user, pass, node);
                    m_accs.push_back(acc);
                    auto call = std::make_shared<OutcomingCall>(*acc);
                    m_call_que->addCaller(call);
                    auto dialplan = dialplans_array[i].asString();
                    LOG_INFO("to push dialplan: {}", dialplan);
                    m_dialplan_que->addDialPlan(dialplan);
                }
            }
            else {
                LOG_ERROR("error call_type: {}", call_type);
            }
        }
        else if (request_type == "auth") {
            // 处理认证
            LOG_INFO("auth");
            const std::string session_id = root["session_id"].asString();
            const std::string access_token = root["access_token"].asString();
            SessionCore::getInstance()->setSessionId(session_id);
            SessionCore::getInstance()->setAccessToken(access_token);
            SessionCore::getInstance()->setCallMethod(m_call_method);
        }
        else {
            LOG_ERROR("error request_type: {}", request_type);
        }
    }

private:
    OutcomingCenter()
        : m_call_que(std::make_shared<OutcomingCallQue>())
        , m_dialplan_que(std::make_shared<OutcomingDialPlanQue>())
        , m_thread_pool(std::thread::hardware_concurrency())
    {
        EventBus::getInstance()->subscribe<OutcomingEvent>([this](const OutcomingEvent &event) {
            // LOG_INFO("OutcomingEvent: {}", event.m_msg);
            this->handleOutcomingEvent(event);
        });
    }

private:
    Json::CharReaderBuilder m_reader_builder;
    std::atomic<bool> m_running {true};
    std::string m_call_type;
    std::string m_call_method;
    std::string m_different;
    std::vector<std::shared_ptr<OutcomingAcc>> m_accs;
    std::shared_ptr<OutcomingCallQue> m_call_que;
    std::shared_ptr<OutcomingDialPlanQue> m_dialplan_que;
    ThreadPool m_thread_pool;
    std::size_t m_batch_remain;
    std::mutex m_batch_mutex;
    std::condition_variable m_batch_cv;
};