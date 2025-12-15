#pragma once

#include "../singleton.hpp"
#include "../event/event.h"
#include "../event/msg.h"
#include "incoming_acc.h"
#include <pjsua2.hpp>
#include <atomic>

// SIP 核心
// 存储 SIP 账号，以及当前的 SIP 状态（呼叫状态）
// 负责挂断、转呼
class SIPCore : public Singleton<SIPCore>
{
    friend class Singleton<SIPCore>;

public:
    enum class State {
        IDLE,    // 空闲状态
        CALLING, // 呼叫状态
        HANGUP,  // 挂断状态
    };
    ~SIPCore() = default;

    void initEvent()
    {
        EventBus::getInstance()->subscribe<HangupEvent>([this](const HangupEvent &) {
            hangup();
        });

        EventBus::getInstance()->subscribe<AnswerEvent>([this](const AnswerEvent &) {
            answer();
        });
    }
    SIPCore::State getState() const { return m_state; }
    void setState(State state) { m_state.store(state); }
    void registerAccount(const std::string &user,
                         const std::string &pass,
                         const std::string &host)
    {
        if (isAccountRegistered()) {
            m_account.reset();
        }
        m_account = std::make_shared<IncomingAcc>(user, pass, host);
    }
    void unRegisterAccount() { m_account.reset(); }
    bool isAccountRegistered() const { return m_account != nullptr; }

    void answer()
    {
        pj::Endpoint::instance().libRegisterThread("sip_core_answer");
        if (m_state == State::IDLE || m_state == State::HANGUP) {
            // todo 接听呼叫
            m_account->answer();
            setState(State::CALLING);
        }
    }

    void hangup()
    {
        pj::Endpoint::instance().libRegisterThread("sip_core_hangup");
        if (m_state == State::CALLING) {
            pj::Endpoint::instance().hangupAllCalls();
            setState(State::HANGUP);
        }
    }

private:
    SIPCore() = default;

private:
    std::atomic<SIPCore::State> m_state {State::IDLE};
    std::shared_ptr<IncomingAcc> m_account;
    std::shared_ptr<IncomingCall> m_incoming_call;
};