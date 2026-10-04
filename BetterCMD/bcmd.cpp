/*================================================================
 Better CMD Ultimate v26.10.1 - 控制台 + GUI 按钮界面
================================================================
 开发团队 : fft工作室 - Fungame Craft总项目组
 编译命令 :
   g++ -std=c++17 -mwindows -static -O2 -m64 bcmd.cpp -o bcmd.exe
       -lws2_32 -liphlpapi -lpsapi -lwinhttp -lshlwapi -lwinmm -lcomctl32

   如果要看控制台输出（调试用），去掉 -mwindows 参数即可。

 日志位置 : log/bcmd.log （相对 exe 所在目录，启动时询问是否保存）
 密钥位置 : miyao.txt （相对 exe 所在目录）
================================================================


================================================================
 第一部分：头文件
================================================================
 注意（MinGW/MSYS2 必看）：
   1. Windows 头文件必须先于 C++ 标准库头文件包含，否则 <cstddef> 里的
      std::byte 与 <rpcndr.h> 里的 typedef unsigned char byte 在 ADL 下
      歧义，导致 <objidl.h>/<wtypes.h> 等大量报 "reference to 'byte' is
      ambiguous" 错误。
   2. winsock2.h 必须在 windows.h 之前包含，否则会与 windows.h 拉入的
      winsock1 冲突。
   3. 不再包含 <wininet.h>，因为它与 <winhttp.h> 在 INTERNET_SCHEME /
      HTTP_VERSION_INFO / URL_COMPONENTS 等类型上冲突。
================================================================
*/
#ifdef _WIN32
// 这些宏必须在 windows.h 之前定义
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// 目标 Windows 7+，启用 inet_ntop 等 API
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
// winsock2.h 必须先于 windows.h
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commctrl.h>
#include <iphlpapi.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <winreg.h>
#include <shellapi.h>
#include <winhttp.h>
#include <powrprof.h>
#include <wtsapi32.h>
#include <aclapi.h>
#include <sddl.h>
#include <mmsystem.h>
#include <shlwapi.h>
#include <dwmapi.h>
#include <conio.h>
#include <richedit.h>
// 注意：不再 #include <wininet.h>，它与 <winhttp.h> 的类型定义冲突
// 注意：#pragma comment(lib, ...) 是 MSVC 专用，MinGW GCC 会忽略，
//       必须在编译命令行用 -l 显式链接（见文件头注释的编译命令）。
#define POPEN _popen
#define PCLOSE _pclose
#endif // _WIN32

// C++ 标准库头文件（必须在 windows.h 之后，避免 std::byte / ::byte 歧义）
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <array>
#include <tuple>
#include <memory>
#include <functional>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <random>
#include <regex>
#include <cctype>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <system_error>

using namespace std;
namespace fs = std::filesystem;

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <termios.h>
#include <utime.h>
#include <ifaddrs.h>
#define POPEN popen
#define PCLOSE pclose
#endif

#define VERSION "26.10.1"
#define BP_FOLDER "BP.cog"

/*
================================================================
 第二部分：多语言支持
================================================================
*/
class Lang {
public:
    enum Code { ZH, EN };
    static Code cur;
    static unordered_map<string, pair<string, string>> dict;
    static void init() {
        // ---- 通用界面 ----
        dict["welcome"]       = {"欢迎使用 Better CMD v" VERSION, "Welcome to Better CMD v" VERSION};
        dict["unknown"]       = {"未知命令", "Unknown command"};
        dict["author"]        = {"本款软件（Better CMD）由 fft工作室 - Fungame Craft总项目组 制作",
                                  "Developed by fft Studio - Fungame Craft Main Project Group"};
        dict["lang_prompt"]   = {"1.中文 2.English: ", "1.Chinese 2.English: "};
        dict["tab_exit"]      = {"Tab 补全，exit 退出。", "Tab to autocomplete, exit to quit."};
        dict["exception"]     = {"异常: ", "Exception: "};
        dict["unknown_ex"]    = {"未知异常", "Unknown exception"};
        dict["none"]          = {"(无)", "(none)"};
        dict["alias_mark"]    = {"[别名]", "[alias]"};

        // ---- 抓包 BP ----
        dict["bp_start"]      = {"[BP] 使用 pktmon 抓包... (需要管理员)", "[BP] Capturing with pktmon... (admin required)"};
        dict["bp_stop"]       = {"[BP] 停止抓包并转换", "[BP] Stop and convert"};
        dict["bp_list"]       = {"[BP] 已捕获文件:", "[BP] Captured files:"};
        dict["bp_no_admin"]   = {"[BP] 需要管理员权限", "[BP] Admin required"};
        dict["bp_running"]    = {"[BP] 已在抓包中", "[BP] Already capturing"};
        dict["bp_start_fail"] = {"[BP] pktmon 启动失败", "[BP] pktmon failed to start"};
        dict["bp_not_run"]    = {"[BP] 未在抓包", "[BP] Not capturing"};

        // ---- 依赖提示 ----
        dict["need_py"]       = {"需要 Python", "Python required"};
        dict["need_ffmpeg"]   = {"需要 ffmpeg", "ffmpeg required"};
        dict["need_sqlite"]   = {"需要 sqlite3.exe", "sqlite3.exe required"};
        dict["need_trans"]    = {"需要 translate-shell", "translate-shell required"};

        // ---- 通用错误 ----
        dict["path_noexist"]  = {"路径不存在: ", "Path not found: "};
        dict["list_fail"]     = {"无法列出目录: ", "Cannot list directory: "};
        dict["cd_fail"]       = {"切换失败: ", "Cannot change dir: "};
        dict["mkdir_fail"]    = {"创建失败: ", "Create failed: "};
        dict["del_fail"]      = {"删除失败: ", "Delete failed: "};
        dict["copy_fail"]     = {"复制失败: ", "Copy failed: "};
        dict["move_fail"]     = {"移动失败: ", "Move failed: "};
        dict["rename_fail"]   = {"重命名失败: ", "Rename failed: "};
        dict["open_fail"]     = {"无法打开: ", "Cannot open: "};
        dict["traverse_fail"] = {"无法遍历: ", "Cannot traverse: "};
        dict["format_blocked"]= {"[!] 格式化已被阻止", "[!] format has been blocked"};
        dict["invalid_num"]   = {"数值无效", "Invalid number"};
        dict["invalid_arg"]   = {"参数无效: ", "Invalid argument: "};

        // ---- 用法提示 ----
        dict["use_copy"]      = {"用法: copy 源 目标", "Usage: copy source target"};
        dict["use_move"]      = {"用法: move 源 目标", "Usage: move source target"};
        dict["use_color"]     = {"用法: color <bg><fg> 例如 color 0A", "Usage: color <bg><fg> e.g. color 0A"};
        dict["use_calc"]      = {"用法: calc x op y (op: + - * /)", "Usage: calc x op y (op: + - * /)"};
        dict["use_base64"]    = {"用法: base64 -e/-d \"内容\"", "Usage: base64 -e/-d \"text\""};
        dict["use_qrcode"]    = {"用法: qrcode 内容", "Usage: qrcode text"};
        dict["use_json"]      = {"用法: json 文件名", "Usage: json filename"};
        dict["use_wget"]      = {"用法: wget URL", "Usage: wget URL"};
        dict["use_encrypt"]   = {"用法: encrypt file pwd", "Usage: encrypt file password"};
        dict["use_decrypt"]   = {"用法: decrypt file.enc pwd", "Usage: decrypt file.enc password"};
        dict["use_sendmail"]  = {"用法: sendmail -to addr -subj \"subject\" -body \"body\" -smtp server", "Usage: sendmail -to addr -subj \"subject\" -body \"body\" -smtp server"};
        dict["use_chat"]      = {"用法: chat host port (telnet or PuTTY)", "Usage: chat host port (telnet or PuTTY)"};
        dict["use_convert"]   = {"用法: convert 值 原单位 目标单位", "Usage: convert value from_unit to_unit"};
        dict["use_barcode"]   = {"用法: barcode 内容", "Usage: barcode text"};
        dict["use_split"]     = {"用法: split 文件 每份MB 输出前缀", "Usage: split file MB_per_part prefix"};
        dict["use_merge"]     = {"用法: merge 输出文件 分片1 分片2 ...", "Usage: merge output part1 part2 ..."};
        dict["use_tail"]      = {"用法: tail [-n N] 文件", "Usage: tail [-n N] file"};
        dict["use_head"]      = {"用法: head [-n N] 文件", "Usage: head [-n N] file"};
        dict["use_ai"]        = {"用法: ai 问题", "Usage: ai question"};

        // ---- 更多通用消息 ----
        dict["empty_content"] = {"内容为空", "Content is empty"};
        dict["match"]         = {"匹配", "Match"};
        dict["no_match"]      = {"不匹配", "No match"};
        dict["bad_regex"]     = {"无效正则", "Invalid regex"};
        dict["need_ps"]       = {"需要 powershell", "PowerShell required"};
        dict["win_only"]      = {"仅 Windows 支持", "Windows only"};
        dict["write_fail"]    = {"无法写: ", "Cannot write: "};
        dict["open_fail2"]    = {"打开失败: ", "Cannot open: "};
        dict["enc_done"]      = {"已加密 -> ", "Encrypted -> "};
        dict["dec_done"]      = {"已解密 -> ", "Decrypted -> "};
        dict["enc_hint"]      = {"[提示] 输入文件不以 .enc 结尾，输出为 ", "[Hint] Input file does not end with .enc, output is "};
        dict["len_invalid"]   = {"长度无效", "Invalid length"};
        dict["len_range"]     = {"长度需在 1-4096 之间", "Length must be 1-4096"};
        dict["sampling"]      = {"3秒后取样...", "Sampling in 3 seconds..."};
        dict["no_telnet"]     = {"未找到 telnet 或 PuTTY", "telnet or PuTTY not found"};
        dict["unsupported"]   = {"不支持: ", "Unsupported: "};
        dict["vcard_done"]    = {"已生成", "Generated "};
        dict["mb_invalid"]    = {"MB 参数无效", "Invalid MB argument"};
        dict["mb_zero"]       = {"MB 必须 > 0", "MB must be > 0"};
        dict["split_done"]    = {"已分成 ", "Split into "};
        dict["split_parts"]   = {" 份", " parts"};
        dict["merge_done"]    = {"已合并 -> ", "Merged -> "};
        dict["skip_part"]     = {"跳过缺失分片: ", "Skipping missing part: "};
        dict["wc_line"]       = {"行:", "Lines:"};
        dict["wc_word"]       = {" 词:", " Words:"};
        dict["wc_char"]       = {" 字符:", " Chars:"};
        dict["n_invalid"]     = {"-n 参数无效", "Invalid -n argument"};
        dict["n_zero"]        = {"-n 必须 > 0", "-n must be > 0"};
        dict["elapsed"]       = {"耗时: ", "Elapsed: "};
        dict["no_ide"]        = {"未设置 IDE", "IDE not set"};
        dict["cur_ide"]       = {"当前 IDE: ", "Current IDE: "};
        dict["ide_set"]       = {"IDE 已设置为 ", "IDE set to "};
        dict["no_api_key"]    = {"未设置 API Key", "API Key not set"};
        dict["div_zero"]      = {"错误: 除数不能为 0", "Error: division by zero"};
        dict["bad_op"]        = {"不支持的运算符: ", "Unsupported operator: "};
        dict["unsup_conv"]    = {"不支持: ", "Unsupported: "};

        // ---- todo/notes/timer/alias/proxy/picture ----
        dict["parse_fail"]    = {"解析失败，原始响应：", "Parse failed, raw response: "};
        dict["added"]         = {"已添加", "Added"};
        dict["done"]          = {"已完成", "Done"};
        dict["idx_range"]     = {"序号超出范围", "Index out of range"};
        dict["idx_invalid"]   = {"序号无效", "Invalid index"};
        dict["use_todo"]      = {"用法: todo [list|add|done]", "Usage: todo [list|add|done]"};
        dict["no_note"]       = {"笔记不存在: ", "Note not found: "};
        dict["deleted"]       = {"已删除", "Deleted"};
        dict["saved"]         = {"已保存", "Saved"};
        dict["use_timer"]     = {"用法: timer 秒数 [提示信息]", "Usage: timer seconds [message]"};
        dict["sec_invalid"]   = {"秒数无效", "Invalid seconds"};
        dict["sec_range"]     = {"秒数需在 1-86400 之间", "Seconds must be 1-86400"};
        dict["timer_start"]   = {"计时开始 ", "Timer started for "};
        dict["timer_sec"]     = {" 秒...", " seconds..."};
        dict["timer_up"]      = {"时间到！ ", "Time's up! "};
        dict["cur_proxy"]     = {"当前代理: ", "Current proxy: "};
        dict["none2"]         = {"无", "None"};
        dict["use_proxy_set"] = {"用法: proxy set 地址", "Usage: proxy set address"};
        dict["set_saved"]     = {"已设置并保存", "Set and saved"};
        dict["unset_done"]    = {"已取消", "Unset"};
        dict["use_proxy"]     = {"用法: proxy [set|unset]", "Usage: proxy [set|unset]"};
        dict["path_illegal"]  = {"路径含非法字符，回退至 C:\\BetterCMD_Screenshots\n", "Path has invalid chars, fallback to C:\\BetterCMD_Screenshots\n"};
        dict["mkdir_fail2"]   = {"无法创建目录，回退\n", "Cannot create dir, fallback\n"};
        dict["use_picture"]   = {"用法: picture -s/-r/-k/-c [-path \"路径\"]\n当前路径: ", "Usage: picture -s/-r/-k/-c [-path \"path\"]\nCurrent path: "};
        dict["shot_saved"]    = {"截图已保存: ", "Screenshot saved: "};
        dict["rec_invalid"]   = {"录制秒数无效，使用默认 60", "Invalid record seconds, using default 60"};
        dict["rec_range"]     = {"录制秒数需在 1-86400 之间", "Record seconds must be 1-86400"};
        dict["rec_start"]     = {"录屏开始...", "Recording started..."};
        dict["need_ffmpeg2"]  = {"需要 ffmpeg 或 Win+Alt+R", "ffmpeg or Win+Alt+R required"};
        dict["use_hotkey"]    = {"用法: picture -k 快捷键组合 (如 Ctrl+Shift+A)", "Usage: picture -k hotkey (e.g. Ctrl+Shift+A)"};
        dict["hk_set"]        = {"快捷键已设置: ", "Hotkey set: "};
        dict["hk_cleared"]    = {"已清除", "Cleared"};
        dict["use_title"]     = {"用法: /title <文本> [-color\"颜色\"] | -nt | -nt -break", "Usage: /title <text> [-color\"color\"] | -nt | -nt -break"};
        dict["no_color"]      = {"不支持颜色:", "Unsupported color: "};
        dict["gui_ex"]        = {"GuiRunCommand 异常", "GuiRunCommand exception"};

        // ---- 命令描述 ----
        dict["desc_dir"]={"列出目录","List directory"}; dict["desc_cd"]={"切换目录","Change directory"};
        dict["desc_md"]={"创建目录","Make directory"}; dict["desc_rd"]={"删除目录","Remove directory"};
        dict["desc_del"]={"删除文件","Delete file"}; dict["desc_copy"]={"复制文件","Copy file"};
        dict["desc_move"]={"移动文件","Move file"}; dict["desc_ren"]={"重命名","Rename"};
        dict["desc_type"]={"显示文件","Type file"}; dict["desc_echo"]={"输出","Echo"};
        dict["desc_cls"]={"清屏","Clear screen"}; dict["desc_date"]={"日期","Date"};
        dict["desc_time"]={"时间","Time"}; dict["desc_ver"]={"版本","Version"};
        dict["desc_title"]={"窗口标题","Window title"}; dict["desc_color"]={"控制台颜色","Console color"};
        dict["desc_find"]={"搜索","Find"}; dict["desc_more"]={"分页","More"};
        dict["desc_sort"]={"排序","Sort"}; dict["desc_fc"]={"比较文件","Compare files"};
        dict["desc_tree"]={"目录树","Directory tree"}; dict["desc_netstat"]={"网络状态","Network status"};
        dict["desc_ping"]={"Ping","Ping"}; dict["desc_tracert"]={"路由跟踪","Traceroute"};
        dict["desc_nslookup"]={"DNS","DNS"}; dict["desc_ipconfig"]={"IP配置","IP config"};
        dict["desc_route"]={"路由表","Route table"}; dict["desc_arp"]={"ARP","ARP"};
        dict["desc_getmac"]={"MAC","MAC"}; dict["desc_systeminfo"]={"系统信息","System info"};
        dict["desc_tasklist"]={"进程列表","Process list"}; dict["desc_taskkill"]={"结束进程","Kill process"};
        dict["desc_shutdown"]={"关机","Shutdown"}; dict["desc_format"]={"格式化(阻止)","Format(blocked)"};
        dict["desc_chkdsk"]={"磁盘检查","Disk check"}; dict["desc_vol"]={"卷标","Volume label"};
        dict["desc_calc"]={"计算器","Calculator"}; dict["desc_rand"]={"随机数","Random"};
        dict["desc_base64"]={"Base64","Base64"}; dict["desc_md5"]={"MD5","MD5"};
        dict["desc_sha256"]={"SHA256","SHA256"}; dict["desc_qrcode"]={"二维码","QR code"};
        dict["desc_json"]={"JSON格式化","JSON format"}; dict["desc_regex"]={"正则","Regex"};
        dict["desc_wget"]={"下载","Download"}; dict["desc_httpserver"]={"HTTP服务","HTTP server"};
        dict["desc_portscan"]={"端口扫描","Port scan"}; dict["desc_procmon"]={"进程监控","Process monitor"};
        dict["desc_clip"]={"剪贴板","Clipboard"}; dict["desc_sysmon"]={"系统监控","System monitor"};
        dict["desc_diskusage"]={"磁盘","Disk usage"}; dict["desc_encrypt"]={"加密","Encrypt"};
        dict["desc_decrypt"]={"解密","Decrypt"}; dict["desc_zip"]={"压缩","Zip"};
        dict["desc_unzip"]={"解压","Unzip"}; dict["desc_schedule"]={"计划任务","Schedule"};
        dict["desc_logview"]={"日志查看","Log viewer"}; dict["desc_sync"]={"同步","Sync"};
        dict["desc_passgen"]={"密码生成","Password generator"}; dict["desc_uuid"]={"UUID","UUID"};
        dict["desc_colorpick"]={"取色","Color picker"}; dict["desc_translate"]={"翻译","Translate"};
        dict["desc_weather"]={"天气","Weather"}; dict["desc_record"]={"录音","Record audio"};
        dict["desc_screenshot"]={"截图","Screenshot"}; dict["desc_rdp"]={"远程桌面","Remote desktop"};
        dict["desc_ftp"]={"FTP","FTP"}; dict["desc_sql"]={"SQLite","SQLite"};
        dict["desc_sendmail"]={"邮件","Send mail"}; dict["desc_chat"]={"聊天","Chat"};
        dict["desc_scicalc"]={"科学计算","Scientific calculator"}; dict["desc_convert"]={"单位转换","Unit convert"};
        dict["desc_vcard"]={"名片","VCard"}; dict["desc_barcode"]={"条形码","Barcode"};
        dict["desc_checksum"]={"校验","Checksum"}; dict["desc_split"]={"分割","Split"};
        dict["desc_merge"]={"合并","Merge"}; dict["desc_wc"]={"文本统计","Word count"};
        dict["desc_linesort"]={"行排序","Line sort"}; dict["desc_uniq"]={"去重","Unique"};
        dict["desc_dirdiff"]={"目录比较","Directory diff"}; dict["desc_service"]={"服务","Services"};
        dict["desc_startup"]={"启动项","Startup items"}; dict["desc_hwinfo"]={"硬件信息","Hardware info"};
        dict["desc_osk"]={"屏幕键盘","On-screen keyboard"}; dict["desc_whois"]={"Whois","Whois"};
        dict["desc_dig"]={"DNS详细","DNS detail"}; dict["desc_ssh"]={"SSH","SSH"};
        dict["desc_hexdump"]={"十六进制","Hexdump"}; dict["desc_tail"]={"尾部","Tail"};
        dict["desc_head"]={"头部","Head"}; dict["desc_bench"]={"性能测试","Benchmark"};
        dict["desc_ide"]={"IDE设置","IDE settings"}; dict["desc_edit"]={"用IDE打开","Open in IDE"};
        dict["desc_ai"]={"AI聊天","AI chat"}; dict["desc_todo"]={"任务","Todo"};
        dict["desc_notes"]={"笔记","Notes"}; dict["desc_timer"]={"倒计时","Timer"};
        dict["desc_alias"]={"别名","Alias"}; dict["desc_proxy"]={"代理","Proxy"};
        dict["desc_picture"]={"截图录屏","Screenshot/Record"}; dict["desc_BP"]={"抓包","Packet capture"};
        dict["desc_author"]={"关于","About"}; dict["desc_/title"]={"提示符","Prompt"};
        dict["desc_help"]={"帮助","Help"}; dict["desc_exit"]={"退出","Exit"};

        // ---- 帮助文本 ----
        dict["help_h1"]  = {"【CMD原生命令】", "[Native CMD]"};
        dict["help_h2"]  = {"【50工具】", "[50 Tools]"};
        dict["help_h3"]  = {"【增强】", "[Enhanced]"};
        dict["help_h4"]  = {"【IDE/AI】", "[IDE/AI]"};
        dict["help_h5"]  = {"【新功能】", "[New]"};
        dict["help_h6"]  = {"【截图录屏】", "[Screenshot/Record]"};
        dict["help_h7"]  = {"【BP抓包】", "[Packet Capture]"};

        // ---- GUI 分类名 ----
        dict["cat_file"]   = {"文件操作", "File"};
        dict["cat_net"]    = {"网络工具", "Network"};
        dict["cat_sys"]    = {"系统工具", "System"};
        dict["cat_text"]   = {"文本处理", "Text"};
        dict["cat_crypto"] = {"加密压缩", "Crypto/Zip"};
        dict["cat_util"]   = {"实用工具", "Utility"};
        dict["cat_pic"]    = {"截图录屏", "Capture"};
        dict["cat_ai"]     = {"AI 与 IDE", "AI/IDE"};
        dict["cat_bp"]     = {"抓包工具", "Packet Capture"};
        dict["cat_other"]  = {"其他", "Other"};

        // ---- GUI 按钮描述（特殊按钮，普通按钮复用 desc_*） ----
        dict["btn_shot_now"]   = {"立即截图", "Screenshot now"};
        dict["btn_rec_start"]  = {"开始录屏", "Start recording"};
        dict["btn_set_hotkey"] = {"设置快捷键", "Set hotkey"};
        dict["btn_clr_hotkey"] = {"清除快捷键", "Clear hotkey"};
        dict["btn_bp_start"]   = {"开始抓包", "Start capture"};
        dict["btn_bp_stop"]    = {"停止", "Stop"};
        dict["btn_bp_list"]    = {"查看文件", "List files"};

        // ---- GUI 消息 ----
        dict["gui_title"]       = {"Better CMD - 可视化界面 v", "Better CMD - Visual UI v"};
        dict["gui_welcome"]      = {"欢迎使用 Better CMD 可视化界面 v", "Welcome to Better CMD Visual UI v"};
        dict["gui_usage1"]      = {"操作方式：点击左侧分类 → 点击中间按钮 → 需要参数会弹出输入框",
                                    "Click a category on the left -> click a button in the middle -> a dialog appears if parameters are needed"};
        dict["gui_usage2"]      = {"所有操作均记录到日志文件。", "All actions are logged to a file."};
        dict["gui_log_saved"]   = {"感谢使用 Better CMD！\n\n本次操作日志已保存到:\n", "Thanks for using Better CMD!\n\nThis session log was saved to:\n"};
        dict["gui_log_title"]   = {"日志已保存", "Log saved"};
        dict["gui_log_ask"]     = {"是否保存本次操作日志到 log 文件夹？\n\n【是】保存日志\n【否】不保存",
                                    "Save this session's operation log to the 'log' folder?\n\n[Yes] Save log\n[No] Don't save"};
        dict["gui_log_ask_title"] = {"日志设置", "Log settings"};
        dict["gui_new_session"]   = {"新会话", "new session"};
        dict["gui_start_time"]    = {"启动时间: ", "Start time: "};
        dict["gui_user"]          = {"用户: ", "User: "};
        dict["gui_session_end"]   = {"会话结束", "Session ended"};
        dict["gui_log_copy"]      = {"复制", "Copy"};
        dict["gui_log_selectall"] = {"全选", "Select All"};
        dict["gui_input"]       = {"[输入] ", "[Input] "};
        dict["gui_done"]        = {"[完成] ", "[Done] "};
        dict["gui_error"]       = {"[错误] ", "[Error] "};
        dict["gui_ready"]       = {"就绪", "Ready"};
        dict["gui_cur_cat"]     = {"当前分类: ", "Category: "};
        dict["gui_switch_cat"]  = {"[操作] 切换到分类: ", "[Action] Switched to category: "};
        dict["gui_close_log"]   = {"[操作] 用户关闭 GUI 窗口", "[Action] User closed GUI window"};
        dict["gui_ex"]          = {"GuiRunCommand 异常", "GuiRunCommand exception"};
        dict["gui_register_fail"] = {"窗口类注册失败", "Window class registration failed"};
        dict["gui_create_fail"]   = {"窗口创建失败", "Window creation failed"};
        dict["gui_lang_title"]  = {"选择语言 / Choose language", "Choose language"};
        dict["gui_lang_prompt"] = {"请选择界面语言：\n\n【是】中文\n【否】English",
                                    "Please choose UI language:\n\n[Yes] Chinese\n[No] English"};
        dict["gui_lang_set_zh"] = {"已切换为中文", "Switched to Chinese"};
        dict["gui_lang_set_en"] = {"已切换为 English", "Switched to English"};
        dict["gui_settings"]    = {"设置 (Settings)", "Settings"};
        dict["gui_lang_menu"]   = {"切换语言 / Switch language", "Switch language"};
        dict["gui_about_menu"]  = {"关于 / About", "About"};
        dict["gui_input_title"] = {"参数输入", "Parameters"};
        dict["gui_ok"]          = {"执行", "Run"};
        dict["gui_cancel"]      = {"取消", "Cancel"};
        dict["gui_run_cmd"]     = {"执行命令: ", "Run command: "};
        dict["gui_optional"]    = {"（可选）", " (optional)"};
        dict["gui_done_status"] = {"已执行: ", "Executed: "};
        dict["gui_run_prefix"]  = {">>> ", ">>> "};
        dict["gui_param_missing"] = {"必填参数为空", "Required parameter is empty"};
        dict["gui_cancelled"]   = {"已取消", "Cancelled"};
        dict["gui_count_fmt"]   = {"  共 ", "  "};
        dict["gui_count_unit"]  = {" 个功能", " commands"};
        dict["gui_startup_err"] = {"需要管理员权限才能运行 Better CMD。\n请右键选择「以管理员身份运行」。",
                                    "Better CMD requires administrator privileges.\nPlease right-click and choose 'Run as administrator'."};
        dict["gui_no_admin_title"] = {"权限不足", "Insufficient privileges"};
        dict["gui_key_fail"]    = {"密钥验证失败！\n\n密钥文件：miyao.txt",
                                    "Key verification failed!\n\nKey file: miyao.txt"};
        dict["gui_key_title"]   = {"Better CMD - 密钥验证失败", "Better CMD - Key verification failed"};
        dict["gui_mode_title"]  = {"Better CMD v", "Better CMD v"};
        dict["gui_mode_msg"]    = {"请选择启动模式：\n\n"
                                    "【是】图形界面 GUI（按钮操作）\n"
                                    "【否】经典控制台 CMD（命令行输入）\n\n"
                                    "GUI 模式下所有命令都已做成按钮，无需记忆命令。\n"
                                    "控制台模式提供完整的命令行体验。",
                                    "Choose startup mode:\n\n"
                                    "[Yes] GUI (button operation)\n"
                                    "[No] Classic console CMD (command line)\n\n"
                                    "In GUI mode, all commands are buttons; no need to memorize them.\n"
                                    "Console mode provides the full command-line experience."};
        dict["gui_mode_q"]      = {" - 启动模式", " - Startup mode"};
        dict["gui_cmd_mode"]    = {"Better CMD - 控制台模式", "Better CMD - Console mode"};
        dict["gui_about_msg"]   = {"Better CMD v" VERSION "\nfft工作室 - Fungame Craft总项目组",
                                    "Better CMD v" VERSION "\nfft Studio - Fungame Craft Main Project Group"};
        dict["gui_about_title"] = {"关于", "About"};

        // ---- help 详细用法（每条命令） ----
        dict["help_title"]    = {"======== Better CMD v" VERSION " 命令清单 ========",
                                  "======== Better CMD v" VERSION " Command List ========"};
        dict["help_legend"]   = {"说明：<命令本体>  [可选参数]  <必填参数>",
                                  "Legend: <command>  [optional]  <required>"};
        dict["help_see_gui"]   = {"提示：GUI 模式下点击按钮即可调用，参数会以输入框形式提示。",
                                   "Tip: In GUI mode, click a button; parameters appear as input fields."};

        // ---- 默认值/提示文本中文化的硬编码兜底 ----
        dict["ph_path"]      = {"目录路径", "Directory path"};
        dict["ph_curdir"]    = {"留空为当前目录", "Empty for current dir"};
        dict["ph_dstdir"]    = {"例如 D:\\Projects", "e.g. D:\\Projects"};
        dict["ph_newfolder"] = {"例如 newfolder", "e.g. newfolder"};
        dict["ph_savepath"]  = {"保存路径", "Save path"};
        dict["ph_default_empty"] = {"留空为默认", "Empty for default"};
        dict["ph_run_cmd"]   = {"要测试的命令", "Command to benchmark"};
        dict["ph_expr"]      = {"数学表达式", "Math expression"};
        dict["ph_json_file"] = {"JSON 文件", "JSON file"};
        dict["ph_zip_file"]  = {"zip 文件", "zip file"};
        dict["ph_log_file"]  = {"日志文件", "Log file"};
        dict["ph_enc_file"]  = {"加密文件", "Encrypted file"};
        dict["ph_dst_zip"]   = {"目标 .zip", "Target .zip"};
        dict["ph_parts"]     = {"分片（空格分隔）", "Parts (space separated)"};
        dict["ph_default_60"] = {"录制秒数", "Record seconds"};
        dict["ph_hotkey"]    = {"快捷键组合", "Hotkey combo"};
        dict["ph_user_at_host"] = {"user@host", "user@host"};
        dict["ph_ide_cmd"]   = {"IDE 命令", "IDE command"};
        dict["ph_to_gen"]    = {"要生成的内容", "Content to generate"};
        dict["ph_to_translate"] = {"要翻译的内容", "Content to translate"};
        dict["ph_question"]  = {"问题", "Question"};
        dict["ph_task"]      = {"任务内容", "Task content"};

        // ---- 下拉选项 (options) ----
        dict["opt_encode"]   = {"编码", "Encode"};
        dict["opt_decode"]   = {"解码", "Decode"};
        dict["opt_view"]     = {"查看", "View"};
        dict["opt_add"]      = {"添加", "Add"};
        dict["opt_done"]     = {"完成", "Done"};
        dict["opt_set"]      = {"设置", "Set"};
        dict["opt_unset"]    = {"取消", "Unset"};
        // 通用参数标签
        dict["lbl_path"]     = {"目录路径", "Directory path"};
        dict["lbl_name"]     = {"目录名", "Directory name"};
        dict["lbl_file"]     = {"文件名", "File name"};
        dict["lbl_src"]      = {"源文件", "Source file"};
        dict["lbl_dst"]      = {"目标位置", "Target"};
        dict["lbl_old"]      = {"原名称", "Old name"};
        dict["lbl_new"]      = {"新名称", "New name"};
        dict["lbl_root"]     = {"根目录", "Root dir"};
        dict["lbl_kw"]       = {"关键词", "Keyword"};
        dict["lbl_f1"]       = {"文件1", "File 1"};
        dict["lbl_f2"]       = {"文件2", "File 2"};
        dict["lbl_host"]     = {"目标地址", "Target host"};
        dict["lbl_n"]        = {"次数", "Count"};
        dict["lbl_domain"]   = {"域名", "Domain"};
        dict["lbl_port"]     = {"端口", "Port"};
        dict["lbl_url"]      = {"下载地址", "URL"};
        dict["lbl_pid"]      = {"进程 PID", "PID"};
        dict["lbl_a"]        = {"数字1", "Number 1"};
        dict["lbl_b"]        = {"数字2", "Number 2"};
        dict["lbl_op"]       = {"运算符", "Operator"};
        dict["lbl_lo"]       = {"最小值", "Min"};
        dict["lbl_hi"]       = {"最大值", "Max"};
        dict["lbl_len"]      = {"长度", "Length"};
        dict["lbl_text"]     = {"内容", "Text"};
        dict["lbl_pattern"]  = {"正则", "Regex"};
        dict["lbl_value"]    = {"数值", "Value"};
        dict["lbl_from"]     = {"原单位", "From"};
        dict["lbl_to"]       = {"目标单位", "To"};
        dict["lbl_expr"]     = {"数学表达式", "Math expression"};
        dict["lbl_sec"]      = {"秒数", "Seconds"};
        dict["lbl_msg"]      = {"提示信息", "Message"};
        dict["lbl_action"]   = {"操作", "Action"};
        dict["lbl_title"]    = {"标题", "Title"};
        dict["lbl_content"]  = {"内容", "Content"};
        dict["lbl_cmd"]      = {"命令", "Command"};
        dict["lbl_addr"]     = {"地址", "Address"};
        dict["lbl_pwd"]      = {"密码", "Password"};
        dict["lbl_mb"]       = {"每份大小(MB)", "MB per part"};
        dict["lbl_prefix"]   = {"输出前缀", "Output prefix"};
        dict["lbl_out"]      = {"输出文件", "Output file"};
        dict["lbl_parts"]    = {"分片（空格分隔）", "Parts (space separated)"};
        dict["lbl_mode"]     = {"模式", "Mode"};
        dict["lbl_key"]      = {"快捷键组合", "Hotkey"};
        dict["lbl_phone"]    = {"电话", "Phone"};
        dict["lbl_prompt"]   = {"问题", "Prompt"};
        dict["lbl_user_host"] = {"user@host", "user@host"};
    }
    static string t(const string& k) { return dict.count(k) ? (cur == ZH ? dict[k].first : dict[k].second) : k; }
};
Lang::Code Lang::cur = Lang::ZH;
unordered_map<string, pair<string, string>> Lang::dict;

