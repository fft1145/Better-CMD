#include <bits/stdc++.h>
#include <windows.h>
#include <time.h>
#include <shellapi.h>
#include <iphlpapi.h>
//#pragma comment(lib, "iphlpapi.lib")

using namespace std;

clock_t start = clock();
string g_miyaoPath; // 保存密钥txt完整路径，用于程序退出删除文件

/**
 * @brief 控制台事件回调，捕获窗口关闭、Ctrl+C信号，删除密钥文件
 * @param event 控制台事件类型
 * @return 返回FALSE继续系统处理
 */
BOOL CALLBACK ConsoleHandler(DWORD event)
{
    if(event == CTRL_CLOSE_EVENT || event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT)
    {
        DeleteFileA(g_miyaoPath.c_str());
    }
    return FALSE;
}

/**
 * @brief 创建文件夹，文件夹已存在不会报错
 * @param dir 目录路径
 * @return true 创建成功 / 已存在；false 创建失败
 */
bool makeDir(const string& dir)
{
    return CreateDirectoryA(dir.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

/**
 * @brief 获取本机局域网IPv4地址，过滤回环地址127.0.0.1
 * @return 成功返回IP字符串；失败返回0.0.0.0
 */
string getLocalIP()
{
    ULONG size = 0;
    GetAdaptersInfo(NULL, &size);
    vector<BYTE> buffer(size);
    PIP_ADAPTER_INFO pAdapter = (PIP_ADAPTER_INFO)buffer.data();
    GetAdaptersInfo(pAdapter, &size);

    while(pAdapter)
    {
        IP_ADDR_STRING* pIp = &pAdapter->IpAddressList;
        while(pIp)
        {
            string ip = pIp->IpAddress.String;
            if(ip != "127.0.0.1" && !ip.empty())
            {
                return ip;
            }
            pIp = pIp->Next;
        }
        pAdapter = pAdapter->Next;
    }
    return "0.0.0.0";
}

/**
 * @brief 获取系统本地日期，格式 YYYY‑MM‑DD
 * @return 日期字符串
 */
string getToday()
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[32];
    sprintf_s(buf, "%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay); // 这里的减号不是键盘'-'！！
    return string(buf);
}

/*
 * @brief 生成密钥，在项目根目录 fgame.cc.cog\project.cog\miyao.txt 写入密钥
 * 密钥格式：fstudio‑fgamecc‑日期‑本机IP
 */
void createKeyFile()
{
    string dir1 = ".\\fgame.cc.cog";
    string dir2 = ".\\fgame.cc.cog\\project.cog";
    makeDir(dir1);
    makeDir(dir2);
    g_miyaoPath = dir2 + "\\miyao.txt";

    string ip = getLocalIP();
    string date = getToday();
    string key = "fstudio-fgamecc-" + date + "-" + ip;

    cout << "\n====生成密钥====\n";
    cout << key << "\n";
    cout << "保存位置：" << g_miyaoPath << "\n\n";

    ofstream fout(g_miyaoPath);
    fout << key;
    fout.close();
}

/**
 * @brief 把主体的两个文件(bcmd.exe / bcmd.cpp)从 BetterCMD\project 移动到前置程序所在目录(根目录)
 *        若目标已存在则删除源文件，避免 MoveFile 失败
 */
void moveMainToRoot()
{
    struct Pair { string src; string dst; };
    Pair files[] = {
        {".\\BetterCMD\\project\\bcmd.exe", ".\\bcmd.exe"},
        {".\\BetterCMD\\project\\bcmd.cpp", ".\\bcmd.cpp"}
    };
    for (auto& f : files)
    {
        if (GetFileAttributesA(f.src.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
        if (GetFileAttributesA(f.dst.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            DeleteFileA(f.src.c_str());   // 目标已存在，删掉源
        }
        else
        {
            MoveFileA(f.src.c_str(), f.dst.c_str());
        }
    }
}

/**
 * @brief 生成一个隐藏批处理，延迟 3 秒后把前置程序自身(cpp+exe)移动到 BetterCMD\project
 *        运行中的 exe 无法被移动，所以必须等前置程序退出后由批处理完成
 */
void createMoveSelfBat()
{
    string batPath = ".\\_move_pre.bat";
    ofstream fout(batPath);
    fout << "@echo off\r\n";
    fout << "timeout /t 3 /nobreak >nul\r\n";
    fout << "if not exist \"BetterCMD\\project\" mkdir \"BetterCMD\\project\"\r\n";
    fout << "move /y \"bcmd_prerequisite_files.cpp\" \"BetterCMD\\project\\\" >nul 2>nul\r\n";
    fout << "move /y \"bcmd_prerequisite_files.exe\" \"BetterCMD\\project\\\" >nul 2>nul\r\n";
    fout << "del \"%~f0\"\r\n";
    fout.close();

    ShellExecuteA(NULL, "open", batPath.c_str(), NULL, NULL, SW_HIDE);
}


/**
 * @brief 模拟加载动画流程，彩色进度条+模拟报错切换下载源动画
 */
void loading()
{
	cout << "Loading.\n";
	start = clock();
	while(clock() - start < 1 * CLOCKS_PER_SEC);
	cout << "Loading..\n";
	start = clock();
	while(clock() - start < 1 * CLOCKS_PER_SEC);
	cout << "Loading...\n";
	start = clock();
	while(clock() - start < 2 * CLOCKS_PER_SEC);

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	cout << "■";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	cout << " ■ ■ ■ ■ ■ ■ ■ ■ ■     10%\n\n";

	start = clock();
	while(clock() - start < 0.5 * CLOCKS_PER_SEC);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	cout << "■ ■";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	cout << " ■ ■ ■ ■ ■ ■ ■ ■     20%\n\n";

	start = clock();
	while(clock() - start < 0.5 * CLOCKS_PER_SEC);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	cout << "■ ■ ■";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	cout << " ■ ■ ■ ■ ■ ■ ■     30%\n\n";

	start = clock();
	while(clock() - start < 0.5 * CLOCKS_PER_SEC);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	cout << "■ ■ ■ ■";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	cout << " ■ ■ ■ ■ ■ ■     40%\n\n";

	start = clock();
	while(clock() - start < 0.5 * CLOCKS_PER_SEC);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	cout << "■ ■ ■ ■ ■";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	cout << " ■ ■ ■ ■ ■     50%\n\n";

	start = clock();
	while(clock() - start < 0.5 * CLOCKS_PER_SEC);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	cout << "■ ■ ■ ■ ■ ■";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	cout << " ■ ■ ■ ■     60%\n\n";

	start = clock();
	while(clock() - start < 0.5 * CLOCKS_PER_SEC);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	cout << "■ ■ ■ ■ ■ ■ ■";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	cout << " ■ ■ ■     70%\n\n";

	start = clock();
	while(clock() - start < 0.5 * CLOCKS_PER_SEC);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	cout << "■ ■ ■ ■ ■ ■ ■ ■";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	cout << " ■ ■     80%\n\n";

	start = clock();
	while(clock() - start < 0.5 * CLOCKS_PER_SEC);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	cout << "■ ■ ■ ■ ■ ■ ■ ■ ■";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	cout << " ■     90%\n\n";

	start = clock();
	while(clock() - start < 1.5 * CLOCKS_PER_SEC);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 4);
	cout << "■ ■ ■ ■ ■ ■ ■ ■";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	cout << " ■ ■     80%\n\n";

	start = clock();
	while(clock() - start < 2.0 * CLOCKS_PER_SEC);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 4);
	cout << "■ ■ ■ ■ ■ ■ ■";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	cout << " ■ ■ ■     70%\n\n";

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
	cout << "[ERROR] 连接下载服务器失败！正在尝试重连！文件下载地址：https://fgame12.netlify.app/index.html/home/download.html/github_com_fftgzs_bcmd_x64\n(Failed to connect to the download server! Trying to reconnect! File download URL:https://fgame12.netlify.app/index.html/home/download.html/github_com_fftgzs_bcmd_x64)\n\n";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	cout << "[Better CMD]正在为你切换下载源\nSwitching your download source for you!";

	start = clock();
	while(clock() - start < 20 * CLOCKS_PER_SEC);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	cout << "\n[Better CMD]下载源切换成功！\nSource download link successful!\n\n";

	for (int i = 1;i <= 30;i++)
	{
		cout << " \n-\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
		start = clock();
		while(clock() - start < 0.1 * CLOCKS_PER_SEC);
		cout << "- \n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
		start = clock();
		while(clock() - start < 0.1 * CLOCKS_PER_SEC);
		cout << "|\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
		start = clock();
		while(clock() - start < 0.1 * CLOCKS_PER_SEC);
		cout << "/ \n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
		start = clock();
		while(clock() - start < 0.1 * CLOCKS_PER_SEC);
		cout << "-\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
		start = clock();
		while(clock() - start < 0.1 * CLOCKS_PER_SEC);
		cout << "- \n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
		start = clock();
		while(clock() - start < 0.1 * CLOCKS_PER_SEC);
		cout << "| \n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
		start = clock();
		while(clock() - start < 0.1 * CLOCKS_PER_SEC);
		cout << "/ \n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
		start = clock();
		while(clock() - start < 0.1 * CLOCKS_PER_SEC);
	}

	start = clock();
	while(clock() - start < 1 * CLOCKS_PER_SEC);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
	cout << "■ ■ ■ ■ ■ ■ ■ ■";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	cout << " ■ ■     80%\n\n";

	start = clock();
	while(clock() - start < 1.0 * CLOCKS_PER_SEC);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	cout << "■ ■ ■ ■ ■ ■ ■ ■ ■";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	cout << " ■     90%\n\n";

	start = clock();
	while(clock() - start < 1.5 * CLOCKS_PER_SEC);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	cout << "■ ■ ■ ■ ■ ■ ■ ■ ■ ■";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	cout << "     100%\n\n";

	start = clock();
	while(clock() - start < 2.0 * CLOCKS_PER_SEC);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 4);
	cout << "■ ■ ■ ■ ■ ■ ■ ■ ■";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	cout << " ■     90%\n\n";

	start = clock();
	while(clock() - start < 6.5 * CLOCKS_PER_SEC);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	cout << "■ ■ ■ ■ ■ ■ ■ ■ ■ ■";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	cout << "     100%\n";

	start = clock();
	while(clock() - start < 0.5 * CLOCKS_PER_SEC);
	cout << "Finish!\n";
}

/**
 * @brief 程序延时函数
 * @param number 等待秒数
 * @return 固定返回0
 */
int countdown(double number)
{
	start = clock();
	while(clock() - start < number * CLOCKS_PER_SEC);
	return 0;
}

/**
 * @brief 设置控制台字体颜色
 * @param colour Windows控制台颜色码
 * @return 固定返回0
 */
int color(int colour)
{
	/*
	0	黑色
	1	蓝色
	2	绿色
	3	浅蓝
	4	红色
	5	紫色
	6	黄色 (暗黄)
	7	白色（默认）
	8	灰色
	9	亮蓝
	10	亮绿
	11	浅青
	12	亮红
	13	亮紫
	14	亮黄
	15	亮白
	*/
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colour);
	return 0;
}

/**
 * @brief 程序入口，注册控制台回调，生成密钥，输出协议，处理用户选择，启动主程序
 * @return int 退出码
 */
int main()
{

	cout << "欢迎来到Better CMD\n";
	countdown(2);
	cout << "使用本软件前，请阅读并同意《用户协议》与《隐私声明》\n";
	cout << "同意全部条款请输入 1，拒绝请输入 2\n\n";

	cout << "==== 用户协议 & 隐私声明 ====\n";
	cout << "Better-CMD(下称\"本软件\")由 fft工作室开发，项目已在 GitHub 开源。\n";
	cout << "1. 当前为前置启动程序：加载、下载相关画面仅为模拟动画，不会执行抓包、发包等网络操作；\n";
	cout << "   运行主程序后将启用真实网络功能，包含Ping数据包发送、网络抓包、数据包修改等能力。\n";
	cout << "2. 使用者承诺：所有抓包、发包、网络测试行为，仅可在本人拥有完全授权的设备与网络环境内执行。\n";
	cout << "   禁止未经授权对第三方设备、服务器实施探测、抓包、篡改数据包等操作。\n";
	cout << "3. 用户必须严格遵守《中华人民共和国网络安全法》《民法典》《中华人民共和国刑法》及治安管理相关法律法规。\n";
	cout << "4. 本项目遵循对应开源许可协议，严禁将本软件用于入侵、网络攻击、骚扰、破坏等任何违法用途。\n";
	cout << "5. 本启动器不会主动收集、上传你的个人信息、键盘输入、本地文件；主程序网络行为由使用者自主触发。\n";
	cout << "6. 程序提供跳转外部网页功能，跳转后的第三方网页内容不受本项目控制，请自行甄别外部网站风险。\n";
	cout << "7. 本软件按现状提供，开发者不对用户违规使用软件所造成的任何后果承担责任。\n";
	cout << "项目临时官网：https://fgame12.netlify.app，后续迁移备案至：https://www.fgame12.top\n";
	cout << "\n";

	cout << "Better-CMD (hereinafter referred to as \"Software\") is developed by fft Studio and open-sourced on GitHub.\n";
	cout << "1. This is the pre-launcher. Loading and download screens are simulated animations. Real network functions\n";
	cout << "   including Ping, network packet capture and packet modification will be available after launching main program.\n";
	cout << "2. You agree that all packet capture and network tests SHALL ONLY be performed on devices and networks you fully own or authorize.\n";
	cout << "   Unauthorized probing, packet capture or tampering against third-party devices or servers is strictly forbidden.\n";
	cout << "3. End-users shall comply with relevant laws of the People's Republic of China.\n";
	cout << "4. This project follows its open-source license. Any malicious or illegal usage is prohibited.\n";
	cout << "5. This pre-launcher does NOT collect or upload your personal data. Network operations in main program are triggered by your own operation.\n";
	cout << "6. The software can open external webpages. Developer takes no responsibility for third-party website contents.\n";
	cout << "7. This software is provided AS-IS. Developer is not liable for any consequences caused by user's improper or illegal usage.\n";
	cout << "Temporary official site: https://fgame12.netlify.app, future site: https://www.fgame12.top\n";
	cout << "\n";

	cout << "If you agree to above terms, please enter 1, otherwise enter 2!\n\n";
	int agreement;
	cin >> agreement;

	if (agreement == 1)
	{
		cout << "正在下载必要运行文件...(Downloading the necessary runtime files...)\n";
		countdown(3);
		loading();
		cout << "==========\n" << "欢迎来到Better CMD\n";
		cout << "1.关注创作者(Follow the creator)\n";
		cout << "2.运行BCMD(Running the BCMD)\n";
		int choose;
		cin >> choose;
		if (choose == 1)
		{
			ShellExecuteA(NULL, "open", "https://space.bilibili.com/3546830010845608?spm_id_from=333.1007.0.0", NULL, NULL, SW_SHOWNORMAL);
		}
		if (choose == 2)
		{
			// 1. 生成密钥（保存到 fgame.cc.cog\project.cog\miyao.txt，不删除，供主体验证）
			createKeyFile();
			// 2. 把主体文件(bcmd.exe / bcmd.cpp)从 BetterCMD\project 移到根目录
			moveMainToRoot();
			// 3. 生成隐藏批处理，等本程序退出后把自身移到 BetterCMD\project
			createMoveSelfBat();
			// 4. 启动主体 bcmd.exe（此时已在根目录）
			ShellExecuteA(NULL, NULL, "bcmd.exe", NULL, NULL, SW_SHOWNORMAL);
		}
		// 密钥文件保留供主体验证，不再 countdown + DeleteFile
	}
	else
	{
		cout << "OK";
		countdown(5);
	}
	    // 注册控制台关闭事件钩子
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
	return 0;
}