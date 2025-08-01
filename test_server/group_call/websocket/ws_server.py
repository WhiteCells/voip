import asyncio
import websockets
import json
import re
import uuid
from datetime import datetime
import os

HOST = "127.0.0.1"
PORT = 8088

# 匹配路径 /ws/client/{client_id}
path_pattern = re.compile(r"^/ws/client/(?P<client_id>[a-f0-9\-]+)$")

# 存储连接的客户端
connected_clients = {}

# JSON文件路径
json_file_paths = {
    'data': "./data/client_call_data.json",
    'verify': "./data/client_call_verify.json"
}

# client_id文件路径
CLIENT_ID_FILE = "./client_id.txt"

# 初始文件
current_file = 'data'


def generate_and_save_client_id():
    """
    生成新的client_id并保存到文件
    """
    client_id = str(uuid.uuid4())
    with open(CLIENT_ID_FILE, "w", encoding="utf-8") as f:
        f.write(client_id)
    print(f"已生成并保存client_id到 {CLIENT_ID_FILE}: {client_id}")
    return client_id


def load_client_id():
    """
    从文件加载client_id
    """
    return generate_and_save_client_id()


# 加载或生成client_id
server_client_id = load_client_id()


async def load_json_data(file_key):
    """
    从指定文件加载JSON数据
    """
    try:
        file_path = json_file_paths[file_key]
        with open(file_path, "r", encoding="utf-8") as f:
            data = json.load(f)
        return data
    except FileNotFoundError:
        print(f"错误: 找不到文件 {file_path}")
        return None
    except json.JSONDecodeError as e:
        print(f"错误: JSON格式不正确 {e}")
        return None


async def send_push_data(websocket, client_id, file_key):
    """
    向客户端推送JSON文件数据
    """
    data = await load_json_data(file_key)
    if data is None:
        print(f"[{client_id}] 无法加载 {file_key} 文件数据")
        return False

    try:
        await websocket.send(json.dumps(data, ensure_ascii=False))
        file_name = "呼叫数据" if file_key == 'data' else "验证数据"
        print(f"[{client_id}] 已推送 {file_name}")
        return True
    except Exception as e:
        print(f"[{client_id}] 推送数据失败: {e}")
        return False


async def handler(websocket):
    """
    处理WebSocket连接和消息
    """
    # 从websocket对象中获取路径
    path = websocket.request.path
    print(f"新连接，路径: {path}")

    # 验证路径格式
    match = path_pattern.match(path)
    if not match:
        await websocket.close(code=4000, reason="路径不合法")
        print(f"连接被拒绝，非法路径: {path}")
        return

    client_id = match.group("client_id")

    # 验证client_id是否匹配
    if client_id != server_client_id:
        await websocket.close(code=4001, reason="client_id不匹配")
        print(f"连接被拒绝，client_id不匹配: {client_id}")
        return

    connected_clients[client_id] = websocket
    print(f"客户端连接: {client_id}，路径: {path}")
    print(f"当前连接客户端数: {len(connected_clients)}")

    # 启动交互式推送任务
    push_task = asyncio.create_task(interactive_push(websocket, client_id))

    try:
        async for message in websocket:
            try:
                # 解析JSON数据
                data = json.loads(message)
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] [{client_id}] 收到数据:")
                print(json.dumps(data, indent=2, ensure_ascii=False))

                # 根据消息类型处理
                request_type = data.get("request_type", -1)

                if request_type == 0:
                    # 处理client_call_data.json数据
                    node = data.get("node", "unknown")
                    phones = data.get("phones", [])
                    task_id = data.get("task_id", "unknown")
                    accounts = data.get("accounts", [])

                    print(f"[{client_id}] 收到呼叫数据:")
                    print(f"  节点: {node}")
                    print(f"  电话号码: {', '.join(phones) if phones else '无'}")
                    print(f"  任务ID: {task_id}")
                    print(f"  账户数: {len(accounts)}")

                    response = {
                        "status": "success",
                        "message": "呼叫数据已接收",
                        "client_id": client_id,
                    }
                    await websocket.send(json.dumps(response, ensure_ascii=False))
                elif request_type == 1:
                    # 处理client_call_verify.json数据
                    accounts = data.get("accounts", [])

                    print(f"[{client_id}] 收到验证数据:")
                    for account in accounts:
                        print(f"  账户 - ID: {account.get('id', 'unknown')}, "
                              f"用户: {account.get('user', 'unknown')}")

                    response = {
                        "status": "success",
                        "message": "验证数据已接收",
                        "client_id": client_id,
                        "account_count": len(accounts),
                    }
                    await websocket.send(json.dumps(response, ensure_ascii=False))
                else:
                    response = {
                        "status": "received",
                        "client_id": client_id,
                        "message": "数据已接收",
                    }
                    await websocket.send(json.dumps(response, ensure_ascii=False))

            except json.JSONDecodeError:
                print(f"[{client_id}] 收到非JSON数据: {message}")
                await websocket.send(json.dumps({
                    "status": "error",
                    "error": "Invalid JSON",
                    "client_id": client_id
                }, ensure_ascii=False))

    except websockets.exceptions.ConnectionClosed:
        print(f"连接已关闭: {client_id}")
    except Exception as e:
        print(f"处理客户端 {client_id} 消息时出错: {e}")
    finally:
        # 取消推送任务
        push_task.cancel()
        # 清理断开连接的客户端
        if client_id in connected_clients:
            del connected_clients[client_id]
            print(f"客户端 {client_id} 已断开连接，当前连接客户端数: {len(connected_clients)}")


async def interactive_push(websocket, client_id):
    """
    交互式推送数据
    """
    global current_file

    print(f"\n[{client_id}] 推送服务已启动")
    print("操作说明:")
    print("  回车键: 推送当前文件数据")
    print("  c: 切换推送文件")
    print("  q: 停止推送(不会退出客户端)")

    try:
        while True:
            user_input = await asyncio.get_event_loop().run_in_executor(
                None, input, f"\n[{client_id}] 请输入命令 (回车推送/c切换/q停止): "
            )

            if user_input.lower() == 'q':
                print(f"[{client_id}] 停止推送服务")
                break
            elif user_input.lower() == 'c':
                # 切换文件
                global current_file
                current_file = 'verify' if current_file == 'data' else 'data'
                file_name = "验证数据" if current_file == 'verify' else "呼叫数据"
                print(f"[{client_id}] 已切换到: {file_name}")
            elif user_input == '':
                # 推送当前文件数据
                file_name = "验证数据" if current_file == 'verify' else "呼叫数据"
                print(f"[{client_id}] 正在推送 {file_name}...")
                await send_push_data(websocket, client_id, current_file)
    except Exception as e:
        print(f"[{client_id}] 推送服务出错: {e}")


async def main():
    """
    启动WebSocket服务器
    """
    print(f"服务启动中: ws://{HOST}:{PORT}/ws/client/{{client_id}}")
    print(f"有效的client_id: {server_client_id}")

    # 启动服务器
    server = await websockets.serve(handler, HOST, PORT)
    print(f"WebSocket服务器已启动，监听地址: ws://{HOST}:{PORT}")
    print("等待客户端连接...")

    await server.wait_closed()


if __name__ == "__main__":
    asyncio.run(main())
