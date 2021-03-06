#include "stdafx.h"
#include "MainWindow.h"
#include "Resource.h"
#include "WindowResolver.h"
#include "AboutWindow.h"
#include "UpdateWindow.h"
#include "Tracker.h"
#include "DriverLoader.h"
#include "htmlayout.h"
#include "NtHlp.h"
#include "Utils.h"
#include "KernelUtils.h"
#include "StringHlp.h"
#include "StringSplit.h"
#include <shlwapi.h>
#include <time.h>
#include <string>
#include "JiYuKiller.h"

using namespace std;

extern int screenWidth, screenHeight;
extern HINSTANCE hInst;
extern WCHAR currentDir[MAX_PATH];
extern WCHAR fullPath[MAX_PATH];
extern WCHAR iniPath[MAX_PATH];
extern WCHAR updateFilePath[MAX_PATH];
WCHAR fromRunner[MAX_PATH];

//sets

extern bool setAutoForceKill;
extern bool setAutoIncludeFullWindow;
int setCKinterval = 4000;
bool setSelfProtect = true;
bool qsStarted = false;
int shutdownTick = 30;
HWND hDlgShut = NULL;

WCHAR statusBuffer[2048] = { 0 };

//var

bool firstShow = true, fullWindow = false, controlled = false,
ck = true, top = false, isUserCancel = false, hideTipShowed = false, forceNoDriver = false;
HWND hWndMain = NULL, hWndTool = NULL;
bool isWin7 = false, isXp = false;

//Timer

#define TIMER_AOP 2
#define TIMER_RESET_PID 3
#define TIMER_TOP_CHECK 4
#define TIMER_CK_DEALY_HIDE 5
#define TIMER_SHUTDOWN_TICK 6

//Global init

bool XCreateMainWindow()
{
	if (XRegisterClass(hInst))
	{
		hWndMain = CreateWindowW(L"XObserver", L"JY Killer", WS_OVERLAPPED |WS_CAPTION | WS_SYSMENU |WS_MINIMIZEBOX,
			CW_USEDEFAULT, 0, 500, 210, nullptr, nullptr, hInst, nullptr);

		if (!hWndMain) return FALSE;

		ShowWindow(hWndMain, SW_SHOW);
		UpdateWindow(hWndMain);

		return TRUE;
	}

	return false;
}
void XDestroyMainWindow()
{
	if (IsWindow(hWndMain))
	{
		DestroyWindow(hWndMain);
		hWndMain = NULL;
	}
}

