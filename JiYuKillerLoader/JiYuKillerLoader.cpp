// JiYuKillerLoader.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "JiYuKillerLoader.h"
#include "resource.h"
#include <windowsx.h>
#include <ShellAPI.h>
#include <shlwapi.h>
#include <winioctl.h>
#include <stdio.h>
#include <io.h>

WCHAR currentDir[MAX_PATH];
WCHAR currentFullPath[MAX_PATH];
WCHAR installDir[MAX_PATH];
WCHAR currentIniPath[MAX_PATH];

WCHAR partMainExePath[MAX_PATH];
WCHAR partMainPath[MAX_PATH];
WCHAR partHtmlayoutPath[MAX_PATH];
WCHAR partVirusPath[MAX_PATH];
WCHAR partDriverPath[MAX_PATH];
WCHAR partIniPath[MAX_PATH];
WCHAR partBatPath[MAX_PATH];

WCHAR partUpdatePath[MAX_PATH];

SHSTDAPI_(BOOL) SHGetSpecialFolderPathW(__reserved HWND hwnd, __out_ecount(MAX_PATH) LPWSTR pszPath, __in int csidl, __in BOOL fCreate);

HINSTANCE hInst;

bool currentIsMain = true;
bool currentShouldStartAfterInstall = false;

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,  LPWSTR lpCmdLine,  int nCmdShow)
{
	hInst = hInstance;

	Loader_GenAllPath(0);

	LPWSTR *szArgList;
	int argCount;

	szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);
	if (szArgList == NULL)
	{
		MessageBox(NULL, L"Unable to parse command line", L"Error", MB_OK);
		return -1;
	}

	//Run command args
	if (argCount > 1)
	{
		if (wcscmp(szArgList[1], L"-uninstall") == 0)
		{
			Loader_ExtractUnInstallBat(false);
			return Loader_UnInstall();
		}
		else if (wcscmp(szArgList[1], L"-uninstall-temp") == 0)
		{
			Loader_ExtractUnInstallBat(true);
			return Loader_UnInstall();
		}
		else if (wcscmp(szArgList[1], L"-can-shut") == 0)
		{
			HWND receiveWindow = FindWindow(NULL, L"JY Killer");
			if (receiveWindow) {
				LPCWSTR buff = L"pub:canshut";
				COPYDATASTRUCT copyData = { 0 };
				copyData.lpData = (PVOID)buff;
				copyData.cbData = sizeof(WCHAR) * (wcslen(buff) + 1);
				SendMessageTimeout(receiveWindow, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&copyData, SMTO_NORMAL, 500, 0);
			}
		}
	}

	LocalFree(szArgList);

	if (Loader_CheckIsUpdater())
		return Loader_RunUpdater();

	if (Loader_IsDesktop(currentDir) || Loader_IsUsbDrv(currentDir))
	{
		WCHAR sysTempPath[MAX_PATH + 1];
		GetTempPath(MAX_PATH, sysTempPath);
		wcscpy_s(installDir, sysTempPath);
		wcscat_s(installDir, L"JiYuKiller\\");
		if (!PathFileExists(installDir) && !CreateDirectory(installDir, NULL)) {
			wcscpy_s(installDir, currentDir);
			currentIsMain = true;
			MessageBox(NULL, L"无法创建临时目录，请尝试使用管理员身份运行本程序", L"错误", MB_ICONERROR);
		}
		else {
			currentIsMain = false;
			currentShouldStartAfterInstall = true;
		}
	}
	else wcscpy_s(installDir, currentDir);

	//WCHAR sr[300];
	//swprintf_s(sr, L"%s currentIsMain : %s", installDir, currentIsMain ? L"true" : L"false");
	//MessageBox(NULL, sr, L"installDir", 0);

	Loader_GenAllPath(1);

	if (!Loader_CheckAndInstall(installDir)) 
		return -1;

	return Loader_RunMain();
}

