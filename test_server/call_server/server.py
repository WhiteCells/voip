import asyncio
import websockets
import json
import re
import uuid
from datetime import datetime
import os
import threading
import logging
from flask import Flask, request, jsonify
import ssl

# ---------------- 日志配置 ----------------
os.makedirs("./logs", exist_ok=True)
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    handlers=[
        logging.FileHandler("./logs/server.log", encoding="utf-8"),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger("server")

# Flask 独立日志
flask_logger = logging.getLogger("flask")
flask_handler = logging.FileHandler("./logs/flask.log", encoding="utf-8")
flask_handler.setFormatter(logging.Formatter("%(asctime)s [%(levelname)s] %(message)s"))
flask_logger.addHandler(flask_handler)
flask_logger.setLevel(logging.INFO)

# Flask app
app = Flask(__name__)

# WebSocket 配置
HOST = "127.0.0.1"
PORT = 8088
path_pattern = re.compile(r"^/eSip/ws/page/client/(?P<client_id>[a-f0-9\-]+)$")
connected_clients = {}

json_file_paths = {
    'data': "./data/client_call_data.json",
    'verify': "./data/client_call_verify.json"
}
CLIENT_ID_FILE = "./client_id.txt"
current_file = 'data'


def generate_and_save_client_id():
    client_id = str(uuid.uuid4())
    with open(CLIENT_ID_FILE, "w", encoding="utf-8") as f:
        f.write(client_id)
    logger.info(f"已生成并保存client_id到 {CLIENT_ID_FILE}: {client_id}")
    return client_id


def load_client_id():
    return generate_and_save_client_id()


server_client_id = load_client_id()


async def load_json_data(file_key):
    try:
        file_path = json_file_paths[file_key]
        with open(file_path, "r", encoding="utf-8") as f:
            data = json.load(f)
        return data
    except FileNotFoundError:
        logger.error(f"找不到文件 {file_path}")
        return None
    except json.JSONDecodeError as e:
        logger.error(f"JSON格式不正确 {e}")
        return None


async def send_push_data(websocket, client_id, file_key):
    data = await load_json_data(file_key)
    if data is None:
        logger.warning(f"[{client_id}] 无法加载 {file_key} 文件数据")
        return False

    try:
        await websocket.send(json.dumps(data, ensure_ascii=False))
        file_name = "呼叫数据" if file_key == 'data' else "验证数据"
        logger.info(f"[{client_id}] 已推送 {file_name}")
        return True
    except Exception as e:
        logger.error(f"[{client_id}] 推送数据失败: {e}")
        return False


async def handler(websocket):
    path = websocket.request.path
    logger.info(f"新连接，路径: {path}")

    match = path_pattern.match(path)
    if not match:
        await websocket.close(code=4000, reason="路径不合法")
        logger.warning(f"连接被拒绝，非法路径: {path}")
        return

    client_id = match.group("client_id")
    if client_id != server_client_id:
        await websocket.close(code=4001, reason="client_id不匹配")
        logger.warning(f"连接被拒绝，client_id不匹配: {client_id}")
        return

    connected_clients[client_id] = websocket
    logger.info(f"客户端连接: {client_id}，当前连接数: {len(connected_clients)}")

    push_task = asyncio.create_task(interactive_push(websocket, client_id))

    try:
        async for message in websocket:
            try:
                data = json.loads(message)
                logger.info(f"[{client_id}] 收到数据: {json.dumps(data, ensure_ascii=False)}")

                request_type = data.get("request_type", -1)

                if request_type == 0:
                    logger.info(
                        f"[{client_id}] 呼叫数据: 节点={data.get('node')}, 任务ID={data.get('task_id')}, 电话数={len(data.get('phones', []))}")
                    response = {
                        "status": "success",
                        "message": "呼叫数据已接收",
                        "client_id": client_id,
                        "timestamp": datetime.utcnow().isoformat() + "Z"
                    }
                    await websocket.send(json.dumps(response, ensure_ascii=False))

                elif request_type == 1:
                    logger.info(f"[{client_id}] 验证数据账户数: {len(data.get('accounts', []))}")
                    response = {
                        "status": "success",
                        "message": "验证数据已接收",
                        "client_id": client_id,
                        "account_count": len(data.get('accounts', [])),
                        "timestamp": datetime.utcnow().isoformat() + "Z"
                    }
                    await websocket.send(json.dumps(response, ensure_ascii=False))

                else:
                    response = {
                        "status": "received",
                        "client_id": client_id,
                        "message": "数据已接收",
                        "timestamp": datetime.utcnow().isoformat() + "Z"
                    }
                    await websocket.send(json.dumps(response, ensure_ascii=False))

            except json.JSONDecodeError:
                logger.error(f"[{client_id}] 收到非JSON数据: {message}")
                await websocket.send(json.dumps({
                    "status": "error",
                    "error": "Invalid JSON",
                    "client_id": client_id
                }, ensure_ascii=False))

    except websockets.exceptions.ConnectionClosed:
        logger.info(f"连接已关闭: {client_id}")
    except Exception as e:
        logger.error(f"处理客户端 {client_id} 消息时出错: {e}")
    finally:
        push_task.cancel()
        if client_id in connected_clients:
            del connected_clients[client_id]
            logger.info(f"客户端 {client_id} 已断开，当前连接数: {len(connected_clients)}")


async def interactive_push(websocket, client_id):
    logger.info(f"[{client_id}] 推送服务已启动")
    try:
        while True:
            user_input = await asyncio.get_event_loop().run_in_executor(
                None, input, f"\n[{client_id}] 请输入命令 (1=呼叫数据, 2=验证数据, q=停止): "
            )

            if user_input.lower() == 'q':
                logger.info(f"[{client_id}] 停止推送服务")
                break
            elif user_input == '1':
                logger.info(f"[{client_id}] 正在推送呼叫数据")
                await send_push_data(websocket, client_id, 'data')
            elif user_input == '2':
                logger.info(f"[{client_id}] 正在推送验证数据")
                await send_push_data(websocket, client_id, 'verify')
            else:
                logger.warning(f"[{client_id}] 无效输入: {user_input}")
    except Exception as e:
        logger.error(f"[{client_id}] 推送服务出错: {e}")


@app.route('/eSip/receive/status/<client_id>', methods=['POST'])
def receive_call_status(client_id):
    data = request.get_json()
    flask_logger.info(f"收到呼叫状态: client_id={client_id}, data={json.dumps(data, ensure_ascii=False)}")
    return jsonify({"status": "200"})


@app.route('/eSip/receive/extStatus/<client_id>', methods=['POST'])
def receive_account_status(client_id):
    data = request.get_json()
    flask_logger.info(f"收到账户状态: client_id={client_id}, data={json.dumps(data, ensure_ascii=False)}")
    return jsonify({"status": "200"})


def run_flask():
    ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    ssl_context.load_cert_chain(certfile="./server.crt", keyfile="./server.key")

    app.run(host="0.0.0.0", port=9999, debug=False, use_reloader=False, ssl_context=ssl_context)


async def run_websocket():
    logger.info(f"WebSocket服务启动: ws://{HOST}:{PORT}/ws/client/{{client_id}}")
    logger.info(f"有效的client_id: {server_client_id}")
    ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    ssl_context.load_cert_chain(certfile="./server.crt", keyfile="./server.key")

    server = await websockets.serve(
        handler,
        HOST,
        PORT,
        ssl=ssl_context  # 关键参数
    )
    await server.wait_closed()


def main():
    flask_thread = threading.Thread(target=run_flask)
    flask_thread.start()
    asyncio.run(run_websocket())


if __name__ == "__main__":
    main()