/*
================================================================
 第三部分：辅助函数
================================================================
*/

// 编码转换：任意 codepage → UTF-8
static string cpToUtf8(const string& src, UINT cp) {
    if (src.empty()) return src;
    int wlen = MultiByteToWideChar(cp, 0, src.c_str(), (int)src.size(), NULL, 0);
    if (wlen <= 0) return src;
    wstring wstr(wlen, 0);
    MultiByteToWideChar(cp, 0, src.c_str(), (int)src.size(), &wstr[0], wlen);
    int ulen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, NULL, 0, NULL, NULL);
    if (ulen <= 0) return src;
    string out(ulen, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, &out[0], ulen, NULL, NULL);
    return out;
}

// 编码转换：UTF-8 → UTF-16（用于 Unicode 控件显示）
static wstring utf8ToWide(const string& src) {
    if (src.empty()) return L"";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), (int)src.size(), NULL, 0);
    if (wlen <= 0) return L"";
    wstring out(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, src.c_str(), (int)src.size(), &out[0], wlen);
    return out;
}

string exec(const char* cmd) {
    array<char, 128> buf; string res;
    unique_ptr<FILE, decltype(&PCLOSE)> pipe(POPEN(cmd, "r"), PCLOSE);
    if (!pipe) return "";
    while (fgets(buf.data(), buf.size(), pipe.get())) res += buf.data();
    // 命令输出为系统 ANSI 编码（中文 Windows 为 GBK），统一转为 UTF-8
#ifdef _WIN32
    res = cpToUtf8(res, CP_ACP);
#endif
    return res;
}
string join(const vector<string>& v, const string& sep = " ") {
    string r; for (auto& s : v) { if (!r.empty()) r += sep; r += s; } return r;
}
bool cmd_exists(const string& cmd) {
#ifdef _WIN32
    string q = "where " + cmd + " >nul 2>nul";
#else
    string q = "which " + cmd + " >/dev/null 2>&1";
#endif
    return system(q.c_str()) == 0;
}
bool isAdmin() {
#ifdef _WIN32
    BOOL admin = FALSE; PSID g = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &g)) {
        CheckTokenMembership(NULL, g, &admin);
        FreeSid(g);
    }
    return admin == TRUE;
#else
    return geteuid() == 0;
#endif
}

// 安全获取环境变量（避免返回 NULL 拼接导致崩溃）
string envOr(const char* key, const string& fallback) {
    const char* v = getenv(key);
    return v ? string(v) : fallback;
}

/*
================================================================
 第四部分：命令解析器
================================================================
*/
struct ParsedCmd { string cmd; vector<string> args; map<string, string> flags; };

// 将字符串按空白拆分，支持双引号包裹（保留引号内整体作为一个 token）
vector<string> tokenize(const string& input) {
    vector<string> tokens; string cur; bool inQ = false;
    for (char c : input) {
        if (c == '"') { inQ = !inQ; }
        else if (isspace((unsigned char)c) && !inQ) { if (!cur.empty()) { tokens.push_back(cur); cur.clear(); } }
        else cur += c;
    }
    if (!cur.empty()) tokens.push_back(cur);
    return tokens;
}

ParsedCmd parse(const string& input) {
    ParsedCmd res;
    auto tokens = tokenize(input);
    if (tokens.empty()) return res;
    res.cmd = tokens[0];
    // 第一个 token 已作为 cmd，剩下的逐个处理：
    //  - "-cout(...)"              : 自定义 cout 输出标志
    //  - "-flag value"             : value 不以 '-' 开头时，配对存入 flags
    //  - "-flag"（末尾或下一个仍以 '-' 开头）: 布尔型 flag，值为空
    //  - 其他                        : 位置参数 args
    for (size_t i = 1; i < tokens.size(); ++i) {
        const string& t = tokens[i];
        if (t.rfind("-cout(", 0) == 0) {
            size_t l = t.find('('), r = t.rfind(')');
            if (l != string::npos && r != string::npos) {
                string c = t.substr(l + 1, r - l - 1);
                if (c.size() >= 2 && c.front() == '"' && c.back() == '"') c = c.substr(1, c.size() - 2);
                res.flags["cout"] = c;
            }
            continue;
        }
        if (!t.empty() && t[0] == '-') {
            // 尝试与下一个 token 配对
            if (i + 1 < tokens.size() && !(tokens[i + 1].size() > 0 && tokens[i + 1][0] == '-')) {
                res.flags[t] = tokens[i + 1];
                ++i;  // 跳过已用作值的 token
            } else {
                res.flags[t] = "";
            }
        } else {
            res.args.push_back(t);
        }
    }
    return res;
}

