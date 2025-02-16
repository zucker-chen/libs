## 目的
python写一个定时执行任务的工具

## 工具说明
- **使用工具**：vscode、marscode（插件，豆包）
- **借鉴资源**：deepseek

## 提示词
用deepseek生成提示词的提示词
```text
写一份AI编程的提示词，提示词完成的任务如下：用python实现一个定时任务，比如每天早上9点执行指定文件（可以是python源码文件也可以是可执行文件），可以同时执行多个文件，不同文件可以定时不同时间，最好是配置文件方式可配，同时支持Windows环境和linux环境
```

## 生成的提示词
### 提示词：用Python实现跨平台的定时任务调度器

**任务描述：**
开发一个Python程序，能够根据配置文件中的设置，在指定的时间（例如每天早上9点）执行指定的文件（可以是Python源码文件或可执行文件）。该程序应支持同时执行多个文件，并且每个文件可以配置不同的执行时间。程序应兼容Windows和Linux环境。

**功能要求：**
1. **配置文件支持**：使用配置文件（如JSON、YAML或INI格式）来定义要执行的文件及其执行时间。
2. **跨平台支持**：确保程序在Windows和Linux环境下都能正常运行。
3. **定时任务调度**：能够根据配置的时间，定时执行指定的文件。
4. **多任务支持**：支持同时配置和执行多个文件，每个文件可以有不同的执行时间。
5. **日志记录**：记录任务的执行情况，包括成功、失败及错误信息。

**配置文件示例（JSON格式）：**
```json
{
    "tasks": [
        {
            "name": "task1",
            "file_path": "/path/to/script1.py",
            "time": "09:00",
            "platform": "linux"
        },
        {
            "name": "task2",
            "file_path": "C:\\path\\to\\executable2.exe",
            "time": "10:00",
            "platform": "windows"
        }
    ]
}



**代码实现提示：**
1. **配置文件解析**：使用`json`、`yaml`或`configparser`模块解析配置文件。
2. **定时任务调度**：使用`schedule`库或`APScheduler`库来实现定时任务调度。
3. **跨平台执行**：使用`subprocess`模块来执行文件，确保兼容Windows和Linux。
4. **日志记录**：使用`logging`模块记录任务的执行情况。


```python
import json
import schedule
import time
import subprocess
import logging
import platform

# 配置日志
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

def execute_task(file_path):
    """执行指定的文件"""
    try:
        logging.info(f"Executing task: {file_path}")
        subprocess.run(file_path, shell=True, check=True)
        logging.info(f"Task completed: {file_path}")
    except subprocess.CalledProcessError as e:
        logging.error(f"Task failed: {file_path} with error: {e}")

def load_config(config_file):
    """加载配置文件"""
    with open(config_file, 'r') as f:
        config = json.load(f)
    return config

def schedule_tasks(config):
    """根据配置文件调度任务"""
    for task in config['tasks']:
        if task['platform'].lower() == platform.system().lower():
            schedule.every().day.at(task['time']).do(execute_task, task['file_path'])
            logging.info(f"Scheduled task: {task['name']} at {task['time']}")

if __name__ == "__main__":
    config = load_config('tasks.json')
    schedule_tasks(config)

    while True:
        schedule.run_pending()
        time.sleep(1)

```

**运行说明：**
1. 将上述代码保存为`tasktimer.py`。
2. 创建配置文件`tasks.json`，按照示例格式配置任务。
3. 在Windows或Linux环境下运行`tasktimer.py`，程序将根据配置定时执行任务。

**注意事项：**
- 确保Python环境中安装了所需的库（如`schedule`、`APScheduler`等）。
- 在Windows环境下，路径使用双反斜杠`\\`或原始字符串`r"path\to\file"`。
- 在Linux环境下，确保文件具有可执行权限。
