#include "stdafx.h"
#include "Tracker.h"
#include "WindowResolver.h"
#include "MainWindow.h"
#include "MsgCenter.h"
#include "NtHlp.h"
#include "Utils.h"
#include "DriverLoader.h"
#include "KernelUtils.h"
#include <Psapi.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <shlwapi.h>
#include "StringHlp.h"

DWORD jiyuPid = 0;
WCHAR jiyuPath[MAX_PATH] = { 0 };

extern HWND hWndMain;
extern WCHAR virusPath[MAX_PATH];
extern bool controlled;

WCHAR failStatText[36] = { 0 };
WCHAR ctlStatText[512] = { 0 };
WCHAR procStatText[36] = { 0 };

bool setAutoForceKill = false;

int msgCenterRetryCount = 0;

LPWSTR TGetLastError() { return failStatText; }
DWORD TGetLastJiYuPid() { return jiyuPid; }
LPWSTR TGetLastJiYuPayh() { return jiyuPath; }
bool TFindTarget(LPDWORD outPid, bool *isNewState)
{
	PROCESSENTRY32 pe;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe)) return 0;
	while (1)
	{
		pe.dwSize = sizeof(PROCESSENTRY32);
		if (Process32Next(hSnapshot, &pe) == FALSE) break;
		if (StrEqual(pe.szExeFile, L"StudentMain.exe"))
		{
			// OpenProcess(PROCESS_ALL_ACCESS | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, pe.th32ProcessID);
			if (jiyuPid != pe.th32ProcessID) {
				jiyuPid = pe.th32ProcessID;
				if (isNewState) *isNewState = true;
			}
			if (outPid)*outPid = pe.th32ProcessID;
			return true;
		}
	}
	if (jiyuPid != 0)
	{
		if (isNewState) *isNewState = true;
		jiyuPid = 0;
	}
	CloseHandle(hSnapshot);
	return false;
}

DWORD TGetLastJyPid() {
	return jiyuPid;
}
bool TLocated() {
	return jiyuPid != 0;
}
LPWSTR TGetCurrStatText() {
	return ctlStatText;
}
LPWSTR TGetCurrProcStatText() {
	return procStatText;
}
bool TReset()
{
	msgCenterRetryCount = 0;
	bool newState = false;
	TFindTarget(NULL, &newState);
	if (newState)
	{
		if (jiyuPid != 0) 
		{
			swprintf_s(procStatText, L"���������������� %d (0x%04x)", jiyuPid, jiyuPid);
			XOutPutStatus(procStatText);
			if(TFindJiYuInstallLocation(true))
				XOutPutStatus(L"�����������ִ���ļ�·����%s", jiyuPath);
			TInstallVirus();
		}
		else {
			wcscpy_s(procStatText, L"δ�ҵ��������");
			XOutPutStatus(procStatText);
		}
	}
	return newState;
}
int TRunCK()
{
	return WRunCk();
}
void TSendCtlStat() 
{
	MsgCenterSendToVirus((LPWSTR)L"hk:ckstat", hWndMain);
}
void TSendCkEnd()
{
	MsgCenterSendToVirus((LPWSTR)L"hk:ckend", hWndMain);
}
void TSendBoom()
{
	MsgCenterSendToVirus((LPWSTR)L"ss:0", hWndMain);
}
void TSendQuit()
{
	MsgCenterSendToVirus((LPWSTR)L"ss2:0", hWndMain);
}

void TFindJiYuPath()
{
	if (wcscmp(jiyuPath, L"") == 0) {
		if (TFindJiYuInstallLocation(false))
			XOutPutStatus(L"�����������ִ���ļ�·����%s", jiyuPath);
	}
}
bool TFindJiYuInstallLocation(bool usePid)
{
	if (usePid && jiyuPid > 4) {
		HANDLE hProcess;
		if (NT_SUCCESS(MOpenProcessNt(jiyuPid, &hProcess)))
			if (MGetProcessFullPathEx(hProcess, jiyuPath))
				return true;
	}
	else {
		const wchar_t* path = L"C:\\Program Files\\Mythware\\e-Learning Class\\StudentMain.exe";
		if (PathFileExists(path)) {
			wcscpy_s(jiyuPath, path);
			return TRUE;
		}
		path = L"C:\\Program Files\\Mythware\\������ù���ϵͳ���V6.0 2016 ������\\StudentMain.exe";
		if (PathFileExists(path)) {
			wcscpy_s(jiyuPath, path);
			return TRUE;
		}
	}
	return false;
}

