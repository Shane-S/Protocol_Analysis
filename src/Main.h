#ifndef MAIN_H
#define MAIN_H

#include <WinSock2.h>
#include <Windows.h>
#include <cstring>
#include "WinStorage.h"

// Name constants
#define CLASS_NAME	TEXT("Assn2")
#define APP_NAME	TEXT("TCP v UDP")

#define DEF_PACKETSIZE	1024
#define DEF_PORTNUM		7000
#define DEF_NUMTOSEND	10

LPTransferProps CreateTransferProps();
int WINAPI WinMain(HINSTANCE hPrevInstance, HINSTANCE hInstance, LPSTR lpszCmdArgs, int iCmdShow);

#endif