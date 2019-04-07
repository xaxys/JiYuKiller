#include "stdafx.h"
#include "UpdateWindow.h"
#include "MainWindow.h"
#include "NetUtils.h"
#include "resource.h"
#include "StringHlp.h"
#include "StringHlp.h"
#include <htmlayout.h>
#include <Wininet.h>
#include <Sensapi.h>
#include <Shlwapi.h>
#include <time.h>

/*

extern HINSTANCE hInst;
extern int screenWidth, screenHeight;

#define CURRENT_VERSION "1.2.0325.1030" 

#define IDC_DOWNLOAD_THREAD_END 901881
#define IDC_USER_CANCELED 901882

bool cancelByUser = false;
bool updateing = false;
bool downloadFail = false;
bool installFinishedAndNeedExit = false;
int downloadFailCode = 0;
HANDLE updateThread = NULL;

extern WCHAR updateFilePath[MAX_PATH];
extern WCHAR fromRunner[MAX_PATH];
extern WCHAR iniPath[MAX_PATH];

bool XCheckLastUpdateDate() {

	if (PathFileExists(iniPath))
	{
		WCHAR w[32];
		WCHAR wnow[32];
		GetPrivateProfileString(L"JYK", L"LastCheckUpdateTime", L"00/00", w, 32, iniPath);

		time_t tt = time(NULL);//这句返回的只是一个时间cuo
		tm* t = localtime(&tt);
		swprintf_s(wnow, L"%00d/%00d", t->tm_mon, t->tm_mday);

		if (StrEqual(w, wnow)) return true;
		else WritePrivateProfileString(L"JYK", L"LastCheckUpdateTime", wnow, iniPath);
	}
	return false;
}
bool XCheckIsInterenet()
{
	return InternetGetConnectedState(NULL, 0);;
}
bool XCheckForUpdate() 
{
	if(XCheckLastUpdateDate())
		return false;
	if(!XCheckIsInterenet())
		return false;

	// test get requery
	string getUrlStr = string("https://www.imyzc.com/softs/jykiller/?checkupdate=") + CURRENT_VERSION;
	string getResponseStr;
	auto res = curl_get_req(getUrlStr, getResponseStr);
	if (res != CURLE_OK) {
		XOutPutStatus(L"检测更新错误：curl_easy_perform() failed:  %S", curl_easy_strerror(res));
		return false;
	}
	else {
		if (getResponseStr == "newupdate") {
			if (MessageBox(0, L"检测到 JiYu Killer 有新的版本了，您是否要升级？", L"提示", MB_ICONINFORMATION | MB_YESNO) == IDYES)
				return XRunUpdate();
			return false;
		}
		else if (getResponseStr == "latest") 	return false;
		else{
			XOutPutStatus(L"检测更新错误：update service return bad result :  %S", getResponseStr.c_str());
			return false;
		}
	}
	return false;
}
bool XRunUpdate() 
{
	// test get requery
	string getUrlStr = string("https://www.imyzc.com/softs/jykiller/?getupdate");
	string getResponseStr;
	auto res = curl_get_req(getUrlStr, getResponseStr);
	if (res != CURLE_OK) {
		XOutPutStatus(L"获取更新错误 : curl_easy_perform() failed:  %S", curl_easy_strerror(res));
		MessageBox(0, L"获取更新错误\n具体错误请参照输出日志", L"更新时发生错误", MB_ICONERROR);
	}
	else if (getResponseStr != "") 
	{
		string downloadUrl = string("https://www.imyzc.com/softs/jykiller/") + getResponseStr;
		updateThread = CreateThread(NULL, NULL, XUpdateDownloadThread, (LPVOID)downloadUrl.c_str(), NULL, NULL);
		XRunUpdateWindow();
		if (cancelByUser || downloadFail) return false;
		if (installFinishedAndNeedExit) return true;
	}
	else {
		XOutPutStatus(L"获取更新错误，空返回值");
		MessageBox(0, L"获取更新错误", L"更新时发生错误", MB_ICONERROR);
	}

	return false;
}

HWND hWndUpdate;
htmlayout::dom::element progress;
htmlayout::dom::element progress_text;

ATOM XRegisterUpdateClass() {

	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = UpdateWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_JIYUKILLER));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"XUpdate";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	if (RegisterClassExW(&wcex) || GetLastError() == ERROR_CLASS_ALREADY_EXISTS)
		return TRUE;
	return FALSE;
}
bool XRunUpdateWindow() 
{
	if (XRegisterUpdateClass())
	{
		hWndUpdate = CreateWindowW(L"XUpdate", L"JY Killer 在线升级", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
			CW_USEDEFAULT, 0, 500, 310, NULL, nullptr, hInst, nullptr);

		if (!hWndUpdate) return FALSE;

		ShowWindow(hWndUpdate, SW_SHOW);
		UpdateWindow(hWndUpdate);

		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return cancelByUser;
}
bool XInstallNewUpdate() {

	if (!StrEmepty(fromRunner) && PathFileExists(fromRunner)) {

		if (DeleteFile(fromRunner))
			if (CopyFile(updateFilePath, fromRunner, TRUE))
				XOutPutStatus(L"安装文件：%s 时发生错误： %d", fromRunner, GetLastError());
	}

	return ((INT)ShellExecute(hWndUpdate, L"runas", updateFilePath, L"-install", 0, SW_SHOW) > 32);
}

DWORD WINAPI XUpdateDownloadThread(LPVOID lpThreadParameter)
{
	updateing = true;
	// init curl
	CURL *curl = curl_easy_init();
	// res code
	CURLcode res;
	if (curl)
	{
		if (PathFileExists(updateFilePath))
			DeleteFileW(updateFilePath);

		FILE *file_param = NULL;
		errno_t err = _wfopen_s(&file_param, updateFilePath, L"wb");
		if (!file_param) {
			XOutPutStatus(L"创建更新文件错误 : fopen:  %d", err);
			downloadFail = true;
			downloadFailCode = 2;
			updateing = false;
			SendMessage(hWndUpdate, WM_COMMAND, IDC_DOWNLOAD_THREAD_END, NULL);
			updateThread = NULL;
			return 0;
		}
		curl_easy_setopt(curl, CURLOPT_URL, (LPCSTR)lpThreadParameter); // url
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false); // if want to use https
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false); // set peer and host verify false
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_HEADER, 1);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3); // set transport and time out time
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, file_param);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, UUpdateProgressFunc);
		// start req
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) 
		{
			XOutPutStatus(L"下载更新文件错误 :: curl_easy_perform() failed :  %S", curl_easy_strerror(res));
			downloadFail = true;
			downloadFailCode = 3;
			updateing = false;
			SendMessage(hWndUpdate, WM_COMMAND, IDC_DOWNLOAD_THREAD_END, NULL);
			fclose(file_param);
			updateThread = NULL;
			return 0;
		}

		fclose(file_param);
	}
	else {
		XOutPutStatus(L"下载更新文件错误 : curl failed ");
		downloadFailCode = 1;
		downloadFail = true;
		updateing = false;
	}

	// release curl
	curl_easy_cleanup(curl);
	SendMessage(hWndUpdate, WM_COMMAND, IDC_DOWNLOAD_THREAD_END, NULL);
	updateing = false;
	downloadFail = false;
	updateThread = NULL;
	return 0;
}

bool UOnExit() 
{
	if (updateing)
	{
		if (MessageBox(0, L"正在下载更新，您是否确定要取消升级？", L"提示", MB_ICONINFORMATION | MB_YESNO) == IDYES)
		{
			cancelByUser = true;
			SendMessage(hWndUpdate, WM_COMMAND, IDC_USER_CANCELED, NULL);
			return true;
		}
	}
	return false;
}
void UOnDocunmentComplete()
{
	htmlayout::dom::root_element root(hWndUpdate);
	progress = root.get_element_by_id(L"progress");
	progress_text = root.get_element_by_id(L"progress_text");
}
void UOnDownThreadFinish(HWND hWnd)
{
	if (downloadFail)
	{
		if (downloadFailCode == 1)
			MessageBox(hWnd, L"未知错误\n具体错误请参照输出日志", L"更新时发生错误", MB_ICONERROR);
		else if (downloadFailCode == 2)
			MessageBox(hWnd, L"创建更新文件错误\n具体错误请参照输出日志", L"更新时发生错误", MB_ICONERROR);
		else if (downloadFailCode == 3)
			MessageBox(hWnd, L"下载更新文件错误\n具体错误请参照输出日志", L"更新时发生错误", MB_ICONERROR);
	}
	else
	{
		ShowWindow(hWnd, SW_HIDE);
		if (!XInstallNewUpdate()) {
			MessageBox(hWnd, L"安装更新时发生错误\n具体错误请参照输出日志", L"更新时发生错误", MB_ICONERROR);
			cancelByUser = true;
			downloadFail = true;
		}
		else 
			installFinishedAndNeedExit = true;
		DestroyWindow(hWnd);
	}
}
void UOnLinkClick(HELEMENT link)
{
	htmlayout::dom::element cBut = link;
	if (StrEqual(cBut.get_attribute("id"), L"link_exit")) SendMessage(hWndUpdate, WM_SYSCOMMAND, SC_CLOSE, 0);
}
void UOnWnCommand(HWND hWnd, WPARAM id) {
	switch (id)
	{
	case IDC_DOWNLOAD_THREAD_END: {
		UOnDownThreadFinish(hWnd);
		break;
	}
	case IDC_USER_CANCELED: {
		if (updateThread) 
			TerminateThread(updateThread, 0);
		break;
	}

	default: break;
	}
}

int UUpdateProgressFunc(void* ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUploaded)
{
	double fractiondownloaded = NowDownloaded / TotalToDownload;
	WCHAR vstr[10];
	swprintf_s(vstr, L"%3.0f%% [", fractiondownloaded * 100);
	progress.set_style_attribute("width", vstr);
	progress_text.set_text(vstr);
	return 0;
}

struct UpdateWindowDOMEventsHandlerType : htmlayout::event_handler
{
	UpdateWindowDOMEventsHandlerType() : event_handler(0xFFFFFFFF) {}
	virtual BOOL handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params)
	{
		switch (params.cmd)
		{
		case BUTTON_CLICK: break;// click on button
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
		case HYPERLINK_CLICK: UOnLinkClick(params.heTarget);  break;// hyperlink click
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
		case DOCUMENT_COMPLETE:UOnDocunmentComplete(); break;
		}
		return FALSE;
	}

} UpdateWindowDOMEventsHandlerType;

LRESULT CALLBACK UpdateWindowHTMLayoutNotifyHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, LPVOID vParam)
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

BOOL ULoadMainHtml(HWND hWnd)
{
	BOOL result = FALSE;
	HRSRC hResource = FindResource(hInst, MAKEINTRESOURCE(IDR_HTML_UPDATE), RT_HTML);
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

LRESULT CALLBACK UpdateWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = 0;
	BOOL bHandled;

	lResult = HTMLayoutProcND(hWnd, message, wParam, lParam, &bHandled);
	if (!bHandled)
	{
		switch (message)
		{
		case WM_CREATE: {
			HTMLayoutSetCallback(hWnd, &UpdateWindowHTMLayoutNotifyHandler, 0);
			// attach DOM events handler so we will be able to receive DOM events like BUTTON_CLICK, HYPERLINK_CLICK, etc.
			htmlayout::attach_event_handler(hWnd, &UpdateWindowDOMEventsHandlerType);
			if (!ULoadMainHtml(hWnd)) return FALSE;
		}
		case WM_SYSCOMMAND: {
			switch (wParam)
			{
			case SC_CLOSE: if(UOnExit()) DestroyWindow(hWnd); break;
			default: return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		}
		case WM_COMMAND: {
			UOnWnCommand(hWnd, wParam);
			break;
		}
		case WM_SHOWWINDOW: {
			if (wParam)
			{
				//窗口居中
				RECT rect; GetWindowRect(hWnd, &rect);
				rect.left = (screenWidth - (rect.right - rect.left)) / 2;
				rect.top = (screenHeight - (rect.bottom - rect.top)) / 2 - 60;
				SetWindowPos(hWnd, 0, rect.left, rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
			}
			break;
		}
		case WM_DESTROY: PostQuitMessage(0); break;
		default: return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	return lResult;
}

*/