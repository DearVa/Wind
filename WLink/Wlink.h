#pragma once
#include <Windows.h>

typedef unsigned int uint;

extern HANDLE wHostMutex, wClientMutex;
extern HANDLE wLinkThread;

#define WMSG_MAGIC_NUMBER 13288861

struct WLinkMsg {
	uint msgType;
	uint msgSize;
};

bool CreateWLink(HANDLE hProcess);
bool OpenWLink();
void SendWMessageH(const uint msgType, const uint msgSize, void *msg);
void SendWMessageC(const uint msgType, const uint msgSize, void *msg);