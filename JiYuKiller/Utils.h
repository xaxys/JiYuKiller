#pragma once
#include "stdafx.h"

BOOL MIsRunasAdmin();
BOOL MIs64BitOS();
BOOL MChooseFileSingal(HWND hWnd, LPCWSTR startDir, LPCWSTR title, LPCWSTR fileFilter, LPCWSTR fileName, LPCWSTR defExt, LPCWSTR strrs, size_t bufsize);
INT XDetermineSystemVersion();
BOOL EnableDebugPriv(const wchar_t * name);