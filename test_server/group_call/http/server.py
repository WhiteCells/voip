from flask import Flask, request, jsonify

app = Flask(__name__)

@app.route('/voip/call/status/<client_id>', methods=['POST'])
def receive_call_status(client_id):
    # print(client_id)
    result = {
        "client_id": client_id,
        "status": "200"
              }
    return jsonify(result)
    # try:
    #     data = request.get_json()
    #     print(f"收到客户端 {client_id} 推送的数据：{data}")
    #
    #     # 检查必填字段
    #     required_fields = ["task_id", "phone", "status", "call_type"]
    #     if not all(field in data for field in required_fields):
    #         return jsonify({"error": "缺少必要字段"}), 400
    #
    #
    #     return jsonify({}), 200
    #
    # except Exception as e:
    #     return jsonify({"error": str(e)}), 500

@app.route('/voip/account/status/<client_id>', methods=['POST'])
def receive_account_status(client_id):
    result = {
        "client_id": client_id,
        "status": "200"
              }
    return jsonify(result)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=9999)