bool TInstallVirus()
{
	if (jiyuPid != 0)
	{
		if (TInstallVirusWithRemoteThread() == TRUE) {
			XOutPutStatus(L"����ע��Զ���̳߳ɹ�");
			return true;
		}
		if (MessageBox(hWndMain, L"�޷���Զ���߳�ģʽ����ע�벡�����Ƿ���ʹ���滻DLLģʽ����ע�룿", L"����", MB_ICONEXCLAMATION | MB_YESNO) == IDYES)
		{
			if (TInstallVirusWithDllApi() == TRUE) {
				XOutPutStatus(L"�����滻�����ɹ�");
				return true;
			}
		}
	}
	return false;
}
int TInstallVirusWithRemoteThread()
{
	HANDLE hRemoteProcess;
	//�򿪽���
	NTSTATUS ntStatus = MOpenProcessNt(jiyuPid, &hRemoteProcess);
	if (!NT_SUCCESS(ntStatus))
	{
		XOutPutStatus(L"�޷�ע�벡�� �򿪽��̴��� : 0x%08X", ntStatus);
		if (DriverLoaded() && ntStatus == STATUS_ACCESS_DENIED)
		{
			int rs = MessageBox(hWndMain, L"�޷�ע�벡�� �򿪽��̴���\n�Ƿ���������ǿ��ע�룿", L"����", MB_YESNO | MB_ICONEXCLAMATION);
			if (rs == IDNO) return -1;
			if (rs == IDYES) {
				XOutPutStatus(L"�޷�����ע�벡�� ��������ǿ��ע��");
				return KFInjectDll(jiyuPid, virusPath);
			}
		}
		else 
		{
			if (setAutoForceKill) {
				XOutPutStatus(L"�޷�ע�벡�� ����ǿ�ƽ���");
				return TForceKill();
			}
			int rs = MessageBox(hWndMain, L"�޷�ע�벡�� �򿪽��̴���\n�Ƿ�����ǿ�ƽ�����", L"����", MB_YESNO | MB_ICONEXCLAMATION);
			if (rs == IDNO) return -1;
			if (rs == IDYES) {
				XOutPutStatus(L"�޷�ע�벡�� ����ǿ�ƽ���");
				return TForceKill();
			}
		}
		return FALSE;
	}

	wchar_t *pszLibFileRemote;

	//ʹ��VirtualAllocEx������Զ�̽��̵��ڴ��ַ�ռ����DLL�ļ����ռ�
	pszLibFileRemote = (wchar_t *)VirtualAllocEx(hRemoteProcess, NULL, sizeof(wchar_t) * (lstrlen(virusPath) + 1), MEM_COMMIT, PAGE_READWRITE);

	//ʹ��WriteProcessMemory������DLL��·����д�뵽Զ�̽��̵��ڴ�ռ�
	WriteProcessMemory(hRemoteProcess, pszLibFileRemote, (void *)virusPath, sizeof(wchar_t) * (lstrlen(virusPath) + 1), NULL);

	//##############################################################################
		//����LoadLibraryA����ڵ�ַ
	PTHREAD_START_ROUTINE pfnStartAddr = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "LoadLibraryW");
	//(����GetModuleHandle������GetProcAddress����)

	//����Զ���߳�LoadLibraryW��ͨ��Զ���̵߳��ô����µ��߳�
	HANDLE hRemoteThread;
	if ((hRemoteThread = CreateRemoteThread(hRemoteProcess, NULL, 0, pfnStartAddr, pszLibFileRemote, 0, NULL)) == NULL)
	{
		XOutPutStatus(L"ע���߳�ʧ��! ����%d", GetLastError());
		if (setAutoForceKill) {
			XOutPutStatus(L"�޷�ע�벡�� ����ǿ�ƽ���");
			return TForceKill();
		}
		int rs = MessageBox(hWndMain, L"�޷�ע�벡�� ע���߳�ʧ�ܡ�\n�Ƿ�����ǿ�ƽ�����", L"����", MB_YESNO | MB_ICONEXCLAMATION);
		if (rs == IDNO) return -1;
		if (rs == IDYES) {
			XOutPutStatus(L"�޷�ע�벡�� ����ǿ�ƽ���");
			return TForceKill();
		}
		return FALSE;
	}

	// �ͷž��

	CloseHandle(hRemoteProcess);
	CloseHandle(hRemoteThread);

	return TRUE;
}
bool TInstallVirusWithDllApi()
{
	if (!PathFileExists(virusPath))
	{
		XOutPutStatus(L"����Դ�ļ�δ�ҵ�����ִ���滻");
		MessageBox(hWndMain, L"����Դ�ļ�δ�ҵ����޷�ִ�в����滻", L"����", MB_ICONEXCLAMATION);
		return false;
	}

	//Get base dir
	if (StrEmepty(jiyuPath))
	{
	RECHOOSE:
		if (!MChooseFileSingal(hWndMain, L"", L"ѡ�� StudentMain.exe ��λ�ã�", L"�����ļ�\0*.*\0Exe Flie\0*.exe\0\0", L"StudentMain.exe", L".exe", jiyuPath, MAX_PATH))
			return false;
		if (_waccess_s(jiyuPath, 0) != 0) {
			if (MessageBox(hWndMain, L"��ѡ���·�������ڻ��޷����ʣ������������ѡ��", L"����", MB_RETRYCANCEL) == IDRETRY)
				goto RECHOOSE;
		}
	}

	//Replace stub
	WCHAR jiyuDirPath[MAX_PATH];
	wcscpy_s(jiyuDirPath, jiyuPath);
	PathRemoveFileSpec(jiyuDirPath);

	WCHAR jiyuTDAPath[MAX_PATH];
	WCHAR jiyuTDAOPath[MAX_PATH];
	wcscpy_s(jiyuTDAPath, jiyuDirPath);
	wcscat_s(jiyuTDAPath, L"\\LibTDAjust.dll");
	wcscpy_s(jiyuTDAOPath, jiyuDirPath);
	wcscat_s(jiyuTDAOPath, L"\\LibTDAjust.dll.bak.dll");

	if (!PathFileExists(jiyuTDAPath)) {
		XOutPutStatus(L"Ŀ���ļ�δ�ҵ���%s���޷�ִ���滻", jiyuTDAPath);
		MessageBox(hWndMain, L"Ŀ���ļ�δ�ҵ����޷�ִ�в����滻", L"����", MB_ICONEXCLAMATION);
		return false;
	}

	//Copy backup file
	if (!CopyFile(jiyuTDAPath, jiyuTDAOPath, FALSE)) {
		XOutPutStatus(L"���Ʊ����ļ�ʧ�ܣ�%s���޷�ִ���滻", jiyuTDAOPath);
		MessageBox(hWndMain, L"���Ʊ����ļ�ʧ�ܣ��޷�ִ�в����滻", L"����", MB_ICONEXCLAMATION);
		return false;
	}

	//Delete old
	if (!DeleteFile(jiyuTDAPath))
	{
		XOutPutStatus(L"ɾ��ԴĿ��ʧ�ܣ�%s���޷�ִ���滻", jiyuTDAPath);
		MessageBox(hWndMain, L"ɾ��ԴĿ��ʧ�ܣ��޷�ִ�в����滻", L"����", MB_ICONEXCLAMATION);
		return false;
	}

	//Copy to new
	if (!CopyFile(virusPath, jiyuTDAPath, FALSE)) {
		XOutPutStatus(L"���Ʋ�����Ŀ���ļ�ʧ�ܣ�%s���޷�ִ���滻", jiyuTDAOPath);
		MessageBox(hWndMain, L"���Ʋ�����Ŀ���ļ�ʧ�ܣ��޷�ִ�в����滻", L"����", MB_ICONEXCLAMATION);
		return false;
	}
	return true;
}