/*
================================================================
 第五部分：数据包捕获
================================================================
*/
class PacketCapture {
    bool capturing = false;
public:
    void start() {
        if (capturing) { cout << Lang::t("bp_running") << endl; return; }
        if (!isAdmin()) { cout << Lang::t("bp_no_admin") << endl; return; }
        fs::create_directory(BP_FOLDER);
        for (auto& e : fs::directory_iterator(BP_FOLDER)) { try { fs::remove(e.path()); } catch (...) {} }
        cout << Lang::t("bp_start") << endl;
        int ret = system("pktmon start --capture --comp nics --pkt-size 0 --file-name " BP_FOLDER "\\capture.etl");
        if (ret != 0) { cout << Lang::t("bp_start_fail") << endl; return; }
        capturing = true;
    }
    void stop() {
        if (!capturing) { cout << Lang::t("bp_not_run") << endl; return; }
        cout << Lang::t("bp_stop") << endl;
        system("pktmon stop");
        system("pktmon format " BP_FOLDER "\\capture.etl -o " BP_FOLDER "\\capture.txt");
        capturing = false;
    }
    void list() {
        cout << Lang::t("bp_list") << endl;
        if (!fs::exists(BP_FOLDER)) { cout << "  " << Lang::t("none") << endl; return; }
        for (auto& e : fs::directory_iterator(BP_FOLDER)) cout << "  " << e.path().filename().string() << endl;
    }
} bpCapture;
/*

================================================================
 第六部分：配置文件、代理、别名、Todo、Notes
================================================================
*/
string configFile() { return envOr("USERPROFILE", ".") + "\\.bcmdrc"; }
string readConfig(const string& key) {
    ifstream f(configFile()); string line;
    while (getline(f, line)) {
        size_t p = line.find('=');
        if (p != string::npos) {
            string k = line.substr(0, p), v = line.substr(p + 1);
            k.erase(0, k.find_first_not_of(" \t")); k.erase(k.find_last_not_of(" \t") + 1);
            v.erase(0, v.find_first_not_of(" \t")); v.erase(v.find_last_not_of(" \t") + 1);
            if (k == key) return v;
        }
    }
    return "";
}
void writeConfig(const string& key, const string& value) {
    vector<string> lines; ifstream ifs(configFile()); string line; bool found = false;
    while (getline(ifs, line)) {
        size_t p = line.find('=');
        if (p != string::npos) {
            string k = line.substr(0, p);
            k.erase(0, k.find_first_not_of(" \t")); k.erase(k.find_last_not_of(" \t") + 1);
            if (k == key) { line = key + "=" + value; found = true; }
        }
        lines.push_back(line);
    }
    ifs.close();
    if (!found) lines.push_back(key + "=" + value);
    ofstream ofs(configFile());
    for (auto& l : lines) ofs << l << "\n";
}

string proxyAddr;
void setProxyEnv(const string& addr) {
    proxyAddr = addr;
#ifdef _WIN32
    _putenv(("http_proxy=" + addr).c_str());
    _putenv(("https_proxy=" + addr).c_str());
#else
    setenv("http_proxy", addr.c_str(), 1);
    setenv("https_proxy", addr.c_str(), 1);
#endif
}
void unsetProxyEnv() {
    proxyAddr.clear();
#ifdef _WIN32
    _putenv("http_proxy="); _putenv("https_proxy=");
#else
    unsetenv("http_proxy"); unsetenv("https_proxy");
#endif
}

unordered_map<string, string> aliases;
void loadAliases() {
    ifstream f(configFile()); string line; bool inA = false;
    while (getline(f, line)) {
        if (line == "[alias]") { inA = true; continue; }
        if (inA && line.empty()) inA = false;
        if (inA) {
            size_t eq = line.find('=');
            if (eq != string::npos) {
                string k = line.substr(0, eq), v = line.substr(eq + 1);
                k.erase(0, k.find_first_not_of(" \t")); k.erase(k.find_last_not_of(" \t") + 1);
                v.erase(0, v.find_first_not_of(" \t")); v.erase(v.find_last_not_of(" \t") + 1);
                aliases[k] = v;
            }
        }
    }
}
void saveAliases() {
    vector<string> lines; ifstream ifs(configFile()); string line;
    bool inA = false, wrote = false;
    while (getline(ifs, line)) {
        if (line == "[alias]") {
            inA = true; lines.push_back(line);
            for (auto& a : aliases) lines.push_back(a.first + "=" + a.second);
            wrote = true;
        } else if (inA && line.empty()) {
            inA = false;
            if (!wrote) { for (auto& a : aliases) lines.push_back(a.first + "=" + a.second); wrote = true; }
            lines.push_back(line);
        } else if (!inA) lines.push_back(line);
    }
    if (!wrote) { lines.push_back("[alias]"); for (auto& a : aliases) lines.push_back(a.first + "=" + a.second); }
    ifs.close(); ofstream ofs(configFile());
    for (auto& l : lines) ofs << l << "\n";
}

string todoFile = envOr("USERPROFILE", ".") + "\\.bcmd_todo.txt";
vector<string> loadTodo() {
    vector<string> t; string l; ifstream f(todoFile);
    while (getline(f, l)) t.push_back(l);
    return t;
}
void saveTodo(const vector<string>& t) { ofstream f(todoFile); for (auto& x : t) f << x << "\n"; }

string notesDir = envOr("USERPROFILE", ".") + "\\.bcmd_notes\\";
void ensureNotesDir() { fs::create_directories(notesDir); }

/*
================================================================
 第七部分：CMD 原生命令实现
================================================================
*/
void cmd_dir(const vector<string>& a)   {
    string p = a.empty() ? "." : a[0];
    error_code ec;
    if (!fs::exists(p, ec)) { cout << Lang::t("path_noexist") << p << endl; return; }
    try { for (auto& e : fs::directory_iterator(p)) cout << (e.is_directory() ? "<DIR> " : "      ") << e.path().filename().string() << endl; }
    catch (const exception& e) { cout << Lang::t("list_fail") << e.what() << endl; }
}
void cmd_cd(const vector<string>& a)    {
    if (a.empty()) { cout << fs::current_path().string() << endl; return; }
    error_code ec;
    if (!fs::exists(a[0], ec)) { cout << Lang::t("path_noexist") << a[0] << endl; return; }
    fs::current_path(a[0], ec);
    if (ec) cout << Lang::t("cd_fail") << ec.message() << endl;
}
void cmd_md(const vector<string>& a)    {
    error_code ec;
    for (auto& d : a) { fs::create_directories(d, ec); if (ec) cout << Lang::t("mkdir_fail") << d << endl; }
}
void cmd_rd(const vector<string>& a)    {
    error_code ec;
    for (auto& d : a) { fs::remove_all(d, ec); if (ec) cout << Lang::t("del_fail") << d << endl; }
}
void cmd_del(const vector<string>& a)   {
    error_code ec;
    for (auto& f : a) { fs::remove(f, ec); if (ec) cout << Lang::t("del_fail") << f << endl; }
}
void cmd_copy(const vector<string>& a)  {
    if (a.size() < 2) { cout << Lang::t("use_copy") << endl; return; }
    error_code ec;
    fs::copy(a[0], a[1], fs::copy_options::overwrite_existing, ec);
    if (ec) cout << Lang::t("copy_fail") << ec.message() << endl;
}
void cmd_move(const vector<string>& a)  {
    if (a.size() < 2) { cout << Lang::t("use_move") << endl; return; }
    error_code ec;
    fs::rename(a[0], a[1], ec);
    if (ec) cout << Lang::t("move_fail") << ec.message() << endl;
}
void cmd_ren(const vector<string>& a)   { if (a.size() >= 2) { error_code ec; fs::rename(a[0], a[1], ec); if (ec) cout << Lang::t("rename_fail") << ec.message() << endl; } }
void cmd_type(const vector<string>& a)  { if (!a.empty()) { ifstream f(a[0]); if (f) cout << f.rdbuf(); else cout << Lang::t("open_fail") << a[0] << endl; } }
void cmd_echo(const vector<string>& a)  { cout << join(a) << endl; }
void cmd_cls()  { system("cls"); }
void cmd_date() { auto t = time(nullptr); cout << put_time(localtime(&t), "%Y-%m-%d") << endl; }
void cmd_time() { auto n = chrono::system_clock::now(); time_t t = chrono::system_clock::to_time_t(n); cout << put_time(localtime(&t), "%H:%M:%S") << endl; }
void cmd_ver()  { cout << "Better CMD v" VERSION << endl; }
#ifdef _WIN32
void cmd_title(const vector<string>& a) { if (!a.empty()) SetConsoleTitleW(utf8ToWide(join(a)).c_str()); }
#else
void cmd_title(const vector<string>&) {}
#endif
void cmd_color(const vector<string>& a) {
    // Windows color 命令需要一个 2 位十六进制参数（如 0A），允许 1 或 2 个 token
#ifdef _WIN32
    if (a.empty()) { system("color"); return; }
    string arg = a.size() >= 2 ? (a[0] + a[1]) : a[0];
    if (arg.size() < 2 || isxdigit((unsigned char)arg[0]) == 0 || isxdigit((unsigned char)arg[1]) == 0) {
        cout << Lang::t("use_color") << endl; return;
    }
    system(("color " + arg).c_str());
#else
    cout << "[color] " << Lang::t("win_only") << endl;
#endif
}
void cmd_find(const vector<string>& a)  { if (a.size() >= 2) { string p = a[0]; ifstream f(a[1]); if (!f) { cout << Lang::t("open_fail") << a[1] << endl; return; } string l; while (getline(f, l)) if (l.find(p) != string::npos) cout << l << endl; } }
void cmd_more(const vector<string>& a)  { if (!a.empty()) { ifstream f(a[0]); if (!f) { cout << Lang::t("open_fail") << a[0] << endl; return; } string l; int n = 0; while (getline(f, l)) { cout << l << endl; if (++n % 20 == 0) { cout << "--More--"; cin.get(); } } } }
void cmd_sort(const vector<string>& a)  { if (!a.empty()) { ifstream f(a[0]); if (!f) { cout << Lang::t("open_fail") << a[0] << endl; return; } vector<string> v; string l; while (getline(f, l)) v.push_back(l); sort(v.begin(), v.end()); for (auto& x : v) cout << x << endl; } }
void cmd_fc(const vector<string>& a)    { if (a.size() >= 2) cout << exec(("fc " + a[0] + " " + a[1]).c_str()); }
void cmd_tree(const vector<string>& a)  {
    string p = a.empty() ? "." : a[0];
    error_code ec;
    if (!fs::exists(p, ec)) { cout << Lang::t("path_noexist") << p << endl; return; }
    try {
        // depth() 是迭代器的方法，不是 directory_entry 的，必须显式用迭代器
        for (auto it = fs::recursive_directory_iterator(p);
             it != fs::recursive_directory_iterator(); ++it) {
            cout << string(it.depth() * 2, ' ') << it->path().filename().string() << endl;
        }
    }
    catch (const exception& e) { cout << Lang::t("traverse_fail") << e.what() << endl; }
}
void cmd_netstat()    { cout << exec("netstat -an"); }
void cmd_tracert(const vector<string>& a) { cout << exec(("tracert " + (a.empty() ? "127.0.0.1" : a[0])).c_str()); }
void cmd_nslookup(const vector<string>& a) { cout << exec(("nslookup " + (a.empty() ? "localhost" : a[0])).c_str()); }
void cmd_ipconfig()   { cout << exec("ipconfig /all"); }
void cmd_route()      { cout << exec("route print"); }
void cmd_arp()        { cout << exec("arp -a"); }
void cmd_getmac()     { cout << exec("getmac"); }
void cmd_systeminfo() { cout << exec("systeminfo"); }
void cmd_tasklist()   { cout << exec("tasklist"); }
void cmd_taskkill(const vector<string>& a) { if (!a.empty()) cout << exec(("taskkill /F /PID " + a[0]).c_str()); }
void cmd_shutdown()   { system("shutdown /s /t 0"); }
void cmd_format()     { cout << Lang::t("format_blocked") << endl; }
void cmd_chkdsk()     { cout << exec("chkdsk"); }
void cmd_vol()        { cout << exec("vol"); }

void cmd_ping_enhanced(const vector<string>& a, map<string, string>& f) {
    string t = a.empty() ? "127.0.0.1" : a[0];
    string cmd = "ping ";
#ifdef _WIN32
    if (f.count("-t")) cmd += "-t ";
    else {
        int n = 4;
        if (f.count("-n")) {
            try { n = stoi(f["-n"]); } catch (...) { cout << Lang::t("invalid_arg") << "-n: " << f["-n"] << endl; return; }
        }
        cmd += "-n " + to_string(n) + " ";
    }
    if (f.count("-l")) {
        // 校验 -l 数值
        try { (void)stoi(f["-l"]); } catch (...) { cout << Lang::t("invalid_arg") << "-l: " << f["-l"] << endl; return; }
        cmd += "-l " + f["-l"] + " ";
    }
    if (f.count("-w")) {
        try { (void)stoi(f["-w"]); } catch (...) { cout << Lang::t("invalid_arg") << "-w: " << f["-w"] << endl; return; }
        cmd += "-w " + f["-w"] + " ";
    }
#endif
    if (f.count("-4")) cmd += "-4 ";
    if (f.count("-6")) cmd += "-6 ";
    cmd += t;
    cout << exec(cmd.c_str());
    if (f.count("cout")) cout << "[CUSTOM] " << f["cout"] << endl;
}

