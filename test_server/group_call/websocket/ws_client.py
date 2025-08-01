import asyncio
import websockets
import uuid
import json

# client_id文件路径
CLIENT_ID_FILE = "./client_id.txt"

def load_client_id():
    """
    从文件加载client_id
    """
    try:
        with open(CLIENT_ID_FILE, "r", encoding="utf-8") as f:
            client_id = f.read().strip()
        print(f"已从 {CLIENT_ID_FILE} 加载client_id: {client_id}")
        return client_id
    except FileNotFoundError:
        print(f"错误: 找不到文件 {CLIENT_ID_FILE}，请确保服务端已生成该文件")
        return None
    except Exception as e:
        print(f"读取client_id文件时出错: {e}")
        return None

# 从文件加载client_id
client_id = load_client_id()
if client_id is None:
    print("无法获取client_id，客户端退出")
    exit(1)

# WebSocket 地址
ws_url = f"ws://127.0.0.1:8088/ws/client/{client_id}"

async def handle_server_push(websocket):
    """
    处理服务器推送的数据
    """
    try:
        async for message in websocket:
            try:
                data = json.loads(message)

                # 检查是否是推送数据
                if "push_type" in data:
                    # 这是服务器推送的数据
                    push_type = data.get("push_type")
                    push_timestamp = data.get("push_timestamp")
                    file_name = "验证数据" if push_type == "verify" else "呼叫数据"

                    print(f"\n=== 收到服务器推送的{file_name} ===")
                    print(f"推送时间: {push_timestamp}")
                    print("数据内容:")
                    # 移除推送相关的额外字段以便清晰显示原始数据
                    display_data = data.copy()
                    display_data.pop("push_type", None)
                    display_data.pop("push_timestamp", None)
                    print(json.dumps(display_data, indent=2, ensure_ascii=False))
                    print("=" * 40)
                else:
                    # 这是其他服务器响应
                    print(f"收到服务器响应: {data}")

            except json.JSONDecodeError:
                print(f"收到非JSON格式数据: {message}")
            except Exception as e:
                print(f"处理推送数据时出错: {e}")
    except websockets.exceptions.ConnectionClosed:
        print("服务器连接已关闭")
    except Exception as e:
        print(f"监听推送数据时出错: {e}")

async def client_receiver():
    """
    客户端接收器
    """
    print(f"正在连接到服务器: {ws_url}")

    try:
        websocket = await websockets.connect(ws_url)
        print(f"连接成功！客户端ID: {client_id}")
        print("客户端已就绪，正在等待服务器推送数据...")

        # 启动一个任务来监听服务器推送
        push_handler_task = asyncio.create_task(handle_server_push(websocket))

        # 等待推送任务完成
        await push_handler_task

    except websockets.exceptions.ConnectionClosed as e:
        if e.code == 4001:
            print("连接被拒绝：client_id不匹配，请检查client_id.txt文件")
        else:
            print("连接已关闭")
    except Exception as e:
        print(f"连接错误: {e}")

# 启动客户端接收器
if __name__ == "__main__":
    asyncio.run(client_receiver())
