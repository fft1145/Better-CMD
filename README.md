# Better CMD (bcmd) v26.9.27.1

> 由 **fft工作室 - Fungame Craft 总项目组** 开发的 Windows 增强命令行工具。
>
> A Windows enhanced command-line tool developed by **fft Studio - Fungame Craft Main Project Group**.

---

## 📖 项目简介 / Overview

**中文**

Better CMD（简称 bcmd）是对 Windows 原生 `cmd` 的全面增强重写，使用 **C++17** 编写，单文件可编译，同时提供：

- **传统控制台交互模式**：保留 cmd 习惯命令，输入即用；
- **GUI 按钮界面**：把常用命令做成可视化按钮，鼠标点选即可执行，零学习成本；
- **中英双语支持**：内置 `Lang` 多语言模块，运行时切换。

**English**

Better CMD (bcmd) is a complete re-implementation and enhancement of the native Windows `cmd`. It is written in **C++17**, compiled from a single source file, and offers:

- **Traditional console mode**: keeps familiar cmd commands, type and run;
- **GUI button interface**: turns common commands into clickable buttons, zero learning curve;
- **Bilingual support (ZH / EN)**: built-in `Lang` module, switchable at runtime.

---

## ✨ 功能特性 / Features

Better CMD 内置 100+ 条命令，覆盖文件、网络、系统、进程、媒体、加密、抓包等场景。
Better CMD ships with 100+ commands covering files, networking, system, processes, media, cryptography, packet capture and more.

| 类别 / Category | 命令示例 / Example commands |
| --- | --- |
| 文件 / 目录 · File / Dir | `dir` `cd` `md` `rd` `del` `copy` `move` `ren` `type` `tree` `find` `more` `sort` `fc` |
| 网络 · Network | `ping` `tracert` `nslookup` `ipconfig` `netstat` `route` `arp` `getmac` `whois` `dig` `portscan` `ssh` `wget` `httpserver` `proxy` |
| 系统 / 进程 · System / Process | `systeminfo` `tasklist` `taskkill` `shutdown` `chkdsk` `vol` `service` `startup` `hwinfo` `sysmon` `diskusage` `procmon` `schedule` `logview` |
| 计算与编码 · Math / Encoding | `calc` `rand` `scicalc` `base64` `md5` `sha256` `uuid` `passgen` `checksum` `hexdump` `json` `regex` `convert` |
| 媒体 / 实用工具 · Media / Utilities | `screenshot` 截图、`record` 录音、`picture` 截图录屏、`translate` 翻译、`weather` 天气、`colorpick` 取色、`qrcode` 二维码、`barcode` 条形码、`vcard` 名片、`osk` 屏幕键盘、`rdp` 远程桌面、`ftp`、`sql` SQLite、`sendmail` 邮件、`chat` 聊天、`ai` AI 对话 |
| 文本处理 · Text processing | `split` 分割、`merge` 合并、`wc` 统计、`linesort` 行排序、`uniq` 去重、`dirdiff` 目录比较、`tail` `head` |
| 压缩 / 加密 · Archive / Crypto | `zip` `unzip` `encrypt` `decrypt` `sync` 同步 |
| 抓包 · Packet capture | `BP`（基于 pktmon，需管理员 / based on pktmon, requires admin） |
| 其他 · Others | `clip` 剪贴板、`timer` 倒计时、`todo` 任务、`notes` 笔记、`alias` 别名、`bench` 性能测试、`ide`/`edit` 用 IDE 打开、`author` 关于、`help` 帮助、`exit` 退出 |

> 危险命令如 `format` 已被显式拦截阻止，避免误操作。
> Dangerous commands such as `format` are explicitly blocked to prevent accidents.

---

## 🔐 授权与防盗版机制 / Authorization & Anti-Piracy

**中文**

本程序**必须先运行前置文件 `bcmd_prerequisite_files.exe`** 再启动主程序 `bcmd.exe`：

1. 前置文件读取本机局域网 IP + 当日日期，生成一个**随机密钥**写入本地 `miyao.txt`；
2. 程序退出 / 窗口关闭 / Ctrl+C 时自动删除该密钥文件；
3. 密钥同时存于**服务端**，主程序启动时联网校验；
4. 密钥每次随机生成，作者本人亦无法预知；作者持有的"万能密钥"仅在其 IP 下生效。

> 因此**请勿尝试解包逆向**，没有合法密钥将无法通过校验。

**English**

You **must run the prerequisite file `bcmd_prerequisite_files.exe` first** before launching the main program `bcmd.exe`:

1. The prerequisite reads the local LAN IP + the current date, generates a **random key** and writes it to a local file `miyao.txt`;
2. The key file is automatically deleted when the program exits / window closes / Ctrl+C;
3. The key is also stored **server-side**, and the main program verifies it online at startup;
4. The key is regenerated randomly each time, even the author cannot predict it; the author's "master key" only works under the author's IP.

> Therefore **do NOT attempt to unpack or reverse-engineer** the files — without a valid key, verification will fail.

---

## 📁 目录结构 / Directory Structure

```
fgame.cc/
├── bcmd_prerequisite_files.cpp / .exe   # 前置文件：生成随机密钥 / Prerequisite: generates random key
├── build_pre.bat                         # 编译前置文件 / Build the prerequisite
├── BetterCMD/project/
│   ├── bcmd.cpp / .exe                   # 主程序源码与可执行文件 / Main source & executable
│   ├── build_main.bat                    # 主程序编译脚本 / Build script for main program
│   └── 预览bcmd.html                     # GUI 预览 / GUI preview
└── 使用教程/                              # 中文版 / English 使用说明 / Tutorials
```

---

## 🛠️ 编译方式 / Build (MinGW / MSYS2)

```bash
g++ -std=c++17 -mwindows -static -O2 -m64 bcmd.cpp -o bcmd.exe \
    -lws2_32 -liphlpapi -lpsapi -lwinhttp -lshlwapi -lwinmm -lcomctl32
```

- 中文：调试时去掉 `-mwindows` 即可看到控制台输出。日志位于 `fgamecc/main/logs/bcmd_gui.log`。
- English: Remove `-mwindows` for debugging to see console output. Logs are written to `fgamecc/main/logs/bcmd_gui.log`.

---

## 🚀 使用步骤 / Usage

**中文**

1. 先运行 `bcmd_prerequisite_files.exe` 获取密钥；
2. 再运行 `BetterCMD/project/bcmd.exe` 启动主程序；
3. 输入 `help` 查看全部命令，或直接点 GUI 按钮操作。

**English**

1. Run `bcmd_prerequisite_files.exe` first to obtain the key;
2. Then run `BetterCMD/project/bcmd.exe` to launch the main program;
3.类型`help` to list all commands, or simply click the GUI buttons.

---

## 💻 平台 / Platform

- 中文：仅支持 **Windows**（依赖 Win32 API / WinHTTP / pktmon 等）。
  
        目前的项目文件（exe）仅支持**x64的Windows系统**，如有除此之外的系统，请自行编译！
- English: **Windows only** (relies on Win32 API / WinHTTP / pktmon, etc.).
  
           The current project files (exe) only support **x64 Windows systems**. If you have a different system, please compile it yourself!
---

## 👤 作者 / Author

fft工作室 - Fungame Craft 总项目组
fft Studio - Fungame Craft Main Project Group
https://fgame12.netlify.app

---

## 📜 许可 / License

- 中文：本项目为开源软件。
- English: This is opened-source software.
