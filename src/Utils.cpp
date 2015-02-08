/*----------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: Utils.cpp
--
-- PROGRAM: Assn2
--
-- FUNCTIONS:
-- int CDECL MessageBoxPrintf(DWORD dwType, TCHAR * szCaption, TCHAR * szFormat, ...);
-- int CDECL DrawTextPrintf(HWND hwnd, TCHAR * szFormat, ...);
-- VOID LogTransferInfo(const char *filename, LPTransferProps props, DWORD dwSentOrRecvd, DWORD dwHostMode);
-- VOID CreateTimestamp(char *buf, SYSTEMTIME *time);
--
-- DATE: February 7th, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- NOTES:	This file contains utility functions for use (mostly) throughout the entire program. The printf functions
--			are wrappers for printing to a message box and to the screen, and the LogTransferInfo and CreateTimestamp
--			functions are used in logging transfer statistics.
-------------------------------------------------------------------------------------------------------------------------*/

#include "Utils.h"

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: MessageBoxPrintf
-- Febrary 7th, 2014
--
-- DESIGNER: Charles Petzold
--
-- PROGRAMMER: Charles Petzold
--
-- INTERFACE: MessageBoxPrintf(DWORD dwType, TCHAR *szCaption, TCHAR *szFormat, ...)
--								DWORD dwType:		The type of message box to display.
--								TCHAR *szCaption:	The caption to display at the top of the message box.
--								TCHAR *szFormat:	The format argument for sprintf.
--								...:				Variadic arguments for use with the format.
--
-- RETURNS: An int; this is the message box's return value.
--
-- NOTES:
-- Wraps sprintf and message box functionality for more convenient custom message box messages.  Note that Petzold also
-- wrote the comments wihtin the function.
---------------------------------------------------------------------------------------------------------------------------*/
int CDECL MessageBoxPrintf(DWORD dwType, TCHAR * szCaption, TCHAR * szFormat, ...)
{
	TCHAR szBuffer[1024];
	va_list pArgList;
	// The va_start macro (defined in STDARG.H) is usually equivalent to:
	// pArgList = (char *) &szFormat + sizeof (szFormat) ;
	va_start(pArgList, szFormat);
	// The last argument to wvsprintf points to the arguments
	_vsntprintf_s(szBuffer, sizeof (szBuffer) / sizeof (TCHAR),
		szFormat, pArgList);
	// The va_end macro just zeroes out pArgList for no good reason
	va_end(pArgList);
	return MessageBox(NULL, szBuffer, szCaption, dwType);
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: DrawTextPrintf
-- Febrary 7th, 2014
--
-- DESIGNER: Charles Petzold
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: DrawTextPrintf(HWND hwnd, TCHAR *szFormat, ...)
--								HWND hwnd:			Handle to the window to which we're drawing.
--								TCHAR *szFormat:	The format argument for sprintf.
--
-- RETURNS: An int; this is DrawText's return value.
--
-- NOTES:
-- Wraps sprintf and DrawText functionality for more convenient custom writing to the window. The comments within the
-- function are also written by Petzold.
---------------------------------------------------------------------------------------------------------------------------*/
int CDECL DrawTextPrintf(HWND hwnd, CHAR * szFormat, ...)
{
	HDC hdc;
	CHAR szBuffer[1024];
	va_list pArgList;
	DWORD dwRet;
	RECT clientRect;
	GetClientRect(hwnd, &clientRect);
	// The va_start macro (defined in STDARG.H) is usually equivalent to:
	// pArgList = (char *) &szFormat + sizeof (szFormat) ;
	va_start(pArgList, szFormat);
	// The last argument to wvsprintf points to the arguments
	_vsnprintf_s(szBuffer, sizeof (szBuffer) / sizeof (TCHAR),
		szFormat, pArgList);
	// The va_end macro just zeroes out pArgList for no good reason
	va_end(pArgList);

	hdc = GetDC(hwnd);
	dwRet = DrawTextA(hdc, szBuffer, strlen(szBuffer), &clientRect, DT_CALCRECT);
	InvalidateRect(hwnd, &clientRect, TRUE);
	UpdateWindow(hwnd);
	ReleaseDC(hwnd, hdc);

	return dwRet;
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: LogTransferInfo
-- Febrary 9th, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: LogTransferInfo(const char *filename, LPTransferProps props, DWORD dwSentOrRecvd, DWORD dwHostMode)
--								char *filename:			The name of the log file.
--								LPTransferProps props:	Pointer to the TransferProps structure containing details about this
--														transfer.
--								DWORD dwSentOrRecvd:	The number of bytes/packets sent or received.
--								DWORD dwHostMode:		The host mode (client or server) in which we're operating.
--
-- RETURNS: void
--
-- NOTES:
-- Logs information about the transfer: start timestamp, end timestamp, transfer time, number of packets
-- sent/received/expected, packet size, and protocol used.
---------------------------------------------------------------------------------------------------------------------------*/
VOID LogTransferInfo(const char *filename, LPTransferProps props, DWORD dwSentOrRecvd, HWND hwnd)
{
	DWORD			dwHostMode = (DWORD)GetWindowLongPtr(hwnd, GWLP_HOSTMODE);
	//FILE			*file;
	FILETIME		ftStartTime, ftEndTime;
	CHAR			startTimestamp[TIMESTAMP_SIZE] = { 0 }, endTimestamp[TIMESTAMP_SIZE] = { 0 };
	ULARGE_INTEGER	ulStartTime, ulEndTime, ulTransferTime;
	CHAR			log[512] = { 0 };
	INT				written = 0;
	TCHAR			logw[512];

	// Jump through the ludicrous amount of hoops to get millisecond resolution on Windows
	SystemTimeToFileTime(&props->startTime, &ftStartTime);
	SystemTimeToFileTime(&props->endTime, &ftEndTime);

	ulStartTime.QuadPart = ftStartTime.dwHighDateTime;
	ulStartTime.QuadPart <<= 32;
	ulStartTime.QuadPart += ftStartTime.dwLowDateTime;

	ulEndTime.QuadPart = ftEndTime.dwHighDateTime;
	ulEndTime.QuadPart <<= 32;
	ulEndTime.QuadPart += ftEndTime.dwLowDateTime;

	ulTransferTime.QuadPart = ulEndTime.QuadPart - ulStartTime.QuadPart;

	CreateTimestamp(startTimestamp, &props->startTime);
	CreateTimestamp(endTimestamp, &props->endTime);

	//errno_t error = fopen_s(&file, filename, "a");
	/*if (file == NULL)
	{
		MessageBoxPrintf(MB_ICONERROR, TEXT("Couldn't Open File"), TEXT("Couldn't open log file %s"), filename);
		return;
	}*/

	// The division by 10 000 is necessary because Windows gives the times in 100ns intervals. Why would you do that. Seriously.
	written += sprintf_s(log, "Start timestamp: %s\r\nEnd timestamp: %s\r\nTransfer time: %dms\r\n", startTimestamp, endTimestamp,
		ulTransferTime.QuadPart / 10000);
	written += sprintf_s((log + written), 256, "Packet size: %d bytes\r\n", props->nPacketSize);
	
	if(dwHostMode == ID_HOSTTYPE_SERVER)
		written += sprintf_s((log + written), 256, "Bytes received: %d\r\nPackets received : %d\r\nPackets expected : %d\r\n", dwSentOrRecvd,
		dwSentOrRecvd / props->nPacketSize, props->nNumToSend);
	else
		written += sprintf_s((log + written), 256, "Packets sent: %d\r\nBytes sent: %d\r\n", dwSentOrRecvd / props->nPacketSize, dwSentOrRecvd);

	written += sprintf_s((log + written), 256, "Protocol: %s\r\n\r\n", (props->nSockType == SOCK_DGRAM) ? "UDP" : "TCP");
	//fprintf(file, "%s", "hello");
	
	CHAR_2_TCHAR(logw, log, 256);
	MessageBoxPrintf(MB_OK, TEXT("Stats"), TEXT("%s"), logw);
	//fclose(file);
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: CreateTimestamp
-- Febrary 9th, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: MessageBoxPrintf(char *buf, SYSTEMTIME *time)
--								char *buf:			A buffer which will hold the timestamp string.
--								SYSTEMTIME *time:	The SYSTEMTIME structure holding the info timestamp for conversion.
--
-- RETURNS: void
--
-- NOTES:
-- Formats the time in the buffer into a standard timestamp. Surprisingly, Windows doesn't seem to already offer a function
-- for this.
---------------------------------------------------------------------------------------------------------------------------*/
VOID CreateTimestamp(char *buf, SYSTEMTIME *time)
{
	sprintf_s(buf, TIMESTAMP_SIZE, "%d-%02d-%02dT%02d:%02d:%02d:%03dTZD", time->wYear, time->wMonth, time->wDay, time->wHour, 
		time->wMinute, time->wSecond, time->wMilliseconds);
}
