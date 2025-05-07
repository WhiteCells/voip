from flask import Flask, jsonify

app = Flask(__name__)

@app.route("/get_account", methods=['GET'])
def get_account():
    return jsonify({
        "account": [
            {"name":"1", "password": "1"},
            {"name":"2", "password": "2"},
            {"name":"3", "password": "3"},
        ]
    })

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)