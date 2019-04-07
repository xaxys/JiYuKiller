#pragma once
#include "stdafx.h"
#include "NtHlp.h"

bool KFShutdown();
bool KFReboot();
bool KForceKill(DWORD pid, NTSTATUS * pStatus);

bool KFSendDriverinitParam(bool isXp, bool isWin7);
bool KFInstallSelfProtect();

bool KFInjectDll(DWORD pid, LPWSTR dllPath);
