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
        if file_path.endswith('.py'):
            subprocess.run(["python", file_path], check=True)
        else:
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
        if task['platform'].lower() == platform.system().lower() and task.get('enabled', True):
            schedule.every().day.at(task['time']).do(execute_task, task['file_path'])
            logging.info(f"Scheduled task: {task['name']} at {task['time']}")

if __name__ == "__main__":
    config = load_config('tasks.json')
    schedule_tasks(config)

    while True:
        schedule.run_pending()
        time.sleep(1)