bool XInitApp()
{
	int oldStatus = XCheckRunningApp();
	if (oldStatus == 1)
	{
		MessageBox(0, L"已经有一个程序正在运行，同时只能运行一个实例，请关闭之前那个", L"错误", MB_ICONERROR);
		return false;
	}
	if (oldStatus == -1)
		return false;

	//if(XCheckForUpdate())
	//	return false;

	EnableDebugPriv(SE_DEBUG_NAME);
	EnableDebugPriv(SE_SHUTDOWN_NAME);
	EnableDebugPriv(SE_LOAD_DRIVER_NAME);

	int os = XDetermineSystemVersion();
	isXp = os == 1;
	isWin7 = os == 2;

	LoadNt();

	XLoadConfig();
	if(!forceNoDriver) XLoadDriver();

	if(!WInitResolver())
		return false;
	if (!XCreateMainWindow())
		return false;

	return true;
}
void XQuitApp() 
{
	XUnLoadDriver();
	TSendCkEnd();
	WUnInitResolver();
	XDestroyMainWindow();
	PostQuitMessage(0);
}
bool XPreReadCommandLine(LPWSTR *szArgList, int argCount) {
	if (argCount >= 2) 
	{
		if (wcscmp(szArgList[1], L"rex1") == 0) {
			MessageBox(NULL, L"刚才极域进行了非常严重的非法操纵，企图威胁本程序，现在极域已经被结束。", L"提示", MB_ICONINFORMATION);
		}
		else if (wcscmp(szArgList[1], L"rex2") == 0) {

		}
		else if (wcscmp(szArgList[1], L"ssss") == 0) {
			if (XLoadDriver()) {
				KFShutdown();
				XUnLoadDriver();
			}
		}
		else if (wcscmp(szArgList[1], L"sss") == 0)
		{
			ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, 0);
		}
		else if (wcscmp(szArgList[1], L"ssr") == 0)
		{
			ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0);
		}
		else if (wcscmp(szArgList[1], L"-from") == 0 && argCount >= 3) {
			wcscpy_s(fromRunner, szArgList[2]);
		}
		else if (wcscmp(szArgList[1], L"-rm-updater") == 0) {
			if (PathFileExists(updateFilePath))
				DeleteFile(updateFilePath);
		}
	}
	return false;
}
void XOutPutStatus(const wchar_t* str, ...)
{
	time_t time_log = time(NULL);
	struct tm tm_log;
	localtime_s(&tm_log, &time_log);
	va_list arg;
	va_start(arg, str);
	wstring format1 = FormatString(L"[%02d:%02d:%02d] %s\n", tm_log.tm_hour, tm_log.tm_min, tm_log.tm_sec, str);
	wstring out = FormatString(format1.c_str(), arg);
	//SendMessage(hListBoxStatus, LB_ADDSTRING, 0, (LPARAM)out.c_str());
	//SendMessage(hListBoxStatus, LB_SETTOPINDEX, ListBox_GetCount(hListBoxStatus) - 1, 0);

	OnSetOutPutStat((LPWSTR)out.c_str());
	OutputDebugString(out.c_str());

	va_end(arg);
}
void XLoadConfig()
{
	if (PathFileExists(iniPath))
	{
		WCHAR w[32];
		GetPrivateProfileString(L"JYK", L"AutoForceKill", L"FALSE", w, 32, iniPath);
		if (StrEqual(w, L"TRUE") || StrEqual(w, L"true") || StrEqual(w, L"1")) setAutoForceKill = true;

		GetPrivateProfileString(L"JYK", L"AutoIncludeFullWindow", L"FALSE", w, 32, iniPath);
		if (StrEqual(w, L"TRUE") || StrEqual(w, L"true") || StrEqual(w, L"1")) setAutoIncludeFullWindow = true;

		GetPrivateProfileString(L"JYK", L"TopMost", L"FALSE", w, 32, iniPath);
		if (StrEqual(w, L"TRUE") || StrEqual(w, L"true") || StrEqual(w, L"1")) top = true;

		GetPrivateProfileString(L"JYK", L"CKinterval", L"4", w, 32, iniPath);
		if (!StrEmepty(w)) {
			int ww = _wtoi(w);
			if (ww < 1 || ww > 10)
			{
				XOutPutStatus(L"CKinterval 输入了一个无效的参数。CKinterval 有效值为 1-10 [%d]", ww);
				setCKinterval = 4000;
			}
			else setCKinterval = ww * 1000;
		}

		GetPrivateProfileString(L"JYK", L"SelfProtect", L"TRUE", w, 32, iniPath);
		if (!StrEqual(w, L"TRUE") && !StrEqual(w, L"true") && !StrEqual(w, L"1")) setSelfProtect = false;

		GetPrivateProfileString(L"JYK", L"ForceDisableDriver", L"FALSE", w, 32, iniPath);
		if (StrEqual(w, L"TRUE") || StrEqual(w, L"true") || StrEqual(w, L"1")) forceNoDriver = true;
		
	}
	else XOutPutStatus(L"未找到配置文件 [%s]，使用默认配置", iniPath);
}
INT XCheckRunningApp()//如果程序已经有一个在运行，则返回true
{
	HWND oldWindow = FindWindow(NULL, L"JY Killer");
	if (oldWindow != NULL) {
		if (!IsWindowVisible(oldWindow)) ShowWindow(oldWindow, SW_SHOW);
		if (IsIconic(oldWindow)) ShowWindow(oldWindow, SW_RESTORE);
		SetForegroundWindow(oldWindow);
		return -1;
	}
	HANDLE  hMutex = CreateMutex(NULL, FALSE, L"JYKiller");
	if (hMutex && (GetLastError() == ERROR_ALREADY_EXISTS))
	{
		CloseHandle(hMutex);
		hMutex = NULL;
		return 1;
	}
	return 0;
}

//Tray

HMENU hMenuTray;
NOTIFYICONDATA nid;
UINT WM_TASKBARCREATED;

ATOM XRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_JIYUKILLER));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"XObserver";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	return RegisterClassExW(&wcex);
}

bool mainDomReday = false;

//Elements

htmlayout::dom::element stat_ctl_text;
htmlayout::dom::element stat_ctl_text2;
htmlayout::dom::element stat_ico_ctl;

htmlayout::dom::element stat_ck_text;
htmlayout::dom::element stat_ico_ck;

htmlayout::dom::element exten_area;
htmlayout::dom::element status_area;

htmlayout::dom::element input_cmd;
htmlayout::dom::element link_more;

htmlayout::dom::element stat_ico_proc;
htmlayout::dom::element stat_proc_text;

htmlayout::dom::element xstate;

//Events

