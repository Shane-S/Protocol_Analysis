#ifndef WIN_STORAGE_H
#define WIN_STORAGE_H

#include <WinSock2.h>
#include <time.h>

#define GWLP_TRANSFERPROPS	0						// Offset value to access the transfer props pointer in wndExtra 
#define GWLP_HOSTMODE		sizeof(LPTransferProps)	// Offset value to access the host mode pointer in wndExtra
#define FILENAME_SIZE		512						// The max file name size (in bytes)
#define HOSTNAME_SIZE		128						// The max host name size (in bytes)

/* This structure contains the properties necessary to perform a transfer. */
typedef struct _TransferProps
{
	WSAOVERLAPPED	wsaOverlapped;
	LPSOCKADDR_IN	paddr_in;
	SOCKET			socket;
	DWORD			nSockType;
	TCHAR			szFileName[FILENAME_SIZE];
	TCHAR			szHostName[HOSTNAME_SIZE];
	DWORD			nPacketSize;
	DWORD			nNumToSend;
	SYSTEMTIME		startTime;
	SYSTEMTIME		endTime;
	DWORD			dwTimeout;
} TransferProps, *LPTransferProps;

#endif