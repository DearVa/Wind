#pragma once
#include <Windows.h>
#include "Hook.h"

#define MAX_THREAD_NUM 1024

struct WThread {
	HANDLE originHandle;
	DWORD originId;

	HANDLE internalHandle;
	DWORD mappedHandleId;  // œ‡Õ¨

	BOOL isAlive();
};

#define MAGIC_NUMBER 12886514

extern WThread wThreads[MAX_THREAD_NUM];
extern HookInfo createRemoteThreadExHookInfo, getThreadIdHookInfo, closeHandleHookInfo, terminateThreadHookInfo;

EXTERN_C_START
_Ret_maybenull_ HANDLE WINAPI WCreateRemoteThreadEx(HANDLE hProcess, LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList, LPDWORD lpThreadId);
DWORD WINAPI WGetThreadId(HANDLE Thread);
BOOL WINAPI WCloseHandle(HANDLE hObject);
BOOL WINAPI WTerminateThread(HANDLE hThread, DWORD dwExitCode);
EXTERN_C_END

int GetAliveThreadCount();