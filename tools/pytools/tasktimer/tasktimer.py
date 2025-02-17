import json
import schedule
import time
import subprocess
import logging
import platform
import sys
import os

# 配置日志
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

def execute_task(file_path):
    """执行指定的文件"""
    try:
        logging.info(f"Executing task: {file_path}")
        if file_path.endswith('.py'):
            subprocess.run(["python", file_path], check=True)
        else:
            subprocess.run(file_path, shell=True, check=True)
        logging.info(f"Task completed: {file_path}")
    except subprocess.CalledProcessError as e:
        logging.error(f"Task failed: {file_path} with error: {e}")

def load_config():
    """加载配置文件"""
    # 始终使用当前目录下的 tasks.json 文件
    config_file = os.path.join(os.path.dirname(sys.executable), 'tasks.json') if getattr(sys, 'frozen', False) else 'tasks.json'
    if not os.path.exists(config_file):
        logging.error(f"Config file {config_file} not found.")
        raise FileNotFoundError(f"Config file {config_file} not found.")
    with open(config_file, 'r') as f:
        config = json.load(f)
    return config

def schedule_tasks(config):
    """根据配置文件调度任务"""
    for task in config['tasks']:
        if task['platform'].lower() == platform.system().lower() and task.get('enabled', True):
            if task['frequency'] == 'day':
                schedule.every().day.at(task['time']).do(execute_task, task['file_path'])
                logging.info(f"Scheduled daily task: {task['name']} at {task['time']}")
            elif task['frequency'] == 'week':
                day_mapping = {
                    "Monday": schedule.every().monday,
                    "Tuesday": schedule.every().tuesday,
                    "Wednesday": schedule.every().wednesday,
                    "Thursday": schedule.every().thursday,
                    "Friday": schedule.every().friday,
                    "Saturday": schedule.every().saturday,
                    "Sunday": schedule.every().sunday
                }
                day = task['day']
                if day in day_mapping:
                    day_mapping[day].at(task['time']).do(execute_task, task['file_path'])
                    logging.info(f"Scheduled weekly task: {task['name']} on {day} at {task['time']}")
            elif task['frequency'] == 'month':
                # 注意：schedule 库本身不直接支持每月调度，这里只是示例，实际可能需要更复杂的实现
                logging.warning(f"Monthly scheduling is not fully supported by schedule library. Task {task['name']} may not work as expected.")
                # 简单示例：假设每月 1 - 31 日都有任务，这里只是占位
                for day in range(1, 32):
                    if day == task['day']:
                        # 这里只是示例，实际需要更复杂的逻辑来处理每月调度
                        schedule.every().day.at(task['time']).do(execute_task, task['file_path'])
                        logging.info(f"Scheduled monthly task: {task['name']} on day {day} at {task['time']}")

if __name__ == "__main__":
    config = load_config()
    schedule_tasks(config)

    while True:
        schedule.run_pending()
        time.sleep(1)
