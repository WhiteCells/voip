#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "ini.h"
#include <pjsua2.hpp>
#ifdef _WIN32
#include <opus.h>
#else
#include <opus/opus.h>
#endif
#include <sstream>
#include <string>
#include <atomic>
#include <memory>
#include "thread_pool.h"

class AgentWsClient;
/**
 * @brief 客户端 ID
 */
extern std::string g_client_id;

/**
 * @brief SIP 端点
 * 全局是为了在线程池中能够调用 endpoint 的注册方法
 */
extern pj::Endpoint endpoint;

void startEndpointLib(unsigned port = 5060);

extern unsigned g_thread_num;

extern std::string backend_host;

extern std::string backend_port;

extern std::string client_id;

extern std::string backend_verify_file;

extern std::string reminder_consumer_remote_host;

extern std::string reminder_consumer_remote_port;

extern std::string reminder_mediator_remote_host;

extern std::string reminder_mediator_remote_port;

extern std::string robot_remote_host;

extern std::string robot_remote_port;

extern std::string asr_server_remote_host;

extern std::string asr_server_remote_port;

extern std::string asr_server_verify_file;

extern std::string agent_session_remote_host;

extern std::string agent_session_remote_port;

extern std::string agent_session_remote_target;

extern std::string agent_session_verify_file;

extern std::string tts_server_remote_host;

extern std::string tts_server_remote_port;

extern std::string tts_server_remote_target;

extern cfg_map cfg;

extern std::string local_hangup;

extern std::shared_ptr<AgentWsClient> g_agent_ws_client;

extern std::shared_ptr<AgentWsClient> g_manual_ws_client;

extern std::unique_ptr<TTSThreadPool> g_tts_thread_pool;

// url
#define URL_NOTIFY            "notify"            // 客户端通知
#define URL_HEARTBEAT         "heartbeat"         // 客户端心跳
#define URL_ACCOUNTS          "account"           // 拉取账号
#define URL_REG_STATUS        "account/status"    // 注册状态
#define URL_DIALPLANS         "dialplan"          // 拉取呼叫计划
#define URL_DIAL_WAV          "dial_wav"          // 推送音频
#define URL_DIAL_STATUS       "dialplan/status"   // 呼叫状态
#define URL_ACCOUNTS_REGSTATE "receive/extStatus" // 推送分级检验号
#define URL_CALL_STATE        "receive/status"    // 推送通话状态
#define URL_GROUP_CALL_STATE  "receive/groupCall" // 推送群呼状态完成

/**
 * @brief 构建请求路径
 */
template <typename... Args>
inline std::string genUrl(Args &&...args)
{
    std::ostringstream oss;
    oss << "/eSip";
    ((oss << "/" << args), ...);
    return oss.str();
}

enum DIAL_STATE {
    DURING,    // 通话中
    CONFIRMED, // 确认（通话）
    HANGUP,    // 挂断
    DISCON,    // 断连
};

#define STATUS_DIALPLAN_PROCESSING "processing"
#define STATUS_DIALPLAN_FINISH     "finish"

enum REG_STATE {
    SUCCESSED,
    FAILED,
};

#define STATUS_ACCOUNT_REGISTERED   "registered"
#define STATUS_ACCOUNT_UNREGISTERED "unregistered"

struct GUIConfig
{
    std::string gui_host;
    std::string gui_port;
    std::string gui_target;
    std::string gui_client_id;
};

struct AccountsRegState
{
    std::string account_id;
    int status;
};

extern GUIConfig g_gui_cfg;

extern std::string g_task_id;

extern OpusEncoder *encoder;
extern OpusDecoder *decoder;

extern std::atomic<bool> m_confirmed;

#include "singleton.hpp"

class Global : public Singleton<Global>
{
public:
    // static
};

#endif // _GLOBAL_H_