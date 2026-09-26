/*================================================================
 Better CMD Ultimate v5.1 - 控制台 + 傻瓜式 GUI 按钮界面
================================================================
 开发团队 : fft工作室 - Fungame Craft总项目组
 编译命令 :
   g++ -std=c++17 -mwindows -static -O2 -m64 bcmd.cpp -o bcmd.exe
       -lws2_32 -liphlpapi -lpsapi -lwinhttp -lshlwapi -lwinmm -lcomctl32

   如果要看控制台输出（调试用），去掉 -mwindows 参数即可。

 日志位置 : fgamecc/main/logs/bcmd_gui.log （相对 exe 所在目录）
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
#include <conio.h>
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

#define VERSION "5.1.0"
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
        dict["welcome"]  = {"欢迎使用 Better CMD v" VERSION, "Welcome to Better CMD v" VERSION};
        dict["unknown"]  = {"未知命令", "Unknown command"};
        dict["bp_start"] = {"[BP] 使用 pktmon 抓包... (需要管理员)", "[BP] Capturing with pktmon..."};
        dict["bp_stop"]  = {"[BP] 停止抓包并转换", "[BP] Stop and convert"};
        dict["bp_list"]  = {"[BP] 已捕获文件:", "[BP] Captured files:"};
        dict["bp_no_admin"] = {"[BP] 需要管理员权限", "[BP] Admin required"};
        dict["author"]   = {
            "本款软件（Better CMD）由 fft工作室 - Fungame Craft总项目组 制作",
            "Developed by fft Studio - Fungame Craft Main Project Group"
        };
        dict["need_py"]     = {"需要 Python", "Python required"};
        dict["need_ffmpeg"] = {"需要 ffmpeg", "ffmpeg required"};
        dict["need_sqlite"] = {"需要 sqlite3.exe", "sqlite3.exe required"};
        dict["need_trans"]  = {"需要 translate-shell", "translate-shell required"};
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
string exec(const char* cmd) {
    array<char, 128> buf; string res;
    unique_ptr<FILE, decltype(&PCLOSE)> pipe(POPEN(cmd, "r"), PCLOSE);
    if (!pipe) return "";
    while (fgets(buf.data(), buf.size(), pipe.get())) res += buf.data();
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
        if (capturing) { cout << "[BP] 已在抓包中" << endl; return; }
        if (!isAdmin()) { cout << Lang::t("bp_no_admin") << endl; return; }
        fs::create_directory(BP_FOLDER);
        for (auto& e : fs::directory_iterator(BP_FOLDER)) { try { fs::remove(e.path()); } catch (...) {} }
        cout << Lang::t("bp_start") << endl;
        int ret = system("pktmon start --capture --comp nics --pkt-size 0 --file-name " BP_FOLDER "\\capture.etl");
        if (ret != 0) { cout << "[BP] pktmon 启动失败" << endl; return; }
        capturing = true;
    }
    void stop() {
        if (!capturing) { cout << "[BP] 未在抓包" << endl; return; }
        cout << Lang::t("bp_stop") << endl;
        system("pktmon stop");
        system("pktmon format " BP_FOLDER "\\capture.etl -o " BP_FOLDER "\\capture.txt");
        capturing = false;
    }
    void list() {
        cout << Lang::t("bp_list") << endl;
        if (!fs::exists(BP_FOLDER)) { cout << "  (无)" << endl; return; }
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
    if (!fs::exists(p, ec)) { cout << "路径不存在: " << p << endl; return; }
    try { for (auto& e : fs::directory_iterator(p)) cout << (e.is_directory() ? "<DIR> " : "      ") << e.path().filename().string() << endl; }
    catch (const exception& e) { cout << "无法列出目录: " << e.what() << endl; }
}
void cmd_cd(const vector<string>& a)    {
    if (a.empty()) { cout << fs::current_path().string() << endl; return; }
    error_code ec;
    if (!fs::exists(a[0], ec)) { cout << "路径不存在: " << a[0] << endl; return; }
    fs::current_path(a[0], ec);
    if (ec) cout << "切换失败: " << ec.message() << endl;
}
void cmd_md(const vector<string>& a)    {
    error_code ec;
    for (auto& d : a) { fs::create_directories(d, ec); if (ec) cout << "创建失败: " << d << endl; }
}
void cmd_rd(const vector<string>& a)    {
    error_code ec;
    for (auto& d : a) { fs::remove_all(d, ec); if (ec) cout << "删除失败: " << d << endl; }
}
void cmd_del(const vector<string>& a)   {
    error_code ec;
    for (auto& f : a) { fs::remove(f, ec); if (ec) cout << "删除失败: " << f << endl; }
}
void cmd_copy(const vector<string>& a)  {
    if (a.size() < 2) { cout << "用法: copy 源 目标" << endl; return; }
    error_code ec;
    fs::copy(a[0], a[1], fs::copy_options::overwrite_existing, ec);
    if (ec) cout << "复制失败: " << ec.message() << endl;
}
void cmd_move(const vector<string>& a)  {
    if (a.size() < 2) { cout << "用法: move 源 目标" << endl; return; }
    error_code ec;
    fs::rename(a[0], a[1], ec);
    if (ec) cout << "移动失败: " << ec.message() << endl;
}
void cmd_ren(const vector<string>& a)   { if (a.size() >= 2) { error_code ec; fs::rename(a[0], a[1], ec); if (ec) cout << "重命名失败: " << ec.message() << endl; } }
void cmd_type(const vector<string>& a)  { if (!a.empty()) { ifstream f(a[0]); if (f) cout << f.rdbuf(); else cout << "无法打开: " << a[0] << endl; } }
void cmd_echo(const vector<string>& a)  { cout << join(a) << endl; }
void cmd_cls()  { system("cls"); }
void cmd_date() { auto t = time(nullptr); cout << put_time(localtime(&t), "%Y-%m-%d") << endl; }
void cmd_time() { auto n = chrono::system_clock::now(); time_t t = chrono::system_clock::to_time_t(n); cout << put_time(localtime(&t), "%H:%M:%S") << endl; }
void cmd_ver()  { cout << "Better CMD v" VERSION << endl; }
#ifdef _WIN32
void cmd_title(const vector<string>& a) { if (!a.empty()) SetConsoleTitleA(join(a).c_str()); }
#else
void cmd_title(const vector<string>&) {}
#endif
void cmd_color(const vector<string>& a) {
    // Windows color 命令需要一个 2 位十六进制参数（如 0A），允许 1 或 2 个 token
#ifdef _WIN32
    if (a.empty()) { system("color"); return; }
    string arg = a.size() >= 2 ? (a[0] + a[1]) : a[0];
    if (arg.size() < 2 || isxdigit((unsigned char)arg[0]) == 0 || isxdigit((unsigned char)arg[1]) == 0) {
        cout << "用法: color <bg><fg> 例如 color 0A" << endl; return;
    }
    system(("color " + arg).c_str());
#else
    cout << "[color] 仅 Windows 支持" << endl;
#endif
}
void cmd_find(const vector<string>& a)  { if (a.size() >= 2) { string p = a[0]; ifstream f(a[1]); if (!f) { cout << "无法打开: " << a[1] << endl; return; } string l; while (getline(f, l)) if (l.find(p) != string::npos) cout << l << endl; } }
void cmd_more(const vector<string>& a)  { if (!a.empty()) { ifstream f(a[0]); if (!f) { cout << "无法打开: " << a[0] << endl; return; } string l; int n = 0; while (getline(f, l)) { cout << l << endl; if (++n % 20 == 0) { cout << "--More--"; cin.get(); } } } }
void cmd_sort(const vector<string>& a)  { if (!a.empty()) { ifstream f(a[0]); if (!f) { cout << "无法打开: " << a[0] << endl; return; } vector<string> v; string l; while (getline(f, l)) v.push_back(l); sort(v.begin(), v.end()); for (auto& x : v) cout << x << endl; } }
void cmd_fc(const vector<string>& a)    { if (a.size() >= 2) cout << exec(("fc " + a[0] + " " + a[1]).c_str()); }
void cmd_tree(const vector<string>& a)  {
    string p = a.empty() ? "." : a[0];
    error_code ec;
    if (!fs::exists(p, ec)) { cout << "路径不存在: " << p << endl; return; }
    try {
        // depth() 是迭代器的方法，不是 directory_entry 的，必须显式用迭代器
        for (auto it = fs::recursive_directory_iterator(p);
             it != fs::recursive_directory_iterator(); ++it) {
            cout << string(it.depth() * 2, ' ') << it->path().filename().string() << endl;
        }
    }
    catch (const exception& e) { cout << "无法遍历: " << e.what() << endl; }
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
void cmd_format()     { cout << "[!] 格式化已被阻止" << endl; }
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
            try { n = stoi(f["-n"]); } catch (...) { cout << "-n 参数无效: " << f["-n"] << endl; return; }
        }
        cmd += "-n " + to_string(n) + " ";
    }
    if (f.count("-l")) {
        // 校验 -l 数值
        try { (void)stoi(f["-l"]); } catch (...) { cout << "-l 参数无效: " << f["-l"] << endl; return; }
        cmd += "-l " + f["-l"] + " ";
    }
    if (f.count("-w")) {
        try { (void)stoi(f["-w"]); } catch (...) { cout << "-w 参数无效: " << f["-w"] << endl; return; }
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
    if (a.size() < 3) { cout << "用法: calc x op y (op: + - * /)" << endl; return; }
    double x, y;
    try { x = stod(a[0]); y = stod(a[2]); }
    catch (...) { cout << "数值无效" << endl; return; }
    char op = a[1][0];
    switch (op) {
        case '+': cout << x + y << endl; break;
        case '-': cout << x - y << endl; break;
        case '*': cout << x * y << endl; break;
        case '/':
            if (y == 0) { cout << "错误: 除数不能为 0" << endl; break; }
            cout << x / y << endl; break;
        default: cout << "不支持的运算符: " << op << endl; break;
    }
}
void f_rand(const vector<string>& a) {
    int lo = 0, hi = 100;
    try {
        if (a.size() > 0) lo = stoi(a[0]);
        if (a.size() > 1) hi = stoi(a[1]);
    } catch (...) { cout << "数值无效" << endl; return; }
    if (lo > hi) swap(lo, hi);
    random_device rd; mt19937 g(rd()); uniform_int_distribution<> d(lo, hi); cout << d(g) << endl;
}
void f_base64(const vector<string>& a, map<string, string>& f) {
    bool enc = f.count("-e") > 0, dec = f.count("-d") > 0;
    if (!enc && !dec) { cout << "用法: base64 -e/-d \"内容\"" << endl; return; }
    string text = enc ? (f["-e"].empty() ? join(a) : f["-e"]) : (f["-d"].empty() ? (a.empty() ? "" : a[0]) : f["-d"]);
    if (text.empty()) { cout << "内容为空" << endl; return; }
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
    if (a.empty()) { cout << "用法: qrcode 内容" << endl; return; }
    string t = join(a, " "), enc;
    for (char c : t) { char buf[8] = {0}; sprintf(buf, "%%%02X", (unsigned char)c); enc += buf; }
    system(("start https://api.qrserver.com/v1/create-qr-code/?size=200x200&data=" + enc).c_str());
}
void f_json(const vector<string>& a) {
    if (a.empty()) { cout << "用法: json 文件名" << endl; return; }
    ifstream chk(a[0]); if (!chk) { cout << "无法打开: " << a[0] << endl; return; }
    cout << exec(("powershell -NoProfile -Command \"Get-Content -Raw -LiteralPath '" + a[0] + "' | ConvertFrom-Json | ConvertTo-Json -Depth 100\"").c_str());
}
void f_regex(const vector<string>& a) { if (a.size() >= 2) { try { regex re(a[0]); cout << (regex_match(a[1], re) ? "匹配" : "不匹配") << endl; } catch (const regex_error&) { cout << "无效正则" << endl; } } }
void f_wget(const vector<string>& a) {
    if (a.empty()) { cout << "用法: wget URL" << endl; return; }
    cout << exec(("curl -L -o download.tmp " + a[0]).c_str());
}
void f_httpserver() { if (cmd_exists("python")) system("start python -m http.server 8080"); else cout << Lang::t("need_py") << endl; }
void f_portscan(const vector<string>& a) {
    if (a.empty()) { cout << exec("netstat -an | findstr LISTENING"); return; }
    if (cmd_exists("powershell")) cout << exec(("powershell -NoProfile -Command \"Test-NetConnection -ComputerName " + a[0] + " -Port " + (a.size() > 1 ? a[1] : "80") + "\"").c_str());
    else cout << "需要 powershell" << endl;
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
    cout << "[clip] 仅 Windows 支持" << endl;
#endif
}
void f_sysmon() { cout << exec("powershell -NoProfile -Command \"Get-Counter '\\Processor(_Total)\\% Processor Time'\""); }
void f_diskusage() { cout << exec("wmic logicaldisk get size,freespace,caption"); }
void f_encrypt(const vector<string>& a) {
    if (a.size() < 2) { cout << "用法: encrypt file pwd" << endl; return; }
    ifstream in(a[0], ios::binary);
    if (!in) { cout << "无法打开: " << a[0] << endl; return; }
    ofstream out(a[0] + ".enc", ios::binary);
    if (!out) { cout << "无法写: " << a[0] << ".enc" << endl; return; }
    const string& p = a[1]; size_t i = 0; char c;
    while (in.get(c)) { c ^= p[i % p.size()]; out.put(c); i++; }
    cout << "已加密 -> " << a[0] << ".enc" << endl;
}
void f_decrypt(const vector<string>& a) {
    if (a.size() < 2) { cout << "用法: decrypt file.enc pwd" << endl; return; }
    ifstream in(a[0], ios::binary);
    if (!in) { cout << "无法打开: " << a[0] << endl; return; }
    // 安全获取输出文件名：输入必须以 .enc 结尾，否则追加 .dec 避免覆盖原文件
    string base = a[0];
    string outName;
    if (base.size() >= 4 && base.compare(base.size() - 4, 4, ".enc") == 0) {
        outName = base.substr(0, base.size() - 4);
    } else {
        outName = base + ".dec";
        cout << "[提示] 输入文件不以 .enc 结尾，输出为 " << outName << endl;
    }
    if (outName == a[0]) { outName = a[0] + ".dec"; }  // 双保险
    ofstream out(outName, ios::binary);
    if (!out) { cout << "无法写: " << outName << endl; return; }
    const string& p = a[1]; size_t i = 0; char c;
    while (in.get(c)) { c ^= p[i % p.size()]; out.put(c); i++; }
    cout << "已解密 -> " << outName << endl;
}
void f_zip(const vector<string>& a) { if (a.size() >= 2) cout << exec(("powershell -NoProfile -Command \"Compress-Archive -LiteralPath '" + a[0] + "' -DestinationPath '" + a[1] + "' -Force\"").c_str()); }
void f_unzip(const vector<string>& a) { if (!a.empty()) cout << exec(("powershell -NoProfile -Command \"Expand-Archive -LiteralPath '" + a[0] + "' -DestinationPath . -Force\"").c_str()); }
void f_schedule(const vector<string>& a) { if (a.size() < 2) return; string t = a[0], cmd = join(vector<string>(a.begin() + 1, a.end()), " "); cout << exec(("schtasks /Create /SC ONCE /ST " + t + " /TR \"" + cmd + "\" /TN BCMD_Task").c_str()); }
void f_logview(const vector<string>& a) { if (!a.empty()) { ifstream f(a[0]); if (!f) { cout << "无法打开: " << a[0] << endl; return; } string l; while (getline(f, l)) if (l.find("ERROR") != string::npos || l.find("Error") != string::npos) cout << l << endl; } }
void f_sync(const vector<string>& a) { if (a.size() >= 2) cout << exec(("robocopy " + a[0] + " " + a[1] + " /MIR").c_str()); }
void f_passgen(const vector<string>& a) {
    int len = 16;
    try { if (!a.empty()) len = stoi(a[0]); } catch (...) { cout << "长度无效" << endl; return; }
    if (len <= 0 || len > 4096) { cout << "长度需在 1-4096 之间" << endl; return; }
    const string c = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    random_device rd; mt19937 g(rd()); uniform_int_distribution<size_t> d(0, c.size() - 1);
    string out; out.reserve(len);
    for (int i = 0; i < len; ++i) out += c[d(g)];
    cout << out << endl;
}
void f_uuid() { cout << exec("powershell -NoProfile -Command \"[guid]::NewGuid().ToString()\""); }
void f_colorpick() {
    cout << "3秒后取样..." << endl; this_thread::sleep_for(3s);
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
        cout << "用法: sendmail -to addr -subj \"主题\" -body \"正文\" -smtp server" << endl;
        return;
    }
    cout << exec(("powershell -NoProfile -Command \"Send-MailMessage -To '" + to + "' -Subject '" + sub + "' -Body '" + body + "' -SmtpServer '" + smtp + "'\"").c_str());
}
void f_chat(const vector<string>& a) {
    if (a.size() < 2) { cout << "用法: chat host port (使用 telnet 或 PuTTY)" << endl; return; }
#ifdef _WIN32
    // Windows 10/11 默认无 telnet，优先尝试 telnet，失败提示启用或用 PuTTY
    if (cmd_exists("telnet")) { system(("start cmd /k telnet " + a[0] + " " + a[1]).c_str()); return; }
    if (cmd_exists("putty")) { system(("start putty telnet://" + a[0] + ":" + a[1]).c_str()); return; }
    cout << "未找到 telnet（控制面板 -> 启用/关闭 Windows 功能 -> Telnet 客户端）或 PuTTY" << endl;
#else
    cout << exec(("telnet " + a[0] + " " + a[1]).c_str());
#endif
}
void f_scicalc(const vector<string>& a) { if (!a.empty()) cout << exec(("powershell -NoProfile -Command \"" + join(a, "") + "\"").c_str()); }
void f_convert(const vector<string>& a) {
    if (a.size() < 3) { cout << "用法: convert 值 原单位 目标单位" << endl; return; }
    double v;
    try { v = stod(a[0]); } catch (...) { cout << "数值无效" << endl; return; }
    string f = a[1], t = a[2];
    if (f == "c" && t == "f") cout << (v * 9 / 5 + 32) << " F" << endl;
    else if (f == "f" && t == "c") cout << ((v - 32) * 5 / 9) << " C" << endl;
    else {
        map<pair<string, string>, double> cv = { {{"cm","inch"},0.393701},{{"inch","cm"},2.54},{{"m","ft"},3.28084},{{"ft","m"},0.3048},{{"kg","lb"},2.20462},{{"lb","kg"},0.453592} };
        auto it = cv.find({ f,t });
        if (it != cv.end()) cout << v * it->second << " " << t << endl;
        else cout << "不支持: " << f << " -> " << t << endl;
    }
}
void f_vcard(const vector<string>& a) { if (a.size() >= 2) { ofstream o(a[0] + ".vcf"); o << "BEGIN:VCARD\nVERSION:3.0\nFN:" << a[0] << "\nTEL:" << a[1] << "\nEND:VCARD\n"; cout << "已生成" << a[0] << ".vcf" << endl; } }
void f_barcode(const vector<string>& a) {
    if (a.empty()) { cout << "用法: barcode 内容" << endl; return; }
    string t = join(a, ""), enc;
    for (char c : t) { char buf[8] = {0}; sprintf(buf, "%%%02X", (unsigned char)c); enc += buf; }
    system(("start https://barcode.tec-it.com/barcode.ashx?data=" + enc + "&code=Code128").c_str());
}
void f_checksum(const vector<string>& a) { if (!a.empty()) cout << exec(("certutil -hashfile \"" + a[0] + "\" SHA1").c_str()); }
void f_split(const vector<string>& a) {
    if (a.size() < 3) { cout << "用法: split 文件 每份MB 输出前缀" << endl; return; }
    ifstream in(a[0], ios::binary);
    if (!in) { cout << "无法打开: " << a[0] << endl; return; }
    size_t ck;
    try { ck = (size_t)stoull(a[1]) * 1024 * 1024; } catch (...) { cout << "MB 参数无效" << endl; return; }
    if (ck == 0) { cout << "MB 必须 > 0" << endl; return; }
    const string& p = a[2];
    vector<char> b(ck); int i = 0;
    while (in.read(b.data(), (streamsize)ck) || in.gcount() > 0) {
        ofstream o(p + "." + to_string(i++), ios::binary);
        o.write(b.data(), in.gcount());
    }
    cout << "已分成 " << i << " 份" << endl;
}
void f_merge(const vector<string>& a) {
    if (a.size() < 2) { cout << "用法: merge 输出文件 分片1 分片2 ..." << endl; return; }
    ofstream o(a[0], ios::binary);
    if (!o) { cout << "无法写: " << a[0] << endl; return; }
    for (size_t i = 1; i < a.size(); i++) {
        ifstream in(a[i], ios::binary);
        if (!in) { cout << "跳过缺失分片: " << a[i] << endl; continue; }
        o << in.rdbuf();
    }
    cout << "已合并 -> " << a[0] << endl;
}
void f_wc(const vector<string>& a) { if (!a.empty()) { ifstream f(a[0]); if (!f) { cout << "无法打开: " << a[0] << endl; return; } string l; int ln = 0, w = 0, c = 0; while (getline(f, l)) { ln++; c += l.size(); istringstream iss(l); string x; while (iss >> x) w++; } cout << "行:" << ln << " 词:" << w << " 字符:" << c << endl; } }
void f_linesort(const vector<string>& a) { cmd_sort(a); }
void f_uniq(const vector<string>& a) { if (!a.empty()) { ifstream f(a[0]); if (!f) { cout << "无法打开: " << a[0] << endl; return; } string l, p; while (getline(f, l)) { if (l != p) cout << l << endl; p = l; } } }
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
    ifstream f(a[0], ios::binary); if (!f) { cout << "打开失败: " << a[0] << endl; return; }
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
    if (f.count("-n")) { try { n = stoi(f["-n"]); } catch (...) { cout << "-n 参数无效" << endl; return; } }
    if (n <= 0) { cout << "-n 必须 > 0" << endl; return; }
    if (!a.empty()) fn = a[0];
    if (fn.empty()) { cout << "用法: tail [-n N] 文件" << endl; return; }
    ifstream in(fn); if (!in) { cout << "无法打开: " << fn << endl; return; }
    vector<string> ls; string l;
    while (getline(in, l)) ls.push_back(l);
    size_t s = ls.size() > (size_t)n ? ls.size() - n : 0;
    for (size_t i = s; i < ls.size(); i++) cout << ls[i] << '\n';
}
void f_head(const vector<string>& a, map<string, string>& f) {
    int n = 10; string fn;
    if (f.count("-n")) { try { n = stoi(f["-n"]); } catch (...) { cout << "-n 参数无效" << endl; return; } }
    if (n <= 0) { cout << "-n 必须 > 0" << endl; return; }
    if (!a.empty()) fn = a[0];
    if (fn.empty()) { cout << "用法: head [-n N] 文件" << endl; return; }
    ifstream in(fn); if (!in) { cout << "无法打开: " << fn << endl; return; }
    string l; int c = 0;
    while (c < n && getline(in, l)) { cout << l << '\n'; c++; }
}
void f_bench(const vector<string>& a) { if (a.empty()) return; string c = join(a, " "); auto s = chrono::high_resolution_clock::now(); int r = system(c.c_str()); (void)r; auto e = chrono::high_resolution_clock::now(); cout << "\n耗时: " << chrono::duration_cast<chrono::milliseconds>(e - s).count() << " ms" << endl; }

/*
================================================================
 第十部分：IDE / AI 集成
================================================================
*/
string getIDE() { string i = envOr("IDE", ""); if (i.empty()) i = envOr("EDITOR", ""); if (i.empty()) i = readConfig("IDE"); return i; }
void cmd_ide(const vector<string>& a) {
    if (a.empty()) { string i = getIDE(); if (i.empty()) cout << "未设置 IDE" << endl; else cout << "当前 IDE: " << i << endl; }
    else if (a[0] == "set") { if (a.size() < 2) return; string p = join(vector<string>(a.begin() + 1, a.end()), " "); writeConfig("IDE", p); cout << "IDE 已设置为 " << p << endl; }
}
#ifdef _WIN32
void cmd_edit(const vector<string>& a) { if (a.empty()) return; string i = getIDE(); if (i.empty()) { cout << "未设置 IDE" << endl; return; } for (auto& f : a) ShellExecuteA(NULL, "open", i.c_str(), f.c_str(), NULL, SW_SHOWNORMAL); }
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
    if (k.empty()) { cout << "未设置 API Key (使用 .bcmdrc 配置 api_key，或设置环境变量 AI_API_KEY)" << endl; return; }
    if (a.empty()) { cout << "用法: ai 问题" << endl; return; }
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
    if (content.empty()) { cout << "解析失败，原始响应：" << endl << resp << endl; return; }
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
    else if (a[0] == "add") { string x = join(vector<string>(a.begin() + 1, a.end()), " "); t.push_back(x); saveTodo(t); cout << "已添加" << endl; }
    else if (a[0] == "done") { if (a.size() < 2) return; try { int i = stoi(a[1]) - 1; if (i >= 0 && i < (int)t.size()) { t.erase(t.begin() + i); saveTodo(t); cout << "已完成" << endl; } else cout << "序号超出范围" << endl; } catch (...) { cout << "序号无效" << endl; } }
    else cout << "用法: todo [list|add|done]" << endl;
}
void cmd_notes(const vector<string>& a) {
    ensureNotesDir();
    if (a.empty() || a[0] == "list") { for (auto& e : fs::directory_iterator(notesDir)) cout << e.path().stem().string() << endl; }
    else if (a[0] == "view") { if (a.size() < 2) return; ifstream f(notesDir + a[1] + ".md"); if (f) cout << f.rdbuf(); else cout << "笔记不存在: " << a[1] << endl; }
    else if (a[0] == "rm") { if (a.size() < 2) return; error_code ec; fs::remove(notesDir + a[1] + ".md", ec); if (ec) cout << "删除失败" << endl; else cout << "已删除" << endl; }
    else { string t = a[0], c = join(vector<string>(a.begin() + 1, a.end()), " "); ofstream f(notesDir + t + ".md"); f << c; cout << "已保存" << endl; }
}
void cmd_timer(const vector<string>& a) {
    if (a.empty()) { cout << "用法: timer 秒数 [提示信息]" << endl; return; }
    int s;
    try { s = stoi(a[0]); } catch (...) { cout << "秒数无效" << endl; return; }
    if (s <= 0 || s > 86400) { cout << "秒数需在 1-86400 之间" << endl; return; }
    string m = a.size() > 1 ? join(vector<string>(a.begin() + 1, a.end()), " ") : "";
    cout << "计时开始 " << s << " 秒..." << endl;
    this_thread::sleep_for(chrono::seconds(s));
    cout << "\a时间到！ " << m << endl;
}
void cmd_alias(const vector<string>& a) {
    if (a.empty()) { for (auto& p : aliases) cout << p.first << " = " << p.second << endl; }
    else if (a[0] == "rm") { if (a.size() < 2) return; aliases.erase(a[1]); saveAliases(); cout << "已删除" << endl; }
    else { if (a.size() < 2) return; aliases[a[0]] = join(vector<string>(a.begin() + 1, a.end()), " "); saveAliases(); cout << "已保存" << endl; }
}
void cmd_proxy(const vector<string>& a) {
    if (a.empty()) { cout << "当前代理: " << (proxyAddr.empty() ? "无" : proxyAddr) << endl; }
    else if (a[0] == "set") { if (a.size() < 2) { cout << "用法: proxy set 地址" << endl; return; } setProxyEnv(a[1]); writeConfig("proxy", a[1]); cout << "已设置并保存" << endl; }
    else if (a[0] == "unset") { unsetProxyEnv(); writeConfig("proxy", ""); cout << "已取消" << endl; }
    else cout << "用法: proxy [set|unset]" << endl;
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
    if (!valid(sp)) { cout << "路径含非法字符，回退至 C:\\BetterCMD_Screenshots\n"; fp = "C:\\BetterCMD_Screenshots"; fb = true; }
    else { error_code ec; fs::create_directories(sp, ec); if (ec) { cout << "无法创建目录，回退\n"; fp = "C:\\BetterCMD_Screenshots"; fb = true; } }
    sp = fp;
    // 判断动作（来自 flags）
    bool actS = f.count("-s") > 0;
    bool actR = f.count("-r") > 0;
    bool actK = f.count("-k") > 0;
    bool actC = f.count("-c") > 0;
    if (!actS && !actR && !actK && !actC) {
        cout << "用法: picture -s/-r/-k/-c [-path \"路径\"]\n当前路径: " << sp << endl;
        return;
    }
    if (actS) {
        string fn = sp + "\\screenshot_" + to_string(time(0)) + ".png";
        string cmd = "powershell -NoProfile -Command \"Add-Type -AssemblyName System.Windows.Forms,System.Drawing; $img=[System.Windows.Forms.Screen]::PrimaryScreen.Bounds; $bmp=New-Object System.Drawing.Bitmap $img.Width,$img.Height; $g=[System.Drawing.Graphics]::FromImage($bmp); $g.CopyFromScreen($img.X,$img.Y,0,0,$img.Size); $bmp.Save('\" + fn + \"',[System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose(); $bmp.Dispose()\"";
        // 注意：fn 路径中若含单引号需要转义，此处简单处理
        string ps = "powershell -NoProfile -Command \"Add-Type -AssemblyName System.Windows.Forms,System.Drawing; $img=[System.Windows.Forms.Screen]::PrimaryScreen.Bounds; $bmp=New-Object System.Drawing.Bitmap $img.Width,$img.Height; $g=[System.Drawing.Graphics]::FromImage($bmp); $g.CopyFromScreen($img.X,$img.Y,0,0,$img.Size); $bmp.Save('";
        ps += fn;
        ps += "',[System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose(); $bmp.Dispose()\"";
        system(ps.c_str()); cout << "截图已保存: " << fn << endl;
        if (pf && !fb) writeConfig("screenshot_path", sp);
    } else if (actR) {
        int d = 60;
        if (f.count("-r") && !f["-r"].empty()) {
            try { d = stoi(f["-r"]); } catch (...) { cout << "录制秒数无效，使用默认 60" << endl; }
        }
        if (d <= 0 || d > 86400) { cout << "录制秒数需在 1-86400 之间" << endl; return; }
        string fn = sp + "\\recording_" + to_string(time(0)) + ".mp4";
        if (cmd_exists("ffmpeg")) { system(("start ffmpeg -f gdigrab -framerate 30 -t " + to_string(d) + " -i desktop \"" + fn + "\"").c_str()); cout << "录屏开始..." << endl; }
        else cout << "需要 ffmpeg 或 Win+Alt+R" << endl;
    } else if (actK) {
        string key = f.count("-k") ? f["-k"] : "";
        if (key.empty()) { cout << "用法: picture -k 快捷键组合 (如 Ctrl+Shift+A)" << endl; return; }
        writeConfig("screenshot_hotkey", key); cout << "快捷键已设置: " << key << endl;
    } else if (actC) {
        writeConfig("screenshot_hotkey", ""); cout << "已清除" << endl;
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
    if (f == string::npos) { cout << "用法: /title <文本> [-color\"颜色\" with \"起 to 止\"] | -nt | -nt -break" << endl; return; }
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
            if (ansi.empty()) { cout << "不支持颜色:" << cn << endl; return; }
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
    cmdDesc["dir"] = "列出目录"; cmdDesc["cd"] = "切换目录"; cmdDesc["md"] = "创建目录"; cmdDesc["rd"] = "删除目录";
    cmdDesc["del"] = "删除文件"; cmdDesc["copy"] = "复制文件"; cmdDesc["move"] = "移动文件"; cmdDesc["ren"] = "重命名";
    cmdDesc["type"] = "显示文件"; cmdDesc["echo"] = "输出"; cmdDesc["cls"] = "清屏"; cmdDesc["date"] = "日期";
    cmdDesc["time"] = "时间"; cmdDesc["ver"] = "版本"; cmdDesc["title"] = "窗口标题"; cmdDesc["color"] = "控制台颜色";
    cmdDesc["find"] = "搜索"; cmdDesc["more"] = "分页"; cmdDesc["sort"] = "排序"; cmdDesc["fc"] = "比较文件";
    cmdDesc["tree"] = "目录树"; cmdDesc["netstat"] = "网络状态"; cmdDesc["ping"] = "Ping"; cmdDesc["tracert"] = "路由跟踪";
    cmdDesc["nslookup"] = "DNS"; cmdDesc["ipconfig"] = "IP配置"; cmdDesc["route"] = "路由表"; cmdDesc["arp"] = "ARP";
    cmdDesc["getmac"] = "MAC"; cmdDesc["systeminfo"] = "系统信息"; cmdDesc["tasklist"] = "进程列表";
    cmdDesc["taskkill"] = "结束进程"; cmdDesc["shutdown"] = "关机"; cmdDesc["format"] = "格式化(阻止)"; cmdDesc["chkdsk"] = "磁盘检查"; cmdDesc["vol"] = "卷标";
    cmdDesc["calc"] = "计算器"; cmdDesc["rand"] = "随机数"; cmdDesc["base64"] = "Base64"; cmdDesc["md5"] = "MD5";
    cmdDesc["sha256"] = "SHA256"; cmdDesc["qrcode"] = "二维码"; cmdDesc["json"] = "JSON格式化"; cmdDesc["regex"] = "正则";
    cmdDesc["wget"] = "下载"; cmdDesc["httpserver"] = "HTTP服务"; cmdDesc["portscan"] = "端口扫描"; cmdDesc["procmon"] = "进程监控";
    cmdDesc["clip"] = "剪贴板"; cmdDesc["sysmon"] = "系统监控"; cmdDesc["diskusage"] = "磁盘"; cmdDesc["encrypt"] = "加密";
    cmdDesc["decrypt"] = "解密"; cmdDesc["zip"] = "压缩"; cmdDesc["unzip"] = "解压"; cmdDesc["schedule"] = "计划任务";
    cmdDesc["logview"] = "日志查看"; cmdDesc["sync"] = "同步"; cmdDesc["passgen"] = "密码生成"; cmdDesc["uuid"] = "UUID";
    cmdDesc["colorpick"] = "取色"; cmdDesc["translate"] = "翻译"; cmdDesc["weather"] = "天气";
    cmdDesc["record"] = "录音"; cmdDesc["screenshot"] = "截图"; cmdDesc["rdp"] = "远程桌面"; cmdDesc["ftp"] = "FTP";
    cmdDesc["sql"] = "SQLite"; cmdDesc["sendmail"] = "邮件"; cmdDesc["chat"] = "聊天"; cmdDesc["scicalc"] = "科学计算";
    cmdDesc["convert"] = "单位转换"; cmdDesc["vcard"] = "名片"; cmdDesc["barcode"] = "条形码"; cmdDesc["checksum"] = "校验";
    cmdDesc["split"] = "分割"; cmdDesc["merge"] = "合并"; cmdDesc["wc"] = "文本统计"; cmdDesc["linesort"] = "行排序";
    cmdDesc["uniq"] = "去重"; cmdDesc["dirdiff"] = "目录比较"; cmdDesc["service"] = "服务"; cmdDesc["startup"] = "启动项";
    cmdDesc["hwinfo"] = "硬件信息"; cmdDesc["osk"] = "屏幕键盘";
    cmdDesc["whois"] = "Whois"; cmdDesc["dig"] = "DNS详细"; cmdDesc["ssh"] = "SSH"; cmdDesc["hexdump"] = "十六进制";
    cmdDesc["tail"] = "尾部"; cmdDesc["head"] = "头部"; cmdDesc["bench"] = "性能测试";
    cmdDesc["ide"] = "IDE设置"; cmdDesc["edit"] = "用IDE打开"; cmdDesc["ai"] = "AI聊天";
    cmdDesc["todo"] = "任务"; cmdDesc["notes"] = "笔记"; cmdDesc["timer"] = "倒计时"; cmdDesc["alias"] = "别名"; cmdDesc["proxy"] = "代理";
    cmdDesc["picture"] = "截图录屏"; cmdDesc["BP"] = "抓包"; cmdDesc["author"] = "关于"; cmdDesc["/title"] = "提示符";
    cmdDesc["help"] = "帮助"; cmdDesc["exit"] = "退出";
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

void help() {
    cout << "======== Better CMD v" VERSION " ========\n";
    cout << "【CMD原生命令】dir, cd, md, rd, del, copy, move, ren, type, echo, cls, date, time, ver, title, color, find, more, sort, fc, tree, netstat, ping, tracert, nslookup, ipconfig, route, arp, getmac, systeminfo, tasklist, taskkill, shutdown, format, chkdsk, vol\n";
    cout << "【50工具】calc, rand, base64, md5, sha256, qrcode, json, regex, wget, httpserver, portscan, procmon, clip, sysmon, diskusage, encrypt, decrypt, zip, unzip, schedule, logview, sync, passgen, uuid, colorpick, translate, weather, record, screenshot, rdp, ftp, sql, sendmail, chat, scicalc, convert, vcard, barcode, checksum, split, merge, wc, linesort, uniq, dirdiff, service, startup, hwinfo, osk\n";
    cout << "【增强】whois, dig, ssh, hexdump, tail, head, bench\n";
    cout << "【IDE/AI】ide, edit, ai\n";
    cout << "【新功能】todo, notes, timer, alias, proxy\n";
    cout << "【截图录屏】picture -s/-r/-k/-c [-path \"路径\"]\n";
    cout << "【BP抓包】BP, BP -stop, BP -list\n";
    cout << "Tab 补全，exit 退出。\n";
}

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
            for (auto& [al, _] : aliases) if (al.find(buf) == 0) matches.push_back({ al, "[别名]" });
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

int consoleMain() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);
#endif
    Lang::init();
    cout << "1.中文 2.English: ";
    int ch; cin >> ch; cin.ignore();
    Lang::cur = (ch == 2) ? Lang::EN : Lang::ZH;
    cout << Lang::t("welcome") << endl;
    promptText = "BCMD"; currentPrompt = promptText + "> ";
    loadAliases();
    string savedProxy = readConfig("proxy");
    if (!savedProxy.empty()) setProxyEnv(savedProxy);
    init_cmds(); initCmdDesc();
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
            catch (const exception& e) { cout << "异常: " << e.what() << endl; }
            catch (...) { cout << "未知异常" << endl; }
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
    static void init() {
        // 使用 exe 所在目录作为基准，确保项目搬迁不受影响
        char exePath[MAX_PATH] = { 0 };
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        fs::path exeDir = fs::path(exePath).parent_path();
        fs::path logDir = exeDir / "fgamecc" / "main" / "logs";
        fs::create_directories(logDir);
        logPath = (logDir / "bcmd_gui.log").string();
        logFile.open(logPath, ios::app);
        if (logFile.is_open()) {
            auto now = chrono::system_clock::now();
            time_t tt = chrono::system_clock::to_time_t(now);
            stringstream ss; ss << put_time(localtime(&tt), "%Y-%m-%d %H:%M:%S");
            logFile << "\n=== Better CMD GUI 新会话 ===" << endl;
            logFile << "启动时间: " << ss.str() << endl;
            logFile << "用户: " << envOr("USERNAME", "unknown") << endl;
            logFile << "=========================" << endl;
        }
    }
    static void write(const string& msg) {
        if (logFile.is_open()) {
            auto now = chrono::system_clock::now();
            time_t tt = chrono::system_clock::to_time_t(now);
            stringstream ss; ss << put_time(localtime(&tt), "[%H:%M:%S] ");
            logFile << ss.str() << msg << endl;
            logFile.flush();
        }
    }
    static void close() {
        if (logFile.is_open()) { logFile << "=== 会话结束 ===" << endl; logFile.close(); }
    }
};
string GuiLogger::logPath;
ofstream GuiLogger::logFile;

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

