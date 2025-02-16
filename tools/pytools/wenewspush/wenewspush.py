"""
This example describes how to use the workflow interface to stream chat.
"""
import json
# 删除未使用的导入
# import os
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
    # Get an access_token through personal access token or oauth.
    coze_api_token = 'pat_WZDsnk7GTKHJxIsU6WaVpLwGFgpPyqNSJy4fn2nIdS8TKOzA5HF7WYgmZkE77Ggw'
    # The default access is api.coze.com, but if you need to access api.coze.cn,
    # please use base_url to configure the api endpoint to access
    coze_api_base = COZE_CN_BASE_URL
    # Create a workflow instance in Coze, copy the last number from the web link as the workflow's ID.
    workflow_id = '7457158504424570891'
    webhook_url = 'https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=ba0dd709-34cf-4a89-b975-455c7e2189e6'
    # Bug 修复：将字典转换为 JSON 字符串后再使用 json.loads
    parameters = json.loads(json.dumps({"BOT_USER_INPUT": "(相机、AI大模型)"}))

    handler = CozeWorkflowHandler(coze_api_token, coze_api_base, workflow_id, webhook_url)
    handler.run_workflow(parameters)
    