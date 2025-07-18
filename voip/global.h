#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "ini.h"

#include <pjsua2.hpp>
#include <sstream>
#include <string>

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

extern cfg_map cfg;

// url
#define URL_NOTIFY      "notify"          // 客户端通知
#define URL_HEARTBEAT   "heartbeat"       // 客户端心跳
#define URL_ACCOUNTS    "account"         // 拉取账号
#define URL_REG_STATUS  "account/status"  // 注册状态
#define URL_DIALPLANS   "dialplan"        // 拉取呼叫计划
#define URL_DIAL_WAV    "dial_wav"        // 推送音频
#define URL_DIAL_STATUS "dialplan/status" // 呼叫状态

/**
 * @brief 构建请求路径
 */
template <typename... Args>
inline std::string genUrl(Args &&...args)
{
    std::ostringstream oss;
    oss << "/voip";
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

#define STATUS_ACCOUNT_REGISTERED "registered"
#define STATUS_ACCOUNT_UNREGISTERED "unregistered"



#endif // _GLOBAL_H_