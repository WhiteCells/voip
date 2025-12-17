#pragma once

#include <string>
#include <chrono>

struct Msg
{
    Msg()
    {
    }
};

// 接收到的 ASR 消息
// 用于通知 LLM 有新的 ASR 文本需要回复
struct ASRTextMsg : public Msg
{
    explicit ASRTextMsg(const std::string &text)
        : m_text(text)
    {
    }

    std::string m_text;
};

// 接收到的 ASR 打断消息
struct ASRStopMsg : public Msg
{
    explicit ASRStopMsg()
    {
    }
};

// 接收到的 LLM 消息
// 用于通知 TTS 有新的 LLM 文本的音频需要生成
struct LLMTextMsg : public Msg
{
    explicit LLMTextMsg(const std::string &text)
        : m_text(text)
    {
    }

    std::string m_text;
};

// 挂断事件
struct HangupEvent : public Msg
{
    explicit HangupEvent()
    {
    }
};

// Outcoming 事件
struct OutcomingEvent : public Msg
{
    explicit OutcomingEvent(const std::string &msg)
        : m_msg(msg)
    {
    }
    std::string m_msg;
};

// Incoming 分机号注册状态事件
struct IncomingAccRegStateMsg : public Msg
{
    explicit IncomingAccRegStateMsg(int code)
        : m_code(code)
    {
    }
    int m_code;
};

// 后端连接状态事件
struct WebConnStateMsg : public Msg
{
    explicit WebConnStateMsg(const std::string &state)
        : m_state(state)
    {
    }
    std::string m_state;
};

// 外呼呼叫号码时间
struct OutcomingDialPlanMsg
{
    explicit OutcomingDialPlanMsg(const std::string &dialplan)
        : m_dialplan(dialplan)
    {
    }
    std::string m_dialplan;
};

// 外呼呼叫状态事件
struct OutcomingCallStateMsg
{
    explicit OutcomingCallStateMsg(const std::string &state)
        : m_state(state)
    {
    }
    std::string m_state;
};
