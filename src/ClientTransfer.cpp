/*----------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: ClientTransfer.cpp
--
-- PROGRAM: Assn2
--
-- FUNCTIONS:
-- BOOL ClientInitSocket(LPTransferProps props);
-- DWORD WINAPI ClientSendData(VOID *hwnd);
-- VOID ClientCleanup(LPTransferProps props);
-- BOOL TCPSendFirst(LPTransferProps props);
-- BOOL UDPSendFirst(LPTransferProps props);
--
-- VOID CALLBACK UDPSendCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,
--		LPOVERLAPPED lpOverlapped, DWORD dwFlags);
-- VOID CALLBACK TCPSendCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,
--		LPOVERLAPPED lpOverlapped, DWORD dwFlags);
--
-- BOOL LoadFile(LPWSABUF wsaBuf, const TCHAR *szFileName, char **buf, LPDWORD lpdwFileSize, LPTransferProps props);
-- BOOL PopulateBuffer(LPWSABUF pwsaBuf, LPTransferProps props, LPDWORD lpdwFileSize);
-- CHAR *CreateBuffer(CHAR data, LPTransferProps props);
--
--
-- DATE: February 2nd, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- NOTES:	Functions in this file compose the client side of the program. ClientSendData where the data is transferred,
--			ClientInitSocket preps a socket for sending, and ClientCleanup frees all allocated memory and the two callback functions are completion routines called by
--			Windows when data was sent, and LoadFile loads a user-specified file into the sending buffer.
-------------------------------------------------------------------------------------------------------------------------*/

#include "ClientTransfer.h"

