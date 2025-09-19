from flask import Flask, request, jsonify
import os
import time
import threading
from collections import deque

app = Flask(__name__)
UPLOAD_DIR = "./uploads"
os.makedirs(UPLOAD_DIR, exist_ok=True)
i = True
j = True

# 返回客户端 ID
@app.route("/voip/notify", methods=["POST"])
def notify():
    return jsonify({
        "code": 200,
        "msg": "success",
        "data": {
            "clientId": "00001"
        }
    }), 200

@app.route("/voip/account/<clientId>", methods=["GET"])
def accounts(clientId: str):
    print(clientId)

    return jsonify({
        "code": 200,
        "msg": "success",
        "data": {
            "accounts": [
                # {"id": 1, "name": "1001", "pwd": "1001", "host": "192.168.10.51:5060"},
                # {"id": 2, "name": "1002", "pwd": "1002", "host": "192.168.10.51:5060"},
                {"id": 3, "name": "1003", "pwd": "1003", "host": "192.168.10.51:5060"},
                # {"name": "1001", "pwd": "1001", "host": "192.168.10.62:5060"},
                # {"name": "1002", "pwd": "1002", "host": "192.168.2.243:5060"},
                # {"name": "1003", "pwd": "1003", "host": "192.168.10.63:5060"},
                # {"name": "1004", "pwd": "1004", "host": "192.168.10.51:5060"},
                # {"name": "1005", "pwd": "1005", "host": "192.168.10.51:5060"},
                # {"name": "1006", "pwd": "1006", "host": "192.168.10.51:5060"},
            ]
        }
    })

dialplan_queue = deque([
    # "818871357225",
    # "818871357226",
    # "818871357227",
    # "818871357228",
    # "818871357229"

    "813831662418",
    "818252353555",
    "818434050770",
    "818648184069",
    "813661601089",
    "813171378333",
])

queue_lock = threading.Lock()  # 锁


@app.route("/voip/dialplan/<clientId>", methods=["GET"])
def get_dialplan(clientId: str):
    return jsonify({
        "code": 200,
        "msg": "success",
        "data": {
            "dialplans": [
                "1001",
            ]
        }
    })

# @app.route("/voip/dialplans/<clientId>", methods=["GET"])
# def dialplans(clientId: str):
#     print(clientId)
#     return jsonify({
#         "code": 200,
#         "msg": "success",
#         "data": {
#             "dialplans": [
#                 # "818871357225", 
#                 # "13329715728",
#                 # "13385281976",

#                 # "13385281971",
#                 # "13385281972",
#                 # "13385281973",
#                 # "813385281975",
#                 # "818871357225",

#                 # "13385281972",
#                 # "13385281971",
#                 # "13385281970",

#                 # "13831662418", # 英文 + 已关机
#                 # "18252353555",
#                 # "18434050770", # 空号 机械声音，仿人声
#                 # "18648184069",
#                 # "13661601089",
#                 "13171378333", # 空号没有提示音

#                 # "813831662418",
#                 # "818252353555",
#                 # "818434050770",
#                 # "818648184069",
#                 # "813661601089",
#                 # "813171378333",

#                 # "100613831662418",
#                 # "100618252353555",
#                 # "100618434050770",
#                 # "100618648184069",
#                 # "100613661601089",
#                 # "100613171378333",
#             ]
#         }
#     })

@app.route("/voip/dialplans2/<clientId>", methods=["GET"])
def dialplans2(clientId: str):
    print(clientId)
    return jsonify({
        "code": 200,
        "msg": "success",
        "data": {
            "dialplans": [
                "813385281971",
                "813385281972",
            ]
        }
    })

@app.route("/voip/heartbeat/<clientId>", methods=["POST"])
def heartbeat(clientId: str):
    print(clientId)
    return jsonify({
        "code": 200,
        "msg": "success",
        "data": {
            "timestamp": int(time.time())
        }
    })

"""
{
    "user": ""
    "status": "" // enum
}
"""
@app.route("/voip/reg_status/<clientId>", methods=["POST"])
def reg_status(clientId: str):
    data = request.get_json()
    print(data)
    print(clientId)
    return jsonify({
        "code": 200,
        "msg": "success",
        "data": {
            "timestamp": int(time.time())
        }
    })

"""
{
    "phoneNum": ""
    "status": "" // enum
}
"""
@app.route("/voip/dial_status/<clientId>", methods=["POST"])
def dial_status(clientId: str):
    data = request.get_json()
    print(data)
    print(clientId)
    return jsonify({
        "code": 200,
        "msg": "success",
        "data": {
            "timestamp": int(time.time())
        }
    })

@app.route("/voip/status/<clientId>", methods=["POST"])
def status(clientId: str):
    print(clientId)
    return jsonify({
        "code": 200,
        "msg": "success",
        "data": {
            "timestamp": int(time.time())
        }
    })

@app.route('/voip/dial_wav/<clientId>', methods=['POST'])
def upload_file(clientId: str):
    print(clientId)
    filename = request.headers.get('filename')
    if not filename:
        return jsonify({"error": "Missing Filename header"}), 400

    if not os.path.exists(UPLOAD_DIR):
        os.mkdir(UPLOAD_DIR)
    
    file_path = os.path.join(UPLOAD_DIR, filename)

    with open(file_path, 'wb') as f:
        f.write(request.data)

    return jsonify({"message": f"File '{filename}' uploaded successfully"}), 200


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