/*
================================================================
 第八部分：50 个新增工具
================================================================
*/
void f_calc(const vector<string>& a) {
    if (a.size() < 3) { cout << Lang::t("use_calc") << endl; return; }
    double x, y;
    try { x = stod(a[0]); y = stod(a[2]); }
    catch (...) { cout << Lang::t("invalid_num") << endl; return; }
    char op = a[1][0];
    switch (op) {
        case '+': cout << x + y << endl; break;
        case '-': cout << x - y << endl; break;
        case '*': cout << x * y << endl; break;
        case '/':
            if (y == 0) { cout << Lang::t("div_zero") << endl; break; }
            cout << x / y << endl; break;
        default: cout << Lang::t("bad_op") << op << endl; break;
    }
}
void f_rand(const vector<string>& a) {
    int lo = 0, hi = 100;
    try {
        if (a.size() > 0) lo = stoi(a[0]);
        if (a.size() > 1) hi = stoi(a[1]);
    } catch (...) { cout << Lang::t("invalid_num") << endl; return; }
    if (lo > hi) swap(lo, hi);
    random_device rd; mt19937 g(rd()); uniform_int_distribution<> d(lo, hi); cout << d(g) << endl;
}
void f_base64(const vector<string>& a, map<string, string>& f) {
    bool enc = f.count("-e") > 0, dec = f.count("-d") > 0;
    if (!enc && !dec) { cout << Lang::t("use_base64") << endl; return; }
    string text = enc ? (f["-e"].empty() ? join(a) : f["-e"]) : (f["-d"].empty() ? (a.empty() ? "" : a[0]) : f["-d"]);
    if (text.empty()) { cout << Lang::t("empty_content") << endl; return; }
    // 通过临时文件传参，避免 PowerShell 引号注入；同时避免命令行长度限制
    string tmp = envOr("TEMP", "/tmp") + "\\bcmd_b64.txt";
    ofstream(tmp, ios::binary).write(text.data(), (streamsize)text.size());
#ifdef _WIN32
    if (enc) {
        cout << exec(("powershell -NoProfile -Command \"$t=Get-Content -Raw -LiteralPath '" + tmp + "'; [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($t))\"").c_str());
    } else {
        cout << exec(("powershell -NoProfile -Command \"[Text.Encoding]::UTF8.GetString([Convert]::FromBase64String((Get-Content -Raw -LiteralPath '" + tmp + "')))\"").c_str());
    }
#else
    // 非 Windows：尝试 openssl，失败提示安装
    if (enc) cout << exec(("openssl base64 -in \"" + tmp + "\"").c_str());
    else {
        // 解码：openssl base64 -d（注意：输出可能含换行）
        string out = exec(("openssl base64 -d -in \"" + tmp + "\"").c_str());
        cout << out;
    }
#endif
    error_code ec; fs::remove(tmp, ec);
}
void f_md5(const vector<string>& a) { if (!a.empty()) cout << exec(("certutil -hashfile \"" + a[0] + "\" MD5").c_str()); }
void f_sha256(const vector<string>& a) { if (!a.empty()) cout << exec(("certutil -hashfile \"" + a[0] + "\" SHA256").c_str()); }
void f_qrcode(const vector<string>& a) {
    if (a.empty()) { cout << Lang::t("use_qrcode") << endl; return; }
    string t = join(a, " "), enc;
    for (char c : t) { char buf[8] = {0}; sprintf(buf, "%%%02X", (unsigned char)c); enc += buf; }
    system(("start https://api.qrserver.com/v1/create-qr-code/?size=200x200&data=" + enc).c_str());
}
void f_json(const vector<string>& a) {
    if (a.empty()) { cout << Lang::t("use_json") << endl; return; }
    ifstream chk(a[0]); if (!chk) { cout << Lang::t("open_fail") << a[0] << endl; return; }
    cout << exec(("powershell -NoProfile -Command \"Get-Content -Raw -LiteralPath '" + a[0] + "' | ConvertFrom-Json | ConvertTo-Json -Depth 100\"").c_str());
}
void f_regex(const vector<string>& a) { if (a.size() >= 2) { try { regex re(a[0]); cout << (regex_match(a[1], re) ? Lang::t("match") : Lang::t("no_match")) << endl; } catch (const regex_error&) { cout << Lang::t("bad_regex") << endl; } } }
void f_wget(const vector<string>& a) {
    if (a.empty()) { cout << Lang::t("use_wget") << endl; return; }
    cout << exec(("curl -L -o download.tmp " + a[0]).c_str());
}
void f_httpserver() { if (cmd_exists("python")) system("start python -m http.server 8080"); else cout << Lang::t("need_py") << endl; }
void f_portscan(const vector<string>& a) {
    if (a.empty()) { cout << exec("netstat -an | findstr LISTENING"); return; }
    if (cmd_exists("powershell")) cout << exec(("powershell -NoProfile -Command \"Test-NetConnection -ComputerName " + a[0] + " -Port " + (a.size() > 1 ? a[1] : "80") + "\"").c_str());
    else cout << Lang::t("need_ps") << endl;
}
void f_procmon() { cout << exec("tasklist"); }
void f_clip(const vector<string>& a) {
#ifdef _WIN32
    if (a.empty()) cout << exec("powershell -NoProfile -Command Get-Clipboard");
    else {
        // 通过临时文件传参，避免引号注入
        string t = join(a, " "), tmp = envOr("TEMP", ".") + "\\bcmd_clip.txt";
        ofstream(tmp, ios::binary).write(t.data(), (streamsize)t.size());
        system(("clip < \"" + tmp + "\"").c_str());
        error_code ec; fs::remove(tmp, ec);
    }
#else
    cout << "[clip] " << Lang::t("win_only") << endl;
#endif
}
void f_sysmon() { cout << exec("powershell -NoProfile -Command \"Get-Counter '\\Processor(_Total)\\% Processor Time'\""); }
void f_diskusage() { cout << exec("wmic logicaldisk get size,freespace,caption"); }
void f_encrypt(const vector<string>& a) {
    if (a.size() < 2) { cout << Lang::t("use_encrypt") << endl; return; }
    ifstream in(a[0], ios::binary);
    if (!in) { cout << Lang::t("open_fail") << a[0] << endl; return; }
    ofstream out(a[0] + ".enc", ios::binary);
    if (!out) { cout << Lang::t("write_fail") << a[0] << ".enc" << endl; return; }
    const string& p = a[1]; size_t i = 0; char c;
    while (in.get(c)) { c ^= p[i % p.size()]; out.put(c); i++; }
    cout << Lang::t("enc_done") << a[0] << ".enc" << endl;
}
void f_decrypt(const vector<string>& a) {
    if (a.size() < 2) { cout << Lang::t("use_decrypt") << endl; return; }
    ifstream in(a[0], ios::binary);
    if (!in) { cout << Lang::t("open_fail") << a[0] << endl; return; }
    // 安全获取输出文件名：输入必须以 .enc 结尾，否则追加 .dec 避免覆盖原文件
    string base = a[0];
    string outName;
    if (base.size() >= 4 && base.compare(base.size() - 4, 4, ".enc") == 0) {
        outName = base.substr(0, base.size() - 4);
    } else {
        outName = base + ".dec";
        cout << Lang::t("enc_hint") << outName << endl;
    }
    if (outName == a[0]) { outName = a[0] + ".dec"; }  // 双保险
    ofstream out(outName, ios::binary);
    if (!out) { cout << Lang::t("write_fail") << outName << endl; return; }
    const string& p = a[1]; size_t i = 0; char c;
    while (in.get(c)) { c ^= p[i % p.size()]; out.put(c); i++; }
    cout << Lang::t("dec_done") << outName << endl;
}
void f_zip(const vector<string>& a) { if (a.size() >= 2) cout << exec(("powershell -NoProfile -Command \"Compress-Archive -LiteralPath '" + a[0] + "' -DestinationPath '" + a[1] + "' -Force\"").c_str()); }
void f_unzip(const vector<string>& a) { if (!a.empty()) cout << exec(("powershell -NoProfile -Command \"Expand-Archive -LiteralPath '" + a[0] + "' -DestinationPath . -Force\"").c_str()); }
void f_schedule(const vector<string>& a) { if (a.size() < 2) return; string t = a[0], cmd = join(vector<string>(a.begin() + 1, a.end()), " "); cout << exec(("schtasks /Create /SC ONCE /ST " + t + " /TR \"" + cmd + "\" /TN BCMD_Task").c_str()); }
void f_logview(const vector<string>& a) { if (!a.empty()) { ifstream f(a[0]); if (!f) { cout << Lang::t("open_fail") << a[0] << endl; return; } string l; while (getline(f, l)) if (l.find("ERROR") != string::npos || l.find("Error") != string::npos) cout << l << endl; } }
void f_sync(const vector<string>& a) { if (a.size() >= 2) cout << exec(("robocopy " + a[0] + " " + a[1] + " /MIR").c_str()); }
void f_passgen(const vector<string>& a) {
    int len = 16;
    try { if (!a.empty()) len = stoi(a[0]); } catch (...) { cout << Lang::t("len_invalid") << endl; return; }
    if (len <= 0 || len > 4096) { cout << Lang::t("len_range") << endl; return; }
    const string c = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    random_device rd; mt19937 g(rd()); uniform_int_distribution<size_t> d(0, c.size() - 1);
    string out; out.reserve(len);
    for (int i = 0; i < len; ++i) out += c[d(g)];
    cout << out << endl;
}
void f_uuid() { cout << exec("powershell -NoProfile -Command \"[guid]::NewGuid().ToString()\""); }
void f_colorpick() {
    cout << Lang::t("sampling") << endl; Sleep(3000);
    // 通过 GDI GetPixel 系统调用取色（原版 PowerShell GetPixel 不存在）
    cout << exec("powershell -NoProfile -Command \"Add-Type -AssemblyName System.Windows.Forms,System.Drawing; $p=[System.Windows.Forms.Cursor]::Position; $bmp=New-Object System.Drawing.Bitmap 1,1; $g=[System.Drawing.Graphics]::FromImage($bmp); $g.CopyFromScreen($p.X,$p.Y,0,0,$bmp.Size); $c=$bmp.GetPixel(0,0); Write-Output ('#{0:X2}{1:X2}{2:X2}' -f $c.R,$c.G,$c.B); $g.Dispose(); $bmp.Dispose()\"");
}
void f_translate(const vector<string>& a) { if (a.empty()) return; if (cmd_exists("trans")) cout << exec(("trans -b :en \"" + join(a, " ") + "\"").c_str()); else cout << Lang::t("need_trans") << endl; }
void f_weather() { cout << exec("curl wttr.in"); }
void f_record() { if (cmd_exists("ffmpeg")) system("start ffmpeg -f dshow -i audio=\"Microphone\" output.wav"); else cout << Lang::t("need_ffmpeg") << endl; }
void f_screenshot() { system("start snippingtool"); }
void f_rdp() { system("start mstsc"); }
void f_ftp() { system("start cmd /k ftp"); }
void f_sql() { if (cmd_exists("sqlite3")) system("start cmd /k sqlite3"); else cout << Lang::t("need_sqlite") << endl; }
void f_sendmail(const vector<string>& a, map<string, string>& f) {
    // 优先从 flags 读取 -to/-subj/-body/-smtp，兼容位置参数
    string to, sub, body, smtp;
    if (f.count("-to")) to = f["-to"];
    if (f.count("-subj")) sub = f["-subj"];
    if (f.count("-body")) body = f["-body"];
    if (f.count("-smtp")) smtp = f["-smtp"];
    // 兼容旧的位置参数：sendmail to subj body smtp
    if (to.empty() && a.size() > 0) to = a[0];
    if (sub.empty() && a.size() > 1) sub = a[1];
    if (body.empty() && a.size() > 2) body = a[2];
    if (smtp.empty() && a.size() > 3) smtp = a[3];
    if (to.empty() || sub.empty() || body.empty() || smtp.empty()) {
        cout << Lang::t("use_sendmail") << endl;
        return;
    }
    cout << exec(("powershell -NoProfile -Command \"Send-MailMessage -To '" + to + "' -Subject '" + sub + "' -Body '" + body + "' -SmtpServer '" + smtp + "'\"").c_str());
}
void f_chat(const vector<string>& a) {
    if (a.size() < 2) { cout << Lang::t("use_chat") << endl; return; }
#ifdef _WIN32
    // Windows 10/11 默认无 telnet，优先尝试 telnet，失败提示启用或用 PuTTY
    if (cmd_exists("telnet")) { system(("start cmd /k telnet " + a[0] + " " + a[1]).c_str()); return; }
    if (cmd_exists("putty")) { system(("start putty telnet://" + a[0] + ":" + a[1]).c_str()); return; }
    cout << Lang::t("no_telnet") << endl;
#else
    cout << exec(("telnet " + a[0] + " " + a[1]).c_str());
#endif
}
void f_scicalc(const vector<string>& a) { if (!a.empty()) cout << exec(("powershell -NoProfile -Command \"" + join(a, "") + "\"").c_str()); }
void f_convert(const vector<string>& a) {
    if (a.size() < 3) { cout << Lang::t("use_convert") << endl; return; }
    double v;
    try { v = stod(a[0]); } catch (...) { cout << Lang::t("invalid_num") << endl; return; }
    string f = a[1], t = a[2];
    if (f == "c" && t == "f") cout << (v * 9 / 5 + 32) << " F" << endl;
    else if (f == "f" && t == "c") cout << ((v - 32) * 5 / 9) << " C" << endl;
    else {
        map<pair<string, string>, double> cv = { {{"cm","inch"},0.393701},{{"inch","cm"},2.54},{{"m","ft"},3.28084},{{"ft","m"},0.3048},{{"kg","lb"},2.20462},{{"lb","kg"},0.453592} };
        auto it = cv.find({ f,t });
        if (it != cv.end()) cout << v * it->second << " " << t << endl;
        else cout << Lang::t("unsupported") << f << " -> " << t << endl;
    }
}
void f_vcard(const vector<string>& a) { if (a.size() >= 2) { ofstream o(a[0] + ".vcf"); o << "BEGIN:VCARD\nVERSION:3.0\nFN:" << a[0] << "\nTEL:" << a[1] << "\nEND:VCARD\n"; cout << Lang::t("vcard_done") << a[0] << ".vcf" << endl; } }
void f_barcode(const vector<string>& a) {
    if (a.empty()) { cout << Lang::t("use_barcode") << endl; return; }
    string t = join(a, ""), enc;
    for (char c : t) { char buf[8] = {0}; sprintf(buf, "%%%02X", (unsigned char)c); enc += buf; }
    system(("start https://barcode.tec-it.com/barcode.ashx?data=" + enc + "&code=Code128").c_str());
}
void f_checksum(const vector<string>& a) { if (!a.empty()) cout << exec(("certutil -hashfile \"" + a[0] + "\" SHA1").c_str()); }
void f_split(const vector<string>& a) {
    if (a.size() < 3) { cout << Lang::t("use_split") << endl; return; }
    ifstream in(a[0], ios::binary);
    if (!in) { cout << Lang::t("open_fail") << a[0] << endl; return; }
    size_t ck;
    try { ck = (size_t)stoull(a[1]) * 1024 * 1024; } catch (...) { cout << Lang::t("mb_invalid") << endl; return; }
    if (ck == 0) { cout << Lang::t("mb_zero") << endl; return; }
    const string& p = a[2];
    vector<char> b(ck); int i = 0;
    while (in.read(b.data(), (streamsize)ck) || in.gcount() > 0) {
        ofstream o(p + "." + to_string(i++), ios::binary);
        o.write(b.data(), in.gcount());
    }
    cout << Lang::t("split_done") << i << " 份" << endl;
}
void f_merge(const vector<string>& a) {
    if (a.size() < 2) { cout << Lang::t("use_merge") << endl; return; }
    ofstream o(a[0], ios::binary);
    if (!o) { cout << Lang::t("write_fail") << a[0] << endl; return; }
    for (size_t i = 1; i < a.size(); i++) {
        ifstream in(a[i], ios::binary);
        if (!in) { cout << Lang::t("skip_part") << a[i] << endl; continue; }
        o << in.rdbuf();
    }
    cout << Lang::t("merge_done") << a[0] << endl;
}
void f_wc(const vector<string>& a) { if (!a.empty()) { ifstream f(a[0]); if (!f) { cout << Lang::t("open_fail") << a[0] << endl; return; } string l; int ln = 0, w = 0, c = 0; while (getline(f, l)) { ln++; c += l.size(); istringstream iss(l); string x; while (iss >> x) w++; } cout << Lang::t("wc_line") << ln << Lang::t("wc_word") << w << Lang::t("wc_char") << c << endl; } }
void f_linesort(const vector<string>& a) { cmd_sort(a); }
void f_uniq(const vector<string>& a) { if (!a.empty()) { ifstream f(a[0]); if (!f) { cout << Lang::t("open_fail") << a[0] << endl; return; } string l, p; while (getline(f, l)) { if (l != p) cout << l << endl; p = l; } } }
void f_dirdiff(const vector<string>& a) { if (a.size() >= 2) cout << exec(("robocopy " + a[0] + " " + a[1] + " /L /NJH /NJS /NP").c_str()); }
void f_service() { cout << exec("sc query"); }
void f_startup() { cout << exec("wmic startup list brief"); }
void f_hwinfo()  { system("start dxdiag"); }
void f_osk()     { system("start osk"); }

/*
================================================================
 第九部分：8 个增强工具
================================================================
*/
void f_whois(const vector<string>& a) { if (!a.empty()) cout << exec(("whois " + a[0]).c_str()); }
void f_dig(const vector<string>& a)   { if (!a.empty()) cout << exec(("nslookup " + a[0]).c_str()); }
void f_ssh(const vector<string>& a)   { if (!a.empty()) system(("start cmd /k ssh " + a[0]).c_str()); }
void f_hexdump(const vector<string>& a) {
    if (a.empty()) return;
    ifstream f(a[0], ios::binary); if (!f) { cout << Lang::t("open_fail2") << a[0] << endl; return; }
    vector<unsigned char> d((istreambuf_iterator<char>(f)), {});
    for (size_t i = 0; i < d.size(); i += 16) {
        printf("%08zx  ", i);
        for (size_t j = 0; j < 16; j++) { if (i + j < d.size()) printf("%02x ", d[i + j]); else printf("   "); if (j == 7) printf(" "); }
        printf(" |");
        for (size_t j = 0; j < 16 && i + j < d.size(); j++) putchar(isprint(d[i + j]) ? d[i + j] : '.');
        printf("|\n");
    }
}
void f_tail(const vector<string>& a, map<string, string>& f) {
    int n = 10; string fn;
    if (f.count("-n")) { try { n = stoi(f["-n"]); } catch (...) { cout << Lang::t("n_invalid") << endl; return; } }
    if (n <= 0) { cout << Lang::t("n_zero") << endl; return; }
    if (!a.empty()) fn = a[0];
    if (fn.empty()) { cout << Lang::t("use_tail") << endl; return; }
    ifstream in(fn); if (!in) { cout << Lang::t("open_fail") << fn << endl; return; }
    vector<string> ls; string l;
    while (getline(in, l)) ls.push_back(l);
    size_t s = ls.size() > (size_t)n ? ls.size() - n : 0;
    for (size_t i = s; i < ls.size(); i++) cout << ls[i] << '\n';
}
void f_head(const vector<string>& a, map<string, string>& f) {
    int n = 10; string fn;
    if (f.count("-n")) { try { n = stoi(f["-n"]); } catch (...) { cout << Lang::t("n_invalid") << endl; return; } }
    if (n <= 0) { cout << Lang::t("n_zero") << endl; return; }
    if (!a.empty()) fn = a[0];
    if (fn.empty()) { cout << Lang::t("use_head") << endl; return; }
    ifstream in(fn); if (!in) { cout << Lang::t("open_fail") << fn << endl; return; }
    string l; int c = 0;
    while (c < n && getline(in, l)) { cout << l << '\n'; c++; }
}
void f_bench(const vector<string>& a) { if (a.empty()) return; string c = join(a, " "); auto s = chrono::high_resolution_clock::now(); int r = system(c.c_str()); (void)r; auto e = chrono::high_resolution_clock::now(); cout << "\n" << Lang::t("elapsed") << chrono::duration_cast<chrono::milliseconds>(e - s).count() << " ms" << endl; }

/*
================================================================
 第十部分：IDE / AI 集成
================================================================
*/
string getIDE() { string i = envOr("IDE", ""); if (i.empty()) i = envOr("EDITOR", ""); if (i.empty()) i = readConfig("IDE"); return i; }
void cmd_ide(const vector<string>& a) {
    if (a.empty()) { string i = getIDE(); if (i.empty()) cout << Lang::t("no_ide") << endl; else cout << Lang::t("cur_ide") << i << endl; }
    else if (a[0] == "set") { if (a.size() < 2) return; string p = join(vector<string>(a.begin() + 1, a.end()), " "); writeConfig("IDE", p); cout << Lang::t("ide_set") << p << endl; }
}
#ifdef _WIN32
void cmd_edit(const vector<string>& a) { if (a.empty()) return; string i = getIDE(); if (i.empty()) { cout << Lang::t("no_ide") << endl; return; } for (auto& f : a) ShellExecuteA(NULL, "open", i.c_str(), f.c_str(), NULL, SW_SHOWNORMAL); }
#else
void cmd_edit(const vector<string>&) {}
#endif

// 转义 JSON 字符串内容（处理 "、\、控制字符）
string jsonEscape(const string& s) {
    string r; r.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  r += "\\\""; break;
            case '\\': r += "\\\\"; break;
            case '\n': r += "\\n";  break;
            case '\r': r += "\\r";  break;
            case '\t': r += "\\t";  break;
            case '\b': r += "\\b";  break;
            case '\f': r += "\\f";  break;
            default:
                if ((unsigned char)c < 0x20) {
                    char buf[8]; sprintf(buf, "\\u%04x", (unsigned char)c); r += buf;
                } else r += c;
        }
    }
    return r;
}
// 从 JSON 字符串中提取第一个 "content":"..." 字段的内容（处理转义）
string extractJsonField(const string& json, const string& field) {
    string key = "\"" + field + "\":\"";
    size_t p = json.find(key);
    if (p == string::npos) return "";
    p += key.size();
    string r;
    while (p < json.size()) {
        char c = json[p];
        if (c == '\\' && p + 1 < json.size()) {
            char n = json[p + 1];
            switch (n) {
                case '"': r += '"'; break;
                case '\\': r += '\\'; break;
                case '/': r += '/'; break;
                case 'n': r += '\n'; break;
                case 'r': r += '\r'; break;
                case 't': r += '\t'; break;
                case 'b': r += '\b'; break;
                case 'f': r += '\f'; break;
                case 'u':
                    if (p + 5 < json.size()) {
                        unsigned u; sscanf(json.c_str() + p + 2, "%4x", &u);
                        // 简单处理 BMP 字符（代理对需要更复杂处理）
                        char buf[8]; sprintf(buf, "%c", (char)u);
                        r += buf;
                        p += 4;
                    }
                    break;
                default: r += n; break;
            }
            p += 2;
        } else if (c == '"') {
            break;
        } else {
            r += c; ++p;
        }
    }
    return r;
}
void cmd_ai(const vector<string>& a) {
    string k = readConfig("api_key"); if (k.empty()) { const char* e = getenv("AI_API_KEY"); if (e) k = e; }
    if (k.empty()) { cout << Lang::t("no_api_key") << endl; return; }
    if (a.empty()) { cout << Lang::t("use_ai") << endl; return; }
    string p = join(a, " ");
    string ep = readConfig("api_endpoint"); if (ep.empty()) ep = "https://api.openai.com/v1/chat/completions";
    string m = readConfig("api_model"); if (m.empty()) m = "gpt-3.5-turbo";
    string body = "{\"model\":\"" + jsonEscape(m) + "\",\"messages\":[{\"role\":\"user\",\"content\":\"" + jsonEscape(p) + "\"}],\"stream\":false}";
    // 通过 stdin 传 body 以避免命令行长度/转义问题
    string tmpBody = envOr("TEMP", "/tmp") + "\\bcmd_ai_body.json";
    ofstream(tmpBody, ios::binary).write(body.data(), (streamsize)body.size());
    string resp = exec(("curl -s -X POST " + ep + " -H \"Content-Type: application/json\" -H \"Authorization: Bearer " + k + "\" --data-binary @" + tmpBody).c_str());
    error_code ec; fs::remove(tmpBody, ec);
    string content = extractJsonField(resp, "content");
    if (content.empty()) { cout << Lang::t("parse_fail") << endl << resp << endl; return; }
    cout << content << endl;
}

