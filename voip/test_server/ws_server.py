import asyncio
import websockets
import json
import time

connected_clients = {}

async def heartbeat_check():
    while True:
        now = time.time()
        offline_clients = []

        for client_id, client_info in connected_clients.items():
            if now - client_info["last_heartbeat"] > 10:
                print(f"Client {client_id} is offline.")
                offline_clients.append(client_id)

        for client_id in offline_clients:
            del connected_clients[client_id]

        await asyncio.sleep(5)

async def handle_client(websocket):
    client_id = websocket.remote_address
    print(f"Client {client_id} connected.")
    connected_clients[client_id] = {
        "websocket": websocket,
        "last_heartbeat": time.time()
    }

    try:
        async for message in websocket:
            try:
                data = json.loads(message)
                if data.get("type") == "heartbeat":
                    # 更新心跳时间戳
                    connected_clients[client_id]["last_heartbeat"] = time.time()
                    print(f"Received heartbeat from {client_id}")
            except json.JSONDecodeError:
                print(f"Invalid JSON from {client_id}: {message}")

    except websockets.ConnectionClosed:
        print(f"Client {client_id} disconnected.")
    finally:
        # 确保客户端从在线列表中移除
        connected_clients.pop(client_id, None)

async def main():
    server = await websockets.serve(handle_client, "localhost", 8765)
    print("WebSocket server started on ws://localhost:8765")
    await asyncio.gather(server.wait_closed(), heartbeat_check())

asyncio.run(main())