// 所有分类
vector<CategoryDef> g_categories = {
    { "文件操作", "[F]", {
        {"dir",   "列出目录",      {{"path","目录路径","留空为当前目录","",true}}},
        {"cd",    "切换目录",      {{"path","目标路径","例如 D:\\Projects",""}}},
        {"md",    "创建目录",      {{"name","目录名","例如 newfolder",""}}},
        {"rd",    "删除目录",      {{"name","目录名","",""}}},
        {"del",   "删除文件",      {{"file","文件名","",""}}},
        {"copy",  "复制文件",      {{"src","源文件","",""},{"dst","目标位置","",""}}},
        {"move",  "移动文件",      {{"src","源文件","",""},{"dst","目标位置","",""}}},
        {"ren",   "重命名",        {{"old","原名称","",""},{"new","新名称","",""}}},
        {"type",  "查看文件",      {{"file","文件名","",""}}},
        {"tree",  "目录树",        {{"path","根目录","留空为当前","",true}}},
        {"find",  "搜索文本",      {{"kw","关键词","",""},{"file","文件名","",""}}},
        {"more",  "分页查看",      {{"file","文件名","",""}}},
        {"sort",  "行排序",        {{"file","文件名","",""}}},
        {"fc",    "比较文件",      {{"f1","文件1","",""},{"f2","文件2","",""}}}
    }},
    { "网络工具", "[N]", {
        {"ping",     "测试网络",   {{"host","目标地址","","8.8.8.8"},{"n","次数","","4",true}}},
        {"tracert",  "路由追踪",   {{"host","目标地址","","8.8.8.8"}}},
        {"nslookup", "DNS 查询",   {{"domain","域名","","baidu.com"}}},
        {"ipconfig", "IP 配置",    {}},
        {"netstat",  "网络连接",   {}},
        {"route",    "路由表",     {}},
        {"arp",      "ARP 表",     {}},
        {"getmac",   "MAC 地址",   {}},
        {"portscan", "端口扫描",   {{"host","目标主机","",""},{"port","端口","","80"}}},
        {"wget",     "下载文件",   {{"url","下载地址","",""}}},
        {"httpserver","HTTP 服务", {}},
        {"whois",    "Whois 查询", {{"domain","域名","",""}}},
        {"dig",      "DNS 详细",   {{"domain","域名","",""}}},
        {"ssh",      "SSH 连接",   {{"host","目标","user@host",""}}}
    }},
    { "系统工具", "[S]", {
        {"systeminfo","系统信息",  {}},
        {"tasklist",  "进程列表",  {}},
        {"taskkill",  "结束进程",  {{"pid","进程 PID","",""}}},
        {"shutdown",  "关机",      {}},
        {"chkdsk",    "磁盘检查",  {}},
        {"vol",       "卷标",      {}},
        {"diskusage", "磁盘使用",  {}},
        {"sysmon",    "系统监控",  {}},
        {"procmon",   "进程监控",  {}},
        {"service",   "服务管理",  {}},
        {"startup",   "启动项",    {}},
        {"hwinfo",    "硬件信息",  {}},
        {"colorpick", "屏幕取色",  {}}
    }},
    { "文本处理", "[T]", {
        {"echo",     "输出文本",   {{"text","内容","",""}}},
        {"wc",       "文本统计",   {{"file","文件名","",""}}},
        {"linesort", "行排序",     {{"file","文件名","",""}}},
        {"uniq",     "去重",       {{"file","文件名","",""}}},
        {"logview",  "日志查看",   {{"file","日志文件","",""}}},
        {"json",     "JSON 格式化",{{"file","JSON 文件","",""}}},
        {"regex",    "正则测试",   {{"pattern","正则","",""},{"text","测试文本","",""}}},
        {"hexdump",  "十六进制",   {{"file","文件名","",""}}},
        {"tail",     "查看尾部",   {{"file","文件名","",""},{"n","行数","","10",true}}},
        {"head",     "查看头部",   {{"file","文件名","",""},{"n","行数","","10",true}}}
    }},
    { "加密压缩", "[C]", {
        {"md5",      "MD5 哈希",   {{"file","文件名","",""}}},
        {"sha256",   "SHA256 哈希",{{"file","文件名","",""}}},
        {"checksum", "校验和",     {{"file","文件名","",""}}},
        {"encrypt",  "文件加密",   {{"file","文件名","",""},{"pwd","密码","",""}}},
        {"decrypt",  "文件解密",   {{"file","加密文件","",""},{"pwd","密码","",""}}},
        {"zip",      "压缩",       {{"src","源","",""},{"dst","目标 .zip","",""}}},
        {"unzip",    "解压",       {{"file","zip 文件","",""}}},
        {"split",    "分割文件",   {{"file","文件名","",""},{"mb","每份大小(MB)","",""},{"prefix","输出前缀","",""}}},
        {"merge",    "合并文件",   {{"out","输出文件","",""},{"parts","分片（空格分隔）","",""}}},
        {"base64",   "Base64",     {{"mode","模式","","-e",false,true,{{"-e","编码"},{"-d","解码"}}},{"text","内容","",""}}}
    }},
    { "实用工具", "[U]", {
        {"calc",    "计算器",    {{"a","数字1","",""},{"op","运算符","","+",false,true,{{"+","+"},{"-","-"},{"*","×"},{"/","÷"}}},{"b","数字2","",""}}},
        {"rand",    "随机数",    {{"lo","最小值","","0"},{"hi","最大值","","100"}}},
        {"passgen", "生成密码",  {{"len","长度","","16"}}},
        {"uuid",    "UUID",      {}},
        {"qrcode",  "二维码",    {{"text","要生成的内容","",""}}},
        {"barcode", "条形码",    {{"text","要生成的内容","",""}}},
        {"vcard",   "名片",      {{"name","姓名","",""},{"phone","电话","",""}}},
        {"convert", "单位转换",  {{"value","数值","",""},{"from","原单位","","cm",false,true,{{"cm","cm"},{"inch","inch"},{"m","m"},{"ft","ft"},{"kg","kg"},{"lb","lb"},{"c","°C"},{"f","°F"}}},{"to","目标单位","","inch",false,true,{{"cm","cm"},{"inch","inch"},{"m","m"},{"ft","ft"},{"kg","kg"},{"lb","lb"},{"c","°C"},{"f","°F"}}}}},
        {"scicalc", "科学计算",  {{"expr","数学表达式","sqrt(2)+3^2",""}}},
        {"timer",   "倒计时",    {{"sec","秒数","",""},{"msg","提示信息","","",true}}},
        {"todo",    "任务管理",  {{"action","操作","","list",false,true,{{"list","查看"},{"add","添加"},{"done","完成"}}},{"text","任务内容","","",true}}},
        {"notes",   "笔记管理",  {{"title","标题","",""},{"content","内容","","",true}}},
        {"alias",   "别名设置",  {{"name","别名","",""},{"cmd","命令","",""}}},
        {"proxy",   "代理设置",  {{"action","操作","","set",false,true,{{"set","设置"},{"unset","取消"}}},{"addr","地址","","",true}}},
        {"weather", "天气",      {}},
        {"translate","翻译",     {{"text","要翻译的内容","",""}}}
    }},
    { "截图录屏", "[P]", {
        {"picture -s","立即截图", {{"path","保存路径","留空为默认","",true}}},
        {"picture -r","开始录屏", {{"sec","录制秒数","","60",true}}},
        {"picture -k","设置快捷键",{{"key","快捷键组合","Ctrl+Shift+A",""}}},
        {"picture -c","清除快捷键",{}},
        {"screenshot","系统截图", {}},
        {"record",    "录音",     {}}
    }},
    { "AI 与 IDE", "[A]", {
        {"ai",   "向 AI 提问",   {{"prompt","问题","",""}}},
        {"ide",  "设置 IDE",     {{"path","IDE 命令","code",""}}},
        {"edit", "用 IDE 打开",  {{"file","文件路径","",""}}},
        {"chat", "简单聊天",     {{"host","目标","",""},{"port","端口","","23"}}}
    }},
    { "抓包工具", "[B]", {
        {"BP",       "开始抓包",   {}},
        {"BP -stop", "停止",       {}},
        {"BP -list", "查看文件",   {}}
    }},
    { "其他", "[?]", {
        {"help",  "帮助",   {}},
        {"ver",   "版本",   {}},
        {"date",  "日期",   {}},
        {"time",  "时间",   {}},
        {"bench", "性能测试",{{"cmd","要测试的命令","",""}}},
        {"author","关于作者",{}}
    }}
};

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