static DWORD	sent = 0;	// The number of bytes or packets sent
static WSABUF	wsaBuf;		// A buffer containing the data to be sent

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: ClientInitSocket
-- Febrary 1st, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: ClientInitSocket(LPTransferProps props)
--							LPTransferProps props:	Pointer to a TransferProps structure containing information about the
--													packet size and number, server IP/host name, etc.
--
-- RETURNS: False if the socket can't be created or the host name can't be resovled; true otherwise.
--
-- NOTES:
-- Preps a socket for sending. Resolves the host name if there is one and creates the socket, storing it in props->socket.
-- If the socket can't be created, props->socket is set to INVALID_SOCKET.
---------------------------------------------------------------------------------------------------------------------------*/
BOOL ClientInitSocket(LPTransferProps props)
{
	if (props->szHostName[0] != 0) // They specified a host name
	{
		struct hostent *hp;
		char		   name[HOSTNAME_SIZE];

		TCHAR_2_CHAR(name, props->szHostName, HOSTNAME_SIZE);
		hp = gethostbyname(name);

		if (hp == NULL)
		{
			props->socket = INVALID_SOCKET;
			switch (h_errno)
			{
			case HOST_NOT_FOUND:
				MessageBoxPrintf(MB_ICONERROR, TEXT("Host Not Found"),
					TEXT("Could not find host %s. Check connection settings."), props->szHostName);
				return FALSE;
			case TRY_AGAIN:
				MessageBoxPrintf(MB_ICONERROR, TEXT("Try Again"), TEXT("Unable to connect. Try again later."));
				return FALSE;
			case NO_RECOVERY:
				MessageBoxPrintf(MB_ICONERROR, TEXT("DNS Error"), TEXT("DNS error. Try again later."));
				return FALSE;
			case NO_ADDRESS:
				MessageBoxPrintf(MB_ICONERROR, TEXT("No IP"), TEXT("No IP address for host %s. Check connection settings."),
					props->szHostName);
				return FALSE;
			default:
				MessageBoxPrintf(MB_ICONERROR, TEXT("Unknown Error"), TEXT("Unkown error %d."), h_errno);
				return FALSE;
			}
		}
		props->paddr_in->sin_addr.s_addr = (ULONG)hp->h_addr_list[0];
	}
	
	if (props->paddr_in->sin_addr.s_addr != 0)
	{
		SOCKET s;
		if ((s = WSASocket(PF_INET, props->nSockType, 0, NULL, NULL, WSA_FLAG_OVERLAPPED)) < 0)
		{
			MessageBox(NULL, TEXT("Could not create socket."), TEXT("No Socket"), MB_ICONERROR);
			props->socket = INVALID_SOCKET;
			return FALSE;
		}

		props->socket = s;
		return TRUE;
	}
	else
	{
		MessageBox(NULL, TEXT("No IP address or host name entered. Check connection settings."), TEXT("No Destination"),
			MB_ICONERROR);
		return FALSE;
	}
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: ClientSendData
-- Febrary 1st, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: ClientSendData(VOID *params)
--							VOID *params: Handle to the main window cast as a VOID *.
--
-- RETURNS: A positive int if the connection fails (TCP), the user-specified file couldn't be opened, or the buffer couldn't be 
--			allocated. Returns 0 on successful sending. 
--
-- NOTES:
-- Sends either a chosen file (if there is one) or a specified number of packets of the specified size.
---------------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI ClientSendData(VOID *params)
{
	HWND			hwnd		= (HWND)params;
	LPTransferProps props		= (LPTransferProps)GetWindowLongPtr(hwnd, GWLP_TRANSFERPROPS);
	BOOL			set			= TRUE;
	SOCKET			s			= props->socket;
	DWORD			dwFileSize	= 0;
	DWORD			sleepRet;
	const char		*logFile	= "SendLog.txt";

	if (!PopulateBuffer(&wsaBuf, props, &dwFileSize))
	{
		ClientCleanup(props);
		return 1;
	}

	if (props->nSockType == SOCK_STREAM && !TCPSendFirst(props))
	{
		ClientCleanup(props);
		return 2;
	}
	else if (props->nSockType == SOCK_DGRAM && !UDPSendFirst(props))
	{
		ClientCleanup(props);
		return 3;
	}

	while (props->dwTimeout)
	{
		sleepRet = SleepEx(COMM_TIMEOUT, TRUE);

		if (sleepRet != WAIT_IO_COMPLETION)
		{
			MessageBox(NULL, TEXT("The connection timed out."), TEXT("Timeout"), MB_ICONERROR);
			props->dwTimeout = 0;
		}
	}

	if (props->szFileName[0] == 0) // We didn't use a file, so log the stats
		LogTransferInfo(logFile, props, sent, hwnd);
	
	ClientCleanup(props);
	return 0;
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: ClientSendData
-- Febrary 1st, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: ClientSendData(VOID *params)
--							VOID *params: Handle to the main window cast as a VOID *.
--
-- RETURNS: A positive int if the connection fails (TCP), the user-specified file couldn't be opened, or the buffer couldn't be
--			allocated. Returns 0 on successful sending.
--
-- NOTES:
-- Sends either a chosen file (if there is one) or a specified number of packets of the specified size.
---------------------------------------------------------------------------------------------------------------------------*/
BOOL TCPSendFirst(LPTransferProps props)
{
	DWORD error;
	DWORD firstSent;

	WSAConnect(props->socket, (sockaddr *)props->paddr_in, sizeof(sockaddr), NULL, NULL, NULL, NULL);
	GetSystemTime(&props->startTime);
	error = WSAGetLastError();

	if (error)
	{
		MessageBox(NULL, TEXT("Could not connect. Check settings and try again."),
			TEXT("Could not connect to socket"), MB_ICONERROR);
		return FALSE;
	}
	else
	{
		WSASend(props->socket, &wsaBuf, 1, &firstSent, 0, (LPOVERLAPPED)props, TCPSendCompletion);
		error = WSAGetLastError();
		if (error && error != WSA_IO_PENDING)
		{
			MessageBoxPrintf(MB_ICONERROR, TEXT("WSASend() Failed"), TEXT("WSASend failed with error %d"), error);
			return FALSE;
		}
	}
	return TRUE;
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: UDPSendFirst
-- Febrary 1st, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: ClientSendData(VOID *params)
--							VOID *params: Handle to the main window cast as a VOID *.
--
-- RETURNS: A positive int if the connection fails (TCP), the user-specified file couldn't be opened, or the buffer couldn't be
--			allocated. Returns 0 on successful sending.
--
-- NOTES:
-- Sends either a chosen file (if there is one) or a specified number of packets of the specified size.
---------------------------------------------------------------------------------------------------------------------------*/
BOOL UDPSendFirst(LPTransferProps props)
{
	DWORD firstSent;
	DWORD error;

	setsockopt(props->socket, SOL_SOCKET, SO_SNDBUF, wsaBuf.buf, props->nPacketSize);
	GetSystemTime(&props->startTime);
	WSASendTo(props->socket, &wsaBuf, 1, &firstSent, 0, (sockaddr *)props->paddr_in, sizeof(sockaddr), (LPOVERLAPPED)props, UDPSendCompletion);
	error = WSAGetLastError();

	if (error && error != WSA_IO_PENDING)
	{
		MessageBoxPrintf(MB_ICONERROR, TEXT("WSASendTo() Failed"),
			TEXT("WSASendTo failed with error %d"), error);
		return FALSE;
	}
	GetSystemTime(&props->startTime);
	return TRUE;
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: UDPSendCompletion
-- Febrary 2nd, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: UDPSendCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped, DWORD dwFlags)
--							DWORD dwErrorCode:					0 if there were no errors; otherwise, a socket error code.
--							DWORD dwNumberOfBytesTransferred:	The number of bytes transferred.
--							LPOVERLAPPED lpOverlapped:			Pointer to an overlapped structure; here, it is a pointer to
--																an LPTransferProps structure.
--							DWORD dwFlags:						Flags specified when the WSASend was posted.
--
-- RETURNS: void
--
-- NOTES:
-- Windows calls this function whenever a UDP packet is sent. It increments the number of packets sent and posts another
-- send. If there are no packets left to send, it obtains the end time and returns. If there is an error, it displays the
-- appropriate error message and returns.
---------------------------------------------------------------------------------------------------------------------------*/
VOID CALLBACK UDPSendCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,
	LPOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	LPTransferProps props = (LPTransferProps)lpOverlapped;

	if (dwErrorCode != 0)
	{
		MessageBoxPrintf(MB_ICONERROR, TEXT("WSASend error"), TEXT("WSASendTo encountered error %d"), dwErrorCode);
		props->dwTimeout = 0;
		return;
	}

	sent += dwNumberOfBytesTransfered;
	if (sent / props->nPacketSize >= props->nNumToSend) // Finished sending
	{
		GetSystemTime(&props->endTime);
		props->dwTimeout = 0;
		return;
	}

	WSASendTo(props->socket, &wsaBuf, 1, NULL, 0, (sockaddr *)props->paddr_in, sizeof(sockaddr), (LPOVERLAPPED)props, UDPSendCompletion);
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: TCPSendCompletion
-- Febrary 2nd, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: TCPSendCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped, DWORD dwFlags)
--							DWORD dwErrorCode:					0 if there were no errors; otherwise, a socket error code.
--							DWORD dwNumberOfBytesTransferred:	The number of bytes transferred.
--							LPOVERLAPPED lpOverlapped:			Pointer to an overlapped structure; here, it is a pointer to
--																an LPTransferProps structure.
--							DWORD dwFlags:						Flags specified when the WSASend was posted.
--
-- RETURNS: void
--
-- NOTES:
-- Windows calls this function whenever a TCP packet is sent. It increments the number of bytes sent and posts another
-- send. If there are no packets left to send, it obtains the end time and returns. If there is an error, it displays the
-- appropriate error message and returns.
---------------------------------------------------------------------------------------------------------------------------*/
VOID CALLBACK TCPSendCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,
	LPOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	LPTransferProps props	= (LPTransferProps)lpOverlapped;

	if (dwErrorCode != 0) // Something's gone wrong; display an error message and get out of there
	{
		MessageBoxPrintf(MB_ICONERROR, TEXT("WSASend() error"), TEXT("WSASend failed with socket error %d."), dwErrorCode);
		props->dwTimeout = 0;
		return;
	}
	sent += dwNumberOfBytesTransfered;

	if (sent / props->nPacketSize >= props->nNumToSend) // We're finished sending
	{
		props->dwTimeout = 0;
		GetSystemTime(&props->endTime);
		return;
	}

	WSASend(props->socket, &wsaBuf, 1, NULL, 0, (LPOVERLAPPED)props, TCPSendCompletion); // Post another send
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: LoadFile
-- Febrary 8th, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: LoadFile(LPWSABUF wsaBuf, const TCHAR *szFileName, char **buf, LPDWORD lpdwFileSize, LPTransferProps)
--						LPWSABUF wsaBuf:		Pointer to a WSABUF structure which will contain the file in its buffer.
--						TCHAR *szFileName:		Name of the file to load.
--						LPDWORD lpdwFileSize:	Pointer to a DWORD which will hold the file size.
--						LPTransferProps props:	Pointer to the TransferProps structure containing information about the 
-												current transfer.
--
-- RETURNS: FALSE if the file couldn't be loaded; TRUE otherwise.
--
-- NOTES:
-- Loads the file into the buffer pointed to by buf, and assigns a pointer to that buffer to wsaBuf->buf. This will later
-- be iterated over to send the appropriate packets.
---------------------------------------------------------------------------------------------------------------------------*/
BOOL LoadFile(LPWSABUF wsaBuf, const TCHAR *szFileName, LPDWORD lpdwFileSize, LPTransferProps props)
{
	DWORD dwFileSize;
	HANDLE hFile;
	DWORD dwRead;
	hFile = CreateFile(props->szFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		MessageBoxPrintf(MB_ICONERROR, TEXT("Couldn't Open File"),
			TEXT("Could not open file %s. Please check the spelling or select a different file. System Error: %d"), 
			props->szFileName, GetLastError());
		return FALSE;
	}

	dwFileSize = GetFileSize(hFile, NULL);
	wsaBuf->buf = (CHAR *)malloc(dwFileSize);
	ReadFile(hFile, wsaBuf->buf, dwFileSize, &dwRead, NULL);
	wsaBuf->len = dwFileSize;
	*lpdwFileSize = dwFileSize;

	props->nNumToSend = dwFileSize / FILE_PACKETSIZE;
	props->nPacketSize = FILE_PACKETSIZE;
	CloseHandle(hFile);

	return TRUE;
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: CreateBuffer
-- Febrary 1st, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: CreateBuffer(CHAR data, LPTransferProps props)
--							CHAR data: Handle to the main window cast as a VOID *.
--
-- RETURNS: A positive int if the connection fails (TCP), the user-specified file couldn't be opened, or the buffer couldn't be
--			allocated. Returns 0 on successful sending.
--
-- NOTES:
-- Sends either a chosen file (if there is one) or a specified number of packets of the specified size.
---------------------------------------------------------------------------------------------------------------------------*/
CHAR *CreateBuffer(CHAR data, LPTransferProps props)
{
	CHAR *buf = (CHAR *)malloc(props->nPacketSize);
	if (buf == NULL)
	{
		MessageBoxPrintf(MB_ICONERROR, TEXT("No Memory Allocated"), TEXT("Windows couldn't allocate memory, error %d"), WSAGetLastError());
		return NULL;
	}
	memset(buf, data, props->nPacketSize);

	// Write the packet size and number to send directly into the packet
	((DWORD *)buf)[0] = props->nNumToSend;
	((DWORD *)buf)[1] = props->nPacketSize;
	return buf;
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: PopulateBuffer
-- Febrary 1st, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: PopulateBuffer(LPWSABUF pwsaBuf, LPTransferProps props, LPDWORD lpdwFileSize)
--							LPWSABUF wsaBuf:		The WSA buffer to populate with data.
--							LPTransferProps props:	Pointer to the TransferProps structure containing details about the transfer.
--							LPDWORD dwFileSize:		Pointer to a DWORD to store the file size.
--
-- RETURNS: False if the one of the functions failed; true otherwise.
--
-- NOTES:
-- Populates the send buffer with either a file or random data.
---------------------------------------------------------------------------------------------------------------------------*/
BOOL PopulateBuffer(LPWSABUF pwsaBuf, LPTransferProps props, LPDWORD lpdwFileSize)
{
	if (props->szFileName[0] != 0)
	{
		props->nPacketSize = FILE_PACKETSIZE;
		if (!LoadFile(pwsaBuf, props->szFileName, lpdwFileSize, props))
			return FALSE;
	}
	else
	{
		if ((pwsaBuf->buf = CreateBuffer('a', props)) == NULL)
			return FALSE;

		pwsaBuf->len = props->nPacketSize;
	}
	return TRUE;
}

/*-------------------------------------------------------------------------------------------------------------------------
-- FUNCTION: ClientCleanup
-- Febrary 9th, 2014
--
-- DESIGNER: Shane Spoor
--
-- PROGRAMMER: Shane Spoor
--
-- INTERFACE: ClientCleanup(LPTranserProps props)
--							LPTransferProps props: 
--
-- RETURNS: void
--
-- NOTES:
-- Sends either a chosen file (if there is one) or a specified number of packets of the specified size.
---------------------------------------------------------------------------------------------------------------------------*/
VOID ClientCleanup(LPTransferProps props)
{
	DWORD error;
	free(wsaBuf.buf);
	closesocket(props->socket);
	error = WSAGetLastError();
	memset(&props->startTime, 0, sizeof(SYSTEMTIME));
	memset(&props->endTime, 0, sizeof(SYSTEMTIME));
	props->dwTimeout = COMM_TIMEOUT;
	sent = 0;
}
