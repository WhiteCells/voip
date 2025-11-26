import requests
import json
import os

API_URL = "http://192.168.2.3/v1/chat-messages"
AUTH_TOKEN = "app-5wlJJ0WDoyKpZ6t460pb4H3m"

CONV_FILE = "conversation_id.txt"
HISTORY_FILE = "conversation_history.json"
USERINFO = "\"王福建\":\"姓名:王福建;身份证:350124199012022***;年龄:32;性别:男;生日:1990-12-2;手机号:183****5950;逾期金额:3155.81;逾期天数:2057;合同数:1;户籍地址:福建省福州市永泰县城峰镇板桥路238号"

def load_conversation_id():
    return open(CONV_FILE).read().strip() if os.path.exists(CONV_FILE) else ""


def save_conversation_id(cid):
    with open(CONV_FILE, "w") as f:
        f.write(cid)


def load_history():
    if os.path.exists(HISTORY_FILE):
        return json.load(open(HISTORY_FILE, "r", encoding="utf-8"))
    return []


def save_history(history):
    json.dump(history, open(HISTORY_FILE, "w", encoding="utf-8"), ensure_ascii=False, indent=2)


def build_query(history, user_input):
    """
    拼接 query 字符串
    """
    items = ", ".join([str(item) for item in history])
    return f"[{items}]当前回复：{{'role': 'client', 'text': '{user_input}'}}"


def send_request(user_input):
    """
    用户输入 → 追加到历史 → 请求服务器 → 保存模型回复
    """

    # 加载历史对话
    history = load_history()
    history.append({"role": "client", "text": user_input})

    # 构造 query
    query_text = build_query(history, user_input)

    # 加载 conversation_id
    conversation_id = load_conversation_id()

    payload = {
        "inputs": {
            "userinformation": USERINFO
        },
        "query": query_text,
        "response_mode": "blocking",
        "conversation_id": conversation_id,
        "user": "abc-123"
    }

    headers = {
        "Content-Type": "application/json",
        "Authorization": f"Bearer {AUTH_TOKEN}"
    }

    resp = requests.post(API_URL, json=payload, headers=headers)
    resp_json = resp.json()

    print(json.dumps(resp_json, indent=2, ensure_ascii=False))

    # 如果有新的 conversation_id → 保存
    if resp_json.get("conversation_id"):
        save_conversation_id(resp_json["conversation_id"])

    # 获取模型回复并添加到历史
    assistant_msg = resp_json.get("answer", "")
    history.append({"role": "server", "text": assistant_msg})

    save_history(history)

    return assistant_msg


if __name__ == "__main__":
    user_msg = input("客户说：")
    reply = send_request(user_msg)
    print("\n模型回复：", reply)
