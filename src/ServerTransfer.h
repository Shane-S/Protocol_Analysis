#ifndef SERVER_TRANSFER_H
#define SERVER_TRANSFER_H

#include <WinSock2.h>
#include <Windows.h>
#include <time.h>
#include "WinStorage.h"
#include "Utils.h"

#define UDP_MAXPACKET	65535	// The maximum datagram size
#ifndef COMM_TIMEOUT			// Time to wait before giving up (used mostly for UDP)
	#define COMM_TIMEOUT 5000
#endif

BOOL ServerInitSocket(LPTransferProps props);
DWORD WINAPI Serve(VOID *hwnd);
BOOL ListenTCP(LPTransferProps props);
BOOL ListenUDP(LPTransferProps props, LPSOCKADDR_IN client);
VOID ServerCleanup(LPTransferProps props);

// Completion routine prototypes
VOID CALLBACK UDPRecvCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,
	LPOVERLAPPED lpOverlapped, DWORD dwFlags);
VOID CALLBACK TCPRecvCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,
	LPOVERLAPPED lpOverlapped, DWORD dwFlags);
#endif