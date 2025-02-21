# 企微群机器人定时推送新闻项目说明

## 目的
实现企微群机器人定时推送新闻

## 工具说明
- **使用工具**：vscode、marscode（插件，豆包）
- **借鉴资源**：deepseek

## 编译执行文件
```bash
pyinstaller --onefile --name=wenewspush wenewspush.py
```

## 具体步骤

### 第一步：实现获取相关主题新闻应用
本文以 coze 工作流为例。

### 第二步：测试通过 API 能正常调用执行获取新闻工作流
示例如下：
```bash
20250214 扣子工作流 API 调用测试
curl -X POST 'https://api.coze.cn/v1/workflow/stream_run' \
-H "Authorization: Bearer pat_boNWeV5zPfnQAmKv3OFPUPvtPdtH49Az1iEkQxGJJbgfjyBxxZGoXjHqXDkxuksy" \
-H "Content-Type: application/json" \
-d '{
  "parameters": {
    "BOT_USER_INPUT": "图像"
  },
  "workflow_id": "7457158504424570891"
}'

```

### 第三步：
1. 创建机器人
在企微群创建机器人，然后会生成一个 Webhook 地址。

2. 推送群消息测试
```bash
curl 'https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=693axxx6-7aoc-4bc4-97a0-0ec2sifa5aaa' \
   -H 'Content-Type: application/json' \
   -d '
   {
        "msgtype": "text",
        "text": {
            "content": "hello world"
        }
   }'
```

#### 第四步：
    写脚本将工作流和信息推送串起来，然后再定时执行该任务即可
    `wenewspush.py`是对应功能的python代码
    其中定时功能需要格外服务实现