void Loader_GenAllPath(int o) 
{
	if (o == 0) 
	{
		GetModuleFileName(0, currentIniPath, MAX_PATH);
		PathRenameExtension(currentIniPath, L".ini");
		GetModuleFileName(0, currentFullPath, MAX_PATH);
		GetModuleFileName(0, currentDir, MAX_PATH);
		PathRemoveFileSpec(currentDir);

		wcscpy_s(partUpdatePath, currentDir);
		wcscat_s(partUpdatePath, L"\\JiYuKiller.New.exe");
	}
	else if (o == 1) 
	{
		wcscpy_s(partMainPath, installDir);
		wcscpy_s(partHtmlayoutPath, installDir);
		wcscpy_s(partVirusPath, installDir);
		wcscpy_s(partDriverPath, installDir);
		wcscpy_s(partMainExePath, installDir);
		wcscpy_s(partIniPath, installDir);

		wcscat_s(partIniPath, L"\\JiYuKiller.ini");
		wcscat_s(partMainExePath, L"\\JiYuKiller.exe");
		wcscat_s(partMainPath, L"\\JiYuKiller.dll");
		wcscat_s(partHtmlayoutPath, L"\\htmlayout.dll");
		wcscat_s(partVirusPath, L"\\JiYuKillerVirus.dll");
		wcscat_s(partDriverPath, L"\\JiYuKillerDriver.sys");

	}
}
bool Loader_IsUsbDrv(const wchar_t *path)
{
	//
	//path: "\\\\?\\F:"
#define IOCTL_STORAGE_QUERY_PROPERTY   CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)
	typedef  struct _STORAGE_DEVICE_DESCRIPTOR
	{
		DWORD Version;                DWORD Size;
		BYTE  DeviceType;             BYTE  DeviceTypeModifier;
		BOOLEAN RemovableMedia;       BOOLEAN CommandQueueing;
		DWORD VendorIdOffset;         DWORD ProductIdOffset;
		DWORD ProductRevisionOffset;  DWORD SerialNumberOffset;
		STORAGE_BUS_TYPE BusType;     DWORD RawPropertiesLength;
		BYTE  RawDeviceProperties[1];
	} STORAGE_DEVICE_DESCRIPTOR;

	HANDLE hDisk;
	STORAGE_DEVICE_DESCRIPTOR devDesc;
	DWORD query[3] = { 0,0,1588180 };

	DWORD cbBytesReturned;

	TCHAR szBuf[300];
	wsprintf(szBuf, L"\\\\?\\%C:", path[0]);
	hDisk = CreateFile(szBuf, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hDisk == INVALID_HANDLE_VALUE)
		return false;

	if (DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
		&devDesc, sizeof(devDesc), &cbBytesReturned, NULL))
	{
		if (devDesc.BusType == BusTypeUsb)
		{
			CloseHandle(hDisk);
			return true;
		}
	}
	return false;
}
bool Loader_IsDesktop(const wchar_t *path)
{
	wchar_t desktopPath[MAX_PATH];
	if (!SHGetSpecialFolderPathW(0, desktopPath, 0x0010, 0))
		return FALSE;
	return wcscmp(desktopPath, path) == 0;
}
bool Loader_CheckAndInstall(const wchar_t *path)
{
	if (!PathFileExists(partMainPath) && !Loader_ExtractFile(IDR_MAIN, partMainPath)) return false;
	if (!PathFileExists(partHtmlayoutPath) && !Loader_ExtractFile(IDR_HTMLAYOUT, partHtmlayoutPath)) return false;
	if (!PathFileExists(partVirusPath) && !Loader_ExtractFile(IDR_VIRUS, partVirusPath)) return false;
	if (!PathFileExists(partDriverPath) && !Loader_ExtractFile(IDR_DRIVER, partDriverPath)) return false;
	if (!PathFileExists(partIniPath) && PathFileExists(currentIniPath))
		CopyFile(currentIniPath, partIniPath, FALSE);

	if (currentShouldStartAfterInstall) 
		if (!PathFileExists(partMainExePath) && !CopyFile(currentFullPath, partMainExePath, FALSE)) return false;

	return true;
}
bool Loader_ExtractFile(int res_id, const wchar_t *to_path)
{
	WCHAR lastError[56];

	HANDLE hFile = CreateFile(to_path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		swprintf_s(lastError, L"安装模块 %s  时发生错误 %d", to_path, GetLastError());
		return false;
	}

	bool success = false;
	HRSRC hResource = FindResource(hInst, MAKEINTRESOURCE(res_id), TEXT("BIN"));
	if (hResource) {
		HGLOBAL hg = LoadResource(hInst, hResource);
		if (hg) {
			LPVOID pData = LockResource(hg);
			if (pData)
			{
				DWORD dwSize = SizeofResource(hInst, hResource);
				DWORD writed;
				if (WriteFile(hFile, pData, dwSize, &writed, NULL))
					success = true;
				else swprintf_s(lastError, L"写入模块 %s  时发生错误 %d", to_path, GetLastError());

				CloseHandle(hFile);
				return success;
			}
			else swprintf_s(lastError, L"提取模块资源 %s  时发生错误(LockResource) %d", to_path, GetLastError());
		}
		else swprintf_s(lastError, L"提取模块资源 %s  时发生错误(LoadResource) %d", to_path, GetLastError());
	}
	else swprintf_s(lastError, L"提取模块资源 %s  时发生错误(FindResource) %d", to_path, GetLastError());
	CloseHandle(hFile);
	MessageBox(0, lastError, L"程序初始化失败", MB_ICONERROR);
	return false;
}
bool Loader_CheckIsUpdater() {
	return (wcscmp(L"", partUpdatePath) != 0 && wcscmp(currentFullPath, partUpdatePath) == 0);
}
bool Loader_RemoveOld()
{
	if (PathFileExists(partMainExePath))
		DeleteFile(partMainExePath);
	if (PathFileExists(partMainPath))
		DeleteFile(partMainPath);
	if (PathFileExists(partHtmlayoutPath))
		DeleteFile(partHtmlayoutPath);
	if (PathFileExists(partVirusPath))
		DeleteFile(partVirusPath);
	if (PathFileExists(partDriverPath))
		DeleteFile(partDriverPath);

	return true;
}
bool Loader_ExtractUnInstallBat(bool temp) {
	wcscpy_s(partBatPath, currentDir);
	wcscat_s(partBatPath, L"\\JiYuKiller.UnInstall.bat");

	FILE *fp = NULL;
	_wfopen_s(&fp, partBatPath, L"w");
	if (fp) {
		if (temp) {
			WCHAR sysTempPath[MAX_PATH + 1];
			GetTempPath(MAX_PATH, sysTempPath);
			wcscat_s(sysTempPath, L"JiYuKiller\\");
			fwprintf_s(fp, L"cd %s\n", sysTempPath);
			fwprintf_s(fp, L"del /F /Q JiYuKiller.exe\n");
			fwprintf_s(fp, L"del /F /Q JiYuKiller.dll\n");
			fwprintf_s(fp, L"del /F /Q JiYuKillerDriver.sys\n");
			fwprintf_s(fp, L"del /F /Q JiYuKillerVirus.dll\n");
			fwprintf_s(fp, L"del /F /Q htmlayout.dll\n");
			fwprintf_s(fp, L"del %%0\n");
			fclose(fp);
		}
		else {
			fwprintf_s(fp, L"cd %s\n", currentDir);
			fwprintf_s(fp, L"del /F /Q JiYuKiller.exe\n");
			fwprintf_s(fp, L"del /F /Q JiYuKiller.dll\n");
			fwprintf_s(fp, L"del /F /Q JiYuKillerDriver.sys\n");
			fwprintf_s(fp, L"del /F /Q JiYuKillerVirus.dll\n");
			fwprintf_s(fp, L"del /F /Q htmlayout.dll\n");
			fwprintf_s(fp, L"del %%0\n");
			fclose(fp);
		}
		return true;
	}
	return false;
}

