#pragma once
#include <Windows.h>
#include "Hook.h"
#include "log.h"

#define MAX_THREAD_NUM 1024

struct WThread {
	BOOL valid;
	HANDLE originHandle;
	DWORD originId;
	DWORD mappedHandleId;  // œ‡Õ¨
};

#define MAGIC_NUMBER 12886514

extern WThread wThreads[MAX_THREAD_NUM];

extern HookInfo createThreadHookInfo, getThreadIdHookInfo, closeHandleHookInfo;
// HookInfo beginthreadexHookInfo, beginthreadHookInfo;

EXTERN_C_START
_Ret_maybenull_ HANDLE WINAPI WCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
DWORD WINAPI WGetThreadId(HANDLE Thread);
BOOL WINAPI WCloseHandle(HANDLE hObject);
EXTERN_C_END