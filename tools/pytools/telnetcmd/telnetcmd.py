import os
import re
import threading
import telnetlib
import tkinter as tk
from tkinter import ttk, messagebox
import xml.etree.ElementTree as ET
from time import sleep


class ConfigManager:
    """配置文件管理类"""
    def __init__(self):
        self.config_path = os.path.join(os.path.dirname(__file__), "cmd.xml")
        self.tree = ET.parse(self.config_path)
        self.root = self.tree.getroot()


    def get(self, field):
        """获取配置字段值"""
        elem = self.root.find(field)
        return elem.text.strip() if elem is not None else ""


    def get_commands(self):
        """获取命令列表"""
        commands = self.root.find("commands").text
        return [cmd.strip() for cmd in commands.split("\n") if cmd.strip()]


class TelnetClient:
    """Telnet 通信功能类"""
    def __init__(self, config):
        self.config = config
        self.tn = None
        self.prompt = ""
        self.ansi_escape = re.compile(r'\x1B\[[0-?]*[ -/]*[@-~]')


    def connect(self, host, port, timeout=2):
        """建立 Telnet 连接"""
        try:
            self.tn = telnetlib.Telnet(host, port, timeout)
            self._login()
            return True, "连接成功"
        except Exception as e:
            return False, f"连接失败: {str(e)}"


    def _login(self):
        """处理登录认证"""
        self.tn.read_until(b"login: ", timeout=5)
        self.tn.write(self.config.get("username").encode() + b"\n")
        self.tn.read_until(b"Password: ", timeout=5)
        self.tn.write(self.config.get("password").encode() + b"\n")
        sleep(0.3)  # 等待返回信息
        login_output = self.tn.read_very_eager().decode()
        self.prompt = self._clean_output(login_output.splitlines()[-1])


    def execute_commands(self, commands):
        """执行命令并返回结果"""
        output = []
        for cmd in commands:
            try:
                self.tn.write(cmd.encode() + b"\n")
                result = self._read_until_prompt()
                output.append(self._clean_output(result))
            except Exception as e:
                output.append(f"命令执行错误: {str(e)}")
        return "".join(output)


    def _read_until_prompt(self):
        """读取直到出现提示符"""
        output = []
        output = self.tn.read_until(self.prompt.encode('utf-8')).decode('utf-8', errors='ignore')
        return output


    def _clean_output(self, text):
        """清理特殊字符和空行"""
        text = self.ansi_escape.sub('', text)
        return text #.replace('\r', '').replace('\n', '').strip()


    def close(self):
        """关闭连接"""
        if self.tn:
            self.tn.close()


class TelnetGUI:
    """GUI 界面类"""
    def __init__(self):
        self.config = ConfigManager()
        self.telnet = TelnetClient(self.config)
        self.window = tk.Tk()
        self._setup_window()
        self._create_widgets()
        self.is_running = False


    def _setup_window(self):
        """初始化窗口设置"""
        self.window.title(self.config.get("window_title"))
        self.window.geometry("800x600")
        self.window.minsize(600, 400)
        self.window.columnconfigure(0, weight=1)
        self.window.rowconfigure(2, weight=1)


    def _create_widgets(self):
        """创建界面组件"""
        # IP 地址输入行
        ip_frame = ttk.Frame(self.window)
        ip_frame.grid(row=0, column=0, padx=10, pady=5, sticky="ew")
        ip_frame.columnconfigure(1, weight=1)


        ttk.Label(ip_frame, text="目标IP地址:").grid(row=0, column=0, sticky="w")
        self.ip_entry = ttk.Entry(ip_frame)
        self.ip_entry.grid(row=0, column=1, sticky="ew", padx=5)
        self.ip_entry.insert(0, self.config.get("ip_address"))


        self.exec_btn = ttk.Button(ip_frame, text="执行", command=self._execute)
        self.exec_btn.grid(row=0, column=2, padx=5)


        # 结果显示区域
        result_frame = ttk.LabelFrame(self.window, text="执行结果")
        result_frame.grid(row=2, column=0, padx=10, pady=5, sticky="nsew")
        result_frame.columnconfigure(0, weight=1)
        result_frame.rowconfigure(0, weight=1)


        self.result_text = tk.Text(result_frame, wrap=tk.WORD, state="disabled")
        self.result_text.grid(row=0, column=0, sticky="nsew")


        scrollbar = ttk.Scrollbar(result_frame, command=self.result_text.yview)
        scrollbar.grid(row=0, column=1, sticky="ns")
        self.result_text.config(yscrollcommand=scrollbar.set)


    def _execute(self):
        """执行按钮点击事件"""
        if self.is_running:
            return


        def run():
            self.is_running = True
            self.exec_btn.config(text="执行中...", state="disabled")
            self._clear_result()
           
            # 验证输入
            ip = self.ip_entry.get()
            if not self._validate_ip(ip):
                self._show_error("无效的IP地址格式")
                return


            try:
                port = int(self.config.get("port"))
                if not (1 <= port <= 65535):
                    raise ValueError
            except:
                self._show_error("端口号无效")
                return


            # 建立连接
            success, msg = self.telnet.connect(ip, port)
            self._append_result(msg)
           
            if success:
                # 执行命令
                commands = self.config.get_commands()
                result = self.telnet.execute_commands(commands)
                self._append_result("\n" + result)
           
            self.telnet.close()
            self.exec_btn.config(text="执行", state="normal")
            self.is_running = False


        threading.Thread(target=run, daemon=True).start()


    def _validate_ip(self, ip):
        """验证IP地址格式"""
        pattern = r"^(\d{1,3}\.){3}\d{1,3}$"
        return re.match(pattern, ip) is not None


    def _append_result(self, text):
        """追加结果到文本框"""
        self.window.after(0, self._update_result, text + "\n")


    def _update_result(self, text):
        """线程安全更新结果"""
        self.result_text.config(state="normal")
        self.result_text.insert(tk.END, text)
        self.result_text.see(tk.END)
        self.result_text.config(state="disabled")


    def _clear_result(self):
        """清空结果框"""
        self.result_text.config(state="normal")
        self.result_text.delete(1.0, tk.END)
        self.result_text.config(state="disabled")


    def _show_error(self, msg):
        """显示错误信息"""
        self.window.after(0, messagebox.showerror, "错误", msg)
        self.exec_btn.config(text="执行", state="normal")
        self.is_running = False


    def run(self):
        """运行主循环"""
        self.window.mainloop()


if __name__ == "__main__":
    app = TelnetGUI()
    app.run()