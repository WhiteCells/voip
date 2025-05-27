#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "ini.h"

#include <pjsua2.hpp>
#include <sstream>
#include <string>

extern std::string g_client_id;

extern pj::Endpoint endpoint;

void startEndpointLib(unsigned port = 5060);

extern unsigned thread_num;

extern std::string backend_host;

extern std::string backend_port;

extern cfg_map cfg;

// url
#define URL_NOTIFY      "notify"      // 客户端通知
#define URL_HEARTBEAT   "heartbeat"   // 客户端心跳
#define URL_ACCOUNTS    "accounts"    // 拉取账号
#define URL_REG_STATUS  "reg_status"  // 注册状态
#define URL_DIALPLANS   "dialplans"   // 拉取呼叫计划
#define URL_DIAL_WAV    "dial_wav"    // 推送音频
#define URL_DIAL_STATUS "dial_status" // 呼叫状态

template <typename... Args>
inline std::string genUrl(Args &&...args)
{
    std::ostringstream oss;
    ((oss << "/" << args), ...);
    return oss.str();
}

enum DIAL_STATE {
    DURING,    // 通话中
    CONFIRMED, // 确认（通话）
    HANGUP,    // 挂断
    DISCON,    // 断连
};

enum REG_STATE {
    SUCCESSED,
    FAILED,
};

#endif // _GLOBAL_H_