#pragma once
#include "stdafx.h"

bool XCheckForUpdate();
bool XRunUpdate();
bool XRunUpdateWindow();
bool XCheckIsInterenet();

DWORD WINAPI XUpdateDownloadThread(LPVOID lpThreadParameter);

LRESULT CALLBACK UpdateWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int UUpdateProgressFunc(void * ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUploaded);
