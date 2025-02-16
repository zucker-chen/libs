import tkinter as tk
from tkinter import scrolledtext
import telnetlib
import re
import ipaddress
import time
import threading


class TelnetClient:
    def __init__(self):
        self.tn = None
        self.prompt = ""
        self.lock = threading.Lock()


    def connect(self, ip, port, username, password):
        with self.lock:
            if self.tn:
                return "已经连接，请勿重复连接。"


            try:
                self.tn = telnetlib.Telnet(ip, port, timeout=2)
                self.tn.read_until(b"login: ")
                self.tn.write(username.encode('utf-8') + b"\n")
                self.tn.read_until(b"Password: ")
                self.tn.write(password.encode('utf-8') + b"\n")


                time.sleep(0.3)
                result = self.tn.read_very_eager().decode('utf-8', errors='ignore')
                result = re.sub(r'\x1b\[[0-9;]*[a-zA-Z]', '', result)
                self.prompt = result.replace('\r', '').replace('\n', '')


                if "Login incorrect" in result:
                    self.tn.close()
                    self.tn = None
                    return "认证失败，请检查用户名和密码。"
                else:
                    self.tn.write(b"\n")
                    result = self.tn.read_until(self.prompt.encode('utf-8')).decode('utf-8', errors='ignore')
                    result = re.sub(r'\x1b\[[0-9;]*[a-zA-Z]', '', result)
                    return "连接成功\n" + result
            except Exception as e:
                if self.tn:
                    self.tn.close()
                    self.tn = None
                return f"连接失败: {str(e)}"


    def disconnect(self):
        with self.lock:
            if self.tn:
                try:
                    self.tn.close()
                    self.tn = None
                    return "连接已断开"
                except Exception as e:
                    return f"断开连接时出错: {str(e)}"


    def send_command(self, command):
        with self.lock:
            if self.tn:
                try:
                    self.tn.write(command.encode('utf-8'))
                    result = self.tn.read_until(self.prompt.encode('utf-8')).decode('utf-8', errors='ignore')
                    result = re.sub(r'\x1b\[[0-9;]*[a-zA-Z]', '', result)
                    result = result.replace(command.strip(), '', 1)
                    return result
                except Exception as e:
                    return f"命令执行出错: {str(e)}"


class TelnetClientApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Telnet 客户端")


        input_frame = tk.Frame(self.root)
        input_frame.pack(pady=10, fill=tk.X)


        tk.Label(input_frame, text="IP 地址:").grid(row=0, column=0, padx=5)
        self.ip_entry = tk.Entry(input_frame)
        self.ip_entry.insert(0, "172.20.122.66")
        self.ip_entry.grid(row=0, column=1, padx=5, sticky=tk.EW)


        tk.Label(input_frame, text="端口号:").grid(row=0, column=2, padx=5)
        self.port_entry = tk.Entry(input_frame)
        self.port_entry.insert(0, "23")
        self.port_entry.grid(row=0, column=3, padx=5, sticky=tk.EW)


        tk.Label(input_frame, text="用户名:").grid(row=1, column=0, padx=5)
        self.username_entry = tk.Entry(input_frame)
        self.username_entry.insert(0, "root")
        self.username_entry.grid(row=1, column=1, padx=5, sticky=tk.EW)


        tk.Label(input_frame, text="密码:").grid(row=1, column=2, padx=5)
        self.password_entry = tk.Entry(input_frame, show="*")
        self.password_entry.insert(0, "4001862505")
        self.password_entry.grid(row=1, column=3, padx=5, sticky=tk.EW)


        input_frame.columnconfigure(1, weight=1)
        input_frame.columnconfigure(3, weight=1)


        self.connect_button = tk.Button(self.root, text="连接", command=self.connect)
        self.connect_button.pack(pady=10)


        self.output_text = scrolledtext.ScrolledText(self.root, height=20, width=80)
        self.output_text.pack(pady=10, fill=tk.BOTH, expand=True)
        self.output_text.bind("<Return>", self.send_command)


        self.telnet_client = TelnetClient()


    def connect(self):
        ip = self.ip_entry.get()
        try:
            port = int(self.port_entry.get())
        except ValueError:
            self.display_message("端口号必须是整数。")
            return
        username = self.username_entry.get()
        password = self.password_entry.get()


        try:
            ipaddress.ip_address(ip)
        except ValueError:
            self.display_message("无效的 IP 地址。")
            return


        if not (0 <= port <= 65535):
            self.display_message("端口号必须在 0 到 65535 之间。")
            return


        self.connect_button.config(text="连接中", state=tk.DISABLED)


        threading.Thread(target=self._connect_thread, args=(ip, port, username, password)).start()


    def _connect_thread(self, ip, port, username, password):
        result = self.telnet_client.connect(ip, port, username, password)
        self.display_message(result)
        if "连接成功" in result:
            self.connect_button.config(text="断开连接", command=self.disconnect, state=tk.NORMAL)
        else:
            self.connect_button.config(text="连接", state=tk.NORMAL)


    def disconnect(self):
        result = self.telnet_client.disconnect()
        self.display_message(result)
        self.connect_button.config(text="连接", command=self.connect)


    def send_command(self, event):
        if self.telnet_client.tn:
            input_text = self.output_text.get("1.0", tk.END)
            if self.telnet_client.prompt in input_text:
                command = input_text.split(self.telnet_client.prompt)[-1].strip() + "\n"
            else:
                command = input_text.strip() + "\n"


            threading.Thread(target=self._send_command_thread, args=(command,)).start()


    def _send_command_thread(self, command):
        result = self.telnet_client.send_command(command)
        self.display_message(result)


    def display_message(self, message):
        self.output_text.insert(tk.END, message)
        self.output_text.see(tk.END)


if __name__ == "__main__":
    root = tk.Tk()
    app = TelnetClientApp(root)
    root.mainloop()