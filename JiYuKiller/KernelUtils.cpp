#include "stdafx.h"
#include "DriverLoader.h"
#include "KernelUtils.h"
#include "NtHlp.h"
#include "JiYuKiller.h"
#include <winioctl.h>
#include "../JiYuKillerDriver/IoCtl.h"
#include "../JiYuKillerDriver/IoStructs.h"

extern HANDLE hKDrv;

bool KFShutdown()
{
	if (DriverLoaded())
	{
		DWORD ReturnLength = 0;
		BOOL rs = DeviceIoControl(hKDrv, CTL_SHUTDOWN, NULL, 0, NULL, 0, &ReturnLength, NULL);
		XOutPutStatus(L"DeviceIoControl CTL_SHUTDOWN %s", rs ? L"TRUE" : L"FALSE");
		if (!rs) XOutPutStatus(L"DeviceIoControl CTL_SHUTDOWN %d", GetLastError());

		return rs;
	}
	else XOutPutStatus(L"Çý¶¯Î´¼ÓÔØ£¡");
	return false;
}
bool KFReboot()
{
	if (DriverLoaded())
	{
		DWORD ReturnLength = 0;
		BOOL rs = DeviceIoControl(hKDrv, CTL_REBOOT, NULL, 0, NULL, 0, &ReturnLength, NULL);
		XOutPutStatus(L"DeviceIoControl CTL_REBOOT %s", rs ? L"TRUE" : L"FALSE");
		if(!rs) XOutPutStatus(L"DeviceIoControl CTL_REBOOT %d",GetLastError());

		return rs;
	}
	else XOutPutStatus(L"Çý¶¯Î´¼ÓÔØ£¡");
	return false;
}
bool KForceKill(DWORD pid, NTSTATUS *pStatus)
{
	if (DriverLoaded())
	{
		NTSTATUS status = STATUS_UNSUCCESSFUL;
		DWORD ReturnLength = 0;
		ULONG_PTR pidb = pid;
		if (DeviceIoControl(hKDrv, CTL_KILL_PROCESS, &pidb, sizeof(ULONG_PTR), &status, sizeof(status), &ReturnLength, NULL))
		{
			if (status == STATUS_SUCCESS)
				return TRUE;
			else XOutPutStatus(L"CTL_KILL_PROCESS ´íÎó£º0x08%X", status);
		}
		else XOutPutStatus(L"DeviceIoControl CTL_KILL_PROCESS ´íÎó£º%d", GetLastError());
		if (pStatus)*pStatus = status;
	}
	else XOutPutStatus(L"Çý¶¯Î´¼ÓÔØ£¡");
	return false;
}
bool KFSendDriverinitParam(bool isXp, bool isWin7) {
	if (DriverLoaded())
	{
		DWORD ReturnLength = 0;

		JDRV_INITPARAM pidb = { 0 };
		pidb.IsWin7 = isWin7;
		pidb.IsWinXP = isXp;
		if (DeviceIoControl(hKDrv, CTL_INITPARAM, &pidb, sizeof(pidb), NULL, 0, &ReturnLength, NULL))
			return TRUE;
		else XOutPutStatus(L"DeviceIoControl CTL_INITPARAM ´íÎó£º%d", GetLastError());
	}
	return false;
}
bool KFInstallSelfProtect() 
{
	if (DriverLoaded())
	{
		DWORD ReturnLength = 0;
		ULONG_PTR pidb = GetCurrentProcessId();
		if (DeviceIoControl(hKDrv, CTL_INITSELFPROTECT, &pidb, sizeof(pidb), NULL, NULL, &ReturnLength, NULL))
				return TRUE;
		else XOutPutStatus(L"DeviceIoControl CTL_INITSELFPROTECT ´íÎó£º%d", GetLastError());
	}
	return false;
}
bool KFInjectDll(DWORD pid, LPWSTR dllPath) {
	if (DriverLoaded())
	{

	}
	return false;
}