/*
================================================================
 第十一部分：新功能（todo / notes / timer / alias / proxy）
================================================================
*/
void cmd_todo(const vector<string>& a) {
    auto t = loadTodo();
    if (a.empty() || a[0] == "list") { for (size_t i = 0; i < t.size(); i++) cout << i + 1 << ". " << t[i] << endl; }
    else if (a[0] == "add") { string x = join(vector<string>(a.begin() + 1, a.end()), " "); t.push_back(x); saveTodo(t); cout << Lang::t("added") << endl; }
    else if (a[0] == "done") { if (a.size() < 2) return; try { int i = stoi(a[1]) - 1; if (i >= 0 && i < (int)t.size()) { t.erase(t.begin() + i); saveTodo(t); cout << Lang::t("done") << endl; } else cout << Lang::t("idx_range") << endl; } catch (...) { cout << Lang::t("idx_invalid") << endl; } }
    else cout << Lang::t("use_todo") << endl;
}
void cmd_notes(const vector<string>& a) {
    ensureNotesDir();
    if (a.empty() || a[0] == "list") { for (auto& e : fs::directory_iterator(notesDir)) cout << e.path().stem().string() << endl; }
    else if (a[0] == "view") { if (a.size() < 2) return; ifstream f(notesDir + a[1] + ".md"); if (f) cout << f.rdbuf(); else cout << Lang::t("no_note") << a[1] << endl; }
    else if (a[0] == "rm") { if (a.size() < 2) return; error_code ec; fs::remove(notesDir + a[1] + ".md", ec); if (ec) cout << Lang::t("del_fail") << endl; else cout << Lang::t("deleted") << endl; }
    else { string t = a[0], c = join(vector<string>(a.begin() + 1, a.end()), " "); ofstream f(notesDir + t + ".md"); f << c; cout << Lang::t("saved") << endl; }
}
void cmd_timer(const vector<string>& a) {
    if (a.empty()) { cout << Lang::t("use_timer") << endl; return; }
    int s;
    try { s = stoi(a[0]); } catch (...) { cout << Lang::t("sec_invalid") << endl; return; }
    if (s <= 0 || s > 86400) { cout << Lang::t("sec_range") << endl; return; }
    string m = a.size() > 1 ? join(vector<string>(a.begin() + 1, a.end()), " ") : "";
    cout << Lang::t("timer_start") << s << Lang::t("timer_sec") << endl;
    Sleep((DWORD)s * 1000);
    cout << "\a" << Lang::t("timer_up") << m << endl;
}
void cmd_alias(const vector<string>& a) {
    if (a.empty()) { for (auto& p : aliases) cout << p.first << " = " << p.second << endl; }
    else if (a[0] == "rm") { if (a.size() < 2) return; aliases.erase(a[1]); saveAliases(); cout << Lang::t("deleted") << endl; }
    else { if (a.size() < 2) return; aliases[a[0]] = join(vector<string>(a.begin() + 1, a.end()), " "); saveAliases(); cout << Lang::t("saved") << endl; }
}
void cmd_proxy(const vector<string>& a) {
    if (a.empty()) { cout << Lang::t("cur_proxy") << (proxyAddr.empty() ? Lang::t("none2") : proxyAddr) << endl; }
    else if (a[0] == "set") { if (a.size() < 2) { cout << Lang::t("use_proxy_set") << endl; return; } setProxyEnv(a[1]); writeConfig("proxy", a[1]); cout << Lang::t("set_saved") << endl; }
    else if (a[0] == "unset") { unsetProxyEnv(); writeConfig("proxy", ""); cout << Lang::t("unset_done") << endl; }
    else cout << Lang::t("use_proxy") << endl;
}

/*
================================================================
 第十二部分：截图/录屏
================================================================
*/
void cmd_picture(const vector<string>& a, map<string, string>& f) {
    string sp = readConfig("screenshot_path");
    if (sp.empty()) sp = envOr("USERPROFILE", ".") + "\\Pictures\\BetterCMD_Screenshots";
    bool pf = false;
    // -path 由解析器作为 flag 配对，值在 f["-path"]
    if (f.count("-path") && !f["-path"].empty()) { sp = f["-path"]; pf = true; }
    auto valid = [](const string& p) { string il = "<>|\"*?"; for (char c : il) if (p.find(c) != string::npos) return false; return !p.empty(); };
    string fp = sp; bool fb = false;
    if (!valid(sp)) { cout << Lang::t("path_illegal"); fp = "C:\\BetterCMD_Screenshots"; fb = true; }
    else { error_code ec; fs::create_directories(sp, ec); if (ec) { cout << Lang::t("mkdir_fail2"); fp = "C:\\BetterCMD_Screenshots"; fb = true; } }
    sp = fp;
    // 判断动作（来自 flags）
    bool actS = f.count("-s") > 0;
    bool actR = f.count("-r") > 0;
    bool actK = f.count("-k") > 0;
    bool actC = f.count("-c") > 0;
    if (!actS && !actR && !actK && !actC) {
        cout << Lang::t("use_picture") << sp << endl;
        return;
    }
    if (actS) {
        string fn = sp + "\\screenshot_" + to_string(time(0)) + ".png";
        string cmd = "powershell -NoProfile -Command \"Add-Type -AssemblyName System.Windows.Forms,System.Drawing; $img=[System.Windows.Forms.Screen]::PrimaryScreen.Bounds; $bmp=New-Object System.Drawing.Bitmap $img.Width,$img.Height; $g=[System.Drawing.Graphics]::FromImage($bmp); $g.CopyFromScreen($img.X,$img.Y,0,0,$img.Size); $bmp.Save('\" + fn + \"',[System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose(); $bmp.Dispose()\"";
        // 注意：fn 路径中若含单引号需要转义，此处简单处理
        string ps = "powershell -NoProfile -Command \"Add-Type -AssemblyName System.Windows.Forms,System.Drawing; $img=[System.Windows.Forms.Screen]::PrimaryScreen.Bounds; $bmp=New-Object System.Drawing.Bitmap $img.Width,$img.Height; $g=[System.Drawing.Graphics]::FromImage($bmp); $g.CopyFromScreen($img.X,$img.Y,0,0,$img.Size); $bmp.Save('";
        ps += fn;
        ps += "',[System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose(); $bmp.Dispose()\"";
        system(ps.c_str()); cout << Lang::t("shot_saved") << fn << endl;
        if (pf && !fb) writeConfig("screenshot_path", sp);
    } else if (actR) {
        int d = 60;
        if (f.count("-r") && !f["-r"].empty()) {
            try { d = stoi(f["-r"]); } catch (...) { cout << Lang::t("rec_invalid") << endl; }
        }
        if (d <= 0 || d > 86400) { cout << Lang::t("rec_range") << endl; return; }
        string fn = sp + "\\recording_" + to_string(time(0)) + ".mp4";
        if (cmd_exists("ffmpeg")) { system(("start ffmpeg -f gdigrab -framerate 30 -t " + to_string(d) + " -i desktop \"" + fn + "\"").c_str()); cout << Lang::t("rec_start") << endl; }
        else cout << Lang::t("need_ffmpeg2") << endl;
    } else if (actK) {
        string key = f.count("-k") ? f["-k"] : "";
        if (key.empty()) { cout << Lang::t("use_hotkey") << endl; return; }
        writeConfig("screenshot_hotkey", key); cout << Lang::t("hk_set") << key << endl;
    } else if (actC) {
        writeConfig("screenshot_hotkey", ""); cout << Lang::t("hk_cleared") << endl;
    }
}

/*
================================================================
 第十三部分：提示符系统
================================================================
*/
string currentPrompt = "BCMD> ";
string promptText = "BCMD";
vector<tuple<size_t, size_t, string>> promptColors;
bool promptShowTime = false;
string promptTimeColor;

unordered_map<string, string> colorNameToAnsi = {
    {"black","\033[30m"},{"red","\033[31m"},{"green","\033[32m"},{"yellow","\033[33m"},
    {"blue","\033[34m"},{"magenta","\033[35m"},{"cyan","\033[36m"},{"white","\033[37m"},
    {"bright_black","\033[90m"},{"bright_red","\033[91m"},{"bright_green","\033[92m"},
    {"bright_yellow","\033[93m"},{"bright_blue","\033[94m"},{"bright_magenta","\033[95m"},
    {"bright_cyan","\033[96m"},{"bright_white","\033[97m"}
};
string getAnsiColor(const string& n) { auto i = colorNameToAnsi.find(n); return i != colorNameToAnsi.end() ? i->second : ""; }

string getColoredPrompt() {
    string r; size_t last = 0;
    for (auto& [s, e, c] : promptColors) { if (s > last) r += promptText.substr(last, s - last); r += c + promptText.substr(s, e - s) + "\033[0m"; last = e; }
    if (last < promptText.size()) r += promptText.substr(last);
    if (promptShowTime) { auto n = chrono::system_clock::now(); time_t t = chrono::system_clock::to_time_t(n); stringstream ss; ss << put_time(localtime(&t), "%Y-%m-%d %H:%M:%S"); r += promptTimeColor.empty() ? " " + ss.str() : promptTimeColor + ss.str() + "\033[0m"; }
    return r;
}

void cmd_title_prompt(const string& input) {
    string r = input.substr(6);
    size_t f = r.find_first_not_of(" \t");
    if (f == string::npos) { cout << Lang::t("use_title") << endl; return; }
    r = r.substr(f);
    if (!r.empty() && r[0] != '-') {
        if (r.front() == '"') { size_t e = r.find('"', 1); if (e == string::npos) return; promptText = r.substr(1, e - 1); r = r.substr(e + 1); promptColors.clear(); }
        else { size_t s = r.find(' '); if (s == string::npos) { promptText = r; r.clear(); } else { promptText = r.substr(0, s); r = r.substr(s); } }
    }
    while (!r.empty()) {
        size_t s = r.find_first_not_of(" \t"); if (s == string::npos) break; r = r.substr(s);
        if (r.rfind("-nt", 0) == 0) {
            r = r.substr(3); promptShowTime = true;
            size_t ns = r.find_first_not_of(" \t");
            if (ns != string::npos && r.substr(ns, 6) == "-break") { promptShowTime = false; r = r.substr(ns + 6); }
            else if (ns != string::npos && r.substr(ns, 6) == "-color") {
                r = r.substr(ns + 6); size_t cs = r.find_first_not_of(" \t");
                if (cs == string::npos || r[cs] != '"') return;
                size_t ce = r.find('"', cs + 1); if (ce == string::npos) return;
                promptTimeColor = getAnsiColor(r.substr(cs + 1, ce - cs - 1));
                r = r.substr(ce + 1);
            }
            continue;
        }
        if (r.rfind("-color", 0) == 0) {
            size_t cs = r.find('"', 6); if (cs == string::npos) return;
            size_t ce = r.find('"', cs + 1); if (ce == string::npos) return;
            string cn = r.substr(cs + 1, ce - cs - 1); string ansi = getAnsiColor(cn);
            if (ansi.empty()) { cout << Lang::t("no_color") << cn << endl; return; }
            r = r.substr(ce + 1);
            size_t ws = r.find_first_not_of(" \t"); if (ws == string::npos || r.substr(ws, 4) != "with") return;
            r = r.substr(ws + 4);
            size_t rs = r.find_first_not_of(" \t"); if (rs == string::npos || r[rs] != '"') return;
            size_t re = r.find('"', rs + 1); if (re == string::npos) return;
            string rng = r.substr(rs + 1, re - rs - 1); size_t tp = rng.find(" to ");
            if (tp == string::npos) return;
            size_t from = stoull(rng.substr(0, tp)), to = stoull(rng.substr(tp + 4));
            if (from >= promptText.size() || to > promptText.size() || from >= to) return;
            promptColors.push_back({ from,to,ansi }); r = r.substr(re + 1);
            continue;
        }
        return;
    }
    currentPrompt = getColoredPrompt() + "> ";
}

/*
================================================================
 第十四部分：命令注册表
================================================================
*/
unordered_map<string, function<void(const vector<string>&, map<string, string>&)>> cmds;
unordered_map<string, string> cmdDesc;

void initCmdDesc() {
    cmdDesc["dir"] = Lang::t("desc_dir"); cmdDesc["cd"] = Lang::t("desc_cd");
    cmdDesc["md"] = Lang::t("desc_md"); cmdDesc["rd"] = Lang::t("desc_rd");
    cmdDesc["del"] = Lang::t("desc_del"); cmdDesc["copy"] = Lang::t("desc_copy");
    cmdDesc["move"] = Lang::t("desc_move"); cmdDesc["ren"] = Lang::t("desc_ren");
    cmdDesc["type"] = Lang::t("desc_type"); cmdDesc["echo"] = Lang::t("desc_echo");
    cmdDesc["cls"] = Lang::t("desc_cls"); cmdDesc["date"] = Lang::t("desc_date");
    cmdDesc["time"] = Lang::t("desc_time"); cmdDesc["ver"] = Lang::t("desc_ver");
    cmdDesc["title"] = Lang::t("desc_title"); cmdDesc["color"] = Lang::t("desc_color");
    cmdDesc["find"] = Lang::t("desc_find"); cmdDesc["more"] = Lang::t("desc_more");
    cmdDesc["sort"] = Lang::t("desc_sort"); cmdDesc["fc"] = Lang::t("desc_fc");
    cmdDesc["tree"] = Lang::t("desc_tree"); cmdDesc["netstat"] = Lang::t("desc_netstat");
    cmdDesc["ping"] = Lang::t("desc_ping"); cmdDesc["tracert"] = Lang::t("desc_tracert");
    cmdDesc["nslookup"] = Lang::t("desc_nslookup"); cmdDesc["ipconfig"] = Lang::t("desc_ipconfig");
    cmdDesc["route"] = Lang::t("desc_route"); cmdDesc["arp"] = Lang::t("desc_arp");
    cmdDesc["getmac"] = Lang::t("desc_getmac"); cmdDesc["systeminfo"] = Lang::t("desc_systeminfo");
    cmdDesc["tasklist"] = Lang::t("desc_tasklist"); cmdDesc["taskkill"] = Lang::t("desc_taskkill");
    cmdDesc["shutdown"] = Lang::t("desc_shutdown"); cmdDesc["format"] = Lang::t("desc_format");
    cmdDesc["chkdsk"] = Lang::t("desc_chkdsk"); cmdDesc["vol"] = Lang::t("desc_vol");
    cmdDesc["calc"] = Lang::t("desc_calc"); cmdDesc["rand"] = Lang::t("desc_rand");
    cmdDesc["base64"] = Lang::t("desc_base64"); cmdDesc["md5"] = Lang::t("desc_md5");
    cmdDesc["sha256"] = Lang::t("desc_sha256"); cmdDesc["qrcode"] = Lang::t("desc_qrcode");
    cmdDesc["json"] = Lang::t("desc_json"); cmdDesc["regex"] = Lang::t("desc_regex");
    cmdDesc["wget"] = Lang::t("desc_wget"); cmdDesc["httpserver"] = Lang::t("desc_httpserver");
    cmdDesc["portscan"] = Lang::t("desc_portscan"); cmdDesc["procmon"] = Lang::t("desc_procmon");
    cmdDesc["clip"] = Lang::t("desc_clip"); cmdDesc["sysmon"] = Lang::t("desc_sysmon");
    cmdDesc["diskusage"] = Lang::t("desc_diskusage"); cmdDesc["encrypt"] = Lang::t("desc_encrypt");
    cmdDesc["decrypt"] = Lang::t("desc_decrypt"); cmdDesc["zip"] = Lang::t("desc_zip");
    cmdDesc["unzip"] = Lang::t("desc_unzip"); cmdDesc["schedule"] = Lang::t("desc_schedule");
    cmdDesc["logview"] = Lang::t("desc_logview"); cmdDesc["sync"] = Lang::t("desc_sync");
    cmdDesc["passgen"] = Lang::t("desc_passgen"); cmdDesc["uuid"] = Lang::t("desc_uuid");
    cmdDesc["colorpick"] = Lang::t("desc_colorpick"); cmdDesc["translate"] = Lang::t("desc_translate");
    cmdDesc["weather"] = Lang::t("desc_weather"); cmdDesc["record"] = Lang::t("desc_record");
    cmdDesc["screenshot"] = Lang::t("desc_screenshot"); cmdDesc["rdp"] = Lang::t("desc_rdp");
    cmdDesc["ftp"] = Lang::t("desc_ftp"); cmdDesc["sql"] = Lang::t("desc_sql");
    cmdDesc["sendmail"] = Lang::t("desc_sendmail"); cmdDesc["chat"] = Lang::t("desc_chat");
    cmdDesc["scicalc"] = Lang::t("desc_scicalc"); cmdDesc["convert"] = Lang::t("desc_convert");
    cmdDesc["vcard"] = Lang::t("desc_vcard"); cmdDesc["barcode"] = Lang::t("desc_barcode");
    cmdDesc["checksum"] = Lang::t("desc_checksum"); cmdDesc["split"] = Lang::t("desc_split");
    cmdDesc["merge"] = Lang::t("desc_merge"); cmdDesc["wc"] = Lang::t("desc_wc");
    cmdDesc["linesort"] = Lang::t("desc_linesort"); cmdDesc["uniq"] = Lang::t("desc_uniq");
    cmdDesc["dirdiff"] = Lang::t("desc_dirdiff"); cmdDesc["service"] = Lang::t("desc_service");
    cmdDesc["startup"] = Lang::t("desc_startup"); cmdDesc["hwinfo"] = Lang::t("desc_hwinfo");
    cmdDesc["osk"] = Lang::t("desc_osk"); cmdDesc["whois"] = Lang::t("desc_whois");
    cmdDesc["dig"] = Lang::t("desc_dig"); cmdDesc["ssh"] = Lang::t("desc_ssh");
    cmdDesc["hexdump"] = Lang::t("desc_hexdump"); cmdDesc["tail"] = Lang::t("desc_tail");
    cmdDesc["head"] = Lang::t("desc_head"); cmdDesc["bench"] = Lang::t("desc_bench");
    cmdDesc["ide"] = Lang::t("desc_ide"); cmdDesc["edit"] = Lang::t("desc_edit");
    cmdDesc["ai"] = Lang::t("desc_ai"); cmdDesc["todo"] = Lang::t("desc_todo");
    cmdDesc["notes"] = Lang::t("desc_notes"); cmdDesc["timer"] = Lang::t("desc_timer");
    cmdDesc["alias"] = Lang::t("desc_alias"); cmdDesc["proxy"] = Lang::t("desc_proxy");
    cmdDesc["picture"] = Lang::t("desc_picture"); cmdDesc["BP"] = Lang::t("desc_BP");
    cmdDesc["author"] = Lang::t("desc_author"); cmdDesc["/title"] = Lang::t("desc_/title");
    cmdDesc["help"] = Lang::t("desc_help"); cmdDesc["exit"] = Lang::t("desc_exit");
}

