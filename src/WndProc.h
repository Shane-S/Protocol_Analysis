#ifndef WND_PROC_H
#define WND_PROC_H

#include "TransferDlgProc.h"
#include <Windows.h>
#include "resource.h"
#include "ClientTransfer.h"
#include "ServerTransfer.h"

LRESULT CALLBACK WndProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);
#endif