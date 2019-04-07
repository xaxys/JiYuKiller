#include "stdafx.h"
#include "Utils.h"
#include <CommDlg.h>

BOOL _Is64BitOS = -1;
BOOL _IsRunasAdmin = -1;

#define PROCESSOR_ARCHITECTURE_ARM64 12

BOOL EnableDebugPriv(const wchar_t * name)
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	LUID luid;
	//�򿪽������ƻ�
	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
	//��ý��̱���ΨһID
	LookupPrivilegeValue(NULL, name, &luid);

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	tp.Privileges[0].Luid = luid;
	//����Ȩ��
	return AdjustTokenPrivileges(hToken, 0, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
}
BOOL MIsRunasAdmin()
{
	if (_IsRunasAdmin = -1)
	{
		BOOL bElevated = FALSE;
		HANDLE hToken = NULL;

		// Get current process token
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
			return FALSE;

		TOKEN_ELEVATION tokenEle;
		DWORD dwRetLen = 0;

		// Retrieve token elevation information
		if (GetTokenInformation(hToken, TokenElevation, &tokenEle, sizeof(tokenEle), &dwRetLen))
		{
			if (dwRetLen == sizeof(tokenEle))
			{
				bElevated = tokenEle.TokenIsElevated;
			}
		}

		CloseHandle(hToken);
		_IsRunasAdmin = bElevated;
	}
	return _IsRunasAdmin;
}
BOOL MIs64BitOS()
{
	if (_Is64BitOS == -1)
	{
		BOOL bRetVal = FALSE;
		SYSTEM_INFO si = { 0 };

		GetNativeSystemInfo(&si);
		if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
			si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 ||
			si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64)
			bRetVal = TRUE;
		_Is64BitOS = bRetVal;
	}
	return _Is64BitOS;
}
BOOL MChooseFileSingal(HWND hWnd, LPCWSTR startDir, LPCWSTR title, LPCWSTR fileFilter, LPCWSTR fileName, LPCWSTR defExt, LPCWSTR strrs, size_t bufsize)
{
	if (strrs) {
		OPENFILENAME ofn;
		TCHAR szFile[MAX_PATH];
		if (fileName != 0 && wcslen(fileName) != 0)
			wcscpy_s(szFile, fileName);
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = hWnd;
		ofn.lpstrFile = szFile;
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(szFile);
		ofn.nFilterIndex = 1;
		ofn.lpstrFilter = fileFilter;
		ofn.lpstrDefExt = defExt;
		ofn.lpstrTitle = (LPWSTR)title;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = startDir;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
		if (GetOpenFileName(&ofn))
		{
			//��ʾѡ����ļ��� szFile
			wcscpy_s((LPWSTR)strrs, bufsize, szFile);
			return TRUE;
		}
	}
	return 0;
}

INT XDetermineSystemVersion() {
	SYSTEM_INFO info;        //��SYSTEM_INFO�ṹ�ж�64λAMD������ 
	GetSystemInfo(&info);    //����GetSystemInfo�������ṹ 
	OSVERSIONINFOEX os;
	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if (GetVersionEx((OSVERSIONINFO *)&os))
	{
		//������ݰ汾��Ϣ�жϲ���ϵͳ���� 
		switch (os.dwMajorVersion)//�ж����汾��
		{
		case 5:
			switch (os.dwMinorVersion)	//�ٱȽ�dwMinorVersion��ֵ
			{
			case 0: return 0;
			case 1:
			case 2: return 1;
			}
			break;

		default:
			if (os.dwMajorVersion >= 6)
				return 2;
			break;
		}
	}
	return 0;
}