BOOL OnWmCreate(HWND hWnd)
{
	//Tray icon
	WM_TASKBARCREATED = RegisterWindowMessage(TEXT("TaskbarCreated"));
	CreateTrayIcon(hWnd);
	hMenuTray = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MAINMENU));
	hMenuTray = GetSubMenu(hMenuTray, 0);

	if (top) {
		SetWindowPos(hWndMain, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		SetTimer(hWndMain, TIMER_AOP, 100, NULL);
	}
	SetTimer(hWnd, TIMER_RESET_PID, 5000, NULL);
	ResetPid();
	TFindJiYuPath();
	SetCK(true);

	return TRUE;
}
void OnWmDestroy(HWND hWnd)
{
	KillTimer(hWnd, TIMER_TOP_CHECK);
	KillTimer(hWnd, TIMER_RESET_PID);

	if (!isUserCancel && controlled)
	{
		TSendBoom();
		PostQuitMessage(0);
		ShellExecute(NULL, L"runas", fullPath, L"rex1", NULL, SW_SHOW);
	}
}
void OnDocumentComplete() 
{
	htmlayout::dom::root_element root(hWndMain);
	stat_ctl_text = root.get_element_by_id(L"stat_ctl_text");
	stat_ctl_text2 = root.get_element_by_id(L"stat_ctl_text2");
	stat_ico_ctl = root.get_element_by_id(L"stat_ico_ctl");
	stat_ck_text = root.get_element_by_id(L"stat_ck_text");
	stat_ico_ck = root.get_element_by_id(L"stat_ico_ck");
	stat_proc_text = root.get_element_by_id(L"stat_proc_text");
	stat_ico_proc = root.get_element_by_id(L"stat_ico_proc");

	exten_area = root.get_element_by_id(L"exten_area");
	status_area = root.get_element_by_id(L"status_area");
	input_cmd = root.get_element_by_id(L"input_cmd");

	link_more = root.get_element_by_id(L"link_more");
	xstate = root.get_element_by_id(L"xstate");

	if (setAutoForceKill) {
		htmlayout::dom::element ele(root.get_element_by_id(L"check_auto_fkill"));
		htmlayout::set_checkbox_bits(ele, json::value(true));
	}
	if (setAutoIncludeFullWindow) {
		htmlayout::dom::element ele(root.get_element_by_id(L"check_auto_fck"));
		htmlayout::set_checkbox_bits(ele, json::value(true));
	}
	if (top) {
		htmlayout::dom::element ele(root.get_element_by_id(L"check_top"));
		htmlayout::set_checkbox_bits(ele, json::value(true));
	}
	mainDomReday = true;
}
void OnRunCmd(vector<wstring> * cmds, int len)
{
	bool succ = true;
	wstring cmd = (*cmds)[0];
	if (cmd == L"killst") {
		if (TForceKill()) {
			SetCtlStatus(false, (LPWSTR)L"已成功结束极域进程");
			XOutPutStatus(L"已成功结束极域进程");
		}
		else XOutPutStatus(L"无法结束极域进程 %s", TGetLastError());
	}
	else if (cmd == L"rerunst") {
		LPWSTR jiyuPath = TGetLastJiYuPayh();
		if (StrEqual(jiyuPath, L"") || !PathFileExists(jiyuPath)) {
			XOutPutStatus(L"无法定位极域主进程位置 %s", jiyuPath);
			MessageBox(hWndMain, L"无法定位极域主进程位置，请手动启动。", L"错误", 0);
		}
		else {
			ShellExecute(hWndMain, L"open", jiyuPath, NULL, NULL, SW_SHOW);
			XOutPutStatus(L"已经启动极域 %s", jiyuPath);
		}
	}
	else if (cmd == L"ss")  TSendBoom();
	else if (cmd == L"sss") ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, 0);
	else if (cmd == L"ssr") ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0);
	else if (cmd == L"ssss") KFShutdown(); 
	else if (cmd == L"sssr") KFReboot();
	else if (cmd == L"ckend") SetCkEnd();
	else if (cmd == L"killjynor") {
		if (TLocated()) {
			HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, TGetLastJiYuPid());
			if (hProcess && hProcess != INVALID_HANDLE_VALUE) {
				XOutPutStatus(L"Open jiyu Process success ! pid: %d hProcess : 0x%08x", TGetLastJiYuPid(), hProcess);
			}
			else XOutPutStatus(L"MOpenProcessNt failed : %d", GetLastError());
		}
		else XOutPutStatus(L"Not found jiyu !");
	}
	else if (cmd == L"openjy") {
		if (TLocated()) {
			HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, TGetLastJiYuPid());
			if (hProcess && hProcess != INVALID_HANDLE_VALUE)
				XOutPutStatus(L"Open jiyu Process success ! pid: %d hProcess : 0x%08x", TGetLastJiYuPid(), hProcess);
			else XOutPutStatus(L"MOpenProcessNt failed : %d", GetLastError());
		}
		else XOutPutStatus(L"Not found jiyu !");
	}
	else if (cmd == L"ntopenjy") {
		if (TLocated()) {
			HANDLE hProcess;
			NTSTATUS ntStatus = MOpenProcessNt(TGetLastJiYuPid(), &hProcess);
			if (NT_SUCCESS(ntStatus))
				XOutPutStatus(L"Open jiyu Process success ! pid: %d hProcess : 0x%08x", TGetLastJiYuPid(), hProcess);
			else XOutPutStatus(L"MOpenProcessNt failed : 0x%08X", ntStatus);
		}
		else XOutPutStatus(L"Not found jiyu !");
	}
	else if (cmd == L"fuldrv") {
		UnLoadKernelDriver(L"JiYuKillerDriver");
	}
	else if (cmd == L"fuljydrv") {
		UnLoadKernelDriver(L"TDProcHook");
	}
	else if (cmd == L"uninstall") {
		ShellExecute(0, L"runas", fullPath, L"-uninstall", NULL, SW_SHOW);
		ExitProcess(0);
		TerminateProcess(GetCurrentProcess(), 0);
	}
	else if (cmd == L"setshutsec") {
		if (len >= 2) shutdownTick = _wtoi((*cmds)[1].c_str());
	}
	else {
		succ = false;
		MessageBox(0, L"未知命令", L"JY Killer", 0);
	}
	if (succ) htmlayout::set_value(input_cmd, json::value(L""));
}
void OnWmCommand(HWND hWnd, WPARAM wmId)
{
	switch (wmId)
	{
	case IDM_SHOWMAIN: {
		if (IsWindowVisible(hWnd))
			ShowWindow(hWnd, SW_HIDE);
		else
		{
			ShowWindow(hWnd, SW_SHOW);
			SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		}
		break;
	}
	case IDM_EXIT: {
		if (!controlled || MessageBox(hWnd, L"极域正在运行，退出本软件将导致极域脱离控制，您是否还要退出？", L"注意", MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
			isUserCancel = true;
			XQuitApp();
		}
		break;
	}
	case IDM_HELP: ARunAboutWindow(hWnd); break;
	default: break;
	}
}
void OnSetOutPutStat(LPWSTR str)
{
	if (wcslen(statusBuffer) < 2000) wcscat_s(statusBuffer, str);
	else wcscpy_s(statusBuffer, str);

	if (mainDomReday) {
		status_area.set_value(json::value(statusBuffer));

		POINT scroll_pos;
		RECT view_rect;
		SIZE content_size;
		status_area.get_scroll_info(scroll_pos, view_rect, content_size);
		scroll_pos.y = content_size.cy;
		status_area.set_scroll_pos(scroll_pos);
	}
}
void OnButtonClick(HELEMENT btn)
{
	htmlayout::dom::element cBut(btn);
	if (StrEqual(cBut.get_attribute("id"), L"check_auto_fck"))
	{
		if (setAutoIncludeFullWindow)
		{
			setAutoIncludeFullWindow = false;
			cBut.remove_attribute("checked");
		}
		else
		{
			setAutoIncludeFullWindow = true;
			cBut.set_attribute("checked", L"checked");
		}
	}
	else if (StrEqual(cBut.get_attribute("id"), L"check_auto_fkill"))
	{
		if (setAutoForceKill)
		{
			setAutoForceKill = false;
			cBut.remove_attribute("checked");
		}
		else
		{
			setAutoForceKill = true;
			cBut.set_attribute("checked", L"checked");
		}
	}
	else if (StrEqual(cBut.get_attribute("id"), L"check_ck"))
	{
		if (ck) 
		{
			ck = false;
			cBut.remove_attribute("checked");
		}
		else
		{
			ck = true;
			cBut.set_attribute("checked", L"checked");
		}
		SetCK(ck);
	}
	else if (StrEqual(cBut.get_attribute("id"), L"check_top"))
	{
		if (top)
		{
			top = false;
			cBut.remove_attribute("checked");

			SetWindowPos(hWndMain, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			KillTimer(hWndMain, TIMER_AOP);
		}
		else
		{
			top = true;
			cBut.set_attribute("checked", L"checked");

			SetWindowPos(hWndMain, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			SetTimer(hWndMain, TIMER_AOP, 100, NULL);
		}
	}
}
void OnLinkClick(HELEMENT link)
{
	htmlayout::dom::element cBut = link;
	if (StrEqual(cBut.get_attribute("id"), L"bottom_about")) SendMessage(hWndMain, WM_COMMAND, IDM_HELP, 0);
	else if (StrEqual(cBut.get_attribute("id"), L"link_exit")) SendMessage(hWndMain, WM_COMMAND, IDM_EXIT, 0);
	else if (StrEqual(cBut.get_attribute("id"), L"link_more"))
	{
		if (fullWindow)
		{
			fullWindow = false;
			RECT rc; GetWindowRect(hWndMain, &rc);
			MoveWindow(hWndMain, rc.left, rc.top, rc.right - rc.left, 210, TRUE);

			exten_area.set_attribute("style", L"display: none;");
			link_more.set_text(L"▼ 高级模式");
		}
		else
		{
			fullWindow = true;
			RECT rc; GetWindowRect(hWndMain, &rc);
			MoveWindow(hWndMain, rc.left, rc.top, rc.right - rc.left, 450, TRUE);

			exten_area.set_attribute("style", L"");
			link_more.set_text(L"▲ 隐藏面板");
		}
	}
	else if (StrEqual(cBut.get_attribute("id"), L"link_kill")) {
		if (TForceKill()) {
			SetCtlStatus(false, (LPWSTR)L"已成功结束极域进程");
			XOutPutStatus(L"已成功结束极域进程");
		}else XOutPutStatus(L"无法结束极域进程 %s", TGetLastError());
	}
	else if (StrEqual(cBut.get_attribute("id"), L"link_rerun"))
	{
		LPWSTR jiyuPath = TGetLastJiYuPayh();
		if (StrEqual(jiyuPath, L"") || !PathFileExists(jiyuPath)) {
			XOutPutStatus(L"无法定位极域主进程位置 %s", jiyuPath);
			MessageBox(hWndMain, L"无法定位极域主进程位置，请手动启动。", L"错误", 0);
		}
		else {
			ShellExecute(hWndMain, L"open", jiyuPath, NULL, NULL, SW_SHOW);
			XOutPutStatus(L"已经启动极域 %s", jiyuPath);
		}
	}
	else if (StrEqual(cBut.get_attribute("id"), L"link_runcmd"))
	{
		json::value v = htmlayout::get_value(input_cmd);
		if (v.is_string()) 
		{
			aux::wchars str = v.get_chars();
			size_t len = str.length + 1;
			LPWSTR cmdsx = (LPWSTR)malloc(sizeof(WCHAR) *len);
			wcscpy_s(cmdsx, len, str.start);

			wstring cmdx(cmdsx);
			if (cmdx == L"") 
				MessageBox(hWndMain, L"请输入命令！", L"命令帮助", MB_ICONEXCLAMATION);		
			else {
				vector<wstring> cmds;
				SplitString(cmdx, cmds, L" ");
				OnRunCmd(&cmds, cmds.size());
			}
		}
	}
	else if (StrEqual(cBut.get_attribute("id"), L"link_cmds"))
	{
		MessageBox(hWndMain, L"killst \nrerunst \nss \nsss \nssss \n", L"命令帮助", 0);
	}
	else if (StrEqual(cBut.get_attribute("id"), L"link_shutdown")) {
		if (MessageBox(hWndMain, L"真的要关机吗？\n在离开之前请注意不要遗忘东西！", L"注意", MB_ICONEXCLAMATION | MB_YESNO) == IDYES)
			QuitAndShutdown();
	}
}
LRESULT OnWmTimer(HWND hWnd, WPARAM timerId)
{
	switch (timerId)
	{
	case TIMER_RESET_PID: ResetPid(); break;
	case TIMER_TOP_CHECK: {
		SetCkStatus(TRunCK(), WGetCkStatText());
		SetTimer(hWnd, TIMER_CK_DEALY_HIDE, 2000, NULL);
		break;
	}
	case TIMER_AOP: {
		SetWindowPos(hWndMain, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
		break;
	}
	case TIMER_CK_DEALY_HIDE: {
		KillTimer(hWnd, TIMER_CK_DEALY_HIDE);
		SetCkStatus(0, (LPWSTR)L"");
		break;
	}
	default:break;
	}
	return 0;
}
void OnHandleMsg(LPWSTR buff) {
	
	wstring act(buff);
	vector<wstring> arr;
	SplitString(act, arr, L":");
	if (arr.size() >= 2) 
	{
		if (arr[0] == L"hkb") 
		{
			if (arr[1] == L"succ") {
				SetCtlStatus(true, TGetCurrStatText());
				XOutPutStatus(L"Receive  ctl success message ");
			}
			else if (arr[1] == L"immck") {
				SendMessage(hWndMain, WM_TIMER, TIMER_TOP_CHECK, NULL);
				XOutPutStatus(L"Receive  immck message ");
			}
		}
		else if (arr[0] == L"wcd")
		{
			//wwcd
			int wcdc = _wtoi(arr[1].c_str());
			if (wcdc % 10 == 0)
				XOutPutStatus(L"Receive  watch dog message %d ", wcdc);
		}
		else if (arr[0] == L"pub") {
			if (arr[1] == L"canshut") {
				SendMessage(hDlgShut, WM_COMMAND, IDC_CANCEL, NULL);
			}
			else if (arr[1] == L"quit") {
				//Quit
			}
		}
	}
}
void OnWmUser(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	if (lParam == WM_LBUTTONDBLCLK && !qsStarted)
		SendMessage(hDlg, WM_COMMAND, IDM_SHOWMAIN, lParam);
	if (lParam == WM_RBUTTONDOWN && !qsStarted)
	{
		POINT pt;
		GetCursorPos(&pt);//取鼠标坐标  
		SetForegroundWindow(hDlg);//解决在菜单外单击左键菜单不消失的问题  
		TrackPopupMenu(hMenuTray, TPM_RIGHTBUTTON, pt.x - 177, pt.y, NULL, hDlg, NULL);//显示菜单并获取选项ID  
	}
}

//Status set

void SetCK(bool enable) 
{
	if (enable)
	{
		SetTimer(hWndMain, TIMER_TOP_CHECK, setCKinterval, NULL);
		XOutPutStatus(L"CK 正在运行");
		SendMessage(hWndMain, WM_TIMER, TIMER_TOP_CHECK, NULL);
	}
	else 
	{
		KillTimer(hWndMain, TIMER_TOP_CHECK);
		SetCkStatus(-1, (LPWSTR)L"CK 已停止");
		XOutPutStatus(L"CK 已停止");
	}
	UpdateLogoState();
}
void ResetPid()
{
	if (TReset())
	{
		bool b = TLocated();
		SetProcStatus(b, b ? TGetCurrProcStatText() : (LPWSTR)L"");
		TSendCtlStat();
		UpdateLogoState();
	}
}

void SetProcStatus(bool ctled, LPWSTR extendInfo)
{
	if (ctled)
		stat_ico_proc.set_attribute("class", L"xico ico-green");
	else {
		stat_ico_proc.set_attribute("class", L"xico ico-red");
		SetCtlStatus(false, NULL);
	}
	if (extendInfo != NULL) stat_proc_text.set_text(extendInfo);
}
void SetCtlStatus(bool ctled, LPWSTR extendInfo) {
	if (controlled != ctled)
	{
		if (ctled) 
		{
			stat_ico_ctl.set_attribute("class", L"xico ico-ctl-yes");
			stat_ctl_text.set_text(L"当前已控制极域主进程");
			stat_ctl_text.set_attribute("class", L"text-green");
		}
		else {
			stat_ico_ctl.set_attribute("class", L"xico ico-ctl-no");
			stat_ctl_text.set_attribute("class", L"text-red");
			stat_ctl_text.set_text(L"当前未控制极域主进程");
		}
		controlled = ctled;
		UpdateLogoState();
	}
	if (extendInfo != NULL) stat_ctl_text2.set_text(extendInfo);
}
void SetCkStatus(int s, LPWSTR str)
{
	if (s == -1) 
		stat_ico_ck.set_attribute("class", L"xico ico-red");
	else if (s == 0) 
		stat_ico_ck.set_attribute("class", L"xico ico-grey");
	else if (s == 1) 
		stat_ico_ck.set_attribute("class", L"xico ico-green");
	if(str!=NULL) stat_ck_text.set_text(str);
	UpdateLogoState();
}
void SetLogoState(const wchar_t* s)
{
	WCHAR cls[65];
	swprintf_s(cls, L"xstate %s", s);
	if (!StrEqual(cls, xstate.get_attribute("class")))
		xstate.set_attribute("class", cls);
}
void UpdateLogoState() 
{
	bool tl = TLocated();
	bool wf = WLastState();
	if (controlled)
	{
		if (wf) SetLogoState(L"xstate-ctled-founded");
		else SetLogoState(L"xstate-ctled-nofound");
	}
	else
	{
		if (tl && wf) SetLogoState(L"xstate-noctl-running-andfounded");
		else if (tl) SetLogoState(L"xstate-noctl-running");
		else SetLogoState(L"xstate-noctl-nojiyu");
	}
}
void SetCkEnd()
{
	SetLogoState(L"xstate-ckend");
	TSendCkEnd();
}
BOOL LoadMainHtml(HWND hWnd)
{
	BOOL result = FALSE;
	HRSRC hResource = FindResource(hInst, MAKEINTRESOURCE(IDR_HTML_MAIN), RT_HTML);
	if (hResource) {
		HGLOBAL hg = LoadResource(hInst, hResource);
		if (hg) {
			LPVOID pData = LockResource(hg);
			if (pData)
				result = HTMLayoutLoadHtml(hWnd, (LPCBYTE)pData, SizeofResource(hInst, hResource));
		}
	}
	hResource = FindResource(hInst, MAKEINTRESOURCE(IDR_CSS_MAIN), L"CSS");
	if (hResource) {
		HGLOBAL hg = LoadResource(hInst, hResource);
		if (hg) {
			LPVOID pData = LockResource(hg);
			if (pData)
				result = HTMLayoutSetCSS(hWnd, (LPCBYTE)pData, SizeofResource(hInst, hResource), L"", L"text/css");
		}
	}
	return result;
}

void QuitAndShutdown()
{
	if (!qsStarted)
	{
		qsStarted = true;
		ShowWindow(hWndMain, SW_HIDE);

		hDlgShut = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SHUTTIP), hWndMain, (DLGPROC)ShutDlgProc);
		ShowWindow(hDlgShut, SW_SHOW);
		UpdateWindow(hDlgShut);

		lstrcpy(nid.szTip, L"JY Killer - 关机程序已启动");
		Shell_NotifyIcon(NIM_MODIFY, &nid);
	}
}

//Tray

void CreateTrayIcon(HWND hDlg) {
	nid.cbSize = sizeof(nid);
	nid.hWnd = hDlg;
	nid.uID = 0;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_INFO | NIF_TIP;
	nid.uCallbackMessage = WM_USER;
	nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_JIYUKILLER));
	lstrcpy(nid.szTip, L"JY Killer");
	Shell_NotifyIcon(NIM_ADD, &nid);
}
void ShowTrayBaloonTip(const wchar_t* title , const wchar_t* text) {
	lstrcpy(nid.szInfo, text);
	nid.dwInfoFlags = NIIF_NONE;
	lstrcpy(nid.szInfoTitle, title);
	Shell_NotifyIcon(NIM_MODIFY, &nid);
}

