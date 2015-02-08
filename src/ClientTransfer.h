#ifndef TRANSFER_H
#define TRANSFER_H

#include <WinSock2.h>
#include <Windows.h>
#include <Winternl.h>
#include <cstdio>
#include <cstring>
#include <time.h>
#include "WinStorage.h"
#include "Utils.h"

#define FILE_PACKETSIZE 4096

#ifndef COMM_TIMEOUT
	#define COMM_TIMEOUT 5000	// Time to wait before giving up
#endif

BOOL ClientInitSocket(LPTransferProps props);
DWORD WINAPI ClientSendData(VOID *hwnd);
BOOL TCPSendFirst(LPTransferProps props);
BOOL UDPSendFirst(LPTransferProps props);
VOID CALLBACK UDPSendCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, 
	LPOVERLAPPED lpOverlapped, DWORD dwFlags);
VOID CALLBACK TCPSendCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, 
	LPOVERLAPPED lpOverlapped, DWORD dwFlags);
BOOL LoadFile(LPWSABUF wsaBuf, const TCHAR *szFileName, LPDWORD lpdwFileSize, LPTransferProps props);
CHAR *CreateBuffer(CHAR data, LPTransferProps props);
BOOL PopulateBuffer(LPWSABUF pwsaBuf, LPTransferProps props, LPDWORD lpdwFileSize);
VOID ClientCleanup(LPTransferProps props);

#endif