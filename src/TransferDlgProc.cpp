/*----------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: TransferDlgProc.cpp
--
-- PROGRAM: Assn2
--
-- FUNCTIONS:
-- INT_PTR CALLBACK TransferDlgProc(_In_ HWND hwndDlg, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);
-- VOID SetDlgDefaults(HWND hwndDlg, DWORD dwHostMode, LPTransferProps props);
-- BOOL FillTransferProps(HWND hwndDlg, DWORD dwHostMode, LPTransferProps props);
-- BOOL GetDlgAddrInfo(HWND hwndDlg, DWORD dwHostMode, LPTransferProps props);
-- VOID OpenFileDlg(HWND hwndDlg, DWORD dwHostMode);
--
-- DATE: January 30th, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- NOTES:	This file contains functions to manage a dialog box which the user can use to set transfer properties.
-------------------------------------------------------------------------------------------------------------------------*/
#include "TransferDlgProc.h"

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: TransferDlgProc
-- 
-- DATE: January 30th, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: INT_PTR CALLBACK TransferDlgProc(_In_ HWND hwndDlg, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
--				HWND hwndDlg:	Handle to the dialog window.
--				UINT uMsg:		The message being passed to this dialog box.
--				WPARAM wParam:	A general-purpose parameter that may contain information relevant to the message.
--				LPARAM lParam:	A general-purpose parameter that may contain information relevant to the message.
--
-- RETURNS: The button selected by the user; IDOK if they click "Lookup", and IDCANCEL if they click "Cancel".
--
-- NOTES:
-- Handles messages related to its controls (users clicking on radio buttons, Resolve, Cancel, etc.) and looks up the
-- fills the transfer properties when the user clicks okay. If an error prevents the properties from being filled, the dialog.
-- is kept open.
---------------------------------------------------------------------------------------------------------------------------*/
INT_PTR CALLBACK TransferDlgProc(_In_ HWND hwndDlg, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	static DWORD dwHostMode;
	static LPTransferProps props = NULL;
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		dwHostMode = (DWORD)GetWindowLongPtr(GetParent(hwndDlg), GWLP_HOSTMODE);
		props = (LPTransferProps(GetWindowLongPtr(GetParent(hwndDlg), GWLP_TRANSFERPROPS)));
		SetDlgDefaults(hwndDlg, dwHostMode, props);
		break;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (FillTransferProps(hwndDlg, dwHostMode, props))
				EndDialog(hwndDlg, LOWORD(wParam));
			return TRUE;
		case IDCANCEL:
			EndDialog(hwndDlg, LOWORD(wParam));
			return TRUE;
		case ID_BUTTON_BROWSE:
			OpenFileDlg(hwndDlg, dwHostMode);
			return TRUE;
		}
	}
	return FALSE;
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: SetDlgDefaults
--
-- DATE: January 30th, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: VOID SetDlgDefaults(HWND hwndDlg, LPTransferProps props)
--									HWND hwndDlg:		Handle to the dialog window.
--									LPTransferProps:	Pointer to the transfer properties structure.
--
-- RETURNS: void
--
-- NOTES:
-- Initialises the dialog box's menu selections. It checks the TransferProps structure to save what the user entered.
---------------------------------------------------------------------------------------------------------------------------*/
VOID SetDlgDefaults(HWND hwndDlg, DWORD dwHostMode, LPTransferProps props)
{
	HWND	hwndSend	= GetDlgItem(hwndDlg, ID_DROPDOWN_SEND);
	HWND	hwndSize	= GetDlgItem(hwndDlg, ID_DROPDOWN_SIZE);
	HWND	hwndTCP		= GetDlgItem(hwndDlg, ID_RADIO_TCP);
	HWND	hwndUDP		= GetDlgItem(hwndDlg, ID_RADIO_UDP);
	HWND	hwndFile	= GetDlgItem(hwndDlg, ID_TEXTBOX_FILE);
	HWND	hwndPort	= GetDlgItem(hwndDlg, ID_TEXTBOX_PORT);
	HWND	hwndIP		= GetDlgItem(hwndDlg, ID_TEXTBOX_HOSTIP);
	TCHAR	buf[FILENAME_SIZE];

	// Set port field
	_stprintf_s(buf, TEXT("%d"), ntohs(props->paddr_in->sin_port));
	SetWindowText(hwndPort, buf);

	// Set radio button to either TCP or UDP
	SendMessage((props->nSockType == SOCK_STREAM) ? hwndTCP : hwndUDP, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

	// Set the file textbox text; doesn't matter if it's blank
	SetWindowText(hwndFile, props->szFileName);

	// We've now done everything for the server, so disable all other controls and return
	if (dwHostMode == ID_HOSTTYPE_SERVER)
	{
		ComboBox_Enable(hwndSend, FALSE);
		ComboBox_Enable(hwndSize, FALSE);
		Edit_Enable(hwndIP, FALSE);
		return;
	}

	// Set initial options for packet size dropdown
	SendMessage(hwndSize, CB_ADDSTRING, 0, (LPARAM)TEXT("Use file size"));
	SendMessage(hwndSize, CB_ADDSTRING, 0, (LPARAM)TEXT("1024"));
	SendMessage(hwndSize, CB_ADDSTRING, 0, (LPARAM)TEXT("4096"));
	SendMessage(hwndSize, CB_ADDSTRING, 0, (LPARAM)TEXT("20480"));
	SendMessage(hwndSize, CB_ADDSTRING, 0, (LPARAM)TEXT("61440"));
	if (props->szFileName[0] == 0)
	{
		_stprintf_s(buf, TEXT("%d"), props->nPacketSize);
		SetWindowText(hwndSize, buf);
	}
	else
		SendMessage(hwndSize, CB_SETCURSEL, 0, 0);

	// Set initial options for packet number dropdown
	SendMessage(hwndSend, CB_ADDSTRING, 0, (LPARAM)TEXT("10"));
	SendMessage(hwndSend, CB_ADDSTRING, 0, (LPARAM)TEXT("100"));
	SendMessage(hwndSend, CB_ADDSTRING, 0, (LPARAM)TEXT("1000"));
	_stprintf_s(buf, TEXT("%d"), props->nNumToSend);
	SetWindowText(hwndSend, buf);

	// Set the host name field
	if (props->szHostName[0] == 0)
	{
		CHAR_2_TCHAR(buf, inet_ntoa(props->paddr_in->sin_addr), FILENAME_SIZE);
		SetWindowText(hwndIP, buf);
	}
	else
		SetWindowText(hwndIP, props->szHostName);
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: FillTransferProps
--
-- DATE: January 31st, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: BOOL FillTransferProps(HWND hwndDlg, DWORD dwHostMode, LPTransferProps props)
--				HWND hwndDlg:			Handle to the dialog window.
--				DWORD dwHostMode:		The current mode that the program is in (client or server).
--				LPTransferProps props:	Pointer to the structure holding information about the transfer.
--
-- RETURNS: Whether the transfer properties were successfully filled.
--
-- NOTES:
-- This function will the number of packets to send, the packet size (or whether to use a file), what file (if any) to use,
-- and the protocol to be used in the transfer.
---------------------------------------------------------------------------------------------------------------------------*/
BOOL FillTransferProps(HWND hwndDlg, DWORD dwHostMode, LPTransferProps props)
{
	TCHAR			buf[FILENAME_SIZE];
	HWND			hwndProtocol	= GetDlgItem(hwndDlg, ID_RADIO_TCP);
	HWND			hwndSize		= GetDlgItem(hwndDlg, ID_DROPDOWN_SIZE);
	HWND			hwndSend		= GetDlgItem(hwndDlg, ID_DROPDOWN_SEND);
	DWORD			dwDropDownSel;
	DWORD			dwPacketSize;
	DWORD			dwSendNum;

	props->nSockType  = (SendMessage(hwndProtocol, BM_GETCHECK, 0, 0) == BST_CHECKED) ? SOCK_STREAM : SOCK_DGRAM;

	if(!GetDlgAddrInfo(hwndDlg, dwHostMode, props))
		return FALSE;

	dwDropDownSel = SendMessage(hwndSize, CB_GETCURSEL, 0, 0);
	GetDlgItemText(hwndDlg, ID_TEXTBOX_FILE, buf, FILENAME_SIZE);
	_tcscpy_s(props->szFileName, buf);
	
	if (dwHostMode == ID_HOSTTYPE_SERVER)
		return TRUE; // Don't need any more info for the server; we're done

	//Get the transfer details: packet size, what file (if any) to use, number of packets to send
	if (dwDropDownSel == DROPDOWN_USEFILESIZE)
	{
		if(buf[0] == 0)
		{
			MessageBox(NULL, TEXT("Please specify a file to send."), TEXT("No file"), MB_ICONERROR);
			return FALSE;
		}
	}
	else
	{
		ComboBox_GetText(hwndSize, buf, FILENAME_SIZE);
		if (_stscanf_s(buf, TEXT("%d"), &dwPacketSize) == 0 || dwPacketSize == 0)
		{
			MessageBox(NULL, TEXT("Please enter a non-zero number for packet size."), TEXT("Invalid byte number"), MB_ICONERROR);
			return FALSE;
		}
		props->nPacketSize = dwPacketSize;

		ComboBox_GetText(hwndSend, buf, FILENAME_SIZE);
		if (_stscanf_s(buf, TEXT("%d"), &dwSendNum) == 0 || dwSendNum <= 0)
		{
			MessageBox(NULL, TEXT("Please enter a non-negative number of packets to send."), TEXT("Invalid send number"), MB_ICONERROR);
			return FALSE;
		}
		props->nNumToSend = dwSendNum;
		props->szFileName[0] = 0;
	}
	return TRUE;
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: GetDlgAddrInfo
-- 
-- DATE: January 31st, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: GetDlgAddrInfo(HWND hwndDlg, LPTransferProps props)
--							HWND hwndDlg:			Handle to the dialog box window.
--							LPTransferProps props:	Pointer to a transfer properties struct.
--
-- RETURNS: False if one of the conversions fails, true otherwise.
--
-- NOTES:
-- Retrieves the port number and IP address/host name of entered by the user and places them in the sock_inaddr struct if
-- there are no errors.
---------------------------------------------------------------------------------------------------------------------------*/
BOOL GetDlgAddrInfo(HWND hwndDlg, DWORD dwHostMode, LPTransferProps props)
{
	TCHAR	buf[HOSTNAME_SIZE];
	DWORD	usPortNum;
	DWORD	dwBinaryAddr;
	char	hostip_string[HOSTNAME_SIZE];

	// Get the information for storage in our in_addr
	GetDlgItemText(hwndDlg, ID_TEXTBOX_PORT, buf, HOSTNAME_SIZE);
	if(_stscanf_s(buf, TEXT("%d"), &usPortNum) == 0)
	{
		MessageBox(NULL, TEXT("The port must be a number."), TEXT("Non-Numeric Port"), MB_ICONERROR);
		return FALSE;
	}
	props->paddr_in->sin_port = htons(usPortNum);

	if (dwHostMode == ID_HOSTTYPE_SERVER)
		return TRUE;

	GetDlgItemText(hwndDlg, ID_TEXTBOX_HOSTIP, buf, HOSTNAME_SIZE);
	if(buf[0] == 0)
	{
		MessageBox(NULL, TEXT("Please enter a host name or IP address."), TEXT("No Host/IP"), MB_ICONERROR);
		return FALSE;
	}

	// IP address (maybe)
	if(isdigit(buf[0]))
	{
		TCHAR_2_CHAR(hostip_string, buf, HOSTNAME_SIZE);
		if((dwBinaryAddr = inet_addr(hostip_string)) == INADDR_NONE)
		{
			MessageBox(NULL, TEXT("IP address must be in the form xxx.xxx.xxx.xxx."), TEXT("Invalid IP"), MB_ICONERROR);
			return FALSE;
		}
		props->paddr_in->sin_addr.s_addr = dwBinaryAddr;
		props->szHostName[0] = 0; // Don't check the host name when sending later 
	}
	else
	{
		props->paddr_in->sin_addr.s_addr = 0; // Get rid of current address

		// We'll check at connection time whether this is a real host; store it for now
		_tcscpy_s(props->szHostName, buf);
	}
	return TRUE;
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: OpenFileDlg
--
-- DATE: January 31st, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: OpenFileDlg(HWND hwndDlg, DWORD dwHostMode)
--						HWND hwndDlg:		Handle to the dialog box window.
--						DWORD dwHostMode:	The current mode (client or server) that the host is in.
--
-- RETURNS: void
--
-- NOTES:
-- Opens the file dialog for the user to choose a file to either read from (for sending) or write to (for received data).
-- The file must exist if it's being used to send, but it may be a new file if it's being written to in the receive routine.
---------------------------------------------------------------------------------------------------------------------------*/
VOID OpenFileDlg(HWND hwndDlg, DWORD dwHostMode)
{
	OPENFILENAME	ofn;
	TCHAR			szFileName[FILENAME_SIZE] = {0};
	HWND			hwndFile = GetDlgItem(hwndDlg, ID_TEXTBOX_FILE);
	DWORD			dwFlags = OFN_EXPLORER | OFN_FORCESHOWHIDDEN | OFN_NONETWORKBUTTON;
	TCHAR			*szText;

	if (dwHostMode == ID_HOSTTYPE_CLIENT)
	{
		dwFlags |= OFN_PATHMUSTEXIST;
		szText = TEXT("Choose a File to Send");
	}
	else
		szText = TEXT("Choose a File to Save to");

	ofn.lStructSize			= sizeof(OPENFILENAME);
	ofn.hwndOwner			= hwndDlg;
	ofn.hInstance			= NULL;
	ofn.Flags				= dwFlags;
	ofn.lpstrFilter			= NULL;
	ofn.lpstrCustomFilter	= NULL;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= 0;
	ofn.lpstrFile			= szFileName;
	ofn.nMaxFile			= FILENAME_SIZE;
	ofn.lpstrFileTitle		= NULL;
	ofn.nMaxFileTitle		= NULL;
	ofn.lpstrInitialDir		= NULL;
	ofn.lpstrTitle			= szText;
	ofn.lpstrDefExt			= NULL;
	ofn.lCustData			= 0;
	ofn.lpfnHook			= NULL;
	ofn.lpTemplateName		= NULL;
	ofn.FlagsEx				= 0;

	GetOpenFileName(&ofn);
	SetWindowText(hwndFile, szFileName);
}
