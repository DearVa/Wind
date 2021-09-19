#pragma once
#include <Windows.h>

typedef unsigned int uint;

extern char wMutexName[9];
extern HANDLE wMutex;
extern HANDLE wLinkThread;

#define WMSG_MAGIC_NUMBER 13288861

struct WLinkMsg {
	uint msgType;
	uint msgSize;
};

bool CreateWLink(HANDLE hProcess);
bool OpenWLink();
void SendWMessage(const uint msgType, const uint msgSize, void *msg);