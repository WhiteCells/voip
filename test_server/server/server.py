from flask import Flask, request, jsonify
import os
import time


app = Flask(__name__)
UPLOAD_DIR = "./uploads"
os.makedirs(UPLOAD_DIR, exist_ok=True)

# 返回客户端 ID
@app.route("/notify", methods=["POST"])
def notify():
    return jsonify({
        "code": 200,
        "msg": "success",
        "data": {
            "clientId": "00001"
        }
    }), 200

@app.route("/accounts/<clientId>", methods=["GET"])
def accounts(clientId: str):
    print(clientId)
    return jsonify({
        "code": 200,
        "msg": "success",
        "data": {
            "accounts": [
                {"user": "1001", "pass": "1001", "host": "192.168.10.62:5060"},
                {"user": "1002", "pass": "1002", "host": "192.168.2.243:5060"},
                {"user": "1003", "pass": "1003", "host": "192.168.10.63:5060"},
                # {"user": "1004", "pass": "1004", "host": "192.168.10.51:5060"},
                # {"user": "1005", "pass": "1005", "host": "192.168.10.51:5060"},
                # {"user": "1006", "pass": "1006", "host": "192.168.10.51:5060"},
            ]
        }
    })

@app.route("/dialplans/<clientId>", methods=["GET"])
def dialplans(clientId: str):
    print(clientId)
    return jsonify({
        "code": 200,
        "msg": "success",
        "data": {
            "dialplans": [
                "18871357225", 
                # "13329715728",
                # "13385281976",

                # "13385281971",
                # "13385281972",
                # "13385281973",
                # "813385281975",
                # "818871357225",

                # "13385281972",
                # "13385281971",
                # "13385281970",

                # "13831662418",
                # "18252353555",
                # "18434050770",
                # "18648184069",
                # "13661601089",
                # "13171378333", # 空号没有提示音

                # "813831662418",
                # "818252353555",
                # "818434050770",
                # "818648184069",
                # "813661601089",
                # "813171378333",

                # "100613831662418",
                # "100618252353555",
                # "100618434050770",
                # "100618648184069",
                # "100613661601089",
                # "100613171378333",
            ]
        }
    })

@app.route("/dialplans2/<clientId>", methods=["GET"])
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

@app.route("/heartbeat/<clientId>", methods=["POST"])
def heartbeat(clientId: str):
    print(clientId)
    return jsonify({
        "code": 200,
        "msg": "success",
        "data": {
            "timestamp": int(time.time())
        }
    })

@app.route("/status/<clientId>", methods=["POST"])
def status(clientId: str):
    print(clientId)
    return jsonify({
        "code": 200,
        "msg": "success",
        "data": {
            "timestamp": int(time.time())
        }
    })

@app.route('/dial_wav/<clientId>', methods=['POST'])
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