// ---- 向日志面板追加文本 ----
void GuiAppendLog(const string& text) {
    if (!g_hLogPanel) return;
    int len = GetWindowTextLengthA(g_hLogPanel);
    SendMessageA(g_hLogPanel, EM_SETSEL, len, len);
    string out;
    for (char c : text) {
        if (c == '\n' && (out.empty() || out.back() != '\r')) out += "\r\n";
        else out += c;
    }
    if (out.empty() || (out.back() != '\n')) out += "\r\n";
    SendMessageA(g_hLogPanel, EM_REPLACESEL, FALSE, (LPARAM)out.c_str());
    SendMessageA(g_hLogPanel, EM_SCROLLCARET, 0, 0);
}

// ---- 写日志（同时写文件） ----
void GuiLog(const string& text) {
    GuiAppendLog(text);
    GuiLogger::write(text);
}

// ---- 状态栏更新 ----
void GuiSetStatus(const string& text) {
    if (g_hStatusBar) SetWindowTextA(g_hStatusBar, text.c_str());
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
            catch (const exception& e) { cout << "异常: " << e.what() << endl; }
            catch (...) { cout << "未知异常" << endl; }
        } else {
            string out = exec(realInput.c_str());
            cout << out;
        }
    } catch (...) {
        cout << "GuiRunCommand 异常" << endl;
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
        char buf[2048] = { 0 };
        if (i < g_paramInputs.size() && g_paramInputs[i]) {
            GetWindowTextA(g_paramInputs[i], buf, sizeof(buf) - 1);
        }
        string val = buf;
        if (val.empty()) {
            if (!p.optional) missing = true;
            continue;
        }
        args.push_back(val);
    }

    CloseParamDialog();
    if (missing) {
        GuiAppendLog("错误：必填参数为空");
        GuiSetStatus("已取消");
        return;
    }

    // 拼接显示
    string fullCmd = cmdName;
    for (auto& a : args) { fullCmd += " "; fullCmd += a; }

    GuiLog(">>> " + fullCmd);
    string out = GuiRunCommand(fullCmd);
    if (!out.empty()) GuiAppendLog(out);
    GuiSetStatus("已执行: " + cmdName);
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
        HWND hTitle = CreateWindowExA(0, "STATIC",
            (g_pendingCmd->name + "  -  " + g_pendingCmd->desc).c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            margin, y, 460, 24, hWnd, NULL, NULL, NULL);
        SendMessageA(hTitle, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
        y += 34;

        // 各参数
        for (size_t i = 0; i < g_pendingCmd->params.size(); ++i) {
            const auto& p = g_pendingCmd->params[i];

            // 标签
            string labelText = p.label;
            if (p.optional) labelText += "（可选）";
            HWND hLabel = CreateWindowExA(0, "STATIC", labelText.c_str(),
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                margin, y, 460, labelH, hWnd, NULL, NULL, NULL);
            SendMessageA(hLabel, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
            g_paramLabels.push_back(hLabel);
            y += labelH;

            // 输入控件
            HWND hInput = NULL;
            if (p.isSelect) {
                hInput = CreateWindowExA(WS_EX_CLIENTEDGE, "COMBOBOX", "",
                    WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                    margin, y, 460, 200, hWnd, NULL, NULL, NULL);
                for (auto& opt : p.options) {
                    int idx = (int)SendMessageA(hInput, CB_ADDSTRING, 0, (LPARAM)opt.second.c_str());
                    SendMessageA(hInput, CB_SETITEMDATA, idx, (LPARAM)opt.first.c_str());
                }
                // 默认选中
                if (!p.defaultValue.empty()) {
                    for (int k = 0; k < (int)p.options.size(); ++k) {
                        if (p.options[k].first == p.defaultValue) {
                            SendMessageA(hInput, CB_SETCURSEL, k, 0);
                            break;
                        }
                    }
                } else if (!p.options.empty()) {
                    SendMessageA(hInput, CB_SETCURSEL, 0, 0);
                }
            } else {
                hInput = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", p.defaultValue.c_str(),
                    WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                    margin, y, 460, ctrlH, hWnd, NULL, NULL, NULL);
                // 设置 placeholder
                if (!p.placeholder.empty() && p.defaultValue.empty()) {
                    SendMessageA(hInput, EM_SETCUEBANNER, TRUE, (LPARAM)p.placeholder.c_str());
                }
            }
            SendMessageA(hInput, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);
            g_paramInputs.push_back(hInput);

            // 第一个输入控件自动聚焦
            if (i == 0) SetFocus(hInput);

            y += ctrlH + gap;
        }

        // 按钮
        y += 10;
        HWND hOk = CreateWindowExA(0, "BUTTON", "执行",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            280, y, 90, 34, hWnd, (HMENU)1, NULL, NULL);
        HWND hCancel = CreateWindowExA(0, "BUTTON", "取消",
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
        GuiLog(">>> " + cmd.name);
        string out = GuiRunCommand(cmd.name);
        if (!out.empty()) GuiAppendLog(out);
        GuiSetStatus("已执行: " + cmd.name);
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

    g_hParamDlg = CreateWindowExA(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        "BetterCMDParamDlg", "参数输入",
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
    int sidebarW = 180;
    int logW = 340;
    int contentX = sidebarW + 10;
    int contentW = rc.right - contentX - logW - 10;
    int contentY = 50;
    int contentH = rc.bottom - contentY - 40;

    // 标题
    HWND hTitle = CreateWindowExA(0, "STATIC",
        (cat.icon + "  " + cat.name + "  共 " + to_string(cat.commands.size()) + " 个功能").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        contentX, 15, contentW, 28, g_hMain, NULL, NULL, NULL);
    SendMessageA(hTitle, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
    g_cmdButtons.push_back(hTitle);

    // 命令按钮网格
    int btnW = 175;
    int btnH = 62;
    int gap = 10;
    int cols = max(1, (contentW + gap) / (btnW + gap));
    int x = contentX;
    int y = contentY;
    int col = 0;

    for (size_t i = 0; i < cat.commands.size(); ++i) {
        const auto& cmd = cat.commands[i];

        // 显示文本：命令名 + 换行 + 描述
        string btnText = cmd.name + "\n" + cmd.desc;

        int id = IDC_CMDBTN_BASE + (int)i;
        HWND hBtn = CreateWindowExA(0, "BUTTON", btnText.c_str(),
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
    GuiSetStatus("当前分类: " + g_categories[sel].name);
    GuiLogger::write("[操作] 切换到分类: " + g_categories[sel].name);
}

// ---- 主窗口过程 ----
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // 字体
        g_hFontUI = CreateFontA(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Microsoft YaHei UI");
        g_hFontMono = CreateFontA(13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            FIXED_PITCH | FF_MODERN, "Consolas");

        RECT rc;
        GetClientRect(hWnd, &rc);
        int sidebarW = 180;
        int logW = 340;
        int statusH = 26;

        // 左侧分类列表
        g_hSidebar = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
            0, 0, sidebarW, rc.bottom - statusH,
            hWnd, (HMENU)IDC_SIDEBAR, NULL, NULL);
        SendMessageA(g_hSidebar, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
        for (auto& c : g_categories) {
            SendMessageA(g_hSidebar, LB_ADDSTRING, 0, (LPARAM)(c.icon + "  " + c.name).c_str());
        }
        SendMessageA(g_hSidebar, LB_SETCURSEL, 0, 0);

        // 右侧日志面板
        g_hLogPanel = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            rc.right - logW, 0, logW, rc.bottom - statusH,
            hWnd, (HMENU)IDC_LOGPANEL, NULL, NULL);
        SendMessageA(g_hLogPanel, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);
        // 深色背景与文字颜色通过下方 WM_CTLCOLOREDIT 处理
        // （EM_SETBKGNDCOLOR 是 RichEdit 专用消息，普通 EDIT 控件不支持）

        // 状态栏
        g_hStatusBar = CreateWindowExA(0, "STATIC", "就绪",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            6, rc.bottom - statusH + 4, rc.right - 12, 20,
            hWnd, (HMENU)IDC_STATUSBAR, NULL, NULL);
        SendMessageA(g_hStatusBar, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

        // 初始日志
        GuiAppendLog("欢迎使用 Better CMD 可视化界面 v" VERSION);
        GuiAppendLog("操作方式：点击左侧分类 → 点击中间按钮 → 需要参数会弹出输入框");
        GuiAppendLog("所有操作均记录到日志文件。");
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
        int sidebarW = 180;
        int logW = 340;
        int statusH = 26;

        SetWindowPos(g_hSidebar, NULL, 0, 0, sidebarW, h - statusH, SWP_NOZORDER);
        SetWindowPos(g_hLogPanel, NULL, w - logW, 0, logW, h - statusH, SWP_NOZORDER);
        SetWindowPos(g_hStatusBar, NULL, 6, h - statusH + 4, w - 12, 20, SWP_NOZORDER);

        // 重新渲染命令按钮（因为中间区域变了）
        RenderCategory(g_currentCat);
        return 0;
    }
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        if ((HWND)lParam == g_hLogPanel) {
            SetTextColor(hdc, RGB(200, 200, 200));
            SetBkColor(hdc, RGB(12, 12, 12));
            static HBRUSH hbr = CreateSolidBrush(RGB(12, 12, 12));
            return (LRESULT)hbr;
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }
    case WM_DESTROY: {
        GuiLogger::write("[操作] 用户关闭 GUI 窗口");
        GuiLogger::close();
        string msg = "感谢使用 Better CMD！\n\n本次操作日志已保存到:\n" + GuiLogger::logPath + "\n\n点击确定退出。";
        MessageBoxA(hWnd, msg.c_str(), "日志已保存", MB_OK | MB_ICONINFORMATION);
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
    Lang::cur = Lang::ZH;
    loadAliases();
    string savedProxy = readConfig("proxy");
    if (!savedProxy.empty()) setProxyEnv(savedProxy);
    init_cmds();
    initCmdDesc();

    // 注册主窗口类
    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "BetterCMDMainWindow";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    if (!RegisterClassExA(&wc)) {
        MessageBoxA(NULL, "窗口类注册失败", "错误", MB_ICONERROR);
        return 1;
    }

    g_hMain = CreateWindowExA(0, "BetterCMDMainWindow",
        "Better CMD - 傻瓜式可视化界面 v" VERSION,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1200, 720,
        NULL, NULL, GetModuleHandle(NULL), NULL);
    if (!g_hMain) {
        MessageBoxA(NULL, "窗口创建失败", "错误", MB_ICONERROR);
        return 1;
    }

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
 *        位置：exe 所在目录的 fgame.cc.cog\project.cog\miyao.txt
 */
void generateKey() {
    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    fs::path exeDir = fs::path(exePath).parent_path();
    fs::path keyDir = exeDir / "fgame.cc.cog" / "project.cog";
    error_code ec;
    fs::create_directories(keyDir, ec);
    fs::path keyFile = keyDir / "miyao.txt";

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
 *        位置：fgame.cc.cog\project.cog\miyao.txt
 * @return true 验证通过；false 未找到或格式错误
 */
bool verifyKey() {
    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    fs::path exeDir = fs::path(exePath).parent_path();

    // 候选密钥位置：exe目录、exe上一级、上两级、当前工作目录
    vector<fs::path> candidates = {
        exeDir / "fgame.cc.cog" / "project.cog" / "miyao.txt",
        exeDir.parent_path() / "fgame.cc.cog" / "project.cog" / "miyao.txt",
        exeDir.parent_path().parent_path() / "fgame.cc.cog" / "project.cog" / "miyao.txt",
        fs::current_path() / "fgame.cc.cog" / "project.cog" / "miyao.txt"
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

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int) {
    // 启动前验证密钥（只检测，不存在则拒绝启动）
    if (!verifyKey()) {
        MessageBoxA(NULL,
            "密钥验证失败！\n\n密钥文件：fgame.cc.cog\\project.cog\\miyao.txt",
            "Better CMD - 密钥验证失败",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    string args = lpCmdLine ? lpCmdLine : "";

    if (args.find("--cmd") != string::npos) {
        // 控制台模式
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stderr);
        SetConsoleTitleA("Better CMD - 控制台模式");
        return consoleMain();
    }
    if (args.find("--gui") != string::npos) {
        return guiMain();
    }

    // 首次启动：让用户选择模式
    int choice = MessageBoxA(NULL,
        "请选择启动模式：\n\n"
        "【是】图形界面 GUI（按钮操作）\n"
        "【否】经典控制台 CMD（命令行输入）\n\n"
        "GUI 模式下所有命令都已做成按钮，无需记忆命令。\n"
        "控制台模式提供完整的命令行体验。",
        "Better CMD v" VERSION " - 启动模式",
        MB_YESNOCANCEL | MB_ICONQUESTION);

    if (choice == IDYES) return guiMain();
    if (choice == IDNO) {
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stderr);
        SetConsoleTitleA("Better CMD - 控制台模式");
        return consoleMain();
    }
    return 0;
}

#else

int main() {
    return consoleMain();
}

#endif