void init_cmds() {
    cmds["dir"] = [](auto& a, auto& f) {cmd_dir(a);}; cmds["cd"] = [](auto& a, auto& f) {cmd_cd(a);};
    cmds["md"] = [](auto& a, auto& f) {cmd_md(a);}; cmds["rd"] = [](auto& a, auto& f) {cmd_rd(a);};
    cmds["del"] = [](auto& a, auto& f) {cmd_del(a);}; cmds["copy"] = [](auto& a, auto& f) {cmd_copy(a);};
    cmds["move"] = [](auto& a, auto& f) {cmd_move(a);}; cmds["ren"] = [](auto& a, auto& f) {cmd_ren(a);};
    cmds["type"] = [](auto& a, auto& f) {cmd_type(a);}; cmds["echo"] = [](auto& a, auto& f) {cmd_echo(a);};
    cmds["cls"] = [](auto& a, auto& f) {cmd_cls();}; cmds["date"] = [](auto& a, auto& f) {cmd_date();};
    cmds["time"] = [](auto& a, auto& f) {cmd_time();}; cmds["ver"] = [](auto& a, auto& f) {cmd_ver();};
    cmds["title"] = [](auto& a, auto& f) {cmd_title(a);}; cmds["color"] = [](auto& a, auto& f) {cmd_color(a);};
    cmds["find"] = [](auto& a, auto& f) {cmd_find(a);}; cmds["more"] = [](auto& a, auto& f) {cmd_more(a);};
    cmds["sort"] = [](auto& a, auto& f) {cmd_sort(a);}; cmds["fc"] = [](auto& a, auto& f) {cmd_fc(a);};
    cmds["tree"] = [](auto& a, auto& f) {cmd_tree(a);}; cmds["netstat"] = [](auto& a, auto& f) {cmd_netstat();};
    cmds["ping"] = [](auto& a, auto& f) {cmd_ping_enhanced(a, f);}; cmds["tracert"] = [](auto& a, auto& f) {cmd_tracert(a);};
    cmds["nslookup"] = [](auto& a, auto& f) {cmd_nslookup(a);}; cmds["ipconfig"] = [](auto& a, auto& f) {cmd_ipconfig();};
    cmds["route"] = [](auto& a, auto& f) {cmd_route();}; cmds["arp"] = [](auto& a, auto& f) {cmd_arp();};
    cmds["getmac"] = [](auto& a, auto& f) {cmd_getmac();}; cmds["systeminfo"] = [](auto& a, auto& f) {cmd_systeminfo();};
    cmds["tasklist"] = [](auto& a, auto& f) {cmd_tasklist();}; cmds["taskkill"] = [](auto& a, auto& f) {cmd_taskkill(a);};
    cmds["shutdown"] = [](auto& a, auto& f) {cmd_shutdown();}; cmds["format"] = [](auto& a, auto& f) {cmd_format();};
    cmds["chkdsk"] = [](auto& a, auto& f) {cmd_chkdsk();}; cmds["vol"] = [](auto& a, auto& f) {cmd_vol();};
    cmds["calc"] = [](auto& a, auto& f) {f_calc(a);}; cmds["rand"] = [](auto& a, auto& f) {f_rand(a);};
    cmds["base64"] = [](auto& a, auto& f) {f_base64(a, f);}; cmds["md5"] = [](auto& a, auto& f) {f_md5(a);};
    cmds["sha256"] = [](auto& a, auto& f) {f_sha256(a);}; cmds["qrcode"] = [](auto& a, auto& f) {f_qrcode(a);};
    cmds["json"] = [](auto& a, auto& f) {f_json(a);}; cmds["regex"] = [](auto& a, auto& f) {f_regex(a);};
    cmds["wget"] = [](auto& a, auto& f) {f_wget(a);}; cmds["httpserver"] = [](auto& a, auto& f) {f_httpserver();};
    cmds["portscan"] = [](auto& a, auto& f) {f_portscan(a);}; cmds["procmon"] = [](auto& a, auto& f) {f_procmon();};
    cmds["clip"] = [](auto& a, auto& f) {f_clip(a);}; cmds["sysmon"] = [](auto& a, auto& f) {f_sysmon();};
    cmds["diskusage"] = [](auto& a, auto& f) {f_diskusage();}; cmds["encrypt"] = [](auto& a, auto& f) {f_encrypt(a);};
    cmds["decrypt"] = [](auto& a, auto& f) {f_decrypt(a);}; cmds["zip"] = [](auto& a, auto& f) {f_zip(a);};
    cmds["unzip"] = [](auto& a, auto& f) {f_unzip(a);}; cmds["schedule"] = [](auto& a, auto& f) {f_schedule(a);};
    cmds["logview"] = [](auto& a, auto& f) {f_logview(a);}; cmds["sync"] = [](auto& a, auto& f) {f_sync(a);};
    cmds["passgen"] = [](auto& a, auto& f) {f_passgen(a);}; cmds["uuid"] = [](auto& a, auto& f) {f_uuid();};
    cmds["colorpick"] = [](auto& a, auto& f) {f_colorpick();}; cmds["translate"] = [](auto& a, auto& f) {f_translate(a);};
    cmds["weather"] = [](auto& a, auto& f) {f_weather();}; cmds["record"] = [](auto& a, auto& f) {f_record();};
    cmds["screenshot"] = [](auto& a, auto& f) {f_screenshot();}; cmds["rdp"] = [](auto& a, auto& f) {f_rdp();};
    cmds["ftp"] = [](auto& a, auto& f) {f_ftp();}; cmds["sql"] = [](auto& a, auto& f) {f_sql();};
    cmds["sendmail"] = [](auto& a, auto& f) {f_sendmail(a, f);}; cmds["chat"] = [](auto& a, auto& f) {f_chat(a);};
    cmds["scicalc"] = [](auto& a, auto& f) {f_scicalc(a);}; cmds["convert"] = [](auto& a, auto& f) {f_convert(a);};
    cmds["vcard"] = [](auto& a, auto& f) {f_vcard(a);}; cmds["barcode"] = [](auto& a, auto& f) {f_barcode(a);};
    cmds["checksum"] = [](auto& a, auto& f) {f_checksum(a);}; cmds["split"] = [](auto& a, auto& f) {f_split(a);};
    cmds["merge"] = [](auto& a, auto& f) {f_merge(a);}; cmds["wc"] = [](auto& a, auto& f) {f_wc(a);};
    cmds["linesort"] = [](auto& a, auto& f) {f_linesort(a);}; cmds["uniq"] = [](auto& a, auto& f) {f_uniq(a);};
    cmds["dirdiff"] = [](auto& a, auto& f) {f_dirdiff(a);}; cmds["service"] = [](auto& a, auto& f) {f_service();};
    cmds["startup"] = [](auto& a, auto& f) {f_startup();}; cmds["hwinfo"] = [](auto& a, auto& f) {f_hwinfo();};
    cmds["osk"] = [](auto& a, auto& f) {f_osk();};
    cmds["whois"] = [](auto& a, auto& f) {f_whois(a);}; cmds["dig"] = [](auto& a, auto& f) {f_dig(a);};
    cmds["ssh"] = [](auto& a, auto& f) {f_ssh(a);}; cmds["hexdump"] = [](auto& a, auto& f) {f_hexdump(a);};
    cmds["tail"] = [](auto& a, auto& f) {f_tail(a, f);}; cmds["head"] = [](auto& a, auto& f) {f_head(a, f);};
    cmds["bench"] = [](auto& a, auto& f) {f_bench(a);};
    cmds["ide"] = [](auto& a, auto& f) {cmd_ide(a);}; cmds["edit"] = [](auto& a, auto& f) {cmd_edit(a);};
    cmds["ai"] = [](auto& a, auto& f) {cmd_ai(a);};
    cmds["todo"] = [](auto& a, auto& f) {cmd_todo(a);}; cmds["notes"] = [](auto& a, auto& f) {cmd_notes(a);};
    cmds["timer"] = [](auto& a, auto& f) {cmd_timer(a);}; cmds["alias"] = [](auto& a, auto& f) {cmd_alias(a);};
    cmds["proxy"] = [](auto& a, auto& f) {cmd_proxy(a);}; cmds["picture"] = [](auto& a, auto& f) {cmd_picture(a, f);};
    cmds["BP"] = [](auto& a, auto& f) {
        // 兼容: BP -stop / BP -list / BP -start
        bool stop = f.count("-stop") || f.count("-break") || (!a.empty() && (a[0] == "-stop" || a[0] == "-break"));
        bool list = f.count("-list") || (!a.empty() && a[0] == "-list");
        if (stop) bpCapture.stop();
        else if (list) bpCapture.list();
        else bpCapture.start();
    };
    cmds["author"] = [](auto& a, auto& f) { cout << Lang::t("author") << endl; };
    cmds["about"] = cmds["author"];
}

// help 函数定义后置于 rebuildCategories 之后（依赖 g_categories）

/*
================================================================
 第十五部分：控制台输入与主循环
================================================================
*/
#ifdef _WIN32
string readlineConsole(const string& prompt) {
    string buf; cout << prompt << flush;
    while (true) {
        int c = _getch();
        if (c == '\r' || c == '\n') { cout << "\n"; return buf; }
        else if (c == '\b' || c == 127) { if (!buf.empty()) { buf.pop_back(); cout << "\b \b"; } }
        else if (c == '\t') {
            if (buf.empty()) continue;
            vector<pair<string, string>> matches;
            for (auto& [cmd, _] : cmds) if (cmd.find(buf) == 0) matches.push_back({ cmd, cmdDesc.count(cmd) ? cmdDesc[cmd] : "" });
            for (auto& [al, _] : aliases) if (al.find(buf) == 0) matches.push_back({ al, Lang::t("alias_mark") });
            if (matches.empty()) cout << '\a';
            else if (matches.size() == 1) {
                string comp = matches[0].first.substr(buf.size());
                buf += comp; cout << comp;
                cout << "\n  " << matches[0].first << " - " << matches[0].second << "\n" << prompt << buf << flush;
            } else {
                cout << "\n";
                for (auto& m : matches) cout << "  " << m.first << " - " << m.second << "\n";
                cout << prompt << buf << flush;
            }
        }
        else if (c == 3) { cout << "^C\n"; buf.clear(); cout << prompt << flush; }  // Ctrl+C
        else if (c >= 32 && c <= 126) { buf += (char)c; cout << (char)c << flush; }
    }
}
#else
string readlineConsole(const string& prompt) {
    string buf; cout << prompt << flush;
    getline(cin, buf);
    return buf;
}
#endif

// 前向声明：GUI 命令元数据填充函数（供 consoleMain 调用，让 help 输出参数语法）
void rebuildCategories();
void help();

int consoleMain() {
#ifdef _WIN32
    // 设置控制台为 UTF-8，确保中文命令输出与 UI 文本正确显示
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);
#endif
    Lang::init();
    cout << Lang::t("lang_prompt");
    int ch; cin >> ch; cin.ignore();
    Lang::cur = (ch == 2) ? Lang::EN : Lang::ZH;
    cout << Lang::t("welcome") << endl;
    promptText = "BCMD"; currentPrompt = promptText + "> ";
    loadAliases();
    string savedProxy = readConfig("proxy");
    if (!savedProxy.empty()) setProxyEnv(savedProxy);
    init_cmds(); initCmdDesc();
    rebuildCategories();   // 控制台模式也重建分类元数据，便于 help 输出参数语法
    string input;
    while (true) {
        input = readlineConsole(currentPrompt);
        if (input == "exit") break;
        if (input == "help") { help(); continue; }
        if (input.empty()) continue;
        if (input.rfind("/title", 0) == 0) { cmd_title_prompt(input); continue; }
        auto it = aliases.find(input);
        if (it != aliases.end()) input = it->second;
        auto cmd = parse(input);
        if (cmds.count(cmd.cmd)) {
            try { cmds[cmd.cmd](cmd.args, cmd.flags); }
            catch (const exception& e) { cout << Lang::t("exception") << e.what() << endl; }
            catch (...) { cout << Lang::t("unknown_ex") << endl; }
        }
        else system(input.c_str());
    }
    return 0;
}

/*
================================================================
 第十六部分：GUI 日志系统
================================================================
*/
class GuiLogger {
public:
    static string logPath;
    static ofstream logFile;
    static bool enabled;   // 是否启用日志（由用户启动时选择）
    static void init() {
        // 询问用户是否保存操作日志
        wstring wAsk = utf8ToWide(Lang::t("gui_log_ask"));
        wstring wAskTitle = utf8ToWide(Lang::t("gui_log_ask_title"));
        int choice = MessageBoxW(NULL, wAsk.c_str(), wAskTitle.c_str(),
            MB_YESNO | MB_ICONQUESTION);
        enabled = (choice == IDYES);
        if (!enabled) return;

        // 日志直接放在 exe 目录下的 log 文件夹中
        char exePath[MAX_PATH] = { 0 };
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        fs::path exeDir = fs::path(exePath).parent_path();
        fs::path logDir = exeDir / "log";
        fs::create_directories(logDir);
        logPath = (logDir / "bcmd.log").string();
        logFile.open(logPath, ios::app);
        if (logFile.is_open()) {
            auto now = chrono::system_clock::now();
            time_t tt = chrono::system_clock::to_time_t(now);
            stringstream ss; ss << put_time(localtime(&tt), "%Y-%m-%d %H:%M:%S");
            logFile << "\n=== Better CMD GUI " << Lang::t("gui_new_session") << " ===" << endl;
            logFile << Lang::t("gui_start_time") << ss.str() << endl;
            logFile << Lang::t("gui_user") << envOr("USERNAME", "unknown") << endl;
            logFile << "=========================" << endl;
        }
    }
    static void write(const string& msg) {
        if (!enabled) return;
        if (logFile.is_open()) {
            auto now = chrono::system_clock::now();
            time_t tt = chrono::system_clock::to_time_t(now);
            stringstream ss; ss << put_time(localtime(&tt), "[%H:%M:%S] ");
            logFile << ss.str() << msg << endl;
            logFile.flush();
        }
    }
    static void close() {
        if (!enabled) return;
        if (logFile.is_open()) { logFile << "=== " << Lang::t("gui_session_end") << " ===" << endl; logFile.close(); }
    }
};
string GuiLogger::logPath;
ofstream GuiLogger::logFile;
bool GuiLogger::enabled = false;

/*
================================================================
 第十七部分：GUI 命令元数据（用于生成按钮和参数对话框）
================================================================
*/
// 参数定义
struct ParamDef {
    string key;                                // 参数键
    string label;                              // 中文标签
    string placeholder;                        // 输入提示
    string defaultValue;                       // 默认值
    bool optional = false;                     // 是否可选
    bool isSelect = false;                     // 是否下拉选择
    vector<pair<string, string>> options;      // 下拉选项 (value, 显示文本)
};

// 命令定义
struct CmdDef {
    string name;                    // 显示的命令名（如 "ping"）
    string desc;                    // 描述
    vector<ParamDef> params;        // 参数列表
};

// 分类定义
struct CategoryDef {
    string name;
    string icon;
    vector<CmdDef> commands;
};

// 所有分类（运行时根据当前语言填充，便于在 GUI 中切换语言）
vector<CategoryDef> g_categories;

// 根据 Lang::cur 重建分类/按钮元数据（中英文）
void rebuildCategories() {
    g_categories.clear();
    g_categories = {
        { Lang::t("cat_file"), "[F]", {
            {"dir",   Lang::t("desc_dir"),   {{"path", Lang::t("lbl_path"), Lang::t("ph_curdir"), "", true}}},
            {"cd",    Lang::t("desc_cd"),    {{"path", Lang::t("lbl_path"), Lang::t("ph_dstdir"), ""}}},
            {"md",    Lang::t("desc_md"),    {{"name", Lang::t("lbl_name"), Lang::t("ph_newfolder"), ""}}},
            {"rd",    Lang::t("desc_rd"),    {{"name", Lang::t("lbl_name"), "", ""}}},
            {"del",   Lang::t("desc_del"),   {{"file", Lang::t("lbl_file"), "", ""}}},
            {"copy",  Lang::t("desc_copy"),  {{"src", Lang::t("lbl_src"), "", ""},{"dst", Lang::t("lbl_dst"), "", ""}}},
            {"move",  Lang::t("desc_move"),  {{"src", Lang::t("lbl_src"), "", ""},{"dst", Lang::t("lbl_dst"), "", ""}}},
            {"ren",   Lang::t("desc_ren"),   {{"old", Lang::t("lbl_old"), "", ""},{"new", Lang::t("lbl_new"), "", ""}}},
            {"type",  Lang::t("desc_type"),  {{"file", Lang::t("lbl_file"), "", ""}}},
            {"tree",  Lang::t("desc_tree"),  {{"path", Lang::t("lbl_root"), Lang::t("ph_curdir"), "", true}}},
            {"find",  Lang::t("desc_find"),  {{"kw", Lang::t("lbl_kw"), "", ""},{"file", Lang::t("lbl_file"), "", ""}}},
            {"more",  Lang::t("desc_more"),  {{"file", Lang::t("lbl_file"), "", ""}}},
            {"sort",  Lang::t("desc_sort"),  {{"file", Lang::t("lbl_file"), "", ""}}},
            {"fc",    Lang::t("desc_fc"),    {{"f1", Lang::t("lbl_f1"), "", ""},{"f2", Lang::t("lbl_f2"), "", ""}}}
        }},
        { Lang::t("cat_net"), "[N]", {
            {"ping",     Lang::t("desc_ping"),     {{"host", Lang::t("lbl_host"), "", "8.8.8.8"},{"n", Lang::t("lbl_n"), "", "4", true}}},
            {"tracert",  Lang::t("desc_tracert"),  {{"host", Lang::t("lbl_host"), "", "8.8.8.8"}}},
            {"nslookup", Lang::t("desc_nslookup"), {{"domain", Lang::t("lbl_domain"), "", "baidu.com"}}},
            {"ipconfig", Lang::t("desc_ipconfig"), {}},
            {"netstat",  Lang::t("desc_netstat"),  {}},
            {"route",    Lang::t("desc_route"),    {}},
            {"arp",      Lang::t("desc_arp"),      {}},
            {"getmac",   Lang::t("desc_getmac"),   {}},
            {"portscan", Lang::t("desc_portscan"), {{"host", Lang::t("lbl_host"), "", ""},{"port", Lang::t("lbl_port"), "", "80"}}},
            {"wget",     Lang::t("desc_wget"),     {{"url", Lang::t("lbl_url"), "", ""}}},
            {"httpserver",Lang::t("desc_httpserver"), {}},
            {"whois",    Lang::t("desc_whois"),    {{"domain", Lang::t("lbl_domain"), "", ""}}},
            {"dig",      Lang::t("desc_dig"),      {{"domain", Lang::t("lbl_domain"), "", ""}}},
            {"ssh",      Lang::t("desc_ssh"),      {{"host", Lang::t("lbl_user_host"), Lang::t("ph_user_at_host"), ""}}}
        }},
        { Lang::t("cat_sys"), "[S]", {
            {"systeminfo",Lang::t("desc_systeminfo"), {}},
            {"tasklist",  Lang::t("desc_tasklist"),  {}},
            {"taskkill",  Lang::t("desc_taskkill"),  {{"pid", Lang::t("lbl_pid"), "", ""}}},
            {"shutdown",  Lang::t("desc_shutdown"),  {}},
            {"chkdsk",    Lang::t("desc_chkdsk"),    {}},
            {"vol",       Lang::t("desc_vol"),       {}},
            {"diskusage", Lang::t("desc_diskusage"), {}},
            {"sysmon",    Lang::t("desc_sysmon"),    {}},
            {"procmon",   Lang::t("desc_procmon"),   {}},
            {"service",   Lang::t("desc_service"),   {}},
            {"startup",   Lang::t("desc_startup"),   {}},
            {"hwinfo",    Lang::t("desc_hwinfo"),    {}},
            {"colorpick", Lang::t("desc_colorpick"), {}}
        }},
        { Lang::t("cat_text"), "[T]", {
            {"echo",     Lang::t("desc_echo"),     {{"text", Lang::t("lbl_text"), "", ""}}},
            {"wc",       Lang::t("desc_wc"),       {{"file", Lang::t("lbl_file"), "", ""}}},
            {"linesort", Lang::t("desc_linesort"), {{"file", Lang::t("lbl_file"), "", ""}}},
            {"uniq",     Lang::t("desc_uniq"),     {{"file", Lang::t("lbl_file"), "", ""}}},
            {"logview",  Lang::t("desc_logview"),  {{"file", Lang::t("ph_log_file"), "", ""}}},
            {"json",     Lang::t("desc_json"),     {{"file", Lang::t("ph_json_file"), "", ""}}},
            {"regex",    Lang::t("desc_regex"),    {{"pattern", Lang::t("lbl_pattern"), "", ""},{"text", Lang::t("lbl_text"), "", ""}}},
            {"hexdump",  Lang::t("desc_hexdump"),  {{"file", Lang::t("lbl_file"), "", ""}}},
            {"tail",     Lang::t("desc_tail"),     {{"file", Lang::t("lbl_file"), "", ""},{"n", Lang::t("lbl_n"), "", "10", true}}},
            {"head",     Lang::t("desc_head"),     {{"file", Lang::t("lbl_file"), "", ""},{"n", Lang::t("lbl_n"), "", "10", true}}}
        }},
        { Lang::t("cat_crypto"), "[C]", {
            {"md5",      Lang::t("desc_md5"),      {{"file", Lang::t("lbl_file"), "", ""}}},
            {"sha256",   Lang::t("desc_sha256"),   {{"file", Lang::t("lbl_file"), "", ""}}},
            {"checksum", Lang::t("desc_checksum"), {{"file", Lang::t("lbl_file"), "", ""}}},
            {"encrypt",  Lang::t("desc_encrypt"),  {{"file", Lang::t("lbl_file"), "", ""},{"pwd", Lang::t("lbl_pwd"), "", ""}}},
            {"decrypt",  Lang::t("desc_decrypt"),  {{"file", Lang::t("ph_enc_file"), "", ""},{"pwd", Lang::t("lbl_pwd"), "", ""}}},
            {"zip",      Lang::t("desc_zip"),       {{"src", Lang::t("lbl_src"), "", ""},{"dst", Lang::t("ph_dst_zip"), "", ""}}},
            {"unzip",    Lang::t("desc_unzip"),     {{"file", Lang::t("ph_zip_file"), "", ""}}},
            {"split",    Lang::t("desc_split"),    {{"file", Lang::t("lbl_file"), "", ""},{"mb", Lang::t("lbl_mb"), "", ""},{"prefix", Lang::t("lbl_prefix"), "", ""}}},
            {"merge",    Lang::t("desc_merge"),     {{"out", Lang::t("lbl_out"), "", ""},{"parts", Lang::t("lbl_parts"), "", ""}}},
            {"base64",   Lang::t("desc_base64"),    {{"mode", Lang::t("lbl_mode"), "", "-e", false, true, {{"-e", Lang::t("opt_encode")}, {"-d", Lang::t("opt_decode")}}},{"text", Lang::t("lbl_text"), "", ""}}}
        }},
        { Lang::t("cat_util"), "[U]", {
            {"calc",    Lang::t("desc_calc"),    {{"a", Lang::t("lbl_a"), "", ""},{"op", Lang::t("lbl_op"), "", "+", false, true, {{"+", "+"},{"-", "-"},{"*", "x"},{"/", "/"}}},{"b", Lang::t("lbl_b"), "", ""}}},
            {"rand",    Lang::t("desc_rand"),    {{"lo", Lang::t("lbl_lo"), "", "0"},{"hi", Lang::t("lbl_hi"), "", "100"}}},
            {"passgen", Lang::t("desc_passgen"), {{"len", Lang::t("lbl_len"), "", "16"}}},
            {"uuid",    Lang::t("desc_uuid"),    {}},
            {"qrcode",  Lang::t("desc_qrcode"),  {{"text", Lang::t("ph_to_gen"), "", ""}}},
            {"barcode", Lang::t("desc_barcode"), {{"text", Lang::t("ph_to_gen"), "", ""}}},
            {"vcard",   Lang::t("desc_vcard"),   {{"name", Lang::t("lbl_name"), "", ""},{"phone", Lang::t("lbl_phone"), "", ""}}},
            {"convert", Lang::t("desc_convert"), {{"value", Lang::t("lbl_value"), "", ""},{"from", Lang::t("lbl_from"), "", "cm", false, true, {{"cm", "cm"},{"inch", "inch"},{"m", "m"},{"ft", "ft"},{"kg", "kg"},{"lb", "lb"},{"c", "C"},{"f", "F"}}},{"to", Lang::t("lbl_to"), "", "inch", false, true, {{"cm", "cm"},{"inch", "inch"},{"m", "m"},{"ft", "ft"},{"kg", "kg"},{"lb", "lb"},{"c", "C"},{"f", "F"}}}}},
            {"scicalc", Lang::t("desc_scicalc"), {{"expr", Lang::t("lbl_expr"), "sqrt(2)+3^2", ""}}},
            {"timer",   Lang::t("desc_timer"),   {{"sec", Lang::t("lbl_sec"), "", ""},{"msg", Lang::t("lbl_msg"), "", "", true}}},
            {"todo",    Lang::t("desc_todo"),    {{"action", Lang::t("lbl_action"), "", "list", false, true, {{"list", Lang::t("opt_view")},{"add", Lang::t("opt_add")},{"done", Lang::t("opt_done")}}},{"text", Lang::t("lbl_text"), "", "", true}}},
            {"notes",   Lang::t("desc_notes"),   {{"title", Lang::t("lbl_title"), "", ""},{"content", Lang::t("lbl_content"), "", "", true}}},
            {"alias",   Lang::t("desc_alias"),    {{"name", Lang::t("lbl_name"), "", ""},{"cmd", Lang::t("lbl_cmd"), "", ""}}},
            {"proxy",   Lang::t("desc_proxy"),    {{"action", Lang::t("lbl_action"), "", "set", false, true, {{"set", Lang::t("opt_set")},{"unset", Lang::t("opt_unset")}}},{"addr", Lang::t("lbl_addr"), "", "", true}}},
            {"weather", Lang::t("desc_weather"),  {}},
            {"translate",Lang::t("desc_translate"), {{"text", Lang::t("ph_to_translate"), "", ""}}}
        }},
        { Lang::t("cat_pic"), "[P]", {
            {"picture -s",Lang::t("btn_shot_now"),  {{"path", Lang::t("ph_savepath"), Lang::t("ph_default_empty"), "", true}}},
            {"picture -r",Lang::t("btn_rec_start"), {{"sec", Lang::t("ph_default_60"), "", "60", true}}},
            {"picture -k",Lang::t("btn_set_hotkey"), {{"key", Lang::t("lbl_key"), "Ctrl+Shift+A", ""}}},
            {"picture -c",Lang::t("btn_clr_hotkey"), {}},
            {"screenshot",Lang::t("desc_screenshot"), {}},
            {"record",    Lang::t("desc_record"),   {}}
        }},
        { Lang::t("cat_ai"), "[A]", {
            {"ai",   Lang::t("desc_ai"),   {{"prompt", Lang::t("lbl_prompt"), "", ""}}},
            {"ide",  Lang::t("desc_ide"),  {{"path", Lang::t("ph_ide_cmd"), "code", ""}}},
            {"edit", Lang::t("desc_edit"), {{"file", Lang::t("lbl_file"), "", ""}}},
            {"chat", Lang::t("desc_chat"), {{"host", Lang::t("lbl_host"), "", ""},{"port", Lang::t("lbl_port"), "", "23"}}}
        }},
        { Lang::t("cat_bp"), "[B]", {
            {"BP",       Lang::t("btn_bp_start"), {}},
            {"BP -stop", Lang::t("btn_bp_stop"),  {}},
            {"BP -list", Lang::t("btn_bp_list"),  {}}
        }},
        { Lang::t("cat_other"), "[?]", {
            {"help",  Lang::t("desc_help"),   {}},
            {"ver",   Lang::t("desc_ver"),    {}},
            {"date",  Lang::t("desc_date"),  {}},
            {"time",  Lang::t("desc_time"),   {}},
            {"bench", Lang::t("desc_bench"), {{"cmd", Lang::t("ph_run_cmd"), "", ""}}},
            {"author",Lang::t("desc_author"),{}}
        }}
    };
}

