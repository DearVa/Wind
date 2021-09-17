#include "HookThread.h"

WThread wThreads[MAX_THREAD_NUM];
HookInfo createThreadHookInfo, getThreadIdHookInfo, closeHandleHookInfo;

EXTERN_C_START
_Ret_maybenull_ HANDLE WINAPI WCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId) {
	// DP0("[Wind] WCreateThread");
	HookOff(createThreadHookInfo);
	const auto hThread = CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
	HookOn(createThreadHookInfo);

	if (hThread == NULL) {
		return NULL;
	}

	int mappedThreadHandle = 0;
	for (int i = 0; i < MAX_THREAD_NUM; i++) {
		if (!wThreads[i].valid) {  // 找到一个空间，分配进程
			mappedThreadHandle = i + MAGIC_NUMBER;
			wThreads[i].mappedHandleId = mappedThreadHandle;
			wThreads[i].originHandle = hThread;
			HookOff(getThreadIdHookInfo);
			wThreads[i].originId = GetThreadId(hThread);
			HookOn(getThreadIdHookInfo);
			wThreads[i].valid = true;
			break;
		}
	}

	if (mappedThreadHandle == 0) {
		MessageBoxA(nullptr, "Cannot Alloc Thread.", "Critical", 0);
		exit(-1);
	}

	DP3("[Wind] call CreateThread, StartAddress: %p, Remap: %p -> %p", lpStartAddress, hThread, mappedThreadHandle);
	return reinterpret_cast<HANDLE>(mappedThreadHandle);
}

DWORD WINAPI WGetThreadId(HANDLE Thread) {
	DP1("[Wind] call GetThreadId, Thread: %p", Thread);

	const auto index = reinterpret_cast<DWORD>(Thread);
	if (index >= MAX_THREAD_NUM) {
		return 0;
	}
	const auto &wThread = wThreads[index];
	if (wThread.valid) {
		return wThread.mappedHandleId;
	}
	return 0;
}

BOOL WINAPI WCloseHandle(HANDLE hObject) {
	DP1("[Wind] call CloseHandle, hObject: %p", hObject);
	if (hObject == nullptr) {
		return false;
	}
	BOOL result;
	const auto index = reinterpret_cast<DWORD>(hObject);
	if (index >= MAGIC_NUMBER && index < MAGIC_NUMBER + MAX_THREAD_NUM) {
		auto &wThread = wThreads[index - MAGIC_NUMBER];
		if (wThread.valid) {
			HookOff(closeHandleHookInfo);
			result = CloseHandle(wThread.originHandle);
			HookOn(closeHandleHookInfo);
			wThread.valid = false;
			DP0("[Wind] Close Mapped thread");
		} else {
			result = false;
		}
	} else {
		HookOff(closeHandleHookInfo);
		result = CloseHandle(hObject);
		HookOn(closeHandleHookInfo);
		DP0("[Wind] Close System Handle");
	}
	return result;
}
EXTERN_C_END