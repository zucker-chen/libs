"""
This example describes how to use the workflow interface to stream chat.
"""
import json
import os
import sys
import json
from cozepy import COZE_CN_BASE_URL, Coze, TokenAuth, Stream, WorkflowEvent, WorkflowEventType  # noqa
import requests
import markdown
import html2text
import re

class CozeWorkflowHandler:
    def __init__(self, api_token, base_url, workflow_id, webhook_url):
        self.coze = Coze(auth=TokenAuth(token=api_token), base_url=base_url)
        self.workflow_id = workflow_id
        self.webhook_url = webhook_url

    def process_message(self, content):
        # 将 Markdown 转换为 HTML
        html_content = markdown.markdown(content)
        # 将 HTML 转换为纯文本
        h = html2text.HTML2Text()
        h.ignore_links = True
        h.ignore_images = True
        content = h.handle(html_content)
        # 去除多余的 Markdown 标记
        content = re.sub(r'\*\*([^*]+)\*\*', r'\1', content)
        # 去除每行开头的空格
        content = '\n'.join(line.lstrip() for line in content.splitlines())
        # 将连续多个换行符替换为单个换行符
        content = re.sub(r'\n+', '\n', content)
        return content

    def send_message_to_webhook(self, content):
        data = {
            "msgtype": "text",
            "text": {
                "content": content
            }
        }
        headers = {'Content-Type': 'application/json'}
        response = requests.post(self.webhook_url, headers=headers, json=data)
        if response.status_code == 200:
            print("消息发送成功")
        else:
            print("消息发送失败", response.text)

    def handle_workflow_iterator(self, stream: Stream[WorkflowEvent]):
        for event in stream:
            if event.event == WorkflowEventType.MESSAGE:
                content = event.message.content
                if content:
                    processed_content = self.process_message(content)
                    print("got message", processed_content)
                    self.send_message_to_webhook(processed_content)
            elif event.event == WorkflowEventType.ERROR:
                print("got error", event.error)
            elif event.event == WorkflowEventType.INTERRUPT:
                self.handle_workflow_iterator(
                    self.coze.workflows.runs.resume(
                        workflow_id=self.workflow_id,
                        event_id=event.interrupt.interrupt_data.event_id,
                        resume_data="hey",
                        interrupt_type=event.interrupt.interrupt_data.type,
                    )
                )

    def run_workflow(self, parameters):
        stream = self.coze.workflows.runs.stream(
            workflow_id=self.workflow_id,
            parameters=parameters,
        )
        self.handle_workflow_iterator(stream)

if __name__ == "__main__":
    # 获取脚本的执行路径
    executable_path = os.path.abspath(sys.argv[0])
    base_path = os.path.dirname(executable_path) if getattr(sys, 'frozen', False) else os.path.dirname(os.path.abspath(__file__))
    config_file_path = os.path.join(base_path, 'users.json')
    print(f"配置文件路径: {config_file_path}")

    # 读取配置文件
    try:
        with open(config_file_path, 'r', encoding='utf-8') as f:
            config = json.load(f)
    except FileNotFoundError:
        print("未找到 users.json 配置文件，请检查。")
        sys.exit(1)
    
    coze_api_token = config.get('coze_api_token')
    coze_api_base = COZE_CN_BASE_URL
    workflow_id = config.get('workflow_id')
    webhook_url = config.get('webhook_url')
    # Bug 修复：将字典转换为 JSON 字符串后再使用 json.loads
    parameters = json.loads(json.dumps(config.get('parameters')))

    handler = CozeWorkflowHandler(coze_api_token, coze_api_base, workflow_id, webhook_url)
    handler.run_workflow(parameters)