// ==================== help 命令 ====================
// 输出每个命令的用法与语法：<命令本体>  [可选参数]  <必填参数>
void help() {
    cout << Lang::t("help_title") << "\n";
    cout << Lang::t("help_legend") << "\n";
    cout << Lang::t("help_see_gui") << "\n";
    cout << string(72, '=') << "\n";

    // 优先使用 g_categories（含参数定义），逐分类、逐命令输出
    if (!g_categories.empty()) {
        for (const auto& cat : g_categories) {
            cout << "\n" << cat.icon << "  " << cat.name << "\n";
            cout << string(48, '-') << "\n";
            for (const auto& cmd : cat.commands) {
                // ---- 第一行：命令本体 + 描述 ----
                // 用 <命令> 表示命令本体，[参数] 表示可选，<参数> 表示必填
                cout << "  <" << cmd.name << ">   " << cmd.desc << "\n";

                // ---- 第二行：完整语法 ----
                if (!cmd.params.empty()) {
                    cout << "      "
                         << Lang::t("help_syntax") << cmd.name;
                    for (const auto& p : cmd.params) {
                        cout << (p.optional ? " [" : " <");
                        cout << p.key;
                        cout << (p.optional ? "]" : ">");
                    }
                    cout << "\n";

                    // ---- 第三行：参数说明 ----
                    for (const auto& p : cmd.params) {
                        cout << "        ";
                        cout << (p.optional ? "[" : "<") << p.key << (p.optional ? "]" : ">");
                        cout << "  " << p.label;
                        if (!p.placeholder.empty()) cout << "  (" << p.placeholder << ")";
                        if (p.isSelect) {
                            cout << "  " << Lang::t("help_options");
                            for (size_t k = 0; k < p.options.size(); ++k) {
                                if (k) cout << " | ";
                                cout << p.options[k].first << "=" << p.options[k].second;
                            }
                        }
                        cout << "\n";
                    }
                }
                cout << "\n";
            }
        }
    }

    // 补充：列出 g_categories 未覆盖的命令
    set<string> covered;
    for (const auto& cat : g_categories)
        for (const auto& cmd : cat.commands) {
            string n = cmd.name;
            auto sp = n.find(' ');
            if (sp != string::npos) n = n.substr(0, sp);
            covered.insert(n);
        }
    covered.insert("BP");
    vector<string> extra;
    for (const auto& kv : cmds) {
        const string& n = kv.first;
        if (n.empty() || n[0] == '/') continue;
        if (covered.count(n)) continue;
        extra.push_back(n);
    }
    if (!extra.empty()) {
        cout << "\n" << Lang::t("help_extra") << "\n";
        cout << string(48, '-') << "\n";
        for (const auto& n : extra) {
            cout << "  <" << n << ">   " << (cmdDesc.count(n) ? cmdDesc[n] : "") << "\n";
        }
        cout << "\n";
    }

    cout << string(72, '=') << "\n";
    cout << Lang::t("tab_exit") << "\n";
}

/*
================================================================
 第十八部分：GUI 主模块（Win32）
================================================================
*/
#ifdef _WIN32

// 控件 ID
#define IDC_SIDEBAR      1001
#define IDC_LOGPANEL     1002
#define IDC_STATUSBAR    1003
#define IDC_CMDBTN_BASE  20000

// 全局状态
HWND g_hMain = NULL;
HWND g_hSidebar = NULL;
HWND g_hLogPanel = NULL;
HWND g_hStatusBar = NULL;
HFONT g_hFontUI = NULL;
HFONT g_hFontMono = NULL;
WNDPROC g_pLogOldProc = NULL;   // 日志面板原始窗口过程（用于子类化拦截右键菜单）
int g_currentCat = 0;
vector<HWND> g_cmdButtons;
unordered_map<int, pair<int, int>> g_btnMap;   // 按钮ID -> (分类索引, 命令索引)

// 当前打开的参数对话框状态
HWND g_hParamDlg = NULL;
const CmdDef* g_pendingCmd = NULL;
int g_pendingCat = 0;
int g_pendingIdx = 0;
vector<HWND> g_paramInputs;                    // 对话框中的输入控件
vector<HWND> g_paramLabels;
string g_pendingCmdFullName;                   // 待执行命令的完整名字

// ---- 日志面板子类化过程：拦截右键菜单，只提供"复制/全选" ----
#define ID_LOG_COPY  5001
#define ID_LOG_SELECTALL 5002
static LRESULT CALLBACK LogPanelProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CONTEXTMENU) {
        POINT pt = { (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam) };
        HMENU hMenu = CreatePopupMenu();
        wstring wCopy = utf8ToWide(Lang::t("gui_log_copy"));
        wstring wSelAll = utf8ToWide(Lang::t("gui_log_selectall"));
        AppendMenuW(hMenu, MF_STRING, ID_LOG_COPY, wCopy.c_str());
        AppendMenuW(hMenu, MF_STRING, ID_LOG_SELECTALL, wSelAll.c_str());
        // 复制是否可用（有选中文本）
        DWORD sel = SendMessageW(hWnd, EM_GETSEL, 0, 0);
        EnableMenuItem(hMenu, ID_LOG_COPY, (HIWORD(sel) != LOWORD(sel)) ? MF_ENABLED : MF_GRAYED);
        int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
        if (cmd == ID_LOG_COPY) SendMessageW(hWnd, WM_COPY, 0, 0);
        else if (cmd == ID_LOG_SELECTALL) SendMessageW(hWnd, EM_SETSEL, 0, -1);
        DestroyMenu(hMenu);
        return 0;
    }
    return CallWindowProcW(g_pLogOldProc, hWnd, msg, wParam, lParam);
}

// ---- 向日志面板追加文本 ----
void GuiAppendLog(const string& text) {
    if (!g_hLogPanel) return;
    // 先过滤控制字符（UTF-8 安全：只处理 ASCII 控制字符，不动多字节序列）
    string filtered;
    filtered.reserve(text.size() + 4);
    for (char c : text) {
        unsigned char uc = (unsigned char)c;
        if (c == '\n') {
            filtered += "\r\n";
        } else if (c == '\t') {
            filtered += "    ";  // Tab 展开为 4 空格
        } else if (c == '\r') {
            continue;  // 丢弃 \r，由 \n 统一处理
        } else if (uc < 32) {
            continue;  // 丢弃其他控制字符
        } else {
            filtered += c;
        }
    }
    if (filtered.empty() || filtered.back() != '\n') filtered += "\r\n";

    // 转为 UTF-16，用 Unicode API 显示（避免中文 Windows 上 GBK/UTF-8 乱码重叠）
    wstring wout = utf8ToWide(filtered);
    int len = GetWindowTextLengthW(g_hLogPanel);
    SendMessageW(g_hLogPanel, EM_SETSEL, len, len);
    SendMessageW(g_hLogPanel, EM_REPLACESEL, FALSE, (LPARAM)wout.c_str());
    SendMessageW(g_hLogPanel, EM_SCROLLCARET, 0, 0);
}

// ---- 写日志（同时写文件） ----
void GuiLog(const string& text) {
    GuiAppendLog(text);
    GuiLogger::write(text);
}

// ---- 状态栏更新 ----
void GuiSetStatus(const string& text) {
    if (g_hStatusBar) {
        wstring w = utf8ToWide(text);
        SetWindowTextW(g_hStatusBar, w.c_str());
    }
}

// ---- 重定向 cout 执行命令 ----
string GuiRunCommand(const string& input) {
    GuiLogger::write("[输入] " + input);
    stringstream ss;
    streambuf* old = cout.rdbuf(ss.rdbuf());

    // RAII 保证 cout 必然还原
    struct Guard {
        streambuf*& target; streambuf* saved;
        Guard(streambuf*& t, streambuf* s) : target(t), saved(s) {}
        ~Guard() { cout.rdbuf(saved); }
    } guard(old, old);
    (void)guard;

    try {
        // 特殊处理 /title
        if (input.rfind("/title", 0) == 0) {
            cmd_title_prompt(input);
            return ss.str();
        }

        // 别名替换
        string realInput = input;
        auto it = aliases.find(realInput);
        if (it != aliases.end()) realInput = it->second;

        // 常规解析
        auto cmd = parse(realInput);
        if (cmds.count(cmd.cmd)) {
            try { cmds[cmd.cmd](cmd.args, cmd.flags); }
            catch (const exception& e) { cout << Lang::t("exception") << e.what() << endl; }
            catch (...) { cout << Lang::t("unknown_ex") << endl; }
        } else {
            string out = exec(realInput.c_str());
            cout << out;
        }
    } catch (...) {
        cout << Lang::t("gui_ex") << endl;
    }
    return ss.str();
}

// ---- 前向声明 ----
void RenderCategory(int catIndex);
LRESULT CALLBACK ParamDlgProc(HWND, UINT, WPARAM, LPARAM);

// ---- 关闭参数对话框 ----
void CloseParamDialog() {
    if (g_hParamDlg) {
        DestroyWindow(g_hParamDlg);
        g_hParamDlg = NULL;
    }
    g_paramInputs.clear();
    g_paramLabels.clear();
    g_pendingCmd = NULL;
    if (g_hMain) EnableWindow(g_hMain, TRUE);
    SetFocus(g_hMain);
}

// ---- 从参数对话框收集参数并执行 ----
void ExecuteFromParamDialog() {
    if (!g_pendingCmd) { CloseParamDialog(); return; }

    // 保存待执行命令信息（因为 CloseParamDialog 会清空 g_pendingCmd）
    string cmdName = g_pendingCmd->name;
    vector<ParamDef> paramsCopy = g_pendingCmd->params;

    // 收集参数：值为空且必填的报错；可选项为空则跳过
    vector<string> args;
    bool missing = false;
    for (size_t i = 0; i < paramsCopy.size(); ++i) {
        const auto& p = paramsCopy[i];
        wchar_t wbuf[2048] = { 0 };
        if (i < g_paramInputs.size() && g_paramInputs[i]) {
            GetWindowTextW(g_paramInputs[i], wbuf, 2047);
        }
        string val;
        int ulen = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, NULL, 0, NULL, NULL);
        if (ulen > 1) {
            val.resize(ulen - 1);
            WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, &val[0], ulen, NULL, NULL);
        }
        if (val.empty()) {
            if (!p.optional) missing = true;
            continue;
        }
        args.push_back(val);
    }

    CloseParamDialog();
    if (missing) {
        GuiAppendLog(Lang::t("gui_error") + Lang::t("gui_param_missing"));
        GuiSetStatus(Lang::t("gui_cancelled"));
        return;
    }

    // 拼接显示
    string fullCmd = cmdName;
    for (auto& a : args) { fullCmd += " "; fullCmd += a; }

    GuiLog(Lang::t("gui_run_prefix") + fullCmd);
    string out = GuiRunCommand(fullCmd);
    if (!out.empty()) GuiAppendLog(out);
    GuiSetStatus(Lang::t("gui_done_status") + cmdName);
}

// ---- 参数对话框窗口过程 ----
LRESULT CALLBACK ParamDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        if (!g_pendingCmd) return 0;

        int y = 15;
        int margin = 20;
        int labelH = 22;
        int ctrlH = 30;
        int gap = 12;

        // 标题
        string titleText = g_pendingCmd->name + "  -  " + g_pendingCmd->desc;
        wstring wDlgTitle = utf8ToWide(titleText);
        HWND hTitle = CreateWindowExW(0, L"STATIC", wDlgTitle.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            margin, y, 460, 24, hWnd, NULL, NULL, NULL);
        SendMessageW(hTitle, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
        y += 34;

        // 各参数
        for (size_t i = 0; i < g_pendingCmd->params.size(); ++i) {
            const auto& p = g_pendingCmd->params[i];

            // 标签
            string labelText = p.label;
            if (p.optional) labelText += Lang::t("gui_optional");
            wstring wLabel = utf8ToWide(labelText);
            HWND hLabel = CreateWindowExW(0, L"STATIC", wLabel.c_str(),
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                margin, y, 460, labelH, hWnd, NULL, NULL, NULL);
            SendMessageW(hLabel, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
            g_paramLabels.push_back(hLabel);
            y += labelH;

            // 输入控件
            HWND hInput = NULL;
            if (p.isSelect) {
                hInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"",
                    WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                    margin, y, 460, 200, hWnd, NULL, NULL, NULL);
                for (auto& opt : p.options) {
                    wstring wOpt = utf8ToWide(opt.second);
                    int idx = (int)SendMessageW(hInput, CB_ADDSTRING, 0, (LPARAM)wOpt.c_str());
                    SendMessageW(hInput, CB_SETITEMDATA, idx, (LPARAM)opt.first.c_str());
                }
                // 默认选中
                if (!p.defaultValue.empty()) {
                    for (int k = 0; k < (int)p.options.size(); ++k) {
                        if (p.options[k].first == p.defaultValue) {
                            SendMessageW(hInput, CB_SETCURSEL, k, 0);
                            break;
                        }
                    }
                } else if (!p.options.empty()) {
                    SendMessageW(hInput, CB_SETCURSEL, 0, 0);
                }
            } else {
                wstring wDefault = utf8ToWide(p.defaultValue);
                hInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", wDefault.c_str(),
                    WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                    margin, y, 460, ctrlH, hWnd, NULL, NULL, NULL);
                // 设置 placeholder
                if (!p.placeholder.empty() && p.defaultValue.empty()) {
                    wstring wPh = utf8ToWide(p.placeholder);
                    SendMessageW(hInput, EM_SETCUEBANNER, TRUE, (LPARAM)wPh.c_str());
                }
            }
            SendMessageW(hInput, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);
            g_paramInputs.push_back(hInput);

            // 第一个输入控件自动聚焦
            if (i == 0) SetFocus(hInput);

            y += ctrlH + gap;
        }

        // 按钮
        y += 10;
        wstring wOk = utf8ToWide(Lang::t("gui_ok"));
        wstring wCancel = utf8ToWide(Lang::t("gui_cancel"));
        HWND hOk = CreateWindowExW(0, L"BUTTON", wOk.c_str(),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            280, y, 90, 34, hWnd, (HMENU)1, NULL, NULL);
        HWND hCancel = CreateWindowExW(0, L"BUTTON", wCancel.c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            380, y, 90, 34, hWnd, (HMENU)2, NULL, NULL);
        SendMessageA(hOk, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
        SendMessageA(hCancel, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

        // 调整窗口大小
        int dlgH = y + 34 + 20 + 30;  // 加标题栏高度
        int dlgW = 520;
        RECT rcMain;
        GetWindowRect(g_hMain, &rcMain);
        int x = (rcMain.left + rcMain.right) / 2 - dlgW / 2;
        int yy = (rcMain.top + rcMain.bottom) / 2 - dlgH / 2;
        SetWindowPos(hWnd, NULL, x, yy, dlgW, dlgH, SWP_NOZORDER);

        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == 1) { ExecuteFromParamDialog(); return 0; }
        if (id == 2) { CloseParamDialog(); return 0; }
        break;
    }
    case WM_CLOSE:
        CloseParamDialog();
        return 0;
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(50, 50, 50));
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

