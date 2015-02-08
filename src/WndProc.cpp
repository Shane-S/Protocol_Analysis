/*----------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: WndProc.cpp
--
-- PROGRAM: Assn2
--
-- FUNCTIONS:
-- INT_PTR CALLBACK HostIPDlgProc(_In_ HWND hwndDlg, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
-- VOID HostIPSetSelect(const DWORD dwSelected, const HWND hwndDlg)
--
-- DATE: January 14th, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- NOTES:	This file contains functions to manage a dialog box with which the user can determine the IP address for a
--			a given host name, or the host name for a given port IP address.
-------------------------------------------------------------------------------------------------------------------------*/

#include "WndProc.h"

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: MessageBoxPrintf
-- Febrary 7th, 2014
--
-- DESIGNER: Charles Petzold
--
-- PROGRAMMER: Charles Petzold
--
-- INTERFACE: LRESULT CALLBACK WndProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
--										HWND hwnd:		Handle the to the window for which the messsages are destined.
--										UINT uMsg:		The message code.
--										WPARAM wParam:	Contains a message-specific value.
--										LPARAM lParam:	Contains a message-specific value.
--
-- RETURNS: 0 if the the message was handled here, or some other int if it was handled by DefWndProc.
--
-- NOTES:
-- Handles manipulation of the menu items and the user quitting. All other functions are handled by DefWndProc.
---------------------------------------------------------------------------------------------------------------------------*/
LRESULT CALLBACK WndProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_FILE_EXIT:
			PostQuitMessage(0);
			return 0;
		case ID_TRANSFER_BEGINTRANSFER:
		{
			DWORD dwHostMode = (DWORD)GetWindowLongPtr(hwnd, GWLP_HOSTMODE);
			LPTransferProps props = (LPTransferProps)GetWindowLongPtr(hwnd, GWLP_TRANSFERPROPS);

			if (dwHostMode == ID_HOSTTYPE_CLIENT)
			{
				if (!ClientInitSocket(props))
					break;
				CreateThread(NULL, 0, ClientSendData, (VOID *)hwnd, 0, NULL);
			}
			else
			{
				if (!ServerInitSocket(props))
					break;
				CreateThread(NULL, 0, Serve, (VOID *)hwnd, 0, NULL);
			}
			break;
		}
		case ID_TRANSFER_PROPERTIES:
			DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), MAKEINTRESOURCE(IDD_DIALOG1), hwnd, TransferDlgProc);
			break;

		case ID_HOSTTYPE_CLIENT:
		case ID_HOSTTYPE_SERVER:
		{
			HMENU hMenu = GetMenu(hwnd);
			DWORD dwCurChecked = GetWindowLongPtr(hwnd, GWLP_HOSTMODE);
			if (dwCurChecked == LOWORD(wParam))
				break;
			CheckMenuItem(hMenu, LOWORD(wParam), MF_CHECKED);
			CheckMenuItem(hMenu, dwCurChecked, MF_UNCHECKED);
			SetWindowLongPtr(hwnd, GWLP_HOSTMODE, (LONG)LOWORD(wParam));
			break;
		}
		}
		return 0;
	case WM_DESTROY:
		free((void *)GetWindowLongPtr(hwnd, GWLP_TRANSFERPROPS));
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