bool TForceKill() 
{
	if (jiyuPid == 0) {
		wcscpy_s(failStatText, L"δ�ҵ�����������");
		XOutPutStatus(failStatText);
		return false;
	}
	if (controlled && MessageBox(hWndMain, L"���Ƿ�ϣ��ʹ�ò������б��ƣ�", L"��ʾ", MB_ICONASTERISK | MB_YESNO) == IDYES) {
		TSendQuit();
		return true;
	}

	HANDLE hProcess;
	NTSTATUS status = MOpenProcessNt(jiyuPid, &hProcess);
	if (!NT_SUCCESS(status)) {
		swprintf_s(failStatText, L"�򿪽��̴���0x%08X�����ֶ�����", status);
		XOutPutStatus(failStatText);
		return FALSE;
	}
	status = MTerminateProcessNt(0, hProcess);
	if (NT_SUCCESS(status)) {
		CloseHandle(hProcess);
		return TRUE;
	}
	else {
		if (status == STATUS_ACCESS_DENIED) 
			goto FORCEKILL;
		else if(status != STATUS_INVALID_CID && status != STATUS_INVALID_HANDLE) {
			swprintf_s(failStatText, L"�������̴���0x%08X�����ֶ�����", status);
			XOutPutStatus(failStatText);
			return false;
		}
	}

FORCEKILL:
	if (DriverLoaded() && MessageBox(hWndMain, L"��ͨ�޷����������Ƿ����������������\n���������ܲ��ȶ��������á���Ҳ����ʹ�� PCHunter �Ȱ�ȫ�������ǿɱ��", L"��ʾ", MB_ICONEXCLAMATION | MB_YESNO) == IDYES)
	{
		if (KForceKill(jiyuPid, &status))
			return true;
		else MessageBox(hWndMain, L"����Ҳ�޷���������ʹ�� PCHunter �������ɣ�", L"����", MB_ICONEXCLAMATION);
	}
	CloseHandle(hProcess);
	return false;
}