// ---- 创建参数对话框 ----
void ShowParamDialog(int catIdx, int cmdIdx) {
    if (catIdx < 0 || catIdx >= (int)g_categories.size()) return;
    const auto& cat = g_categories[catIdx];
    if (cmdIdx < 0 || cmdIdx >= (int)cat.commands.size()) return;
    const auto& cmd = cat.commands[cmdIdx];

    // 无参数直接执行
    if (cmd.params.empty()) {
        GuiLog(Lang::t("gui_run_prefix") + cmd.name);
        string out = GuiRunCommand(cmd.name);
        if (!out.empty()) GuiAppendLog(out);
        GuiSetStatus(Lang::t("gui_done_status") + cmd.name);
        return;
    }

    // 保存状态
    g_pendingCmd = &cmd;
    g_pendingCat = catIdx;
    g_pendingIdx = cmdIdx;
    g_paramInputs.clear();
    g_paramLabels.clear();

    // 注册对话框类
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXA wc = { 0 };
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = ParamDlgProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = "BetterCMDParamDlg";
        RegisterClassExA(&wc);
        registered = true;
    }

    // 禁用主窗口（模态）
    EnableWindow(g_hMain, FALSE);

    wstring wInputTitle = utf8ToWide(Lang::t("gui_input_title"));
    g_hParamDlg = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"BetterCMDParamDlg", wInputTitle.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 520, 300,
        g_hMain, NULL, GetModuleHandle(NULL), NULL);
    ShowWindow(g_hParamDlg, SW_SHOW);
    UpdateWindow(g_hParamDlg);
}

// ---- 按钮命令处理 ----
void OnCommandButtonClick(int btnId) {
    auto it = g_btnMap.find(btnId);
    if (it == g_btnMap.end()) return;
    ShowParamDialog(it->second.first, it->second.second);
}

// ---- 渲染分类下的命令按钮 ----
void RenderCategory(int catIndex) {
    // 销毁旧按钮
    for (HWND h : g_cmdButtons) if (h) DestroyWindow(h);
    g_cmdButtons.clear();
    g_btnMap.clear();

    if (catIndex < 0 || catIndex >= (int)g_categories.size()) return;
    const auto& cat = g_categories[catIndex];

    // 获取中间区域大小
    RECT rc;
    GetClientRect(g_hMain, &rc);
    int sidebarW = 200;
    int logW = 360;
    int contentX = sidebarW + 10;
    int contentW = rc.right - contentX - logW - 10;
    int contentY = 50;
    int contentH = rc.bottom - contentY - 40;

    // 标题
    string titleStr = cat.icon + "  " + cat.name + Lang::t("gui_count_fmt") + to_string(cat.commands.size()) + Lang::t("gui_count_unit");
    wstring wTitle = utf8ToWide(titleStr);
    HWND hTitle = CreateWindowExW(0, L"STATIC", wTitle.c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        contentX, 15, contentW, 28, g_hMain, NULL, NULL, NULL);
    SendMessageA(hTitle, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
    g_cmdButtons.push_back(hTitle);

    // 命令按钮网格（卡片式，大尺寸易点击）
    int btnW = 200;
    int btnH = 72;
    int gap = 14;
    int cols = max(1, (contentW + gap) / (btnW + gap));
    int x = contentX;
    int y = contentY;
    int col = 0;

    for (size_t i = 0; i < cat.commands.size(); ++i) {
        const auto& cmd = cat.commands[i];

        // 显示文本：命令名 + 换行 + 描述
        string btnText = cmd.name + "\n" + cmd.desc;
        wstring wBtnText = utf8ToWide(btnText);

        int id = IDC_CMDBTN_BASE + (int)i;
        HWND hBtn = CreateWindowExW(0, L"BUTTON", wBtnText.c_str(),
            WS_CHILD | WS_VISIBLE | BS_MULTILINE | BS_PUSHBUTTON,
            x, y, btnW, btnH,
            g_hMain, (HMENU)(INT_PTR)id, NULL, NULL);
        SendMessageA(hBtn, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
        g_cmdButtons.push_back(hBtn);

        g_btnMap[id] = { catIndex, (int)i };

        col++;
        if (col >= cols) { col = 0; x = contentX; y += btnH + gap; }
        else x += btnW + gap;
    }
}

// ---- 侧边栏分类点击 ----
void OnCategoryClick(int sel) {
    if (sel < 0 || sel >= (int)g_categories.size()) return;
    g_currentCat = sel;
    RenderCategory(sel);
    GuiSetStatus(Lang::t("gui_cur_cat") + g_categories[sel].name);
    GuiLogger::write(Lang::t("gui_switch_cat") + g_categories[sel].name);
}

// ---- 主窗口过程 ----
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // 现代化字体（UI 用 Segoe UI；日志面板用微软雅黑，中英文均支持，避免字符重叠）
        g_hFontUI = CreateFontA(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        g_hFontMono = CreateFontA(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Microsoft YaHei UI");

        RECT rc;
        GetClientRect(hWnd, &rc);
        int sidebarW = 200;
        int logW = 360;
        int statusH = 28;

        // 左侧分类列表（Unicode 版本，支持中文）
        g_hSidebar = CreateWindowExW(0, L"LISTBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            0, 0, sidebarW, rc.bottom - statusH,
            hWnd, (HMENU)IDC_SIDEBAR, NULL, NULL);
        SendMessageW(g_hSidebar, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
        for (auto& c : g_categories) {
            wstring wItem = utf8ToWide(c.icon + "  " + c.name);
            SendMessageW(g_hSidebar, LB_ADDSTRING, 0, (LPARAM)wItem.c_str());
        }
        SendMessageW(g_hSidebar, LB_SETCURSEL, 0, 0);

        // 右侧日志面板（RichEdit，原生支持 Unicode/CJK，大量文本不重叠）
        // 先加载 msftedit.dll（提供 RICHEDIT50W 类）
        static HMODULE hRichEdit = LoadLibraryA("msftedit.dll");
        const wchar_t* editClass = hRichEdit ? L"RICHEDIT50W" : L"EDIT";
        g_hLogPanel = CreateWindowExW(0, editClass, L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            rc.right - logW, 0, logW, rc.bottom - statusH,
            hWnd, (HMENU)IDC_LOGPANEL, NULL, NULL);
        SendMessageW(g_hLogPanel, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);
        // 禁用自动换行（按原始宽度显示），设置只读
        SendMessageW(g_hLogPanel, EM_SETTARGETDEVICE, 0, 0);
        SendMessageW(g_hLogPanel, EM_SETREADONLY, TRUE, 0);
        // 子类化日志面板，拦截右键菜单（只提供复制/全选，去掉"插入 Unicode 控制字符"等）
        g_pLogOldProc = (WNDPROC)SetWindowLongPtrW(g_hLogPanel, GWLP_WNDPROC, (LONG_PTR)LogPanelProc);

        // 状态栏
        wstring wReady = utf8ToWide(Lang::t("gui_ready"));
        g_hStatusBar = CreateWindowExW(0, L"STATIC", wReady.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            6, rc.bottom - statusH + 4, rc.right - 12, 22,
            hWnd, (HMENU)IDC_STATUSBAR, NULL, NULL);
        SendMessageA(g_hStatusBar, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

        // 初始日志
        GuiAppendLog(Lang::t("gui_welcome") + VERSION);
        GuiAppendLog(Lang::t("gui_usage1"));
        GuiAppendLog(Lang::t("gui_usage2"));
        GuiAppendLog("");

        // 渲染第一个分类
        OnCategoryClick(0);
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);

        // 侧边栏选择
        if (id == IDC_SIDEBAR && code == LBN_SELCHANGE) {
            int sel = (int)SendMessageA(g_hSidebar, LB_GETCURSEL, 0, 0);
            OnCategoryClick(sel);
            return 0;
        }

        // 命令按钮
        if (id >= IDC_CMDBTN_BASE && id < IDC_CMDBTN_BASE + 10000) {
            OnCommandButtonClick(id);
            return 0;
        }
        break;
    }
    case WM_SIZE: {
        if (!g_hSidebar || !g_hLogPanel) break;
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        int sidebarW = 200;
        int logW = 360;
        int statusH = 28;

        SetWindowPos(g_hSidebar, NULL, 0, 0, sidebarW, h - statusH, SWP_NOZORDER);
        SetWindowPos(g_hLogPanel, NULL, w - logW, 0, logW, h - statusH, SWP_NOZORDER);
        SetWindowPos(g_hStatusBar, NULL, 6, h - statusH + 4, w - 12, 22, SWP_NOZORDER);

        // 重新渲染命令按钮（因为中间区域变了）
        RenderCategory(g_currentCat);
        return 0;
    }
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        if ((HWND)lParam == g_hLogPanel) {
            SetTextColor(hdc, RGB(204, 204, 204));
            SetBkColor(hdc, RGB(37, 37, 38));
            static HBRUSH hbrLog = CreateSolidBrush(RGB(37, 37, 38));
            return (LRESULT)hbrLog;
        }
        SetTextColor(hdc, RGB(220, 220, 220));
        SetBkColor(hdc, RGB(45, 45, 48));
        static HBRUSH hbrEdit = CreateSolidBrush(RGB(45, 45, 48));
        return (LRESULT)hbrEdit;
    }
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(220, 220, 220));
        SetBkColor(hdc, RGB(45, 45, 48));
        static HBRUSH hbrList = CreateSolidBrush(RGB(45, 45, 48));
        return (LRESULT)hbrList;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(200, 200, 200));
        static HBRUSH hbrStatic = CreateSolidBrush(RGB(30, 30, 30));
        return (LRESULT)hbrStatic;
    }
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(240, 240, 240));
        SetBkColor(hdc, RGB(60, 60, 65));
        static HBRUSH hbrBtn = CreateSolidBrush(RGB(60, 60, 65));
        return (LRESULT)hbrBtn;
    }
    case WM_DESTROY: {
        GuiLogger::write(Lang::t("gui_close_log"));
        GuiLogger::close();
        if (GuiLogger::enabled) {
            string msg = Lang::t("gui_log_saved") + GuiLogger::logPath;
            wstring wMsg = utf8ToWide(msg);
            wstring wTitle = utf8ToWide(Lang::t("gui_log_title"));
            MessageBoxW(hWnd, wMsg.c_str(), wTitle.c_str(), MB_OK | MB_ICONINFORMATION);
        }
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

// ---- GUI 主函数 ----
int guiMain() {
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    GuiLogger::init();
    GuiLogger::write("[启动] GUI 模式启动");

    Lang::init();
    // 让用户选择界面语言（提示同时含中英文，确保任何用户都能看懂）
    wstring wLangPrompt = utf8ToWide(Lang::t("gui_lang_prompt"));
    wstring wLangTitle = utf8ToWide(Lang::t("gui_lang_title"));
    int langChoice = MessageBoxW(NULL, wLangPrompt.c_str(), wLangTitle.c_str(),
        MB_YESNO | MB_ICONQUESTION);
    Lang::cur = (langChoice == IDNO) ? Lang::EN : Lang::ZH;

    loadAliases();
    string savedProxy = readConfig("proxy");
    if (!savedProxy.empty()) setProxyEnv(savedProxy);
    init_cmds();
    initCmdDesc();
    rebuildCategories();   // 根据当前语言重建分类/按钮元数据

    // 注册主窗口类
    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(30, 30, 30));
    wc.lpszClassName = "BetterCMDMainWindow";
    wc.hIcon = LoadIconA(GetModuleHandle(NULL), MAKEINTRESOURCEA(1));
    wc.hIconSm = (HICON)LoadImageA(GetModuleHandle(NULL), MAKEINTRESOURCEA(1),
        IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    if (!RegisterClassExA(&wc)) {
        wstring wRegFail = utf8ToWide(Lang::t("gui_register_fail"));
        wstring wErr = utf8ToWide(Lang::t("gui_error"));
        MessageBoxW(NULL, wRegFail.c_str(), wErr.c_str(), MB_ICONERROR);
        return 1;
    }

    wstring wWinTitle = utf8ToWide(Lang::t("gui_title") + VERSION);
    g_hMain = CreateWindowExW(0, L"BetterCMDMainWindow",
        wWinTitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1200, 720,
        NULL, NULL, GetModuleHandle(NULL), NULL);
    if (!g_hMain) {
        wstring wCreateFail = utf8ToWide(Lang::t("gui_create_fail"));
        wstring wErr2 = utf8ToWide(Lang::t("gui_error"));
        MessageBoxW(NULL, wCreateFail.c_str(), wErr2.c_str(), MB_ICONERROR);
        return 1;
    }

    // 暗色标题栏（Windows 10 1809+）
    BOOL darkTitle = TRUE;
    DwmSetWindowAttribute(g_hMain, 20, &darkTitle, sizeof(darkTitle));

    ShowWindow(g_hMain, SW_SHOW);
    UpdateWindow(g_hMain);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        // 参数对话框作为模态，不转发消息给主窗口
        if (g_hParamDlg && IsDialogMessageA(g_hParamDlg, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

#endif // _WIN32

/*
================================================================
 第十九部分：启动器
================================================================
*/
#ifdef _WIN32

/**
 * @brief 生成密钥文件（如果不存在）
 *        密钥格式：fstudio-fgamecc-YYYY-MM-DD-本机IP
 *        位置：exe 所在目录的 miyao.txt
 */
void generateKey() {
    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    fs::path exeDir = fs::path(exePath).parent_path();
    fs::path keyFile = exeDir / "miyao.txt";

    // 获取本机 IP
    string ip = "0.0.0.0";
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) == 0) {
        char hostname[256] = {0};
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            struct addrinfo hints = {0}, *res = NULL;
            hints.ai_family = AF_INET;
            if (getaddrinfo(hostname, NULL, &hints, &res) == 0 && res) {
                struct sockaddr_in* sa = (struct sockaddr_in*)res->ai_addr;
                char ipBuf[INET_ADDRSTRLEN] = {0};
                inet_ntop(AF_INET, &sa->sin_addr, ipBuf, sizeof(ipBuf));
                ip = ipBuf;
                freeaddrinfo(res);
            }
        }
        WSACleanup();
    }

    // 获取日期
    time_t now = time(nullptr);
    struct tm tmv;
    localtime_s(&tmv, &now);
    char dateBuf[32];
    strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d", &tmv);

    string key = "fstudio-fgamecc-" + string(dateBuf) + "-" + ip;
    ofstream fout(keyFile);
    if (fout) { fout << key; fout.close(); }
}

/**
 * @brief 验证密钥文件是否存在且格式正确（只检测，不生成）
 *        密钥格式：fstudio-fgamecc-YYYY-MM-DD-本机IP
 *        位置：miyao.txt（exe 目录 / 上级目录 / 当前工作目录）
 * @return true 验证通过；false 未找到或格式错误
 */
bool verifyKey() {
    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    fs::path exeDir = fs::path(exePath).parent_path();

    // 候选密钥位置：exe目录、exe上一级、上两级、当前工作目录
    vector<fs::path> candidates = {
        exeDir / "miyao.txt",
        exeDir.parent_path() / "miyao.txt",
        exeDir.parent_path().parent_path() / "miyao.txt",
        fs::current_path() / "miyao.txt"
    };

    string content;
    bool found = false;
    for (auto& p : candidates) {
        ifstream f(p);
        if (f) { getline(f, content); found = true; break; }
    }
    if (!found) return false;

    // 校验格式：fstudio-fgamecc- 开头
    if (content.rfind("fstudio-fgamecc-", 0) != 0) return false;

    // 校验后续至少包含日期段和 IP 段（至少两段）
    string body = content.substr(string("fstudio-fgamecc-").size());
    int sep = 0;
    for (char c : body) if (c == '-') sep++;
    return sep >= 1;
}

/**
 * @brief 检测当前进程是否以管理员权限运行
 * @return true 已提权；false 未提权
 */
bool IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin == TRUE;
}

/**
 * @brief 若未以管理员运行，则以 runas 重新启动本程序并退出
 * @param lpCmdLine 原始命令行参数（透传给新进程）
 */
void RequestAdminElevation(const char* lpCmdLine) {
    if (IsRunningAsAdmin()) return;

    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    string exeDir = fs::path(exePath).parent_path().string();

    HINSTANCE hInst = ShellExecuteA(NULL, "runas", exePath,
        lpCmdLine ? lpCmdLine : "", exeDir.c_str(), SW_SHOWNORMAL);

    if ((intptr_t)hInst <= 32) {
        wstring wStartupErr = utf8ToWide(Lang::t("gui_startup_err"));
        wstring wNoAdmin = utf8ToWide(Lang::t("gui_no_admin_title"));
        MessageBoxW(NULL, wStartupErr.c_str(), wNoAdmin.c_str(), MB_OK | MB_ICONERROR);
    }
    exit(0);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int) {
    // 启动前先初始化语言（默认中文，提示均含中英对照，方便任意用户阅读）
    Lang::init();

    // 启动前自动获取管理员权限
    RequestAdminElevation(lpCmdLine);

    // 启动前验证密钥（只检测，不存在则拒绝启动）
    if (!verifyKey()) {
        wstring wKeyFail = utf8ToWide(Lang::t("gui_key_fail"));
        wstring wKeyTitle = utf8ToWide(Lang::t("gui_key_title"));
        MessageBoxW(NULL, wKeyFail.c_str(), wKeyTitle.c_str(), MB_OK | MB_ICONERROR);
        return 1;
    }

    string args = lpCmdLine ? lpCmdLine : "";

    if (args.find("--cmd") != string::npos) {
        // 控制台模式
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stderr);
        SetConsoleTitleW(utf8ToWide(Lang::t("gui_cmd_mode")).c_str());
        return consoleMain();
    }
    if (args.find("--gui") != string::npos) {
        return guiMain();
    }

    // 首次启动：让用户选择模式
    wstring wModeMsg = utf8ToWide(Lang::t("gui_mode_msg"));
    wstring wModeTitle = utf8ToWide(Lang::t("gui_mode_title") + VERSION + Lang::t("gui_mode_q"));
    int choice = MessageBoxW(NULL, wModeMsg.c_str(), wModeTitle.c_str(),
        MB_YESNOCANCEL | MB_ICONQUESTION);

    if (choice == IDYES) return guiMain();
    if (choice == IDNO) {
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stderr);
        SetConsoleTitleW(utf8ToWide(Lang::t("gui_cmd_mode")).c_str());
        return consoleMain();
    }
    return 0;
}

#else

int main() {
    return consoleMain();
}

#endif