int Loader_RunUpdater()
{
	Loader_GenAllPath(1);

	wcscpy_s(installDir, currentDir);

	Loader_RemoveOld();
	Loader_CheckAndInstall(installDir);
	
	if(CopyFile(currentFullPath, partMainExePath, FALSE))
		ShellExecute(0, L"runas", partMainExePath, L"-rm-updater", NULL, SW_SHOW);

	return 0;
}
int Loader_RunMain() 
{
	if (currentIsMain)
	{
		HMODULE hMain = LoadLibrary(partMainPath);
		if (!hMain) {
			MessageBox(0, L"没有加载主模块", L"程序初始化失败", MB_ICONERROR);
			return -1;
		}

		typedef int(*fnJRM)();

		fnJRM jrm = (fnJRM)GetProcAddress(hMain, "JRunMain");
		if (!jrm) {
			MessageBox(0, L"加载主模块已损坏", L"程序初始化失败", MB_ICONERROR);
			return -1;
		}
		return jrm();
	}
	else if (currentShouldStartAfterInstall) {
		WCHAR arg[MAX_PATH];
		swprintf_s(arg, L"-from %s", currentFullPath);
		ShellExecute(0, L"runas", partMainExePath, arg, NULL, SW_SHOW);
	}
	return 0;
}
int Loader_UnInstall() {
	ShellExecute(0, L"runas", partBatPath, 0, 0, SW_HIDE);
	TerminateProcess(GetCurrentProcess(), 0);
	return 0;
}