import json
import requests
import asyncio
import os

CONSERVATIVE_TOKEN = "app-UPEvSmbcvYAb2TECJGUNWDsQ"
RADICAL_TOKEN = "app-qapQxfIBGSJZWGNSg0k2kxCB"
API_URL = "http://192.168.2.3/v1/chat-messages"
HISTORY_FILE = "history.json"
USERINFORMATION = "姓名:王福建;身份证:350124199012022***;年龄:32;..."


class ConversationHistory:
    def __init__(self, filename=HISTORY_FILE):
        self.filename = filename
        self.history = self.load()

    def load(self):
        if os.path.exists(self.filename):
            try:
                with open(self.filename, "r", encoding="utf-8") as f:
                    return json.load(f)
            except Exception as Ex:
                print(f"[ERROR] history file load failed: {Ex}")
                pass
        return []

    def save(self):
        with open(self.filename, "w", encoding="utf-8") as f:
            json.dump(self.history, f, ensure_ascii=False, indent=2)

    def add(self, role, text):
        self.history.append({"role": role, "text": text})
        self.save()

    def to_str(self):
        return json.dumps(self.history, ensure_ascii=False, separators=(",", ":"))


async def call_api(history_str: str, userinfo: str, query: str, token: str):
    try:
        payload = {
            "inputs": {"history": history_str, "userinformation": userinfo},
            "query": query,
            "response_mode": "blocking",
            "conversation_id": "",
            "user": "abc-123",
        }

        headers = {
            "Authorization": f"Bearer {token}",
            "Content-Type": "application/json",
        }

        resp = requests.post(API_URL, json=payload, headers=headers)
        j = resp.json()
        return j.get("answer")

    except Exception as e:
        print(f"[ERROR] call api failed: {e}")
        return None


async def main():
    print("===== 话术提醒对话 CLI 工具 =====")
    print("1. 增加对话记录")
    print("2. 调用保守模型（需要先输入 客户 内容）")
    print("3. 调用激进模型（需要先输入 客户 内容）")
    print("4. 退出")
    print("=========================\n")

    history = ConversationHistory()
    userinfo = USERINFORMATION

    while True:
        action = input("\n请选择操作 (1/2/3/4)：").strip()

        if action == "4":
            print("exit")
            break

        if action not in ("1", "2", "3"):
            print("输入必须是 1/2/3/4")
            continue

        if action == "1":
            role = input("角色 (client/server)：").strip()
            if role not in ("client", "server"):
                print("角色必须是 client 或 server")
                continue

            text = input("内容：").strip()
            history.add(role, text)
            print("已写入历史")
            continue

        print("\n请输入 客户 的说话内容（作为 query）：")
        client_text = input("client 内容：").strip()
        if not client_text:
            print("client 内容不能为空")
            continue

        history.add("client", client_text)

        if action == "2":
            print("\n调用保守模型...")
            answer = await call_api(
                history.to_str(), userinfo, client_text, CONSERVATIVE_TOKEN
            )

        elif action == "3":
            print("\n调用激进模型...")
            answer = await call_api(
                history.to_str(), userinfo, client_text, RADICAL_TOKEN
            )

        # 处理接口返回
        if answer:
            print(f"\n模型回复：{answer}\n")
            history.add("server", answer)
        else:
            print("API call failed\n")


if __name__ == "__main__":
    asyncio.run(main())
