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

accounts = {
    "123": [
        {"name": "1000", "password": "1000"},
        {"name": "1001", "password": "1001"}
    ]
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
def get_account(id):
    acc = accounts[id]
    # return jsonify({
    #     "accounts": [
    #         {"name":"1", "password": "1"},
    #         {"name":"2", "password": "2"},
    #         {"name":"3", "password": "3"},
    #     ]
    # })
    return jsonify({
        "accounts": acc
    })


"""
/dialplans/<id>
客户端获取 UUID 对应的 Dial Plan
{
    "dialplans": [
        {"dial": "xxx", "status": "pending"},
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
@app.route('/notify', methods=['GET'])
def post_notify():
    return jsonify({
        "id": uuid.uuid4()
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

    # for k, v in clients:
    #     print(k, v)

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

@app.route('/', methods=['POST'])
def handle_request():
    # 获取 URL 参数
    param1 = request.args.get("param1")
    param2 = request.args.get("param2")

    print(param1)
    print(param2)

    # 获取 body 数据（GET 请求不会有 body，一般用 POST/PUT/PATCH）
    body = request.get_json(silent=True)  # `silent=True` 避免无 body 时抛异常

    print(body)

    return jsonify({
        "params": {
            "param1": param1,
            "param2": param2
        },
        "body": body
    })

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)