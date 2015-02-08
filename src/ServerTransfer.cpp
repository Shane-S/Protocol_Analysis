/*----------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: ServerTransfer.cpp
--
-- PROGRAM: Assn2
--
-- FUNCTIONS:
-- BOOL ServerInitSocket(LPTransferProps props);
-- DWORD WINAPI Serve(VOID *hwnd);
-- VOID ServerCleanup(LPTransferProps props);
-- BOOL ListenTCP(LPTransferProps props);
-- BOOL ListenUDP(LPTransferProps props);
-- 
-- VOID CALLBACK UDPRecvCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,
--		LPOVERLAPPED lpOverlapped, DWORD dwFlags);
-- VOID CALLBACK TCPRecvCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,
--		LPOVERLAPPED lpOverlapped, DWORD dwFlags);
--
-- DATE: February 6th, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- NOTES:	Functions in this file compose the server side of the program. Serve is the server thread, ServerInitSocket
--			initialises a server socket, and ServerCleanup resets the transfer state variables to their defaults. The two
--			Listen functions handle incoming connections for TCP and UDP. The two callback functions are completion 
--			routines called by Windows when the server receives data.
-------------------------------------------------------------------------------------------------------------------------*/

#include "ServerTransfer.h"

// "Global" variables (used only in this file)
static DWORD	recvd	= 0;	// The number of bytes or packets received
static WSABUF	wsaBuf;			// A buffer to contain the received data
static HANDLE	destFile;		// A file to store the transferred data (if specified by the user)

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: ServerInitSocket
-- Febrary 6th, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: ClientInitSocket(LPTransferProps props)
--							LPTransferProps props:	Pointer to a TransferProps structure containing information about the
--													packet size and number, server IP/host name, etc.
--
-- RETURNS: False if the socket can't be created or bound; true otherwise.
--
-- NOTES:
-- Preps a socket for receiving. This will socket will listen for packets (TCP) or receive the initial packet (UDP) when
-- the server is listening. The socket is created and bound here. The function also prevents the user from creating another
-- listening thread (it will return an error message if the address is already bound).
---------------------------------------------------------------------------------------------------------------------------*/
BOOL ServerInitSocket(LPTransferProps props)
{
	SOCKET s = WSASocket(AF_INET, props->nSockType, 0, NULL, NULL, WSA_FLAG_OVERLAPPED);
	BOOL set = TRUE;
	props->nPacketSize = 0;
	props->nNumToSend = 0;

	props->paddr_in->sin_addr.s_addr = htonl(INADDR_ANY);

	if (s == INVALID_SOCKET)
	{
		MessageBoxPrintf(MB_ICONERROR, TEXT("WSASocket Failed"), TEXT("Could not create socket, error %d"), WSAGetLastError());
		return FALSE;
	}

	if (bind(s, (sockaddr *)props->paddr_in, sizeof(sockaddr)) == SOCKET_ERROR)
	{
		MessageBoxPrintf(MB_ICONERROR, TEXT("bind Failed"), TEXT("Could not bind socket, error %d"), WSAGetLastError());
		return FALSE;
	}
	props->socket = s;
	return TRUE;
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: Serve
-- Febrary 6th, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: Serve(VOID *hwnd)
--					VOID *hwnd: Handle to the parent window.
--
-- RETURNS: A status code inidicating the thread's state when it exited. This is a positive integer if an error occurred,
--			or zero if the thread ran successfully.
--
-- NOTES:
-- Listens for incoming connection requests/packets. Once a connection has been established or a packet received, the
-- thread continues to receive the packets until there are no more to receive (UDP) or the client sends FIN, ACK (TCP).
---------------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI Serve(VOID *hwnd)
{
	BOOL			set		= TRUE;
	DWORD			flags	= 0;
	LPTransferProps props	= (LPTransferProps)GetWindowLongPtr((HWND)hwnd, GWLP_TRANSFERPROPS);
	DWORD			dwSleepRet;
	SOCKADDR_IN		client;
	char			buf[UDP_MAXPACKET];

	wsaBuf.buf = buf;
	wsaBuf.len = UDP_MAXPACKET;

	if (props->szFileName[0])
	{
		destFile = CreateFile(props->szFileName, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
		if (destFile == INVALID_HANDLE_VALUE)
		{
			MessageBoxPrintf(MB_ICONERROR, TEXT("CreateFile Failed"), TEXT("CreateFile failed with error %d"), GetLastError());
			return -1;
		}
	}

	if (props->nSockType == SOCK_STREAM && !ListenTCP(props))
	{
		ServerCleanup(props);
		return 1;
	}
	else if (props->nSockType == SOCK_DGRAM && !ListenUDP(props, &client))
	{
		ServerCleanup(props);
		return 2;
	}

	while (props->dwTimeout)
	{
		dwSleepRet = SleepEx(props->dwTimeout, TRUE);
		if (dwSleepRet != WAIT_IO_COMPLETION)
			break; // We've lost some packets; just exit the loop
	}

	if (props->szFileName[0] == 0)
		LogTransferInfo("ReceiveLog.txt", props, recvd, (HWND)hwnd);

	ServerCleanup(props);
	return 0;
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: UDPRecvCompletion
-- Febrary 7th, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: UDPSendCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped, DWORD dwFlags)
--								DWORD dwErrorCode:					0 if there were no errors; otherwise, a socket error code.
--								DWORD dwNumberOfBytesTransferred:	The number of bytes transferred.
--								LPOVERLAPPED lpOverlapped :			Pointer to an overlapped structure; here, it is a pointer to
--																	an LPTransferProps structure.
--								DWORD dwFlags:						Flags specified when the WSASend was posted.
--
-- RETURNS: void
--
-- NOTES:
-- Windows calls this function whenever a UDP packet is received. It increments the number of packets received and posts another
-- WSARecvFrom. If there are no packets left to receive, it obtains the end time and returns. If there is an error, it displays
-- the appropriate error message and returns.
---------------------------------------------------------------------------------------------------------------------------*/
VOID CALLBACK UDPRecvCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,
	LPOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	LPTransferProps props = (LPTransferProps)lpOverlapped;
	static BOOL		useFile = props->szFileName[0] != 0;
	DWORD flags = 0;
	SOCKADDR_IN client;
	INT client_size = sizeof(client);

	if (dwErrorCode != 0)
	{
		MessageBoxPrintf(MB_ICONERROR, TEXT("UDP Recv Error"), TEXT("Error receiving UDP packet; error code %d"), dwErrorCode);
		props->dwTimeout = 0;
		return;
	}
	recvd += dwNumberOfBytesTransfered;

	props->nNumToSend = ((DWORD *)wsaBuf.buf)[0];
	props->nPacketSize = dwNumberOfBytesTransfered;
	GetSystemTime(&props->endTime);

	if (useFile)
		WriteFile(destFile, (VOID *)wsaBuf.buf, dwNumberOfBytesTransfered, &dwNumberOfBytesTransfered, NULL);
	
	// Finished receiving
	if (!useFile && recvd/dwNumberOfBytesTransfered == props->nNumToSend)
	{
		props->dwTimeout = 0;
		return;
	}

	// This is the first packet
	if (props->dwTimeout == INFINITE)
	{
		props->dwTimeout = COMM_TIMEOUT;
		GetSystemTime(&props->startTime);
	}

	WSARecvFrom(props->socket, &wsaBuf, 1, NULL, &flags, (sockaddr *)&client, &client_size, (LPOVERLAPPED)props, UDPRecvCompletion);
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: TCPRecvCompletion
-- Febrary 7th 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: UDPSendCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped, DWORD dwFlags)
--								DWORD dwErrorCode:					0 if there were no errors; otherwise, a socket error code.
--								DWORD dwNumberOfBytesTransferred:	The number of bytes transferred.
--								LPOVERLAPPED lpOverlapped :			Pointer to an overlapped structure; here, it is a pointer to
--																	an LPTransferProps structure.
--								DWORD dwFlags:						Flags specified when the WSASend was posted.
--
-- RETURNS: void
--
-- NOTES:
-- Windows calls this function whenever a TCP packet is received. It increments the number of bytes received and posts another
-- WSARecv. If there are no packets left to receive, it obtains the end time and returns. If there is an error, it displays the
-- appropriate error message and returns.
---------------------------------------------------------------------------------------------------------------------------*/
VOID CALLBACK TCPRecvCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,
	LPOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	LPTransferProps props	= (LPTransferProps)lpOverlapped;
	static BOOL		useFile = props->szFileName[0] != 0;
	DWORD			flags	= 0;

	if (dwErrorCode != 0)
	{
		MessageBoxPrintf(MB_ICONERROR, TEXT("TCP Recv Error"), TEXT("Error receiving TCP packet; error code %d"), dwErrorCode);
		props->dwTimeout = 0;
		return;
	}

	if (useFile)
		WriteFile(destFile, (VOID *)wsaBuf.buf, dwNumberOfBytesTransfered, &dwNumberOfBytesTransfered, NULL);

	if (props->nPacketSize == 0)
	{
		props->nNumToSend	= ((DWORD *)wsaBuf.buf)[0]; // extract the original number to send
		props->nPacketSize	= ((DWORD *)wsaBuf.buf)[1]; // extract the original packet size
	}

	if (dwNumberOfBytesTransfered == 0)
	{
		GetSystemTime(&props->endTime);
		props->dwTimeout = 0;
		return;
	}

	recvd += dwNumberOfBytesTransfered;
	WSARecv(props->socket, &wsaBuf, 1, NULL, &flags, (LPOVERLAPPED)props, TCPRecvCompletion);
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: ServerCleanup
-- Febrary 10th 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: ServerCleanup(LPTransferProps props)
--							LPTransferProps props:  Pointer to the TransferProps structure containing the details for this
--													transfer.
--
-- RETURNS: void
--
-- NOTES:
-- Resets all parameters used in the transfer to their default values in preparation for receiving again.
---------------------------------------------------------------------------------------------------------------------------*/
VOID ServerCleanup(LPTransferProps props)
{
	recvd = 0;
	closesocket(props->socket);
	DWORD error = WSAGetLastError();
	props->nPacketSize = 0;
	props->nNumToSend = 0;
	props->dwTimeout = COMM_TIMEOUT;
	closesocket(props->socket);
	CloseHandle(destFile);
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: ListenTCP
-- Febrary 10th 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: ListenTCP(LPTransferProps props)
--						LPTransferProps props:  Pointer to the TransferProps structure containing the details for this
--													transfer.
--
-- RETURNS: void
--
-- NOTES:
-- Listens for and accepts a TCP connection, then posts a WSARecv on the socket to activate the completion routine.
---------------------------------------------------------------------------------------------------------------------------*/
BOOL ListenTCP(LPTransferProps props)
{
	SOCKET accept;
	DWORD flags = 0, error = 0;

	if (listen(props->socket, 5) == SOCKET_ERROR)
	{
		MessageBoxPrintf(MB_ICONERROR, TEXT("listen() Failed"), TEXT("listen() failed with socket error %d"), WSAGetLastError());
		return FALSE;
	}

	if ((accept = WSAAccept(props->socket, NULL, NULL, NULL, NULL)) == SOCKET_ERROR)
	{
		MessageBoxPrintf(MB_ICONERROR, TEXT("WSAAccept Failed"), TEXT("WSAAccept() failed with socket error %d"), WSAGetLastError());
		return FALSE;
	}
	GetSystemTime(&props->startTime); // Record the start time

	closesocket(props->socket); // close the listening socket
	props->socket = accept;		// assign the new socket to props->socket

	WSARecv(props->socket, &wsaBuf, 1, NULL, &flags, (LPOVERLAPPED)props, TCPRecvCompletion);

	error = WSAGetLastError();

	if (error && error != WSA_IO_PENDING)
	{
		MessageBoxPrintf(MB_ICONERROR, TEXT("WSARecv Error"), TEXT("WSARecv encountered error %d"), error);
		props->dwTimeout = 0;
		return FALSE;
	}
	return TRUE;
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: ListenUDP
-- Febrary 10th 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: ListenUDP(LPTransferProps props, LPSOCKADDR_IN client)
--							LPTransferProps props:  Pointer to the TransferProps structure containing the details for this
--													transfer.
--							LPSOCKADDR_IN	client:	Pointer to a SOCKADDR_IN structure to hold the client's info. This must
--													be passed from the thread to ensure that it's in scope when the
--													completion routine is called.
--
-- RETURNS: void
--
-- NOTES:
-- Posts a receive request on the socket to wait for a UDP packet.
---------------------------------------------------------------------------------------------------------------------------*/
BOOL ListenUDP(LPTransferProps props, LPSOCKADDR_IN client)
{
	DWORD		flags = 0, error = 0;
	INT			client_size = sizeof(*client);

	props->dwTimeout = INFINITE;

	WSARecvFrom(props->socket, &wsaBuf, 1, NULL, &flags, (sockaddr *)client, &client_size, (LPOVERLAPPED)props,
		UDPRecvCompletion);

	error = WSAGetLastError();
	if (error && error != WSA_IO_PENDING)
	{
		MessageBoxPrintf(MB_ICONERROR, TEXT("WSARecvFrom Error"), TEXT("WSARecvFrom encountered error %d"), error);
		props->dwTimeout = 0;
		return FALSE;
	}
	return TRUE;
}