struct MainWindowDOMEventsHandlerType : htmlayout::event_handler
{
	MainWindowDOMEventsHandlerType() : event_handler(0xFFFFFFFF) {}
	virtual BOOL handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params)
	{
		switch (params.cmd)
		{
		case BUTTON_CLICK: OnButtonClick(params.heTarget); break;// click on button
		case BUTTON_PRESS: break;// mouse down or key down in button
		case BUTTON_STATE_CHANGED:      break;
		case EDIT_VALUE_CHANGING:       break;// before text change
		case EDIT_VALUE_CHANGED:        break;//after text change
		case SELECT_SELECTION_CHANGED:  break;// selection in <select> changed
		case SELECT_STATE_CHANGED:      break;// node in select expanded/collapsed, heTarget is the node
		case POPUP_REQUEST: break;// request to show popup just received, 
				  //     here DOM of popup element can be modifed.
		case POPUP_READY:               break;// popup element has been measured and ready to be shown on screen,
											  //     here you can use functions like ScrollToView.
		case POPUP_DISMISSED:           break;// popup element is closed,
											  //     here DOM of popup element can be modifed again - e.g. some items can be removed
											  //     to free memory.
		case MENU_ITEM_ACTIVE:                // menu item activated by mouse hover or by keyboard
			break;
		case MENU_ITEM_CLICK:                 // menu item click 
			break;
			// "grey" event codes  - notfications from behaviors from this SDK 
		case HYPERLINK_CLICK: OnLinkClick(params.heTarget);  break;// hyperlink click
		case TABLE_HEADER_CLICK:        break;// click on some cell in table header, 
											  //     target = the cell, 
											  //     reason = index of the cell (column number, 0..n)
		case TABLE_ROW_CLICK:           break;// click on data row in the table, target is the row
											  //     target = the row, 
											  //     reason = index of the row (fixed_rows..n)
		case TABLE_ROW_DBL_CLICK:       break;// mouse dbl click on data row in the table, target is the row
											  //     target = the row, 
											  //     reason = index of the row (fixed_rows..n)

		case ELEMENT_COLLAPSED:         break;// element was collapsed, so far only behavior:tabs is sending these two to the panels
		case ELEMENT_EXPANDED:          break;// element was expanded,
		case DOCUMENT_COMPLETE: OnDocumentComplete(); break;
		}
		return FALSE;
	}

} MainWindowDOMEventsHandlerType;

