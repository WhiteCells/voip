#ifndef _VCALL_H_
#define _VCALL_H_

#include "coordinator.h"
#include <pjsua2.hpp>
#include <string>
#include <memory>
#include <ctime>

class CallerQueue;

class IWSSender;

namespace voip {

class VAccount;

/**
 * @brief 呼叫者
 * 继承 pj::Call
 * 通过重载父类虚函数实现对状态的获取
 */
class Caller :
    public pj::Call,
    public std::enable_shared_from_this<Caller>
{
public:
    Caller(VAccount &acc, int call_id = PJSUA_INVALID_ID);
    Caller(const Caller &) = delete;
    Caller &operator=(const Caller &) = delete;
    ~Caller();

    // 状态吗改变
    virtual void onCallTsxState(pj::OnCallTsxStateParam &prm) override;

    // 呼叫状态改变
    virtual void onCallState(pj::OnCallStateParam &prm) override;

    // 呼叫状态改变
    virtual void onCallMediaState(pj::OnCallMediaStateParam &prm) override;

    // virtual void onStreamCreated(pj::OnStreamCreatedParam &prm) override;

    /**
     * @brief 呼叫方法
     *
     * @param phone 呼叫手机号
     * @param client_id 客户端 ID
     * @param que 呼叫者队列，用于在呼叫完成后回收呼叫者
     * @param caller 需要回收的呼叫者
     */
    void call(const std::string &phone,
              const std::string &client_id,
              const int dialplan_id,
              std::shared_ptr<Coordinator> coordinator,
              std::shared_ptr<IWSSender> sender);

    void single_call(const std::string &phone,
                     const std::string &client_id,
                     const int dialplan_id,
                     std::shared_ptr<Coordinator> coordinator,
                     std::shared_ptr<IWSSender> sender);

    void hangup_();

private:
    int m_dialplan_id;
    std::time_t now_time;
    std::string m_filename;
    VAccount &acc_;

    std::string m_phone;
    std::string m_client_id;
    std::shared_ptr<Coordinator> m_coordinator;
    std::shared_ptr<IWSSender> m_sender;
    int m_call_status = 0;
    int call_type;
    std::string hangup_direction;
};

} // namespace voip

#endif // _VCALL_H_
