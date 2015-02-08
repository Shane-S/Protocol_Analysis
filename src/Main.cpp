/*----------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: Main.cpp
--
-- PROGRAM: Assn2
--
-- FUNCTIONS:
-- LPTransferProps CreateTransferProps();
-- int WINAPI WinMain(HINSTANCE hPrevInstance, HINSTANCE hInstance, LPSTR lpszCmdArgs, int iCmdShow);
--
-- DATE: February 1st, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- NOTES: This file contains functions that set up the window and transfer properties for use. It is also the entry point
--		  to the program.
-------------------------------------------------------------------------------------------------------------------------*/

#include "Main.h"
#include "WndProc.h"

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: WinMain
-- January 30th, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: int WINAPI WinMain(HINSTANCE hPrevInstance, HINSTANCE hInstance, LPSTR lpszCmdArgs, int iCmdShow)
--								HINSTANCE hPrevInstance:	Handle to the a previous copy of the program (not used here).
--								HINSTANCE hInstance:		Handle to the current instance of the program.
--								LPSTR lpszCmdArgs:			Pointer to the command arguments (if any).
--								int iCmdShow:				Determines how the window will be shown.
--
-- RETURNS: void
--
-- NOTES:
-- This is the entry point to the program. It sets up the window, creates a TransferProps structure, and sets the default
-- host mode before displaying the GUI.
---------------------------------------------------------------------------------------------------------------------------*/
int WINAPI WinMain(HINSTANCE hPrevInstance, HINSTANCE hInstance, LPSTR lpszCmdArgs, int iCmdShow)
{
	HWND			hwnd;
	MSG				msg;
	WNDCLASS		wndclass;
	WSADATA			wsaData;
	LPTransferProps props = CreateTransferProps();

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = sizeof(DWORD) + sizeof(LPTransferProps);
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	wndclass.lpszClassName = CLASS_NAME;

	if (!RegisterClass(&wndclass))
	{
		MessageBox(NULL, TEXT("Fatal error; extiting the program"), TEXT("Error"), MB_ICONERROR);
		return 0;
	}

	WSAStartup(MAKEWORD(2, 2), &wsaData);

	if ((props = CreateTransferProps()) == NULL)
	{
		MessageBox(NULL, TEXT("Unable to allocate memory for transfer properties."), TEXT("Error"), MB_ICONERROR);
		return -1;
	}

	hwnd = CreateWindow(CLASS_NAME, TEXT("Test Program"), WS_OVERLAPPEDWINDOW,
		0, 0, 600, 600, NULL, NULL, hInstance, NULL);

	SetWindowLongPtr(hwnd, GWLP_TRANSFERPROPS, (LONG)props);
	SetWindowLongPtr(hwnd, GWLP_HOSTMODE, ID_HOSTTYPE_CLIENT);

	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	WSACleanup();
	return msg.wParam;
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: CreateTransferProps
-- 
-- DATE: Febrary 1st, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: CreateTransferProps()
--
-- RETURNS: A pointer to a TransferProps structure, or NULL if the structure couldn't be created (due to insufficient memory).
--
-- NOTES:
-- This function allocates memory for and initialises a TransferProps structure. It sets the default transfer properties
-- which will be used in case the user decides to begin transferrring without actually choosing options.
---------------------------------------------------------------------------------------------------------------------------*/
LPTransferProps CreateTransferProps()
{
	LPTransferProps props = (LPTransferProps)malloc(sizeof(TransferProps));
	if(props == NULL)
		return NULL;
	
	memset(props->szFileName, 0, FILENAME_SIZE);
	memset(props->szHostName, 0, HOSTNAME_SIZE);

	props->nPacketSize = DEF_PACKETSIZE;
	props->nNumToSend = DEF_NUMTOSEND;

	props->paddr_in = (LPSOCKADDR_IN)malloc(sizeof(SOCKADDR_IN));
	if(props->paddr_in == NULL)
	{
		free(props);
		return NULL;
	}

	memset(props->paddr_in, 0, sizeof(sockaddr_in));
	props->paddr_in->sin_addr.s_addr	= inet_addr("127.0.0.1");
	props->paddr_in->sin_family			= AF_INET;
	props->paddr_in->sin_port			= htons(DEF_PORTNUM);
	
	props->socket = 0;
	props->nSockType = SOCK_STREAM;

	memset(&props->wsaOverlapped, 0, sizeof(WSAOVERLAPPED));

	memset(&props->startTime, 0, sizeof(SYSTEMTIME));
	memset(&props->endTime, 0, sizeof(SYSTEMTIME));

	props->dwTimeout = COMM_TIMEOUT;
	return props;
}