LRESULT CALLBACK MainWindowHTMLayoutNotifyHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, LPVOID vParam)
{
	// all HTMLayout notification are comming here.
	NMHDR*  phdr = (NMHDR*)lParam;

	switch (phdr->code)
	{
	case HLN_CREATE_CONTROL:    break; //return OnCreateControl((LPNMHL_CREATE_CONTROL) lParam);
	case HLN_CONTROL_CREATED:   break; //return OnControlCreated((LPNMHL_CREATE_CONTROL) lParam);
	case HLN_DESTROY_CONTROL:   break; //return OnDestroyControl((LPNMHL_DESTROY_CONTROL) lParam);
	case HLN_LOAD_DATA:         break;
	case HLN_DATA_LOADED:       break; //return OnDataLoaded((LPNMHL_DATA_LOADED)lParam);
	case HLN_DOCUMENT_COMPLETE: break; //return OnDocumentComplete();
	case HLN_ATTACH_BEHAVIOR:   break;
	}
	return 0;
}

//Main WndProc
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = 0;
	BOOL bHandled;

	lResult = HTMLayoutProcND(hWnd, message, wParam, lParam, &bHandled);
	if (!bHandled)
	{
		switch (message)
		{
		case WM_CREATE: { 
			hWndMain = hWnd;

			HTMLayoutSetCallback(hWnd, &MainWindowHTMLayoutNotifyHandler, 0);
			// attach DOM events handler so we will be able to receive DOM events like BUTTON_CLICK, HYPERLINK_CLICK, etc.
			htmlayout::attach_event_handler(hWnd, &MainWindowDOMEventsHandlerType);

			if (!LoadMainHtml(hWnd)) return FALSE;  

			OnDocumentComplete();

			return OnWmCreate(hWnd); 
		}
		case WM_COMMAND: OnWmCommand(hWnd, wParam); break;
		case WM_SYSCOMMAND: {
			switch (wParam)
			{
			case SC_RESTORE: ShowWindow(hWnd, SW_RESTORE); SetForegroundWindow(hWnd); break;
			case SC_MINIMIZE: ShowWindow(hWnd, SW_MINIMIZE);  break;
			case SC_CLOSE: {
				ShowWindow(hWnd, SW_HIDE);
				if (!hideTipShowed) {
					ShowTrayBaloonTip(L"JiYu Killer 提示", L"窗口隐藏到此处了，双击这里显示主界面");
					hideTipShowed = true;
				}
				break;
			}
			default: return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		}
		case WM_COPYDATA: {
			PCOPYDATASTRUCT  pCopyDataStruct = (PCOPYDATASTRUCT)lParam;
			if (pCopyDataStruct->cbData > 0)
			{
				WCHAR recvData[256] = { 0 };
				wcsncpy_s(recvData, (WCHAR *)pCopyDataStruct->lpData, pCopyDataStruct->cbData);
				OnHandleMsg(recvData);
			}
			break;
		}
		case WM_SHOWWINDOW: {
			if (wParam)
			{
				if (firstShow) 
				{
					//窗口居中
					RECT rect; GetWindowRect(hWnd, &rect);
					rect.left = (screenWidth - (rect.right - rect.left)) / 2;
					rect.top = (screenHeight - (rect.bottom - rect.top)) / 2 - 60;
					SetWindowPos(hWnd, 0, rect.left, rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

					firstShow = false;
				}
			}
			break;
		}
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			break;
		}
		case WM_DESTROY:  OnWmDestroy(hWnd); break;
		case WM_TIMER: return OnWmTimer(hWnd, wParam);
		case WM_USER: OnWmUser(hWnd, wParam, lParam); break;
		default: return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	return lResult;
}

HFONT hFontShutDlgBase;
HFONT hFontShutDlg2;


//Shut dlg WndProc
INT_PTR CALLBACK ShutDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG: {
		hFontShutDlgBase = CreateFontW(27, 0, 0, 0, 0, FALSE, FALSE, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");//创建字体
		hFontShutDlg2 = CreateFontW(20, 0, 0, 0, 0, FALSE, FALSE, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");//创建字体
		SendDlgItemMessage(hDlg, IDC_SHUT_SEC, WM_SETFONT, (WPARAM)hFontShutDlgBase, TRUE);//发送设置字体消息
		SendDlgItemMessage(hDlg, IDC_TIP_TEXT, WM_SETFONT, (WPARAM)hFontShutDlg2, TRUE);//发送设置字体消息
		SetTimer(hDlg, TIMER_SHUTDOWN_TICK, 1000, NULL);
		SendMessage(hDlg, WM_TIMER, TIMER_SHUTDOWN_TICK, NULL);
		if (!StrEmepty(fromRunner)) {
			if (!PathFileExists(fromRunner)) 
				SetDlgItemText(hDlg, IDC_TIP_TEXT, L"已开始执行任务，请注意是否有东西遗忘！");
		}
		SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		break;
	}
	case WM_DEVICECHANGE: {
		if (!StrEmepty(fromRunner)) {
			if (!PathFileExists(fromRunner))
				SetDlgItemText(hDlg, IDC_TIP_TEXT, L"U 盘已拔出，请注意是否有其他东西遗忘！。\n已开始执行任务");
		}
		break;
	}
	case WM_DESTROY: {
		DeleteObject(hFontShutDlgBase);
		DeleteObject(hFontShutDlg2);
		break;
	}
	case WM_SYSCOMMAND: {
		break;
	}
	case WM_COMMAND: {
		switch (wParam)
		{
		case IDC_SHUTNOW: 
			ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, 0);
			DestroyWindow(hDlg);
			PostQuitMessage(0);
			break;
		case IDC_CANCEL: {
			ShowWindow(hDlg, SW_HIDE);
			ShowWindow(hWndMain, SW_SHOW);
			shutdownTick = 30;
			qsStarted = false;
			MessageBox(hWndMain, L"关机计划已取消！", L"提示", MB_ICONINFORMATION);
			DestroyWindow(hDlg);
			break;
		}
		default: break;
		}
		break;
	}
	case WM_TIMER: {
		if (wParam == TIMER_SHUTDOWN_TICK) {

			WCHAR str[16]; swprintf_s(str, L"%d 秒", shutdownTick);
			SetDlgItemText(hDlg, IDC_SHUT_SEC, str);
			if (shutdownTick <= 0) {
				KillTimer(hDlg, TIMER_SHUTDOWN_TICK);
				ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, 0);
				DestroyWindow(hDlg);
				PostQuitMessage(0);
			}
			shutdownTick--;
		}
		break;
	}
	case WM_ENDSESSION:
	case WM_QUERYENDSESSION: {
		PostQuitMessage(0);
		break;
	}
	}
	return (INT_PTR)FALSE;
}