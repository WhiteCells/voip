from flask import Flask, json, request, jsonify
import uuid
import time

app = Flask(__name__)


# 使用 redis 记录每个 id 的状态及上次连接时间
# 客户端产生心跳后更新 id 的状态和上次连接时间

clients = {
    "123": {"status": "online", "last_seen": int(time.time())},
    "456": {"status": "offline", "last_seen": int(time.time())}
}

"""
/accounts/<id>
客户端获取对应的 Account
{
    "accounts": [
        {},
        {},
        {},
    ]
}
"""
@app.route('/accounts/<id>', methods=['GET'])
def get_account():
    return jsonify({
        "accounts": [
            {"name":"1", "password": "1"},
            {"name":"2", "password": "2"},
            {"name":"3", "password": "3"},
        ]
    })


"""
/dialplans/<id>
客户端获取 UUID 对应的 Dial Plan
{
    "dialplans": [
        {"dial": "xxx", "status": "pending"}
    ]
}
"""
@app.route('/dialplan/<id>', methods=['GET'])
def get_plan():
    pass


"""
/notify
客户端通知服务端已上线，分配一个空闲的 uuid
UUID 是已经设置好的，UUID 会对应拨号计划
返回 UUID
{
    "uuid": "xxx"
}
"""
@app.route('/notify', methods=['POST'])
def post_notify():
    return jsonify({
        "uuid": uuid.uuid4()
    })

"""
/heartbeat/<id>
客户端心跳
"""
@app.route('/heartbeat/<id>', methods=['POST'])
def post_heartbeat(id):
    if id not in clients:
        return jsonify({"error": "Client ID not found"})

    timestamp = int(time.time())

    clients[id] = {
        "status": "online",
        "last_seen": timestamp
    }

    return jsonify({
        "id": id,
        "timestamp": timestamp
    })


"""
/status/<id>
客户端获取服务器状态，如果服务端离线，则客户端无法运行
"""
@app.route('/status/<id>', methods=['GET'])
def get_status(id):
    return jsonify({
        "id": id
    })


"""
/dial_res/<id>
服务端接受客户端结果
"""
@app.route('/dial_res/<id>', methods=['POST'])
def post_dial_res():
    return jsonify({
        "id": id
    })


"""
/dial_wav/<id>
服务端接受客户端的音频文件
"""
@app.route('/dial_wav/<id>', methods=['POST'])
def post_dial_wav(id):
    return jsonify({
        "id": id
    })
    pass


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)