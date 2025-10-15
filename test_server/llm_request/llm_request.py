import requests
import json
from flask import Flask, request, jsonify
import re

app = Flask(__name__)

class LLMRequestHandler:
    def __init__(self):
        self.url = "http://192.168.2.3/v1/chat-messages"
        self.headers = {
            'Authorization': 'Bearer app-5wlJJ0WDoyKpZ6t460pb4H3m',
            'Content-Type': 'application/json;charset=utf-8'
        }
        self.history = ""
        self.llm_answer = ""

    @staticmethod
    def convert_history(history):
        try:
            return json.dumps(history, ensure_ascii=False)
        except Exception:
            return "[]"

    def llm_request_config(self, history,userinformation,query,response_mode,user,conversation_id=None):

        def split_text(text):
            """
            根据中文和英文标点符号分句
            """
            sentences = re.split(r'(?<=[。！？!?,，；;])', text)
            sentences = [s.strip() for s in sentences if s.strip()]
            return sentences

        # print("历史信息",history)
        payload = json.dumps({
            "inputs": {
                "history": history,
                "userinformation": userinformation
            },
            "query": query,
            "response_mode": response_mode,
            "conversation_id":conversation_id,
            "user": user
        })

        response = requests.request("POST", self.url, headers=self.headers, data=payload)
        response = response.json()
        answer = response['answer']
        test = split_text(answer)
        self.llm_answer =  test

        # print("LLM返回结果:", test)

        if not self.history:
            self.history = []

        self.history.append({"role": "client", "text": query})
        self.history.append({"role": "server", "text": answer})
        print("历史信息:", self.history)

    def get_request(self, query):
        userinformation = "\"王福建\":\"姓名:王福建;身份证:350124199012022***;年龄:32;性别:男;生日:1990-12-2;手机号:183****5950;逾期金额:3155.81;逾期天数:2057;合同数:1;户籍地址:福建省福州市永泰县城峰镇板桥路238号\""
        history_test = self.convert_history(self.history)

        try:
            # 调用 llm_request 方法获取响应
            self.llm_request_config(
                query=query,
                history=history_test,
                userinformation=userinformation,
                response_mode="blocking",
                user="abc-123"
            )

        except Exception as e:
            print(f"调用LLM出错: {e}")
            return None

llm_request_handler = LLMRequestHandler()
@app.route('/api/llm_request', methods=['POST'])
def llm_request():
    try:
        data = request.get_json()
        user_status = data.get('status')
        user_text = data.get('user_text')

        if user_status == "reset":
            llm_request_handler.history = []

        if user_text == '':
            return jsonify({'message': 'carry off', 'data': []})
        else:
            llm_request_handler.get_request(user_text)

        return jsonify({'message': 'carry off', 'data': llm_request_handler.llm_answer})
    except Exception:
        return jsonify({'error': 'Invalid JSON data'}), 400


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=50000)

