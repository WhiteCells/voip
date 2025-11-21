### Speech Reminder

根据 `web_ws_client` 参数切换 `人工群呼` 与 `话术提醒`

`web_ws_client` 与 `agent_ws_client` 之间需要进行通信

### 后续拓展

1. 信令安全（TLS）
2. 媒体安全（音频数据）
3. 鉴权机制（客户端）

```sh
cmake -B build \
-DENABLE_SSL=ON \
-DFEATURE=REMINDER # ROBOT
```

## BUG记录
### caller.cc
- 在voip::Caller::onCallState() 的 case PJSIP_INV_STATE_DISCONNECTED 的内部添加如下代码
```
else if (local_hangup == "customer") {
 // 被叫方挂断
    LOG_INFO("{}: 被叫方挂断", m_phone);
    hangup_direction = "customer";
    if (g_agent_ws_client) {                                //添加
        g_agent_ws_client->end_timeout_check();             //添加
    }                                                       //添加
}
```
会导致voip::pushCallState发送的内容为空
